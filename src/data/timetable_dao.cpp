#include "timetable_dao.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "../core/database_manager.h"

namespace
{
QString timetableSelect()
{
    return QStringLiteral(
        "SELECT id, user_id, subject_id, day_of_week, start_time, end_time, room "
        "FROM timetable ");
}

Timetable readTimetable(const QSqlQuery &query)
{
    return Timetable(query.value(0).toInt(),
                     query.value(1).toInt(),
                     query.value(2).toInt(),
                     query.value(3).toInt(),
                     query.value(4).toString(),
                     query.value(5).toString(),
                     query.value(6).toString());
}

bool fail(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

bool queryFailed(QString *errorMessage, const QString &operation, const QSqlQuery &query)
{
    return fail(errorMessage,
                QStringLiteral("%1: %2").arg(operation, query.lastError().text()));
}

QSqlDatabase openDatabase(DatabaseManager *databaseManager, QString *errorMessage)
{
    if (!databaseManager || !databaseManager->isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The database is not available.");
        }
        return {};
    }
    return databaseManager->database();
}

bool validTimetableFields(const Timetable &timetable, QString *errorMessage)
{
    if (timetable.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required."));
    }
    if (timetable.subjectId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid subject is required."));
    }
    if (timetable.startTime().trimmed().isEmpty()
        || timetable.endTime().trimmed().isEmpty()) {
        return fail(errorMessage, QStringLiteral("Start and end times are required."));
    }
    return true;
}

QList<Timetable> readTimetables(QSqlQuery &query, QString *errorMessage,
                                const QString &operation)
{
    QList<Timetable> timetables;
    while (query.next()) {
        timetables.append(readTimetable(query));
    }
    if (query.lastError().isValid()) {
        queryFailed(errorMessage, operation, query);
        timetables.clear();
    }
    return timetables;
}
}

TimetableDAO::TimetableDAO(DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

bool TimetableDAO::insertTimetable(const Timetable &timetable,
                                   QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validTimetableFields(timetable, errorMessage)) {
        return false;
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "INSERT INTO timetable "
            "(user_id, subject_id, day_of_week, start_time, end_time, room) "
            "SELECT :user_id, :subject_id, :day_of_week, :start_time, :end_time, :room "
            "WHERE EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id)"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare timetable insert"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), timetable.userId());
    query.bindValue(QStringLiteral(":subject_id"), timetable.subjectId());
    query.bindValue(QStringLiteral(":day_of_week"), timetable.dayOfWeek());
    query.bindValue(QStringLiteral(":start_time"), timetable.startTime().trimmed());
    query.bindValue(QStringLiteral(":end_time"), timetable.endTime().trimmed());
    query.bindValue(QStringLiteral(":room"), timetable.room().trimmed());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not add timetable entry"),
                           query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage,
                    QStringLiteral("The subject does not belong to the specified user."));
    }
    return true;
}

QList<Timetable> TimetableDAO::getTimetablesByUserId(
    int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        fail(errorMessage, QStringLiteral("A valid user is required."));
        return {};
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QSqlQuery query(database);
    if (!query.prepare(timetableSelect()
                       + QStringLiteral("WHERE user_id = :user_id "
                                        "ORDER BY day_of_week, start_time, id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare timetable query"), query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load timetable"), query);
        return {};
    }
    return readTimetables(query, errorMessage, QStringLiteral("Could not load timetable"));
}

QList<Timetable> TimetableDAO::getTimetablesByUserIdAndDay(
    int userId, int dayOfWeek, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        fail(errorMessage, QStringLiteral("A valid user is required."));
        return {};
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QSqlQuery query(database);
    if (!query.prepare(timetableSelect()
                       + QStringLiteral("WHERE user_id = :user_id "
                                        "AND day_of_week = :day_of_week "
                                        "ORDER BY start_time, id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare daily timetable query"),
                    query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":day_of_week"), dayOfWeek);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load daily timetable"), query);
        return {};
    }
    return readTimetables(query, errorMessage,
                          QStringLiteral("Could not load daily timetable"));
}

Timetable TimetableDAO::getTimetableById(int userId, int timetableId,
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

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QSqlQuery query(database);
    if (!query.prepare(timetableSelect()
                       + QStringLiteral("WHERE user_id = :user_id AND id = :id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare timetable lookup"), query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), timetableId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load timetable entry"), query);
        return {};
    }
    if (!query.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Timetable entry not found.");
        }
        return {};
    }
    return readTimetable(query);
}

bool TimetableDAO::updateTimetable(const Timetable &timetable,
                                   QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (timetable.id() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid timetable entry is required."));
    }
    if (!validTimetableFields(timetable, errorMessage)) {
        return false;
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "UPDATE timetable SET subject_id = :subject_id, "
            "day_of_week = :day_of_week, start_time = :start_time, "
            "end_time = :end_time, room = :room "
            "WHERE user_id = :user_id AND id = :id "
            "AND EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id)"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare timetable update"),
                           query);
    }
    query.bindValue(QStringLiteral(":subject_id"), timetable.subjectId());
    query.bindValue(QStringLiteral(":day_of_week"), timetable.dayOfWeek());
    query.bindValue(QStringLiteral(":start_time"), timetable.startTime().trimmed());
    query.bindValue(QStringLiteral(":end_time"), timetable.endTime().trimmed());
    query.bindValue(QStringLiteral(":room"), timetable.room().trimmed());
    query.bindValue(QStringLiteral(":user_id"), timetable.userId());
    query.bindValue(QStringLiteral(":id"), timetable.id());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not update timetable entry"),
                           query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage,
                    QStringLiteral("Timetable entry not found or subject does not belong "
                                   "to the specified user."));
    }
    return true;
}

bool TimetableDAO::deleteTimetable(int userId, int timetableId,
                                   QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || timetableId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and timetable entry are required."));
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "DELETE FROM timetable WHERE user_id = :user_id AND id = :id"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare timetable deletion"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), timetableId);
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not delete timetable entry"),
                           query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage, QStringLiteral("Timetable entry not found."));
    }
    return true;
}
