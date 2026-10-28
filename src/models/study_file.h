// src/models/study_file.h
#pragma once

#include <QString>
#include <utility>

class StudyFile
{
public:
    StudyFile();
    StudyFile(int id, int userId, int subjectId, QString originalFilename,
              QString managedFilename, QString managedFilePath,
              QString fileType, qint64 fileSize, QString importedAt);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    int subjectId() const { return m_subjectId; }
    void setSubjectId(int subjectId) { m_subjectId = subjectId; }

    QString originalFilename() const { return m_originalFilename; }
    void setOriginalFilename(QString originalFilename) { m_originalFilename = std::move(originalFilename); }

    QString managedFilename() const { return m_managedFilename; }
    void setManagedFilename(QString managedFilename) { m_managedFilename = std::move(managedFilename); }

    QString managedFilePath() const { return m_managedFilePath; }
    void setManagedFilePath(QString managedFilePath) { m_managedFilePath = std::move(managedFilePath); }

    QString fileType() const { return m_fileType; }
    void setFileType(QString fileType) { m_fileType = std::move(fileType); }

    qint64 fileSize() const { return m_fileSize; }
    void setFileSize(qint64 fileSize) { m_fileSize = fileSize; }

    QString importedAt() const { return m_importedAt; }
    void setImportedAt(QString importedAt) { m_importedAt = std::move(importedAt); }

private:
    int m_id{0};
    int m_userId{0};
    int m_subjectId{0};
    QString m_originalFilename;
    QString m_managedFilename;
    QString m_managedFilePath;
    QString m_fileType;
    qint64 m_fileSize{0};
    QString m_importedAt;
};
