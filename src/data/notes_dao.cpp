#include "notes_dao.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "../core/database_manager.h"
#include "../models/note.h"

namespace
{

QString noteSelect()
{
    return QStringLiteral(
        "SELECT id, user_id, subject_id, title, content, created_at, updated_at "
        "FROM notes ");
}

Note readNote(const QSqlQuery &query)
{
    return Note(query.value(0).toInt(),
                query.value(1).toInt(),
                query.value(2).toInt(),
                query.value(3).toString(),
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

bool queryFailed(QString *errorMessage, const QString &operation,
                 const QSqlQuery &query)
{
    return fail(errorMessage, QStringLiteral("%1: %2")
                    .arg(operation, query.lastError().text()));
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

bool validReferences(const Note &note, QString *errorMessage)
{
    if (note.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required."));
    }
    return true;
}

} // namespace

NotesDAO::NotesDAO(DatabaseManager *databaseManager)
    : m_databaseManager(databaseManager)
{
}

bool NotesDAO::insertNote(const Note &note, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (!validReferences(note, errorMessage)) {
        return false;
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "INSERT INTO notes (user_id, subject_id, title, content, created_at, updated_at) "
            "SELECT :user_id, :subject_id, :title, :content, :created_at, :updated_at "
            "WHERE EXISTS (SELECT 1 FROM users WHERE id = :user_id) "
            "AND (:subject_id IS NULL OR EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id))"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare note insert"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), note.userId());
    query.bindValue(QStringLiteral(":subject_id"),
                    note.subjectId() > 0 ? QVariant(note.subjectId()) : QVariant());
    query.bindValue(QStringLiteral(":title"), note.title());
    query.bindValue(QStringLiteral(":content"),
                    note.content().isEmpty() ? QVariant() : note.content());
    query.bindValue(QStringLiteral(":created_at"), note.createdAt());
    query.bindValue(QStringLiteral(":updated_at"), note.updatedAt());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not add note"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage,
                    QStringLiteral("User not found or subject does not belong "
                                   "to the specified user."));
    }
    return true;
}
QList<Note> NotesDAO::getNotesByUserId(int userId,
                                       QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    QList<Note> notes;
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return notes;
    }

    QSqlQuery query(database);
    if (!query.prepare(noteSelect() +
                       QStringLiteral("WHERE user_id = :user_id "
                                      "ORDER BY updated_at DESC, id DESC"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare note list"),
                    query);
        return notes;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load notes"), query);
        return notes;
    }
    while (query.next()) {
        notes.append(readNote(query));
    }
    return notes;
}

Note NotesDAO::getNoteById(int userId, int noteId,
                           QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || noteId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user and note are required.");
        }
        return {};
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return {};
    }

    QSqlQuery query(database);
    if (!query.prepare(noteSelect() +
                       QStringLiteral("WHERE user_id = :user_id AND id = :id"))) {
        queryFailed(errorMessage, QStringLiteral("Could not prepare note load"),
                    query);
        return {};
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), noteId);
    if (!query.exec()) {
        queryFailed(errorMessage, QStringLiteral("Could not load note"), query);
        return {};
    }
    if (!query.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Note not found.");
        }
        return {};
    }
    return readNote(query);
}

bool NotesDAO::updateNote(const Note &note, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (note.id() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid note is required."));
    }
    if (!validReferences(note, errorMessage)) {
        return false;
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "UPDATE notes SET subject_id = :subject_id, title = :title, "
            "content = :content, updated_at = :updated_at "
            "WHERE user_id = :user_id AND id = :id "
            "AND (:subject_id IS NULL OR EXISTS (SELECT 1 FROM subjects "
            "WHERE id = :subject_id AND user_id = :user_id))"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare note update"),
                           query);
    }
    query.bindValue(QStringLiteral(":subject_id"),
                    note.subjectId() > 0 ? QVariant(note.subjectId()) : QVariant());
    query.bindValue(QStringLiteral(":title"), note.title());
    query.bindValue(QStringLiteral(":content"),
                    note.content().isEmpty() ? QVariant() : note.content());
    query.bindValue(QStringLiteral(":updated_at"), note.updatedAt());
    query.bindValue(QStringLiteral(":user_id"), note.userId());
    query.bindValue(QStringLiteral(":id"), note.id());
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not update note"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage,
                    QStringLiteral("Note not found or subject does not belong "
                                   "to the specified user."));
    }
    return true;
}

bool NotesDAO::deleteNote(int userId, int noteId,
                          QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || noteId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and note are required."));
    }
    const QSqlDatabase database = openDatabase(m_databaseManager, errorMessage);
    if (!database.isValid() || !database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
            "DELETE FROM notes WHERE user_id = :user_id AND id = :id"))) {
        return queryFailed(errorMessage, QStringLiteral("Could not prepare note deletion"),
                           query);
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":id"), noteId);
    if (!query.exec()) {
        return queryFailed(errorMessage, QStringLiteral("Could not delete note"), query);
    }
    if (query.numRowsAffected() == 0) {
        return fail(errorMessage, QStringLiteral("Note not found."));
    }
    return true;
}