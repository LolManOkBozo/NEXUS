#pragma once

#include <QString>

#include <utility>

class Exam
{
public:
    Exam() = default;
    Exam(int id, int userId, int subjectId, QString title,
         QString examDate, QString startTime, QString room);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    int subjectId() const { return m_subjectId; }
    void setSubjectId(int subjectId) { m_subjectId = subjectId; }

    QString title() const { return m_title; }
    void setTitle(QString title) { m_title = std::move(title); }

    QString examDate() const { return m_examDate; }
    void setExamDate(QString examDate) { m_examDate = std::move(examDate); }

    QString startTime() const { return m_startTime; }
    void setStartTime(QString startTime) { m_startTime = std::move(startTime); }

    QString room() const { return m_room; }
    void setRoom(QString room) { m_room = std::move(room); }

private:
    int m_id{0};
    int m_userId{0};
    int m_subjectId{0};
    QString m_title;
    QString m_examDate;
    QString m_startTime;
    QString m_room;
};
