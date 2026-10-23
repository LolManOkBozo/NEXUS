#include <QString>

class NotesDAO {
public:
    virtual QString createNote(const QString& title, const QString& content, const QString& subjectId, const QString& userId) = 0;
    virtual QString updateNote(const QString& id, const QString& title, const QString& content, const QString& subjectId, const QString& userId) = 0;
    virtual QString deleteNote(const QString& id) = 0;
    virtual QList<Note> listNotes(const QString& userId) = 0;
};