#include "assignment_dao.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "../core/database_manager.h"

namespace
{
QString assignmentSelect()
{
    return QStringLiteral(
        "SELECT id, user_id, subject_id, title, description, deadline, "
        "priority, status, created_at FROM assignments ");
}

Assignment readAssignment(const QSqlQuery &query)
{
    const QString deadline = query.value(5).toString();
    const qsizetype timeSeparator = deadline.indexOf(QLatin1Char('T'));
    const QString dueDate = timeSeparator >= 0 ? deadline.left(timeSeparator) : deadline;
    const QString dueTime = timeSeparator >= 0 ? deadline.mid(timeSeparator + 1) : QString();
    return Assignment(query.value(0).toInt(),
                      query.value(1).toInt(),
                      query.value(2).toInt(),
                      query.value(3).toString(),
                      query.value(4).toString(),
                      dueDate,
                      dueTime,
                      query.value(6).toString(),
                      query.value(7).toString(),
                      query.value(8).toString());
}

QString assignmentDeadline(const Assignment &assignment)
{
    if (assignment.dueTime().isEmpty()) {
        return assignment.dueDate();
    }
    return assignment.dueDate() + QLatin1Char('T') + assignment.dueTime();
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

bool validAssignmentReferences(const Assignment &assignment, QString *errorMessage)
{
    if (assignment.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required."));
    }
    if (assignment.subjectId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid subject is required."));
    }
    return true;
}

QList<Assignment> readAssignments(QSqlQuery &query, QString *errorMessage)
{
    QList<Assignment> assignments;
    while (query.next()) {
        assignments.append(readAssignment(query));
    }
    if (query.lastError().isValid()) {
        queryFailed(errorMessage, QStringLiteral("Could not load assignments"), query);
        assignments.clear();
    }
    return assignments;
}
}

AssignmentDAO::AssignmentDAO(DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

bool AssignmentDAO::insertAssignment(const Assignment &assignment,
                                     QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validAssignmentReferences(assignment, errorMessage)) {
        return false;
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "INSERT INTO assignments "
            "(user_id, subject_id, title, description, deadline, priority, status, created_at) "
            "SELECT :user_id, :subject_id, :title, :description, :deadline, "
            ":priority, :status, :created_at "
            "WHERE EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id)"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare assignment insert"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), assignment.userId());
    query.bindValue(QStringLiteral(":subject_id"), assignment.subjectId());
    query.bindValue(QStringLiteral(":title"), assignment.title());
    query.bindValue(QStringLiteral(":description"), assignment.description());
    query.bindValue(QStringLiteral(":deadline"), assignmentDeadline(assignment));
    query.bindValue(QStringLiteral(":priority"), assignment.priority());
    query.bindValue(QStringLiteral(":status"), assignment.status());
    query.bindValue(QStringLiteral(":created_at"), assignment.createdAt());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not add assignment"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage,
                    QStringLiteral("The subject does not belong to the specified user."));
    }
    return true;
}

QList<Assignment> AssignmentDAO::getAssignmentsByUserId(
    int userId, int subjectId, const QString &status, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        fail(errorMessage, QStringLiteral("A valid user is required."));
        return {};
    }
    if (subjectId < 0) {
        fail(errorMessage, QStringLiteral("A valid subject filter is required."));
        return {};
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QString sql = assignmentSelect() + QStringLiteral("WHERE user_id = :user_id ");
    if (subjectId > 0) {
        sql += QStringLiteral("AND subject_id = :subject_id ");
    }
    if (!status.isEmpty()) {
        sql += QStringLiteral("AND status = :status ");
    }
    sql += QStringLiteral("ORDER BY deadline IS NULL, deadline, id");

    QSqlQuery query(database);
    if (!query.prepare(sql)) {
        fail(errorMessage, QStringLiteral("Could not prepare assignment query: %1")
                               .arg(query.lastError().text()));
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (subjectId > 0) {
        query.bindValue(QStringLiteral(":subject_id"), subjectId);
    }
    if (!status.isEmpty()) {
        query.bindValue(QStringLiteral(":status"), status);
    }
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load assignments"), query);
        return {};
    }
    return readAssignments(query, errorMessage);
}

Assignment AssignmentDAO::getAssignmentById(int userId, int assignmentId,
                                            QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || assignmentId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user and assignment are required.");
        }
        return {};
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QSqlQuery query(database);
    if (!query.prepare(assignmentSelect()
                       + QStringLiteral("WHERE user_id = :user_id AND id = :id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare assignment lookup"), query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), assignmentId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load assignment"), query);
        return {};
    }
    if (!query.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Assignment not found.");
        }
        return {};
    }
    return readAssignment(query);
}

bool AssignmentDAO::updateAssignment(const Assignment &assignment,
                                     QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (assignment.id() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid assignment is required."));
    }
    if (!validAssignmentReferences(assignment, errorMessage)) {
        return false;
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "UPDATE assignments SET subject_id = :subject_id, title = :title, "
            "description = :description, deadline = :deadline, priority = :priority, "
            "status = :status WHERE user_id = :user_id AND id = :id "
            "AND EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id)"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare assignment update"),
                           query);
    }
    query.bindValue(QStringLiteral(":subject_id"), assignment.subjectId());
    query.bindValue(QStringLiteral(":title"), assignment.title());
    query.bindValue(QStringLiteral(":description"), assignment.description());
    query.bindValue(QStringLiteral(":deadline"), assignmentDeadline(assignment));
    query.bindValue(QStringLiteral(":priority"), assignment.priority());
    query.bindValue(QStringLiteral(":status"), assignment.status());
    query.bindValue(QStringLiteral(":user_id"), assignment.userId());
    query.bindValue(QStringLiteral(":id"), assignment.id());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not update assignment"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage,
                    QStringLiteral("Assignment not found or subject does not belong "
                                   "to the specified user."));
    }
    return true;
}

bool AssignmentDAO::deleteAssignment(int userId, int assignmentId,
                                     QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || assignmentId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and assignment are required."));
    }

    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "DELETE FROM assignments WHERE user_id = :user_id AND id = :id"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare assignment deletion"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), assignmentId);
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not delete assignment"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage, QStringLiteral("Assignment not found."));
    }
    return true;
}
