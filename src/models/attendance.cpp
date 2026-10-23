#include "attendance.h"

#include <utility>

Attendance::Attendance(int id, int userId, int subjectId, QDate date, Status status)
    : m_id(id)
    , m_userId(userId)
    , m_subjectId(subjectId)
    , m_date(std::move(date))
    , m_status(status)
{
}

bool Attendance::isValid() const
{
    const bool validStatus = m_status == Status::Present
        || m_status == Status::Absent
        || m_status == Status::Leave;
    return m_id >= 0 && m_userId > 0 && m_subjectId > 0 && m_date.isValid()
        && validStatus;
}

QString Attendance::dateToString(const QDate &date)
{
    return date.isValid() ? date.toString(Qt::ISODate) : QString();
}

QDate Attendance::stringToDate(const QString &date)
{
    const QDate parsedDate = QDate::fromString(date, Qt::ISODate);
    return parsedDate.isValid() && parsedDate.toString(Qt::ISODate) == date
        ? parsedDate
        : QDate();
}
