#pragma once

#include <QString>

#include <utility>

class Timetable
{
public:
    Timetable() = default;
    Timetable(int id, int userId, int subjectId, int dayOfWeek,
              QString startTime, QString endTime, QString room);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    int subjectId() const { return m_subjectId; }
    void setSubjectId(int subjectId) { m_subjectId = subjectId; }

    int dayOfWeek() const { return m_dayOfWeek; }
    void setDayOfWeek(int dayOfWeek) { m_dayOfWeek = dayOfWeek; }

    QString startTime() const { return m_startTime; }
    void setStartTime(QString startTime) { m_startTime = std::move(startTime); }

    QString endTime() const { return m_endTime; }
    void setEndTime(QString endTime) { m_endTime = std::move(endTime); }

    QString room() const { return m_room; }
    void setRoom(QString room) { m_room = std::move(room); }

private:
    int m_id{0};
    int m_userId{0};
    int m_subjectId{0};
    int m_dayOfWeek{0};
    QString m_startTime;
    QString m_endTime;
    QString m_room;
};
