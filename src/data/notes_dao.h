#pragma once

#include <QList>
#include <QString>

class DatabaseManager;
class Note;

class NotesDAO {
public:
    explicit NotesDAO(DatabaseManager *databaseManager);
    ~NotesDAO() = default;

    bool insertNote(const Note &note, QString *errorMessage = nullptr) const;
    QList<Note> getNotesByUserId(int userId, QString *errorMessage = nullptr) const;
    Note getNoteById(int userId, int noteId, QString *errorMessage = nullptr) const;
    bool updateNote(const Note &note, QString *errorMessage = nullptr) const;
    bool deleteNote(int userId, int noteId, QString *errorMessage = nullptr) const;

private:
    DatabaseManager *m_databaseManager;
};