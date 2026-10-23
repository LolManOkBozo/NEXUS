#include "attendance_service.h"

#include "../data/attendance_dao.h"

namespace
{
bool fail(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

bool validStatus(Attendance::Status status)
{
    return status == Attendance::Status::Present
        || status == Attendance::Status::Absent
        || status == Attendance::Status::Leave;
}
}

AttendanceService::AttendanceService(AttendanceDAO *attendanceDAO)
    : m_attendanceDAO(attendanceDAO)
{
}

bool AttendanceService::validateAttendance(const Attendance &attendance,
                                           bool requireId,
                                           QString *errorMessage)
{
    if (attendance.userId() <= 0) {
        return fail(errorMessage,
                    QStringLiteral("A valid user is required for attendance."));
    }
    if (attendance.subjectId() <= 0) {
        return fail(errorMessage,
                    QStringLiteral("A valid subject is required for attendance."));
    }
    if ((requireId && attendance.id() <= 0)
        || (!requireId && attendance.id() != 0)) {
        return fail(errorMessage,
                    requireId
                        ? QStringLiteral("A valid attendance record is required to update it.")
                        : QStringLiteral("A new attendance record cannot have an ID."));
    }
    if (!attendance.date().isValid()) {
        return fail(errorMessage,
                    QStringLiteral("A valid attendance date is required."));
    }
    if (!validStatus(attendance.status())) {
        return fail(errorMessage,
                    QStringLiteral("A valid attendance status is required."));
    }
    return true;
}

bool AttendanceService::validateDateRange(const QDate &startDate,
                                          const QDate &endDate,
                                          QString *errorMessage)
{
    if (!startDate.isValid() || !endDate.isValid()) {
        return fail(errorMessage,
                    QStringLiteral("Valid attendance range dates are required."));
    }
    if (startDate > endDate) {
        return fail(errorMessage,
                    QStringLiteral(
                        "The attendance range start date must not be after its end date."));
    }
    return true;
}

bool AttendanceService::addAttendance(const Attendance &attendance,
                                      QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validateAttendance(attendance, false, errorMessage)) {
        return false;
    }
    if (!m_attendanceDAO) {
        return fail(errorMessage,
                    QStringLiteral("The attendance data service is unavailable."));
    }
    return m_attendanceDAO->insertAttendance(attendance, errorMessage);
}

Attendance AttendanceService::getAttendance(int userId, int subjectId,
                                            const QDate &date,
                                            QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || subjectId <= 0) {
        fail(errorMessage,
             QStringLiteral("A valid user and subject are required to load attendance."));
        return {};
    }
    if (!date.isValid()) {
        fail(errorMessage, QStringLiteral("A valid attendance date is required."));
        return {};
    }
    if (!m_attendanceDAO) {
        fail(errorMessage,
             QStringLiteral("The attendance data service is unavailable."));
        return {};
    }
    return m_attendanceDAO->getAttendanceByUserSubjectAndDate(
        userId, subjectId, date, errorMessage);
}

QList<Attendance> AttendanceService::getAttendances(
    int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        fail(errorMessage,
             QStringLiteral("A valid user is required to load attendance."));
        return {};
    }
    if (!m_attendanceDAO) {
        fail(errorMessage,
             QStringLiteral("The attendance data service is unavailable."));
        return {};
    }
    return m_attendanceDAO->getAttendanceByUserId(userId, errorMessage);
}

QList<Attendance> AttendanceService::getAttendancesForSubject(
    int userId, int subjectId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || subjectId <= 0) {
        fail(errorMessage,
             QStringLiteral("A valid user and subject are required to load attendance."));
        return {};
    }
    if (!m_attendanceDAO) {
        fail(errorMessage,
             QStringLiteral("The attendance data service is unavailable."));
        return {};
    }
    return m_attendanceDAO->getAttendanceByUserIdAndSubjectId(
        userId, subjectId, errorMessage);
}

QList<Attendance> AttendanceService::getAttendancesInDateRange(
    int userId, const QDate &startDate, const QDate &endDate,
    QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        fail(errorMessage,
             QStringLiteral("A valid user is required to load attendance."));
        return {};
    }
    if (!validateDateRange(startDate, endDate, errorMessage)) {
        return {};
    }
    if (!m_attendanceDAO) {
        fail(errorMessage,
             QStringLiteral("The attendance data service is unavailable."));
        return {};
    }
    return m_attendanceDAO->getAttendanceByUserIdInDateRange(
        userId, startDate, endDate, errorMessage);
}

QList<Attendance> AttendanceService::getAttendancesForSubjectInDateRange(
    int userId, int subjectId, const QDate &startDate, const QDate &endDate,
    QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || subjectId <= 0) {
        fail(errorMessage,
             QStringLiteral("A valid user and subject are required to load attendance."));
        return {};
    }
    if (!validateDateRange(startDate, endDate, errorMessage)) {
        return {};
    }
    if (!m_attendanceDAO) {
        fail(errorMessage,
             QStringLiteral("The attendance data service is unavailable."));
        return {};
    }
    return m_attendanceDAO->getAttendanceByUserIdAndSubjectIdInDateRange(
        userId, subjectId, startDate, endDate, errorMessage);
}

bool AttendanceService::updateAttendance(int userId,
                                         const Attendance &attendance,
                                         QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        return fail(errorMessage,
                    QStringLiteral("A valid user is required to update attendance."));
    }
    if (!validateAttendance(attendance, true, errorMessage)) {
        return false;
    }
    if (attendance.userId() != userId) {
        return fail(errorMessage,
                    QStringLiteral(
                        "The attendance record does not belong to the specified user."));
    }
    if (!m_attendanceDAO) {
        return fail(errorMessage,
                    QStringLiteral("The attendance data service is unavailable."));
    }
    return m_attendanceDAO->updateAttendance(userId, attendance, errorMessage);
}

bool AttendanceService::deleteAttendance(int userId, int attendanceId,
                                         QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || attendanceId <= 0) {
        return fail(errorMessage,
                    QStringLiteral("A valid user and attendance record are required."));
    }
    if (!m_attendanceDAO) {
        return fail(errorMessage,
                    QStringLiteral("The attendance data service is unavailable."));
    }
    return m_attendanceDAO->deleteAttendance(userId, attendanceId, errorMessage);
}

std::optional<double> AttendanceService::attendancePercentage(
    int userId, int subjectId, const QDate &startDate, const QDate &endDate,
    QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || subjectId <= 0) {
        fail(errorMessage,
             QStringLiteral("A valid user and subject are required to calculate attendance."));
        return std::nullopt;
    }
    if (!validateDateRange(startDate, endDate, errorMessage)) {
        return std::nullopt;
    }
    if (!m_attendanceDAO) {
        fail(errorMessage,
             QStringLiteral("The attendance data service is unavailable."));
        return std::nullopt;
    }

    QString queryError;
    const QList<Attendance> records =
        m_attendanceDAO->getAttendanceByUserIdAndSubjectIdInDateRange(
            userId, subjectId, startDate, endDate, &queryError);
    if (!queryError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = queryError;
        }
        return std::nullopt;
    }

    return calculatePercentage(records, userId, subjectId);
}

std::optional<double> AttendanceService::overallAttendancePercentage(
    int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        fail(errorMessage,
             QStringLiteral("A valid user is required to calculate attendance."));
        return std::nullopt;
    }
    if (!m_attendanceDAO) {
        fail(errorMessage,
             QStringLiteral("The attendance data service is unavailable."));
        return std::nullopt;
    }

    QString queryError;
    const QList<Attendance> records =
        m_attendanceDAO->getAttendanceByUserId(userId, &queryError);
    if (!queryError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = queryError;
        }
        return std::nullopt;
    }
    return calculatePercentage(records, userId);
}

std::optional<double> AttendanceService::calculatePercentage(
    const QList<Attendance> &records, int userId, int subjectId)
{
    int presentCount = 0;
    int absentCount = 0;
    for (const Attendance &record : records) {
        if (record.userId() != userId
            || (subjectId > 0 && record.subjectId() != subjectId)) {
            continue;
        }
        if (record.status() == Attendance::Status::Present) {
            ++presentCount;
        } else if (record.status() == Attendance::Status::Absent) {
            ++absentCount;
        }
    }
    const int countedRecords = presentCount + absentCount;
    if (countedRecords == 0) {
        return std::nullopt;
    }
    return static_cast<double>(presentCount) * 100.0
        / static_cast<double>(countedRecords);
}
