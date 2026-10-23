#include <QString>
#include <QList>
#include <QMap>

class NotesService {
public:
    virtual QList<Note> listNotes(const QString& userId) = 0;
    virtual QString createNote(const QString& title, const QString& content, const QString& subjectId, const QString& userId) = 0;
    virtual QString updateNote(const QString& id, const QString& title, const QString& content, const QString& subjectId, const QString& userId) = 0;
    virtual QString deleteNote(const QString& id) = 0;
};