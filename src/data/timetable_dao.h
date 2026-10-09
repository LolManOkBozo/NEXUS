#pragma once

#include <QList>
#include <QString>

#include "../models/timetable.h"

class DatabaseManager;

class TimetableDAO
{
public:
    explicit TimetableDAO(DatabaseManager *databaseManager);

    bool insertTimetable(const Timetable &timetable,
                         QString *errorMessage = nullptr) const;
    QList<Timetable> getTimetablesByUserId(int userId,
                                           QString *errorMessage = nullptr) const;
    QList<Timetable> getTimetablesByUserIdAndDay(
        int userId, int dayOfWeek, QString *errorMessage = nullptr) const;
    Timetable getTimetableById(int userId, int timetableId,
                               QString *errorMessage = nullptr) const;
    bool updateTimetable(const Timetable &timetable,
                         QString *errorMessage = nullptr) const;
    bool deleteTimetable(int userId, int timetableId,
                         QString *errorMessage = nullptr) const;

private:
    DatabaseManager *m_databaseManager;
};
