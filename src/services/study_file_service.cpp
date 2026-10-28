// src/services/study_file_service.cpp
#include "study_file_service.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include "../data/study_file_dao.h"
#include "../models/study_file.h"

namespace
{
bool fail(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}
}

StudyFileService::StudyFileService(StudyFileDAO *studyFileDAO)
    : m_studyFileDAO(studyFileDAO)
{
}

QString StudyFileService::managedStorageDirectory(int userId)
{
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataDir.isEmpty()) {
        return QString();
    }
    return QDir(appDataDir).filePath(QStringLiteral("library_files/user_%1").arg(userId));
}

QString StudyFileService::determineFileType(const QString &filename)
{
    const QString ext = QFileInfo(filename).suffix().toLower();
    if (ext == QStringLiteral("pdf")) {
        return QStringLiteral("PDF");
    }
    if (ext == QStringLiteral("docx") || ext == QStringLiteral("doc")) {
        return QStringLiteral("DOCX");
    }
    if (ext == QStringLiteral("pptx") || ext == QStringLiteral("ppt")) {
        return QStringLiteral("PPTX");
    }
    if (ext == QStringLiteral("txt") || ext == QStringLiteral("md") || ext == QStringLiteral("csv")) {
        return QStringLiteral("TXT");
    }
    if (ext == QStringLiteral("png") || ext == QStringLiteral("jpg") || ext == QStringLiteral("jpeg")
        || ext == QStringLiteral("gif") || ext == QStringLiteral("webp") || ext == QStringLiteral("bmp")) {
        return QStringLiteral("Image");
    }
    if (ext.isEmpty()) {
        return QStringLiteral("File");
    }
    return ext.toUpper();
}

bool StudyFileService::importFile(int userId, int subjectId, const QString &sourceFilePath,
                                  QString *errorMessage, StudyFile *outCreatedFile) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required to import files."));
    }
    if (sourceFilePath.isEmpty()) {
        return fail(errorMessage, QStringLiteral("Source file path cannot be empty."));
    }

    QFileInfo sourceInfo(sourceFilePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile() || !sourceInfo.isReadable()) {
        return fail(errorMessage, QStringLiteral("The selected source file does not exist or is unreadable."));
    }

    const QString storageDirPath = managedStorageDirectory(userId);
    if (storageDirPath.isEmpty()) {
        return fail(errorMessage, QStringLiteral("Unable to determine managed storage location."));
    }

    QDir storageDir(storageDirPath);
    if (!storageDir.exists() && !storageDir.mkpath(QStringLiteral("."))) {
        return fail(errorMessage, QStringLiteral("Failed to create managed storage directory."));
    }

    const QString originalFilename = sourceInfo.fileName();
    QString baseName = sourceInfo.completeBaseName();
    QString suffix = sourceInfo.suffix();
    if (baseName.isEmpty()) {
        baseName = QStringLiteral("file");
    }

    // Prevent collision safely without overwriting
    QString managedFilename = originalFilename;
    QString targetFilePath = storageDir.filePath(managedFilename);
    int counter = 1;
    while (QFileInfo::exists(targetFilePath)) {
        managedFilename = suffix.isEmpty()
            ? QStringLiteral("%1_%2").arg(baseName, QString::number(counter))
            : QStringLiteral("%1_%2.%3").arg(baseName, QString::number(counter), suffix);
        targetFilePath = storageDir.filePath(managedFilename);
        counter++;
    }

    // Copy file into managed storage without modifying its contents
    if (!QFile::copy(sourceFilePath, targetFilePath)) {
        return fail(errorMessage, QStringLiteral("Failed to copy file into managed storage."));
    }
    // Ensure copied file is writable so it can be managed/deleted later
    QFile::setPermissions(targetFilePath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser);

    const QString fileType = determineFileType(originalFilename);
    const qint64 fileSize = sourceInfo.size();
    const QString importedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    StudyFile studyFile(0, userId, subjectId, originalFilename, managedFilename,
                        targetFilePath, fileType, fileSize, importedAt);

    if (!m_studyFileDAO) {
        QFile::remove(targetFilePath);
        return fail(errorMessage, QStringLiteral("The study file data service is unavailable."));
    }

    if (!m_studyFileDAO->insertStudyFile(studyFile, errorMessage)) {
        QFile::remove(targetFilePath);
        return false;
    }

    if (outCreatedFile) {
        *outCreatedFile = studyFile;
    }
    return true;
}

QList<StudyFile> StudyFileService::getStudyFiles(int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        fail(errorMessage, QStringLiteral("A valid user is required."));
        return {};
    }
    if (!m_studyFileDAO) {
        fail(errorMessage, QStringLiteral("The study file data service is unavailable."));
        return {};
    }
    return m_studyFileDAO->getStudyFilesByUserId(userId, errorMessage);
}

StudyFile StudyFileService::getStudyFile(int userId, int fileId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || fileId <= 0) {
        fail(errorMessage, QStringLiteral("A valid user and file ID are required."));
        return {};
    }
    if (!m_studyFileDAO) {
        fail(errorMessage, QStringLiteral("The study file data service is unavailable."));
        return {};
    }
    return m_studyFileDAO->getStudyFileById(userId, fileId, errorMessage);
}

bool StudyFileService::removeStudyFile(int userId, int fileId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || fileId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and file ID are required."));
    }
    if (!m_studyFileDAO) {
        return fail(errorMessage, QStringLiteral("The study file data service is unavailable."));
    }

    const StudyFile file = m_studyFileDAO->getStudyFileById(userId, fileId, errorMessage);
    if (file.id() <= 0) {
        return false;
    }

    // Delete database record first
    if (!m_studyFileDAO->deleteStudyFile(userId, fileId, errorMessage)) {
        return false;
    }

    // Remove managed file from disk (never delete user's original source file)
    if (!file.managedFilePath().isEmpty() && QFileInfo::exists(file.managedFilePath())) {
        QFile::remove(file.managedFilePath());
    }

    return true;
}
