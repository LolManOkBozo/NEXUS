#pragma once

#include <QList>
#include <QString>

#include "../models/timetable.h"

class TimetableDAO;

class TimetableService
{
public:
    explicit TimetableService(TimetableDAO *timetableDAO);

    bool addTimetable(const Timetable &timetable,
                      QString *errorMessage = nullptr) const;
    QList<Timetable> getTimetables(int userId,
                                   QString *errorMessage = nullptr) const;
    QList<Timetable> getTimetablesForDay(int userId, int dayOfWeek,
                                         QString *errorMessage = nullptr) const;
    Timetable getTimetable(int userId, int timetableId,
                           QString *errorMessage = nullptr) const;
    bool updateTimetable(const Timetable &timetable,
                         QString *errorMessage = nullptr) const;
    bool deleteTimetable(int userId, int timetableId,
                         QString *errorMessage = nullptr) const;

private:
    static bool validateAndNormalize(Timetable &timetable, QString *errorMessage);
    bool hasScheduleConflict(const Timetable &timetable, int excludedTimetableId,
                             QString *errorMessage) const;

    TimetableDAO *m_timetableDAO;
};
