// src/services/study_file_service.h
#pragma once

#include <QList>
#include <QString>

class StudyFileDAO;
class StudyFile;

class StudyFileService
{
public:
    explicit StudyFileService(StudyFileDAO *studyFileDAO);
    ~StudyFileService() = default;

    bool importFile(int userId, int subjectId, const QString &sourceFilePath,
                    QString *errorMessage = nullptr, StudyFile *outCreatedFile = nullptr) const;
    QList<StudyFile> getStudyFiles(int userId, QString *errorMessage = nullptr) const;
    StudyFile getStudyFile(int userId, int fileId, QString *errorMessage = nullptr) const;
    bool removeStudyFile(int userId, int fileId, QString *errorMessage = nullptr) const;

    static QString managedStorageDirectory(int userId);
    static QString determineFileType(const QString &filename);

private:
    StudyFileDAO *m_studyFileDAO;
};
