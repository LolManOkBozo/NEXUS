#pragma once

#include <QDate>
#include <QList>
#include <QString>

#include <optional>

#include "../models/attendance.h"

class AttendanceDAO;

class AttendanceService
{
public:
    explicit AttendanceService(AttendanceDAO *attendanceDAO);

    bool addAttendance(const Attendance &attendance,
                       QString *errorMessage = nullptr) const;
    Attendance getAttendance(int userId, int subjectId, const QDate &date,
                             QString *errorMessage = nullptr) const;
    QList<Attendance> getAttendances(int userId,
                                     QString *errorMessage = nullptr) const;
    QList<Attendance> getAttendancesForSubject(
        int userId, int subjectId, QString *errorMessage = nullptr) const;
    QList<Attendance> getAttendancesInDateRange(
        int userId, const QDate &startDate, const QDate &endDate,
        QString *errorMessage = nullptr) const;
    QList<Attendance> getAttendancesForSubjectInDateRange(
        int userId, int subjectId, const QDate &startDate, const QDate &endDate,
        QString *errorMessage = nullptr) const;
    bool updateAttendance(int userId, const Attendance &attendance,
                          QString *errorMessage = nullptr) const;
    bool deleteAttendance(int userId, int attendanceId,
                          QString *errorMessage = nullptr) const;

    // Returns no value when the range has no Present or Absent records.
    // On DAO failure, returns no value and sets errorMessage when supplied.
    std::optional<double> attendancePercentage(
        int userId, int subjectId, const QDate &startDate, const QDate &endDate,
        QString *errorMessage = nullptr) const;
    std::optional<double> overallAttendancePercentage(
        int userId, QString *errorMessage = nullptr) const;

private:
    static bool validateAttendance(const Attendance &attendance, bool requireId,
                                   QString *errorMessage);
    static bool validateDateRange(const QDate &startDate, const QDate &endDate,
                                  QString *errorMessage);
    static std::optional<double> calculatePercentage(
        const QList<Attendance> &records, int userId, int subjectId = 0);

    AttendanceDAO *m_attendanceDAO;
};
