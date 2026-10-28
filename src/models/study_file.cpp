// src/models/study_file.cpp
#include "study_file.h"

StudyFile::StudyFile() = default;

StudyFile::StudyFile(int id, int userId, int subjectId, QString originalFilename,
                     QString managedFilename, QString managedFilePath,
                     QString fileType, qint64 fileSize, QString importedAt)
    : m_id(id)
    , m_userId(userId)
    , m_subjectId(subjectId)
    , m_originalFilename(std::move(originalFilename))
    , m_managedFilename(std::move(managedFilename))
    , m_managedFilePath(std::move(managedFilePath))
    , m_fileType(std::move(fileType))
    , m_fileSize(fileSize)
    , m_importedAt(std::move(importedAt))
{
}
