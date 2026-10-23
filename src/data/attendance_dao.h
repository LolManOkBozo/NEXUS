#pragma once

#include <QDate>
#include <QList>
#include <QString>

#include "../models/attendance.h"

class DatabaseManager;

class AttendanceDAO
{
public:
    explicit AttendanceDAO(DatabaseManager *databaseManager);

    bool insertAttendance(const Attendance &attendance,
                          QString *errorMessage = nullptr) const;
    Attendance getAttendanceByUserSubjectAndDate(
        int userId, int subjectId, const QDate &date,
        QString *errorMessage = nullptr) const;
    QList<Attendance> getAttendanceByUserId(
        int userId, QString *errorMessage = nullptr) const;
    QList<Attendance> getAttendanceByUserIdAndSubjectId(
        int userId, int subjectId, QString *errorMessage = nullptr) const;
    QList<Attendance> getAttendanceByUserIdInDateRange(
        int userId, const QDate &startDate, const QDate &endDate,
        QString *errorMessage = nullptr) const;
    QList<Attendance> getAttendanceByUserIdAndSubjectIdInDateRange(
        int userId, int subjectId, const QDate &startDate, const QDate &endDate,
        QString *errorMessage = nullptr) const;
    bool updateAttendance(int userId, const Attendance &attendance,
                          QString *errorMessage = nullptr) const;
    bool deleteAttendance(int userId, int attendanceId,
                          QString *errorMessage = nullptr) const;

private:
    DatabaseManager *m_databaseManager;
};
