#include "exam_dao.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "../core/database_manager.h"

namespace
{
QString examSelect()
{
    return QStringLiteral(
        "SELECT id, user_id, subject_id, title, exam_date, start_time, room "
        "FROM exams ");
}

Exam readExam(const QSqlQuery &query)
{
    return Exam(query.value(0).toInt(), query.value(1).toInt(),
                query.value(2).toInt(), query.value(3).toString(),
                query.value(4).toString(), query.value(5).toString(),
                query.value(6).toString());
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

QSqlDatabase openDatabase(DatabaseManager *databaseManager,
                          QString *errorMessage)
{
    if (!databaseManager || !databaseManager->isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The database is not available.");
        }
        return {};
    }
    return databaseManager->database();
}

bool validReferences(const Exam &exam, QString *errorMessage)
{
    if (exam.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required."));
    }
    if (exam.subjectId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid subject is required."));
    }
    return true;
}

QList<Exam> readExams(QSqlQuery &query, QString *errorMessage)
{
    QList<Exam> exams;
    while (query.next()) {
        exams.append(readExam(query));
    }
    if (query.lastError().isValid()) {
        queryFailed(errorMessage, QStringLiteral("Could not load exams"), query);
        exams.clear();
    }
    return exams;
}
}

ExamDAO::ExamDAO(DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

bool ExamDAO::insertExam(const Exam &exam, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validReferences(exam, errorMessage)) {
        return false;
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "INSERT INTO exams (user_id, subject_id, title, exam_date, start_time, room) "
            "SELECT :user_id, :subject_id, :title, :exam_date, :start_time, :room "
            "WHERE EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id)"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare exam insert"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), exam.userId());
    query.bindValue(QStringLiteral(":subject_id"), exam.subjectId());
    query.bindValue(QStringLiteral(":title"), exam.title());
    query.bindValue(QStringLiteral(":exam_date"), exam.examDate());
    query.bindValue(QStringLiteral(":start_time"),
                    exam.startTime().isEmpty() ? QVariant() : exam.startTime());
    query.bindValue(QStringLiteral(":room"),
                    exam.room().isEmpty() ? QVariant() : exam.room());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not add exam"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage,
                    QStringLiteral("The subject does not belong to the specified user."));
    }
    return true;
}

QList<Exam> ExamDAO::getExamsByUserId(int userId,
                                      QString *errorMessage) const
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
    if (!query.prepare(examSelect()
                       + QStringLiteral("WHERE user_id = :user_id "
                                        "ORDER BY exam_date, start_time, id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare exam query"), query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load exams"), query);
        return {};
    }
    return readExams(query, errorMessage);
}

Exam ExamDAO::getExamById(int userId, int examId,
                          QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || examId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user and exam are required.");
        }
        return {};
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QSqlQuery query(database);
    if (!query.prepare(examSelect()
                       + QStringLiteral("WHERE user_id = :user_id AND id = :id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare exam lookup"), query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), examId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load exam"), query);
        return {};
    }
    if (!query.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Exam not found.");
        }
        return {};
    }
    return readExam(query);
}

bool ExamDAO::updateExam(const Exam &exam, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (exam.id() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid exam is required."));
    }
    if (!validReferences(exam, errorMessage)) {
        return false;
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "UPDATE exams SET subject_id = :subject_id, title = :title, "
            "exam_date = :exam_date, start_time = :start_time, room = :room "
            "WHERE user_id = :user_id AND id = :id "
            "AND EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id)"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare exam update"),
                           query);
    }
    query.bindValue(QStringLiteral(":subject_id"), exam.subjectId());
    query.bindValue(QStringLiteral(":title"), exam.title());
    query.bindValue(QStringLiteral(":exam_date"), exam.examDate());
    query.bindValue(QStringLiteral(":start_time"),
                    exam.startTime().isEmpty() ? QVariant() : exam.startTime());
    query.bindValue(QStringLiteral(":room"),
                    exam.room().isEmpty() ? QVariant() : exam.room());
    query.bindValue(QStringLiteral(":user_id"), exam.userId());
    query.bindValue(QStringLiteral(":id"), exam.id());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not update exam"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage,
                    QStringLiteral("Exam not found or subject does not belong "
                                   "to the specified user."));
    }
    return true;
}

bool ExamDAO::deleteExam(int userId, int examId,
                         QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || examId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and exam are required."));
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "DELETE FROM exams WHERE user_id = :user_id AND id = :id"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare exam deletion"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), examId);
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not delete exam"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage, QStringLiteral("Exam not found."));
    }
    return true;
}
