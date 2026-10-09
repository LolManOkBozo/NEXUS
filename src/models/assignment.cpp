#include "assignment.h"

Assignment::Assignment(int id, int userId, int subjectId, QString title,
                       QString description, QString dueDate, QString dueTime,
                       QString priority, QString status, QString createdAt)
    : m_id(id)
    , m_userId(userId)
    , m_subjectId(subjectId)
    , m_title(std::move(title))
    , m_description(std::move(description))
    , m_dueDate(std::move(dueDate))
    , m_dueTime(std::move(dueTime))
    , m_priority(std::move(priority))
    , m_status(std::move(status))
    , m_createdAt(std::move(createdAt))
{
}
