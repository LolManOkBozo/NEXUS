#include "attendance_dao.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "../core/database_manager.h"

namespace
{
QString attendanceSelect()
{
    return QStringLiteral(
        "SELECT id, user_id, subject_id, date, status FROM attendance ");
}

QString statusToString(Attendance::Status status)
{
    switch (status) {
    case Attendance::Status::Present:
        return QStringLiteral("Present");
    case Attendance::Status::Absent:
        return QStringLiteral("Absent");
    case Attendance::Status::Leave:
        return QStringLiteral("Leave");
    }
    return {};
}

Attendance::Status statusFromString(const QString &status)
{
    if (status == QStringLiteral("Present")) {
        return Attendance::Status::Present;
    }
    if (status == QStringLiteral("Absent")) {
        return Attendance::Status::Absent;
    }
    if (status == QStringLiteral("Leave")) {
        return Attendance::Status::Leave;
    }
    return static_cast<Attendance::Status>(-1);
}

Attendance readAttendance(const QSqlQuery &query)
{
    return Attendance(query.value(0).toInt(),
                      query.value(1).toInt(),
                      query.value(2).toInt(),
                      Attendance::stringToDate(query.value(3).toString()),
                      statusFromString(query.value(4).toString()));
}

bool fail(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

bool queryFailed(QString *errorMessage, const QString &operation,
                 const QSqlQuery &query)
{
    return fail(errorMessage,
                QStringLiteral("%1: %2").arg(operation, query.lastError().text()));
}

bool attendanceDuplicateWriteFailed(QString *errorMessage, const QString &operation,
                                    const QSqlQuery &query)
{
    if (query.lastError().text().contains(
            QStringLiteral("UNIQUE constraint failed: attendance.user_id, "
                           "attendance.subject_id, attendance.date"),
            Qt::CaseInsensitive)) {
        return fail(errorMessage,
                    QStringLiteral("Attendance already exists for this subject and date."));
    }
    return queryFailed(errorMessage, operation, query);
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

bool validStatus(Attendance::Status status)
{
    return !statusToString(status).isEmpty();
}

bool validIdentity(const Attendance &attendance, QString *errorMessage,
                   bool requireId)
{
    if (attendance.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required."));
    }
    if (attendance.subjectId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid subject is required."));
    }
    if ((requireId && attendance.id() <= 0)
        || (!requireId && attendance.id() != 0)) {
        return fail(errorMessage,
                    requireId ? QStringLiteral("A valid attendance record is required.")
                              : QStringLiteral("A new attendance record cannot have an ID."));
    }
    if (!attendance.date().isValid()) {
        return fail(errorMessage, QStringLiteral("A valid attendance date is required."));
    }
    if (!validStatus(attendance.status())) {
        return fail(errorMessage, QStringLiteral("A valid attendance status is required."));
    }
    return true;
}

bool subjectBelongsToUser(const QSqlDatabase &database, int userId, int subjectId,
                          QString *errorMessage)
{
    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "SELECT 1 FROM subjects WHERE id = :subject_id AND user_id = :user_id"))) {
        return queryFailed(errorMessage,
                           QStringLiteral("Could not prepare attendance subject check"),
                           query);
    }
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not check attendance subject"),
                           query);
    }
    if (!query.next()) {
        return fail(errorMessage,
                    QStringLiteral("The subject does not belong to the specified user."));
    }
    return true;
}

bool duplicateExists(const QSqlDatabase &database, int userId, int subjectId,
                     const QDate &date, int excludedAttendanceId,
                     bool *duplicate, QString *errorMessage)
{
    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "SELECT 1 FROM attendance "
            "WHERE user_id = :user_id AND subject_id = :subject_id AND date = :date "
            "AND id <> :excluded_id LIMIT 1"))) {
        return queryFailed(errorMessage,
                           QStringLiteral("Could not prepare attendance duplicate check"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":date"), Attendance::dateToString(date));
    query.bindValue(QStringLiteral(":excluded_id"), excludedAttendanceId);
    if (!query.exec()) {
        return queryFailed(errorMessage,
                           QStringLiteral("Could not check for duplicate attendance"),
                           query);
    }
    *duplicate = query.next();
    if (query.lastError().isValid()) {
        return queryFailed(errorMessage,
                           QStringLiteral("Could not check for duplicate attendance"),
                           query);
    }
    return true;
}

bool attendanceExists(const QSqlDatabase &database, int userId, int attendanceId,
                      bool *exists, QString *errorMessage)
{
    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "SELECT 1 FROM attendance WHERE user_id = :user_id AND id = :id"))) {
        return queryFailed(errorMessage,
                           QStringLiteral("Could not prepare attendance lookup"), query);
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), attendanceId);
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not load attendance"), query);
    }
    *exists = query.next();
    return true;
}

QList<Attendance> readAttendances(QSqlQuery &query, QString *errorMessage)
{
    QList<Attendance> attendances;
    while (query.next()) {
        attendances.append(readAttendance(query));
    }
    if (query.lastError().isValid()) {
        queryFailed(errorMessage, QStringLiteral("Could not load attendance"), query);
        attendances.clear();
    }
    return attendances;
}

QList<Attendance> getAttendance(const QSqlDatabase &database, const QString &sql,
                                int userId, int subjectId,
                                const QDate &startDate, const QDate &endDate,
                                QString *errorMessage)
{
    QSqlQuery query(database);
    if (!query.prepare(attendanceSelect() + sql
                       + QStringLiteral(" ORDER BY date DESC, id DESC"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare attendance query"),
                    query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (subjectId > 0) {
        query.bindValue(QStringLiteral(":subject_id"), subjectId);
    }
    if (startDate.isValid()) {
        query.bindValue(QStringLiteral(":start_date"),
                        Attendance::dateToString(startDate));
        query.bindValue(QStringLiteral(":end_date"),
                        Attendance::dateToString(endDate));
    }
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load attendance"), query);
        return {};
    }
    return readAttendances(query, errorMessage);
}

bool validateUserId(int userId, QString *errorMessage)
{
    return userId > 0
        || fail(errorMessage, QStringLiteral("A valid user is required."));
}

bool validateSubjectId(int subjectId, QString *errorMessage)
{
    return subjectId > 0
        || fail(errorMessage, QStringLiteral("A valid subject is required."));
}

bool validateDateRange(const QDate &startDate, const QDate &endDate,
                       QString *errorMessage)
{
    if (!startDate.isValid() || !endDate.isValid()) {
        return fail(errorMessage,
                    QStringLiteral("Valid attendance range dates are required."));
    }
    if (startDate > endDate) {
        return fail(errorMessage,
                    QStringLiteral("The attendance range start date must not be after its end date."));
    }
    return true;
}
}

AttendanceDAO::AttendanceDAO(DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

bool AttendanceDAO::insertAttendance(const Attendance &attendance,
                                     QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validIdentity(attendance, errorMessage, false)) {
        return false;
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    if (!subjectBelongsToUser(database, attendance.userId(), attendance.subjectId(),
                              errorMessage)) {
        return false;
    }
    bool duplicate = false;
    if (!duplicateExists(database, attendance.userId(), attendance.subjectId(),
                         attendance.date(), 0, &duplicate, errorMessage)) {
        return false;
    }
    if (duplicate) {
        return fail(errorMessage,
                    QStringLiteral("Attendance already exists for this subject and date."));
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "INSERT INTO attendance (user_id, subject_id, date, status) "
            "SELECT :user_id, :subject_id, :date, :status "
            "WHERE EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id)"))) {
        return queryFailed(errorMessage,
                           QStringLiteral("Could not prepare attendance insert"), query);
    }
    query.bindValue(QStringLiteral(":user_id"), attendance.userId());
    query.bindValue(QStringLiteral(":subject_id"), attendance.subjectId());
    query.bindValue(QStringLiteral(":date"),
                    Attendance::dateToString(attendance.date()));
    query.bindValue(QStringLiteral(":status"), statusToString(attendance.status()));
    if (!query.exec()) {
        return attendanceDuplicateWriteFailed(
            errorMessage, QStringLiteral("Could not add attendance"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage,
                    QStringLiteral("The subject does not belong to the specified user."));
    }
    return true;
}

Attendance AttendanceDAO::getAttendanceByUserSubjectAndDate(
    int userId, int subjectId, const QDate &date, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validateUserId(userId, errorMessage)
        || !validateSubjectId(subjectId, errorMessage)) {
        return {};
    }
    if (!date.isValid()) {
        fail(errorMessage, QStringLiteral("A valid attendance date is required."));
        return {};
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QSqlQuery query(database);
    if (!query.prepare(attendanceSelect()
                       + QStringLiteral("WHERE user_id = :user_id "
                                        "AND subject_id = :subject_id AND date = :date"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare attendance lookup"),
                    query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":date"), Attendance::dateToString(date));
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load attendance"), query);
        return {};
    }
    if (!query.next()) {
        fail(errorMessage, QStringLiteral("Attendance record not found."));
        return {};
    }
    return readAttendance(query);
}

QList<Attendance> AttendanceDAO::getAttendanceByUserId(
    int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validateUserId(userId, errorMessage)) {
        return {};
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }
    return getAttendance(database, QStringLiteral("WHERE user_id = :user_id"),
                         userId, 0, {}, {}, errorMessage);
}

QList<Attendance> AttendanceDAO::getAttendanceByUserIdAndSubjectId(
    int userId, int subjectId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validateUserId(userId, errorMessage)
        || !validateSubjectId(subjectId, errorMessage)) {
        return {};
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }
    return getAttendance(database,
                         QStringLiteral("WHERE user_id = :user_id "
                                        "AND subject_id = :subject_id"),
                         userId, subjectId, {}, {}, errorMessage);
}

QList<Attendance> AttendanceDAO::getAttendanceByUserIdInDateRange(
    int userId, const QDate &startDate, const QDate &endDate,
    QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validateUserId(userId, errorMessage)
        || !validateDateRange(startDate, endDate, errorMessage)) {
        return {};
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }
    return getAttendance(database,
                         QStringLiteral("WHERE user_id = :user_id "
                                        "AND date >= :start_date AND date <= :end_date"),
                         userId, 0, startDate, endDate, errorMessage);
}

QList<Attendance> AttendanceDAO::getAttendanceByUserIdAndSubjectIdInDateRange(
    int userId, int subjectId, const QDate &startDate, const QDate &endDate,
    QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validateUserId(userId, errorMessage)
        || !validateSubjectId(subjectId, errorMessage)
        || !validateDateRange(startDate, endDate, errorMessage)) {
        return {};
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }
    return getAttendance(
        database,
        QStringLiteral("WHERE user_id = :user_id AND subject_id = :subject_id "
                       "AND date >= :start_date AND date <= :end_date"),
        userId, subjectId, startDate, endDate, errorMessage);
}

bool AttendanceDAO::updateAttendance(int userId, const Attendance &attendance,
                                     QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validateUserId(userId, errorMessage)
        || !validIdentity(attendance, errorMessage, true)) {
        return false;
    }
    if (attendance.userId() != userId) {
        return fail(errorMessage,
                    QStringLiteral("The attendance record does not belong to the specified user."));
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    bool exists = false;
    if (!attendanceExists(database, userId, attendance.id(), &exists, errorMessage)) {
        return false;
    }
    if (!exists) {
        return fail(errorMessage, QStringLiteral("Attendance record not found."));
    }
    if (!subjectBelongsToUser(database, userId, attendance.subjectId(), errorMessage)) {
        return false;
    }
    bool duplicate = false;
    if (!duplicateExists(database, userId, attendance.subjectId(), attendance.date(),
                         attendance.id(), &duplicate, errorMessage)) {
        return false;
    }
    if (duplicate) {
        return fail(errorMessage,
                    QStringLiteral("Attendance already exists for this subject and date."));
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "UPDATE attendance SET subject_id = :subject_id, date = :date, "
            "status = :status WHERE id = :id AND user_id = :user_id "
            "AND EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id)"))) {
        return queryFailed(errorMessage,
                           QStringLiteral("Could not prepare attendance update"), query);
    }
    query.bindValue(QStringLiteral(":subject_id"), attendance.subjectId());
    query.bindValue(QStringLiteral(":date"),
                    Attendance::dateToString(attendance.date()));
    query.bindValue(QStringLiteral(":status"), statusToString(attendance.status()));
    query.bindValue(QStringLiteral(":id"), attendance.id());
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        return attendanceDuplicateWriteFailed(
            errorMessage, QStringLiteral("Could not update attendance"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage, QStringLiteral("Attendance record not found."));
    }
    return true;
}

bool AttendanceDAO::deleteAttendance(int userId, int attendanceId,
                                     QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validateUserId(userId, errorMessage) || attendanceId <= 0) {
        if (attendanceId <= 0 && errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("A valid attendance record is required.");
        }
        return false;
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "DELETE FROM attendance WHERE user_id = :user_id AND id = :id"))) {
        return queryFailed(errorMessage,
                           QStringLiteral("Could not prepare attendance deletion"), query);
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), attendanceId);
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not delete attendance"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage, QStringLiteral("Attendance record not found."));
    }
    return true;
}
