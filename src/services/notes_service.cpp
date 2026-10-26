#include "notes_service.h"

#include <QDateTime>

#include "../data/notes_dao.h"
#include "../models/note.h"

namespace
{

bool fail(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

} // namespace

NotesService::NotesService(NotesDAO *notesDAO)
    : m_notesDAO(notesDAO)
{
}

bool NotesService::validateAndNormalize(Note &note, QString *errorMessage)
{
    note.setTitle(note.title().trimmed());
    note.setContent(note.content().trimmed());

    if (note.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required for this note."));
    }
    if (note.title().isEmpty()) {
        return fail(errorMessage, QStringLiteral("Enter a note title."));
    }
    return true;
}

bool NotesService::addNote(const Note &note, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Note normalized = note;
    if (!validateAndNormalize(normalized, errorMessage)) {
        return false;
    }
    if (!m_notesDAO) {
        return fail(errorMessage, QStringLiteral("The note data service is unavailable."));
    }
    if (normalized.createdAt().isEmpty()) {
        normalized.setCreatedAt(
            QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }
    normalized.setUpdatedAt(normalized.createdAt());
    return m_notesDAO->insertNote(normalized, errorMessage);
}

QList<Note> NotesService::getNotes(int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user is required to load notes.");
        }
        return {};
    }
    if (!m_notesDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The note data service is unavailable.");
        }
        return {};
    }
    return m_notesDAO->getNotesByUserId(userId, errorMessage);
}

Note NotesService::getNote(int userId, int noteId,
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
    if (!m_notesDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The note data service is unavailable.");
        }
        return {};
    }
    return m_notesDAO->getNoteById(userId, noteId, errorMessage);
}

bool NotesService::updateNote(const Note &note, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Note normalized = note;
    if (normalized.id() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid note is required to update it."));
    }
    if (!validateAndNormalize(normalized, errorMessage)) {
        return false;
    }
    if (!m_notesDAO) {
        return fail(errorMessage, QStringLiteral("The note data service is unavailable."));
    }
    normalized.setUpdatedAt(
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return m_notesDAO->updateNote(normalized, errorMessage);
}

bool NotesService::deleteNote(int userId, int noteId,
                              QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || noteId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and note are required."));
    }
    if (!m_notesDAO) {
        return fail(errorMessage, QStringLiteral("The note data service is unavailable."));
    }
    return m_notesDAO->deleteNote(userId, noteId, errorMessage);
}