#include "exam.h"

Exam::Exam(int id, int userId, int subjectId, QString title,
           QString examDate, QString startTime, QString room)
    : m_id(id)
    , m_userId(userId)
    , m_subjectId(subjectId)
    , m_title(std::move(title))
    , m_examDate(std::move(examDate))
    , m_startTime(std::move(startTime))
    , m_room(std::move(room))
{
}
