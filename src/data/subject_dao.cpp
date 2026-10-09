#include "subject_dao.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "../core/database_manager.h"

namespace
{
QString subjectSelect()
{
    return QStringLiteral(
        "SELECT id, user_id, name, code, teacher, credits, semester, created_at "
        "FROM subjects ");
}

Subject readSubject(const QSqlQuery &query)
{
    return Subject(query.value(0).toInt(),
                   query.value(1).toInt(),
                   query.value(2).toString(),
                   query.value(3).toString(),
                   query.value(4).toString(),
                   query.value(5).toInt(),
                   query.value(6).toInt(),
                   query.value(7).toString());
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
    return fail(errorMessage, QStringLiteral("%1: %2").arg(operation, query.lastError().text()));
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
}

SubjectDAO::SubjectDAO(DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

bool SubjectDAO::insertSubject(const Subject &subject, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "INSERT INTO subjects "
            "(user_id, name, code, teacher, credits, semester, created_at) "
            "VALUES (:user_id, :name, :code, :teacher, :credits, :semester, :created_at)"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare subject insert"), query);
    }

    query.bindValue(QStringLiteral(":user_id"), subject.userId());
    query.bindValue(QStringLiteral(":name"), subject.name());
    query.bindValue(QStringLiteral(":code"), subject.code());
    query.bindValue(QStringLiteral(":teacher"), subject.teacher());
    query.bindValue(QStringLiteral(":credits"), subject.credits());
    query.bindValue(QStringLiteral(":semester"), subject.semester());
    query.bindValue(QStringLiteral(":created_at"), subject.createdAt());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not add subject"), query);
    }
    return true;
}

QList<Subject> SubjectDAO::getSubjectsByUserId(int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    QList<Subject> subjects;
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return subjects;
    }

    QSqlQuery query(database);
    if (!query.prepare(subjectSelect()
                       + QStringLiteral("WHERE user_id = :user_id "
                                        "ORDER BY name COLLATE NOCASE, id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare subject query"), query);
        return subjects;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load subjects"), query);
        return subjects;
    }
    while (query.next()) {
        subjects.append(readSubject(query));
    }
    return subjects;
}

Subject SubjectDAO::getSubjectById(int userId, int subjectId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QSqlQuery query(database);
    if (!query.prepare(subjectSelect()
                       + QStringLiteral("WHERE user_id = :user_id AND id = :id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare subject lookup"), query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), subjectId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load subject"), query);
        return {};
    }
    if (!query.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Subject not found.");
        }
        return {};
    }
    return readSubject(query);
}

bool SubjectDAO::updateSubject(const Subject &subject, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "UPDATE subjects SET name = :name, code = :code, teacher = :teacher, "
            "credits = :credits, semester = :semester "
            "WHERE user_id = :user_id AND id = :id"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare subject update"), query);
    }
    query.bindValue(QStringLiteral(":name"), subject.name());
    query.bindValue(QStringLiteral(":code"), subject.code());
    query.bindValue(QStringLiteral(":teacher"), subject.teacher());
    query.bindValue(QStringLiteral(":credits"), subject.credits());
    query.bindValue(QStringLiteral(":semester"), subject.semester());
    query.bindValue(QStringLiteral(":user_id"), subject.userId());
    query.bindValue(QStringLiteral(":id"), subject.id());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not update subject"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage, QStringLiteral("Subject not found."));
    }
    return true;
}

bool SubjectDAO::deleteSubject(int userId, int subjectId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "DELETE FROM subjects WHERE user_id = :user_id AND id = :id"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare subject deletion"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), subjectId);
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not delete subject"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage, QStringLiteral("Subject not found."));
    }
    return true;
}

QList<Subject> SubjectDAO::searchSubjects(int userId, const QString &searchTerm,
                                          QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    QList<Subject> subjects;
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return subjects;
    }

    QString escapedTerm = searchTerm;
    escapedTerm.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escapedTerm.replace(QLatin1Char('%'), QStringLiteral("\\%"));
    escapedTerm.replace(QLatin1Char('_'), QStringLiteral("\\_"));
    const QString pattern = QStringLiteral("%") + escapedTerm + QStringLiteral("%");

    QSqlQuery query(database);
    if (!query.prepare(subjectSelect()
                       + QStringLiteral("WHERE user_id = :user_id AND "
                                        "(name LIKE :term ESCAPE '\\' OR "
                                        "code LIKE :term ESCAPE '\\' OR "
                                        "teacher LIKE :term ESCAPE '\\') "
                                        "ORDER BY name COLLATE NOCASE, id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare subject search"), query);
        return subjects;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":term"), pattern);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not search subjects"), query);
        return subjects;
    }
    while (query.next()) {
        subjects.append(readSubject(query));
    }
    return subjects;
}
