// src/data/study_file_dao.cpp
#include "study_file_dao.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "../core/database_manager.h"
#include "../models/study_file.h"

namespace
{
QString studyFileSelect()
{
    return QStringLiteral(
        "SELECT id, user_id, subject_id, original_filename, managed_filename, "
        "managed_file_path, file_type, file_size, imported_at FROM study_files ");
}

StudyFile readStudyFile(const QSqlQuery &query)
{
    return StudyFile(
        query.value(0).toInt(),
        query.value(1).toInt(),
        query.value(2).isNull() ? 0 : query.value(2).toInt(),
        query.value(3).toString(),
        query.value(4).toString(),
        query.value(5).toString(),
        query.value(6).toString(),
        query.value(7).toLongLong(),
        query.value(8).toString());
}

bool fail(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

bool queryFailed(QString *errorMessage, const QString &operation, const QSqlQuery &query)
{
    return fail(errorMessage,
                QStringLiteral("%1: %2").arg(operation, query.lastError().text()));
}

QSqlDatabase openDatabase(DatabaseManager *databaseManager, QString *errorMessage)
{
    if (!databaseManager || !databaseManager->isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The database is not available.");
        }
        return {};
    }
    return databaseManager->database();
}
}

StudyFileDAO::StudyFileDAO(DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

bool StudyFileDAO::insertStudyFile(StudyFile &studyFile, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (studyFile.userId() <= 0 || studyFile.originalFilename().isEmpty()
        || studyFile.managedFilePath().isEmpty()) {
        return fail(errorMessage, QStringLiteral("A valid user, filename, and file path are required."));
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "INSERT INTO study_files "
            "(user_id, subject_id, original_filename, managed_filename, "
            "managed_file_path, file_type, file_size, imported_at) "
            "VALUES (:user_id, :subject_id, :original_filename, :managed_filename, "
            ":managed_file_path, :file_type, :file_size, :imported_at)"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare study file insert"), query);
    }
    query.bindValue(QStringLiteral(":user_id"), studyFile.userId());
    if (studyFile.subjectId() > 0) {
        query.bindValue(QStringLiteral(":subject_id"), studyFile.subjectId());
    } else {
        query.bindValue(QStringLiteral(":subject_id"), QVariant(QVariant::Int));
    }
    query.bindValue(QStringLiteral(":original_filename"), studyFile.originalFilename());
    query.bindValue(QStringLiteral(":managed_filename"), studyFile.managedFilename());
    query.bindValue(QStringLiteral(":managed_file_path"), studyFile.managedFilePath());
    query.bindValue(QStringLiteral(":file_type"), studyFile.fileType());
    query.bindValue(QStringLiteral(":file_size"), studyFile.fileSize());
    query.bindValue(QStringLiteral(":imported_at"), studyFile.importedAt());

    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not save study file metadata"), query);
    }
    studyFile.setId(query.lastInsertId().toInt());
    return true;
}

QList<StudyFile> StudyFileDAO::getStudyFilesByUserId(int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    QList<StudyFile> files;
    if (userId <= 0) {
        fail(errorMessage, QStringLiteral("A valid user is required."));
        return files;
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return files;
    }

    QSqlQuery query(database);
    if (!query.prepare(studyFileSelect()
                       + QStringLiteral("WHERE user_id = :user_id ORDER BY imported_at DESC, id DESC"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare study files query"), query);
        return files;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load study files"), query);
        return files;
    }
    while (query.next()) {
        files.append(readStudyFile(query));
    }
    return files;
}

StudyFile StudyFileDAO::getStudyFileById(int userId, int fileId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || fileId <= 0) {
        fail(errorMessage, QStringLiteral("A valid user and file ID are required."));
        return {};
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QSqlQuery query(database);
    if (!query.prepare(studyFileSelect()
                       + QStringLiteral("WHERE user_id = :user_id AND id = :id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare study file lookup"), query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), fileId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load study file"), query);
        return {};
    }
    if (!query.next()) {
        fail(errorMessage, QStringLiteral("Study file not found."));
        return {};
    }
    return readStudyFile(query);
}

bool StudyFileDAO::deleteStudyFile(int userId, int fileId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || fileId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and file ID are required."));
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral("DELETE FROM study_files WHERE user_id = :user_id AND id = :id"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare study file deletion"), query);
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), fileId);
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not delete study file metadata"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage, QStringLiteral("Study file not found."));
    }
    return true;
}
