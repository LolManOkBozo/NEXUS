#include <QtTest/QtTest>

#include <QDateTime>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/database_manager.h"
#include "../src/data/notes_dao.h"
#include "../src/data/subject_dao.h"
#include "../src/models/note.h"
#include "../src/models/subject.h"
#include "../src/services/notes_service.h"
#include "../src/services/subject_service.h"

class NotesServiceTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    DatabaseManager *m_databaseManager{nullptr};
    NotesDAO *m_notesDAO{nullptr};
    SubjectDAO *m_subjectDAO{nullptr};
    NotesService *m_notesService{nullptr};
    int m_firstUserId{0};
    int m_secondUserId{0};
    int m_firstSubjectId{0};
    int m_otherUsersSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    Note makeNote(int userId, int subjectId,
                  const QString &title = QStringLiteral("Note Title")) const;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void addRetrieveUpdateDelete();
    void isolateUsersAndSubjects();
    void validateRequiredFields();
    void daoScopesEveryOperationToUser();
    void reportUnavailableDatabase();
};

void NotesServiceTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString connectionName = QStringLiteral("notes_service_tests_%1")
                                       .arg(reinterpret_cast<quintptr>(this));
    m_databaseManager = new DatabaseManager(
        connectionName, m_temporaryDirectory.filePath(QStringLiteral("notes.db")));
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    m_notesDAO = new NotesDAO(m_databaseManager);
    m_subjectDAO = new SubjectDAO(m_databaseManager);
    m_notesService = new NotesService(m_notesDAO);
}

void NotesServiceTests::init()
{
    QSqlQuery query(m_databaseManager->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM notes")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM users")));

    m_firstUserId = insertUser(QStringLiteral("First User"),
                               QStringLiteral("first-notes@example.test"));
    m_secondUserId = insertUser(QStringLiteral("Second User"),
                                QStringLiteral("second-notes@example.test"));
    QVERIFY(m_firstUserId > 0);
    QVERIFY(m_secondUserId > 0);
    m_firstSubjectId = insertSubject(m_firstUserId, QStringLiteral("First Subject"));
    m_otherUsersSubjectId = insertSubject(m_secondUserId,
                                          QStringLiteral("Other User Subject"));
    QVERIFY(m_firstSubjectId > 0);
    QVERIFY(m_otherUsersSubjectId > 0);
}

void NotesServiceTests::cleanupTestCase()
{
    delete m_notesService;
    delete m_subjectDAO;
    delete m_notesDAO;
    delete m_databaseManager;
    m_notesService = nullptr;
    m_subjectDAO = nullptr;
    m_notesDAO = nullptr;
    m_databaseManager = nullptr;
}

int NotesServiceTests::insertUser(const QString &name, const QString &email)
{
    QSqlQuery query(m_databaseManager->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO users (name, email, created_at) "
            "VALUES (:name, :email, :created_at)"))) {
        return 0;
    }
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":email"), email);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

int NotesServiceTests::insertSubject(int userId, const QString &name)
{
    QSqlQuery query(m_databaseManager->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO subjects (user_id, name, code, teacher, credits, semester, created_at) "
            "VALUES (:user_id, :name, :code, :teacher, :credits, :semester, :created_at)"))) {
        return 0;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":code"), QStringLiteral("SUBJ"));
    query.bindValue(QStringLiteral(":teacher"), QStringLiteral("Staff"));
    query.bindValue(QStringLiteral(":credits"), 3);
    query.bindValue(QStringLiteral(":semester"), 1);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

Note NotesServiceTests::makeNote(int userId, int subjectId,
                                 const QString &title) const
{
    Note note;
    note.setUserId(userId);
    note.setSubjectId(subjectId);
    note.setTitle(title);
    note.setContent(QStringLiteral("Note content."));
    note.setCreatedAt(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    note.setUpdatedAt(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return note;
}

void NotesServiceTests::addRetrieveUpdateDelete()
{
    QString errorMessage;
    const Note note = makeNote(m_firstUserId, m_firstSubjectId);
    QVERIFY(m_notesService->addNote(note, &errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const QList<Note> notes = m_notesService->getNotes(m_firstUserId, &errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(notes.size(), 1);
    const int noteId = notes.first().id();
    QVERIFY(noteId > 0);
    QCOMPARE(notes.first().title(), QStringLiteral("Note Title"));
    QCOMPARE(notes.first().subjectId(), m_firstSubjectId);

    const Note retrieved = m_notesService->getNote(m_firstUserId, noteId,
                                                   &errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(retrieved.id(), noteId);
    QCOMPARE(retrieved.title(), QStringLiteral("Note Title"));

    Note updated = retrieved;
    updated.setTitle(QStringLiteral("Updated Note Title"));
    updated.setContent(QStringLiteral("Updated content."));
    QVERIFY(m_notesService->updateNote(updated, &errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const Note afterUpdate = m_notesService->getNote(m_firstUserId, noteId,
                                                     &errorMessage);
    QCOMPARE(afterUpdate.title(), QStringLiteral("Updated Note Title"));
    QCOMPARE(afterUpdate.content(), QStringLiteral("Updated content."));
    QVERIFY(!afterUpdate.updatedAt().isEmpty());

    QVERIFY(m_notesService->deleteNote(m_firstUserId, noteId, &errorMessage));
    QVERIFY(errorMessage.isEmpty());
    const QList<Note> remaining = m_notesService->getNotes(m_firstUserId,
                                                           &errorMessage);
    QVERIFY(remaining.isEmpty());
}

void NotesServiceTests::isolateUsersAndSubjects()
{
    QString errorMessage;
    const Note note = makeNote(m_firstUserId, m_firstSubjectId);
    QVERIFY(m_notesService->addNote(note, &errorMessage));

    const QList<Note> firstUserNotes =
        m_notesService->getNotes(m_firstUserId, &errorMessage);
    QCOMPARE(firstUserNotes.size(), 1);

    const QList<Note> secondUserNotes =
        m_notesService->getNotes(m_secondUserId, &errorMessage);
    QCOMPARE(secondUserNotes.size(), 0);

    const int noteId = firstUserNotes.first().id();
    QVERIFY(m_notesService->getNote(m_secondUserId, noteId, &errorMessage).id() == 0);
    QCOMPARE(errorMessage, QStringLiteral("Note not found."));
    QVERIFY(!m_notesService->deleteNote(m_secondUserId, noteId, &errorMessage));
}

void NotesServiceTests::validateRequiredFields()
{
    QString errorMessage;
    Note invalid = makeNote(m_firstUserId, m_firstSubjectId,
                            QStringLiteral("   \t"));
    QVERIFY(!m_notesService->addNote(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("title")));

    invalid = makeNote(0, m_firstSubjectId);
    QVERIFY(!m_notesService->addNote(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("user")));
}

void NotesServiceTests::daoScopesEveryOperationToUser()
{
    QString errorMessage;
    const Note note = makeNote(m_firstUserId, m_firstSubjectId);
    QVERIFY(m_notesDAO->insertNote(note, &errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const QList<Note> notes = m_notesDAO->getNotesByUserId(m_firstUserId,
                                                           &errorMessage);
    QCOMPARE(notes.size(), 1);
    const int noteId = notes.first().id();
    QVERIFY(m_notesDAO->getNoteById(m_secondUserId, noteId, &errorMessage).id() == 0);
    QCOMPARE(errorMessage, QStringLiteral("Note not found."));
    QVERIFY(!m_notesDAO->deleteNote(m_secondUserId, noteId, &errorMessage));
    QVERIFY(m_notesDAO->getNoteById(m_firstUserId, noteId, &errorMessage).id() > 0);

    Note invalidSubject = makeNote(m_firstUserId, m_otherUsersSubjectId);
    QVERIFY(!m_notesDAO->insertNote(invalidSubject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("specified user")));
    invalidSubject.setId(noteId);
    QVERIFY(!m_notesDAO->updateNote(invalidSubject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("specified user")));
}

void NotesServiceTests::reportUnavailableDatabase()
{
    QString errorMessage;
    m_databaseManager->close();
    QVERIFY(m_notesService->getNotes(m_firstUserId, &errorMessage).isEmpty());
    QCOMPARE(errorMessage, QStringLiteral("The database is not available."));
    QVERIFY(!m_notesService->addNote(makeNote(m_firstUserId, m_firstSubjectId),
                                     &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("The database is not available."));
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
}

QTEST_GUILESS_MAIN(NotesServiceTests)
#include "notes_service_tests.moc"