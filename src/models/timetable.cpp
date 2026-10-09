#include "timetable.h"

#include <utility>

Timetable::Timetable(int id, int userId, int subjectId, int dayOfWeek,
                     QString startTime, QString endTime, QString room)
    : m_id(id)
    , m_userId(userId)
    , m_subjectId(subjectId)
    , m_dayOfWeek(dayOfWeek)
    , m_startTime(std::move(startTime))
    , m_endTime(std::move(endTime))
    , m_room(std::move(room))
{
}
