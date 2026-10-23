#pragma once

#include <QDate>
#include <QString>

class Attendance
{
public:
    enum class Status {
        Present,
        Absent,
        Leave
    };

    Attendance() = default;
    Attendance(int id, int userId, int subjectId, QDate date, Status status);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    int subjectId() const { return m_subjectId; }
    void setSubjectId(int subjectId) { m_subjectId = subjectId; }

    QDate date() const { return m_date; }
    void setDate(QDate date) { m_date = date; }

    Status status() const { return m_status; }
    void setStatus(Status status) { m_status = status; }

    bool isValid() const;

    static QString dateToString(const QDate &date);
    static QDate stringToDate(const QString &date);

private:
    int m_id{0};
    int m_userId{0};
    int m_subjectId{0};
    QDate m_date;
    Status m_status{Status::Present};
};
