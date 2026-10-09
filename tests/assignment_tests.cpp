#include <QtTest/QtTest>

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/database_manager.h"
#include "../src/data/assignment_dao.h"
#include "../src/models/assignment.h"

class AssignmentTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    DatabaseManager *m_databaseManager{nullptr};
    AssignmentDAO *m_assignmentDAO{nullptr};
    int m_firstUserId{0};
    int m_secondUserId{0};
    int m_firstSubjectId{0};
    int m_secondSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    Assignment makeAssignment(int userId, int subjectId,
                              const QString &title = QStringLiteral("Lab report"),
                              const QString &dueDate = QStringLiteral("2026-10-20"),
                              const QString &dueTime = QStringLiteral("14:30")) const;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void insertListLookupUpdateAndDelete();
    void filterBySubjectAndStatus();
    void isolateUsersAndRejectForeignSubject();
    void rejectInvalidIds();
    void preserveAssignmentsAfterReopen();
    void cascadeDeleteWithSubject();
    void reportUnavailableDatabase();
};

void AssignmentTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString connectionName = QStringLiteral("assignment_tests_%1")
                                       .arg(reinterpret_cast<quintptr>(this));
    const QString databasePath =
        m_temporaryDirectory.filePath(QStringLiteral("assignments.db"));
    m_databaseManager = new DatabaseManager(connectionName, databasePath);
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    m_assignmentDAO = new AssignmentDAO(m_databaseManager);
}

void AssignmentTests::init()
{
    QSqlQuery query(m_databaseManager->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM assignments")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM users")));

    m_firstUserId = insertUser(QStringLiteral("First User"),
                               QStringLiteral("first@example.test"));
    m_secondUserId = insertUser(QStringLiteral("Second User"),
                                QStringLiteral("second@example.test"));
    QVERIFY(m_firstUserId > 0);
    QVERIFY(m_secondUserId > 0);
    m_firstSubjectId = insertSubject(m_firstUserId, QStringLiteral("First Subject"));
    m_secondSubjectId = insertSubject(m_secondUserId, QStringLiteral("Second Subject"));
    QVERIFY(m_firstSubjectId > 0);
    QVERIFY(m_secondSubjectId > 0);
}

void AssignmentTests::cleanupTestCase()
{
    delete m_assignmentDAO;
    delete m_databaseManager;
    m_assignmentDAO = nullptr;
    m_databaseManager = nullptr;
}

int AssignmentTests::insertUser(const QString &name, const QString &email)
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
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

int AssignmentTests::insertSubject(int userId, const QString &name)
{
    QSqlQuery query(m_databaseManager->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO subjects (user_id, name, created_at) "
            "VALUES (:user_id, :name, :created_at)"))) {
        return 0;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

Assignment AssignmentTests::makeAssignment(int userId, int subjectId,
                                          const QString &title, const QString &dueDate,
                                          const QString &dueTime) const
{
    return Assignment(0, userId, subjectId, title,
                      QStringLiteral("Complete the assigned work"),
                      dueDate, dueTime, QStringLiteral("High"),
                      QStringLiteral("Not Started"),
                      QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
}

void AssignmentTests::insertListLookupUpdateAndDelete()
{
    QString errorMessage;
    Assignment assignment = makeAssignment(m_firstUserId, m_firstSubjectId);
    QVERIFY2(m_assignmentDAO->insertAssignment(assignment, &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(errorMessage.isEmpty());

    QList<Assignment> assignments =
        m_assignmentDAO->getAssignmentsByUserId(m_firstUserId, 0, QString(),
                                                &errorMessage);
    QCOMPARE(assignments.size(), 1);
    Assignment stored = assignments.first();
    QVERIFY(stored.id() > 0);
    QCOMPARE(stored.userId(), m_firstUserId);
    QCOMPARE(stored.subjectId(), m_firstSubjectId);
    QCOMPARE(stored.title(), QStringLiteral("Lab report"));
    QCOMPARE(stored.description(), QStringLiteral("Complete the assigned work"));
    QCOMPARE(stored.dueDate(), QStringLiteral("2026-10-20"));
    QCOMPARE(stored.dueTime(), QStringLiteral("14:30"));
    QCOMPARE(stored.priority(), QStringLiteral("High"));
    QCOMPARE(stored.status(), QStringLiteral("Not Started"));
    QVERIFY(!stored.createdAt().isEmpty());

    stored = m_assignmentDAO->getAssignmentById(m_firstUserId, stored.id(),
                                                &errorMessage);
    QCOMPARE(stored.title(), QStringLiteral("Lab report"));
    QCOMPARE(stored.dueTime(), QStringLiteral("14:30"));

    stored.setTitle(QStringLiteral("Revised lab report"));
    stored.setDescription(QStringLiteral("Updated instructions"));
    stored.setDueDate(QStringLiteral("2026-10-22"));
    stored.setDueTime(QString());
    stored.setPriority(QStringLiteral("Medium"));
    stored.setStatus(QStringLiteral("In Progress"));
    QVERIFY2(m_assignmentDAO->updateAssignment(stored, &errorMessage),
             qPrintable(errorMessage));
    const Assignment updated =
        m_assignmentDAO->getAssignmentById(m_firstUserId, stored.id(), &errorMessage);
    QCOMPARE(updated.title(), QStringLiteral("Revised lab report"));
    QCOMPARE(updated.description(), QStringLiteral("Updated instructions"));
    QCOMPARE(updated.dueDate(), QStringLiteral("2026-10-22"));
    QVERIFY(updated.dueTime().isEmpty());
    QCOMPARE(updated.priority(), QStringLiteral("Medium"));
    QCOMPARE(updated.status(), QStringLiteral("In Progress"));

    QVERIFY(m_assignmentDAO->deleteAssignment(m_firstUserId, stored.id(), &errorMessage));
    QVERIFY(m_assignmentDAO->getAssignmentsByUserId(
                m_firstUserId, 0, QString(), &errorMessage).isEmpty());
    QVERIFY(!m_assignmentDAO->deleteAssignment(m_firstUserId, stored.id(), &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("Assignment not found."));
}

void AssignmentTests::filterBySubjectAndStatus()
{
    QString errorMessage;
    Assignment first = makeAssignment(m_firstUserId, m_firstSubjectId,
                                     QStringLiteral("First"));
    Assignment second = makeAssignment(m_firstUserId, m_firstSubjectId,
                                       QStringLiteral("Second"),
                                       QStringLiteral("2026-10-21"),
                                       QStringLiteral("09:00"));
    second.setStatus(QStringLiteral("Completed"));
    Assignment third = makeAssignment(m_firstUserId, m_firstSubjectId,
                                      QStringLiteral("Third"),
                                      QStringLiteral("2026-10-19"),
                                      QStringLiteral("12:00"));
    third.setSubjectId(insertSubject(m_firstUserId, QStringLiteral("Other Subject")));
    QVERIFY(first.subjectId() > 0);
    QVERIFY(m_assignmentDAO->insertAssignment(first, &errorMessage));
    QVERIFY(m_assignmentDAO->insertAssignment(second, &errorMessage));
    QVERIFY(m_assignmentDAO->insertAssignment(third, &errorMessage));

    const QList<Assignment> statusFiltered =
        m_assignmentDAO->getAssignmentsByUserId(
            m_firstUserId, 0, QStringLiteral("Completed"), &errorMessage);
    QCOMPARE(statusFiltered.size(), 1);
    QCOMPARE(statusFiltered.first().title(), QStringLiteral("Second"));

    const QList<Assignment> subjectFiltered =
        m_assignmentDAO->getAssignmentsByUserId(
            m_firstUserId, m_firstSubjectId, QString(), &errorMessage);
    QCOMPARE(subjectFiltered.size(), 2);
    QCOMPARE(subjectFiltered.first().title(), QStringLiteral("First"));
    QCOMPARE(subjectFiltered.last().title(), QStringLiteral("Second"));
}

void AssignmentTests::isolateUsersAndRejectForeignSubject()
{
    QString errorMessage;
    Assignment first = makeAssignment(m_firstUserId, m_firstSubjectId);
    Assignment second = makeAssignment(m_secondUserId, m_secondSubjectId,
                                       QStringLiteral("Other user's assignment"));
    QVERIFY(m_assignmentDAO->insertAssignment(first, &errorMessage));
    QVERIFY(m_assignmentDAO->insertAssignment(second, &errorMessage));

    const QList<Assignment> firstUsersAssignments =
        m_assignmentDAO->getAssignmentsByUserId(
            m_firstUserId, 0, QString(), &errorMessage);
    const QList<Assignment> secondUsersAssignments =
        m_assignmentDAO->getAssignmentsByUserId(
            m_secondUserId, 0, QString(), &errorMessage);
    QCOMPARE(firstUsersAssignments.size(), 1);
    QCOMPARE(secondUsersAssignments.size(), 1);
    QVERIFY(m_assignmentDAO->getAssignmentById(
                m_firstUserId, secondUsersAssignments.first().id(), &errorMessage).id() == 0);

    Assignment foreignSubject =
        makeAssignment(m_firstUserId, m_secondSubjectId, QStringLiteral("Invalid owner"));
    QVERIFY(!m_assignmentDAO->insertAssignment(foreignSubject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("does not belong")));

    Assignment invalidUpdate = firstUsersAssignments.first();
    invalidUpdate.setSubjectId(m_secondSubjectId);
    QVERIFY(!m_assignmentDAO->updateAssignment(invalidUpdate, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject")));
    QCOMPARE(m_assignmentDAO->getAssignmentById(
                 m_firstUserId, invalidUpdate.id(), &errorMessage).subjectId(),
             m_firstSubjectId);

    QVERIFY(!m_assignmentDAO->deleteAssignment(
        m_firstUserId, secondUsersAssignments.first().id(), &errorMessage));
    QCOMPARE(m_assignmentDAO->getAssignmentById(
                 m_secondUserId, secondUsersAssignments.first().id(), &errorMessage).userId(),
             m_secondUserId);

    Assignment crossUserUpdate = firstUsersAssignments.first();
    crossUserUpdate.setUserId(m_secondUserId);
    QVERIFY(!m_assignmentDAO->updateAssignment(crossUserUpdate, &errorMessage));
    QCOMPARE(m_assignmentDAO->getAssignmentById(
                 m_firstUserId, firstUsersAssignments.first().id(), &errorMessage).userId(),
             m_firstUserId);
}

void AssignmentTests::rejectInvalidIds()
{
    QString errorMessage;
    QVERIFY(!m_assignmentDAO->insertAssignment(
        makeAssignment(0, m_firstSubjectId), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("user")));
    QVERIFY(!m_assignmentDAO->insertAssignment(
        makeAssignment(m_firstUserId, 0), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject")));

    QVERIFY(!m_assignmentDAO->getAssignmentsByUserId(
                 0, 0, QString(), &errorMessage).size());
    QVERIFY(errorMessage.contains(QStringLiteral("user")));
    QVERIFY(m_assignmentDAO->getAssignmentsByUserId(
                m_firstUserId, -1, QString(), &errorMessage).isEmpty());
    QVERIFY(errorMessage.contains(QStringLiteral("subject")));

    QVERIFY(m_assignmentDAO->getAssignmentById(0, 1, &errorMessage).id() == 0);
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(m_assignmentDAO->getAssignmentById(m_firstUserId, 0, &errorMessage).id() == 0);
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(!m_assignmentDAO->updateAssignment(
        makeAssignment(m_firstUserId, m_firstSubjectId), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("valid assignment")));
    QVERIFY(!m_assignmentDAO->deleteAssignment(m_firstUserId, 0, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());
}

void AssignmentTests::preserveAssignmentsAfterReopen()
{
    QString errorMessage;
    QVERIFY(m_assignmentDAO->insertAssignment(
        makeAssignment(m_firstUserId, m_firstSubjectId), &errorMessage));
    const Assignment original =
        m_assignmentDAO->getAssignmentsByUserId(
            m_firstUserId, 0, QString(), &errorMessage).first();

    m_databaseManager->close();
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    const Assignment restored =
        m_assignmentDAO->getAssignmentById(m_firstUserId, original.id(), &errorMessage);
    QCOMPARE(restored.subjectId(), m_firstSubjectId);
    QCOMPARE(restored.title(), QStringLiteral("Lab report"));
    QCOMPARE(restored.dueDate(), QStringLiteral("2026-10-20"));
    QCOMPARE(restored.dueTime(), QStringLiteral("14:30"));
}

void AssignmentTests::cascadeDeleteWithSubject()
{
    QString errorMessage;
    QVERIFY(m_assignmentDAO->insertAssignment(
        makeAssignment(m_firstUserId, m_firstSubjectId), &errorMessage));

    QSqlQuery query(m_databaseManager->database());
    query.prepare(QStringLiteral("DELETE FROM subjects WHERE id = :id AND user_id = :user_id"));
    query.bindValue(QStringLiteral(":id"), m_firstSubjectId);
    query.bindValue(QStringLiteral(":user_id"), m_firstUserId);
    QVERIFY(query.exec());
    QCOMPARE(query.numRowsAffected(), 1);

    const QList<Assignment> assignments =
        m_assignmentDAO->getAssignmentsByUserId(
            m_firstUserId, 0, QString(), &errorMessage);
    QVERIFY(assignments.isEmpty());
}

void AssignmentTests::reportUnavailableDatabase()
{
    m_databaseManager->close();
    QString errorMessage;
    QVERIFY(m_assignmentDAO->getAssignmentsByUserId(
                m_firstUserId, 0, QString(), &errorMessage).isEmpty());
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(m_assignmentDAO->getAssignmentById(m_firstUserId, 1, &errorMessage).id() == 0);
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(!m_assignmentDAO->insertAssignment(
        makeAssignment(m_firstUserId, m_firstSubjectId), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(!m_assignmentDAO->updateAssignment(
        Assignment(1, m_firstUserId, m_firstSubjectId, QStringLiteral("Title"),
                  QString(), QString(), QString(), QString(), QString(), QString()),
        &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(!m_assignmentDAO->deleteAssignment(m_firstUserId, 1, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(m_databaseManager->initialize());
}

QTEST_GUILESS_MAIN(AssignmentTests)
#include "assignment_tests.moc"
