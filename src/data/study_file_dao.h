// src/data/study_file_dao.h
#pragma once

#include <QList>
#include <QString>

class DatabaseManager;
class StudyFile;

class StudyFileDAO
{
public:
    explicit StudyFileDAO(DatabaseManager *databaseManager);
    ~StudyFileDAO() = default;

    bool insertStudyFile(StudyFile &studyFile, QString *errorMessage = nullptr) const;
    QList<StudyFile> getStudyFilesByUserId(int userId, QString *errorMessage = nullptr) const;
    StudyFile getStudyFileById(int userId, int fileId, QString *errorMessage = nullptr) const;
    bool deleteStudyFile(int userId, int fileId, QString *errorMessage = nullptr) const;

private:
    DatabaseManager *m_databaseManager;
};
