#pragma once

#include <QList>
#include <QString>

class NotesDAO;
class Note;

class NotesService {
public:
    explicit NotesService(NotesDAO *notesDAO);
    ~NotesService() = default;

    bool addNote(const Note &note, QString *errorMessage = nullptr) const;
    QList<Note> getNotes(int userId, QString *errorMessage = nullptr) const;
    Note getNote(int userId, int noteId, QString *errorMessage = nullptr) const;
    bool updateNote(const Note &note, QString *errorMessage = nullptr) const;
    bool deleteNote(int userId, int noteId, QString *errorMessage = nullptr) const;

private:
    static bool validateAndNormalize(Note &note, QString *errorMessage);

    NotesDAO *m_notesDAO;
};