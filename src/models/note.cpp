#include "note.h"

Note::Note(int id, int userId, int subjectId, QString title, QString content,
           QString createdAt, QString updatedAt)
    : m_id(id)
    , m_userId(userId)
    , m_subjectId(subjectId)
    , m_title(std::move(title))
    , m_content(std::move(content))
    , m_createdAt(std::move(createdAt))
    , m_updatedAt(std::move(updatedAt))
{
}