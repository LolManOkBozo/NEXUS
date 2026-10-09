#include "timetable_service.h"

#include <QTime>

#include "../data/timetable_dao.h"

namespace
{
bool fail(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

}

TimetableService::TimetableService(TimetableDAO *timetableDAO)
    : m_timetableDAO(timetableDAO)
{
}

bool TimetableService::validateAndNormalize(Timetable &timetable,
                                            QString *errorMessage)
{
    timetable.setStartTime(timetable.startTime().trimmed());
    timetable.setEndTime(timetable.endTime().trimmed());
    timetable.setRoom(timetable.room().trimmed());

    if (timetable.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required for this timetable entry."));
    }
    if (timetable.subjectId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid subject is required for this timetable entry."));
    }
    if (timetable.dayOfWeek() < 1 || timetable.dayOfWeek() > 7) {
        return fail(errorMessage, QStringLiteral("Weekday must be between Monday (1) and Sunday (7)."));
    }

    const QTime startTime = QTime::fromString(timetable.startTime(), QStringLiteral("HH:mm"));
    const QTime endTime = QTime::fromString(timetable.endTime(), QStringLiteral("HH:mm"));
    if (!startTime.isValid()
        || startTime.toString(QStringLiteral("HH:mm")) != timetable.startTime()
        || !endTime.isValid()
        || endTime.toString(QStringLiteral("HH:mm")) != timetable.endTime()) {
        return fail(errorMessage,
                    QStringLiteral("Start and end times must use the 24-hour HH:mm format."));
    }
    if (startTime >= endTime) {
        return fail(errorMessage, QStringLiteral("Start time must be earlier than end time."));
    }
    return true;
}

bool TimetableService::hasScheduleConflict(const Timetable &timetable,
                                           int excludedTimetableId,
                                           QString *errorMessage) const
{
    QString queryError;
    const QList<Timetable> dayEntries =
        m_timetableDAO->getTimetablesByUserIdAndDay(
            timetable.userId(), timetable.dayOfWeek(), &queryError);
    if (!queryError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = queryError;
        }
        return true;
    }

    const QTime startTime =
        QTime::fromString(timetable.startTime(), QStringLiteral("HH:mm"));
    const QTime endTime =
        QTime::fromString(timetable.endTime(), QStringLiteral("HH:mm"));
    for (const Timetable &existing : dayEntries) {
        if (existing.id() == excludedTimetableId) {
            continue;
        }

        const QTime existingStart =
            QTime::fromString(existing.startTime(), QStringLiteral("HH:mm"));
        const QTime existingEnd =
            QTime::fromString(existing.endTime(), QStringLiteral("HH:mm"));
        if (!existingStart.isValid() || !existingEnd.isValid()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral(
                    "An existing timetable entry has an invalid time range.");
            }
            return true;
        }
        if (startTime < existingEnd && existingStart < endTime) {
            if (errorMessage) {
                *errorMessage = QStringLiteral(
                    "This timetable entry overlaps another entry on the selected day.");
            }
            return true;
        }
    }
    return false;
}

bool TimetableService::addTimetable(const Timetable &timetable,
                                    QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Timetable normalizedTimetable = timetable;
    if (!validateAndNormalize(normalizedTimetable, errorMessage)) {
        return false;
    }
    if (!m_timetableDAO) {
        return fail(errorMessage, QStringLiteral("The timetable data service is unavailable."));
    }
    if (hasScheduleConflict(normalizedTimetable, 0, errorMessage)) {
        return false;
    }
    return m_timetableDAO->insertTimetable(normalizedTimetable, errorMessage);
}

QList<Timetable> TimetableService::getTimetables(int userId,
                                                 QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user is required to load timetable entries.");
        }
        return {};
    }
    if (!m_timetableDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The timetable data service is unavailable.");
        }
        return {};
    }
    return m_timetableDAO->getTimetablesByUserId(userId, errorMessage);
}

QList<Timetable> TimetableService::getTimetablesForDay(
    int userId, int dayOfWeek, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user is required to load timetable entries.");
        }
        return {};
    }
    if (dayOfWeek < 1 || dayOfWeek > 7) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Weekday must be between Monday (1) and Sunday (7).");
        }
        return {};
    }
    if (!m_timetableDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The timetable data service is unavailable.");
        }
        return {};
    }
    return m_timetableDAO->getTimetablesByUserIdAndDay(userId, dayOfWeek,
                                                        errorMessage);
}

Timetable TimetableService::getTimetable(int userId, int timetableId,
                                         QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || timetableId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user and timetable entry are required.");
        }
        return {};
    }
    if (!m_timetableDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The timetable data service is unavailable.");
        }
        return {};
    }
    return m_timetableDAO->getTimetableById(userId, timetableId, errorMessage);
}

bool TimetableService::updateTimetable(const Timetable &timetable,
                                       QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Timetable normalizedTimetable = timetable;
    if (normalizedTimetable.id() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid timetable entry is required to update it."));
    }
    if (!validateAndNormalize(normalizedTimetable, errorMessage)) {
        return false;
    }
    if (!m_timetableDAO) {
        return fail(errorMessage, QStringLiteral("The timetable data service is unavailable."));
    }
    if (hasScheduleConflict(normalizedTimetable, normalizedTimetable.id(),
                            errorMessage)) {
        return false;
    }
    return m_timetableDAO->updateTimetable(normalizedTimetable, errorMessage);
}

bool TimetableService::deleteTimetable(int userId, int timetableId,
                                       QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || timetableId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and timetable entry are required."));
    }
    if (!m_timetableDAO) {
        return fail(errorMessage, QStringLiteral("The timetable data service is unavailable."));
    }
    return m_timetableDAO->deleteTimetable(userId, timetableId, errorMessage);
}
