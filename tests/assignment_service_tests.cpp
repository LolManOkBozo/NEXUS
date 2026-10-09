#include <QtTest/QtTest>

#include <QDate>
#include <QDateTime>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTime>
#include <QTimeZone>

#include "../src/core/database_manager.h"
#include "../src/data/assignment_dao.h"
#include "../src/data/subject_dao.h"
#include "../src/models/assignment.h"
#include "../src/services/assignment_service.h"

class AssignmentServiceTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    DatabaseManager *m_databaseManager{nullptr};
    AssignmentDAO *m_assignmentDAO{nullptr};
    SubjectDAO *m_subjectDAO{nullptr};
    AssignmentService *m_assignmentService{nullptr};
    int m_firstUserId{0};
    int m_secondUserId{0};
    int m_firstSubjectId{0};
    int m_secondSubjectId{0};
    int m_otherUsersSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    Assignment makeAssignment(int userId, int subjectId,
                              const QString &title = QStringLiteral("Lab report"),
                              const QString &dueDate = QStringLiteral("2026-10-20"),
                              const QString &dueTime = QString()) const;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void validateRequiredAndEnumeratedFields();
    void addUpdateDeleteAndRetrieve();
    void filterBySubjectAndStatus();
    void isolateUsersAndVerifySubjectOwnership();
    void sortDeadlinesAndBreakTiesById();
    void parseStrictDateAndTimeFormats();
    void classifyDeadlineBoundariesAndCompletion();
    void rejectMalformedStoredDeadlinesDuringClassification();
    void sortMalformedStoredDeadlinesLast();
    void reportUnavailableDatabase();
};

void AssignmentServiceTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString connectionName = QStringLiteral("assignment_service_tests_%1")
                                       .arg(reinterpret_cast<quintptr>(this));
    m_databaseManager = new DatabaseManager(
        connectionName, m_temporaryDirectory.filePath(QStringLiteral("assignments.db")));
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    m_assignmentDAO = new AssignmentDAO(m_databaseManager);
    m_subjectDAO = new SubjectDAO(m_databaseManager);
    m_assignmentService = new AssignmentService(m_assignmentDAO, m_subjectDAO);
}

void AssignmentServiceTests::init()
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
    m_secondSubjectId = insertSubject(m_firstUserId, QStringLiteral("Second Subject"));
    m_otherUsersSubjectId = insertSubject(m_secondUserId,
                                          QStringLiteral("Other User Subject"));
    QVERIFY(m_firstSubjectId > 0);
    QVERIFY(m_secondSubjectId > 0);
    QVERIFY(m_otherUsersSubjectId > 0);
}

void AssignmentServiceTests::cleanupTestCase()
{
    delete m_assignmentService;
    delete m_subjectDAO;
    delete m_assignmentDAO;
    delete m_databaseManager;
    m_assignmentService = nullptr;
    m_subjectDAO = nullptr;
    m_assignmentDAO = nullptr;
    m_databaseManager = nullptr;
}

int AssignmentServiceTests::insertUser(const QString &name, const QString &email)
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

int AssignmentServiceTests::insertSubject(int userId, const QString &name)
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

Assignment AssignmentServiceTests::makeAssignment(int userId, int subjectId,
                                                  const QString &title,
                                                  const QString &dueDate,
                                                  const QString &dueTime) const
{
    return Assignment(0, userId, subjectId, title,
                      QStringLiteral("Instructions"), dueDate, dueTime,
                      QStringLiteral("Medium"), QStringLiteral("Not Started"),
                      QString());
}

void AssignmentServiceTests::validateRequiredAndEnumeratedFields()
{
    QString errorMessage;
    Assignment invalid = makeAssignment(m_firstUserId, m_firstSubjectId,
                                        QStringLiteral(" \t "));
    QVERIFY(!m_assignmentService->addAssignment(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("title")));

    const QStringList invalidDates{
        QString(), QStringLiteral("2026-02-30"), QStringLiteral("2026-2-03"),
        QStringLiteral("2026-10-20T10:00"), QStringLiteral("not-a-date")
    };
    for (const QString &date : invalidDates) {
        invalid = makeAssignment(m_firstUserId, m_firstSubjectId,
                                 QStringLiteral("Title"), date);
        QVERIFY(!m_assignmentService->addAssignment(invalid, &errorMessage));
        QVERIFY(errorMessage.contains(QStringLiteral("due date")));
    }

    const QStringList invalidTimes{
        QStringLiteral("9:00"), QStringLiteral("24:00"), QStringLiteral("12:60"),
        QStringLiteral("10:00:00"), QStringLiteral("malformed")
    };
    for (const QString &time : invalidTimes) {
        invalid = makeAssignment(m_firstUserId, m_firstSubjectId);
        invalid.setDueTime(time);
        QVERIFY(!m_assignmentService->addAssignment(invalid, &errorMessage));
        QVERIFY(errorMessage.contains(QStringLiteral("due time")));
    }

    invalid = makeAssignment(m_firstUserId, m_firstSubjectId);
    invalid.setPriority(QStringLiteral("Urgent"));
    QVERIFY(!m_assignmentService->addAssignment(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("Priority")));
    invalid = makeAssignment(m_firstUserId, m_firstSubjectId);
    invalid.setStatus(QStringLiteral("Blocked"));
    QVERIFY(!m_assignmentService->addAssignment(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("Status")));

    for (const QString &priority : {QStringLiteral("Low"), QStringLiteral("Medium"),
                                    QStringLiteral("High")}) {
        for (const QString &status : {QStringLiteral("Not Started"),
                                      QStringLiteral("In Progress"),
                                      QStringLiteral("Completed")}) {
            Assignment valid = makeAssignment(
                m_firstUserId, m_firstSubjectId,
                priority + QLatin1Char(' ') + status);
            valid.setPriority(priority);
            valid.setStatus(status);
            QVERIFY2(m_assignmentService->addAssignment(valid, &errorMessage),
                     qPrintable(errorMessage));
        }
    }
    QCOMPARE(m_assignmentService->getAssignments(
                 m_firstUserId, 0, QString(), &errorMessage).size(),
             9);
}

void AssignmentServiceTests::addUpdateDeleteAndRetrieve()
{
    QString errorMessage;
    Assignment assignment = makeAssignment(
        m_firstUserId, m_firstSubjectId, QStringLiteral("  Lab report  "),
        QStringLiteral(" 2026-10-20 "), QStringLiteral(" 14:30 "));
    QVERIFY2(m_assignmentService->addAssignment(assignment, &errorMessage),
             qPrintable(errorMessage));

    QList<Assignment> assignments =
        m_assignmentService->getAssignments(m_firstUserId, 0, QString(),
                                            &errorMessage);
    QCOMPARE(assignments.size(), 1);
    Assignment stored = assignments.first();
    QVERIFY(stored.id() > 0);
    QCOMPARE(stored.title(), QStringLiteral("Lab report"));
    QCOMPARE(stored.dueDate(), QStringLiteral("2026-10-20"));
    QCOMPARE(stored.dueTime(), QStringLiteral("14:30"));
    QVERIFY(!stored.createdAt().isEmpty());
    QCOMPARE(m_assignmentService->getAssignment(
                 m_firstUserId, stored.id(), &errorMessage).id(),
             stored.id());

    stored.setTitle(QStringLiteral("Revised report"));
    stored.setDueDate(QStringLiteral("2026-10-22"));
    stored.setDueTime(QString());
    stored.setPriority(QStringLiteral("High"));
    stored.setStatus(QStringLiteral("In Progress"));
    QVERIFY2(m_assignmentService->updateAssignment(stored, &errorMessage),
             qPrintable(errorMessage));
    const Assignment updated =
        m_assignmentService->getAssignment(m_firstUserId, stored.id(), &errorMessage);
    QCOMPARE(updated.title(), QStringLiteral("Revised report"));
    QCOMPARE(updated.dueDate(), QStringLiteral("2026-10-22"));
    QVERIFY(updated.dueTime().isEmpty());
    QCOMPARE(updated.priority(), QStringLiteral("High"));
    QCOMPARE(updated.status(), QStringLiteral("In Progress"));

    QVERIFY(!m_assignmentService->updateAssignment(
        makeAssignment(m_firstUserId, m_firstSubjectId), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("assignment")));
    QVERIFY(!m_assignmentService->updateAssignment(
        Assignment(stored.id(), m_firstUserId, m_firstSubjectId,
                   QStringLiteral(" "), QString(), QStringLiteral("2026-10-20"),
                   QString(), QStringLiteral("Low"), QStringLiteral("Completed"),
                   stored.createdAt()),
        &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("title")));

    QVERIFY(m_assignmentService->deleteAssignment(m_firstUserId, stored.id(),
                                                  &errorMessage));
    QVERIFY(m_assignmentService->getAssignments(
                m_firstUserId, 0, QString(), &errorMessage).isEmpty());
    QVERIFY(!m_assignmentService->deleteAssignment(
        m_firstUserId, stored.id(), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("not found")));
}

void AssignmentServiceTests::filterBySubjectAndStatus()
{
    QString errorMessage;
    Assignment first = makeAssignment(m_firstUserId, m_firstSubjectId,
                                      QStringLiteral("First"),
                                      QStringLiteral("2026-10-20"));
    Assignment second = makeAssignment(m_firstUserId, m_firstSubjectId,
                                       QStringLiteral("Second"),
                                       QStringLiteral("2026-10-21"));
    second.setStatus(QStringLiteral("Completed"));
    Assignment third = makeAssignment(m_firstUserId, m_secondSubjectId,
                                      QStringLiteral("Third"),
                                      QStringLiteral("2026-10-22"));
    QVERIFY(m_assignmentService->addAssignment(first, &errorMessage));
    QVERIFY(m_assignmentService->addAssignment(second, &errorMessage));
    QVERIFY(m_assignmentService->addAssignment(third, &errorMessage));

    const QList<Assignment> subjectAssignments =
        m_assignmentService->getAssignments(
            m_firstUserId, m_firstSubjectId, QString(), &errorMessage);
    QCOMPARE(subjectAssignments.size(), 2);
    const QList<Assignment> completedAssignments =
        m_assignmentService->getAssignments(
            m_firstUserId, 0, QStringLiteral("Completed"), &errorMessage);
    QCOMPARE(completedAssignments.size(), 1);
    QCOMPARE(completedAssignments.first().title(), QStringLiteral("Second"));
    QVERIFY(m_assignmentService->getAssignments(
                m_firstUserId, 0, QStringLiteral("Invalid"), &errorMessage).isEmpty());
    QVERIFY(errorMessage.contains(QStringLiteral("Status filter")));
    QVERIFY(m_assignmentService->getAssignments(
                m_firstUserId, m_otherUsersSubjectId, QString(), &errorMessage).isEmpty());
    QVERIFY(errorMessage.contains(QStringLiteral("subject"), Qt::CaseInsensitive));
}

void AssignmentServiceTests::isolateUsersAndVerifySubjectOwnership()
{
    QString errorMessage;
    Assignment first = makeAssignment(m_firstUserId, m_firstSubjectId,
                                      QStringLiteral("First user"));
    Assignment second = makeAssignment(m_secondUserId, m_otherUsersSubjectId,
                                       QStringLiteral("Second user"));
    QVERIFY(m_assignmentService->addAssignment(first, &errorMessage));
    QVERIFY(m_assignmentService->addAssignment(second, &errorMessage));
    QCOMPARE(m_assignmentService->getAssignments(
                 m_firstUserId, 0, QString(), &errorMessage).size(),
             1);
    QCOMPARE(m_assignmentService->getAssignments(
                 m_secondUserId, 0, QString(), &errorMessage).size(),
             1);
    QVERIFY(m_assignmentService->getAssignment(
                m_firstUserId,
                m_assignmentService->getAssignments(
                    m_secondUserId, 0, QString(), &errorMessage).first().id(),
                &errorMessage).id() == 0);

    Assignment foreignSubject = makeAssignment(
        m_firstUserId, m_otherUsersSubjectId, QStringLiteral("Wrong owner"));
    QVERIFY(!m_assignmentService->addAssignment(foreignSubject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject"), Qt::CaseInsensitive));

    first = m_assignmentService->getAssignments(
        m_firstUserId, 0, QString(), &errorMessage).first();
    first.setSubjectId(m_otherUsersSubjectId);
    QVERIFY(!m_assignmentService->updateAssignment(first, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject"), Qt::CaseInsensitive));
    QCOMPARE(m_assignmentService->getAssignment(
                 m_firstUserId, first.id(), &errorMessage).subjectId(),
             m_firstSubjectId);

    QVERIFY(!m_assignmentService->deleteAssignment(
        m_firstUserId,
        m_assignmentService->getAssignments(
            m_secondUserId, 0, QString(), &errorMessage).first().id(),
        &errorMessage));
}

void AssignmentServiceTests::sortDeadlinesAndBreakTiesById()
{
    QString errorMessage;
    Assignment later = makeAssignment(m_firstUserId, m_firstSubjectId,
                                      QStringLiteral("Later"),
                                      QStringLiteral("2026-10-22"));
    Assignment tieFirst = makeAssignment(m_firstUserId, m_firstSubjectId,
                                         QStringLiteral("Tie first"),
                                         QStringLiteral("2026-10-21"),
                                         QStringLiteral("12:00"));
    Assignment earlier = makeAssignment(m_firstUserId, m_firstSubjectId,
                                        QStringLiteral("Earlier"),
                                        QStringLiteral("2026-10-21"),
                                        QStringLiteral("09:00"));
    Assignment tieSecond = makeAssignment(m_firstUserId, m_secondSubjectId,
                                          QStringLiteral("Tie second"),
                                          QStringLiteral("2026-10-21"),
                                          QStringLiteral("12:00"));
    QVERIFY(m_assignmentService->addAssignment(later, &errorMessage));
    QVERIFY(m_assignmentService->addAssignment(tieFirst, &errorMessage));
    QVERIFY(m_assignmentService->addAssignment(earlier, &errorMessage));
    QVERIFY(m_assignmentService->addAssignment(tieSecond, &errorMessage));

    const QList<Assignment> sorted =
        m_assignmentService->getAssignments(m_firstUserId, 0, QString(),
                                            &errorMessage);
    QCOMPARE(sorted.size(), 4);
    QCOMPARE(sorted.at(0).title(), QStringLiteral("Earlier"));
    QCOMPARE(sorted.at(1).title(), QStringLiteral("Tie first"));
    QCOMPARE(sorted.at(2).title(), QStringLiteral("Tie second"));
    QCOMPARE(sorted.at(3).title(), QStringLiteral("Later"));
    QVERIFY(sorted.at(1).id() < sorted.at(2).id());
}

void AssignmentServiceTests::parseStrictDateAndTimeFormats()
{
    Assignment assignment = makeAssignment(
        m_firstUserId, m_firstSubjectId, QStringLiteral("Date only"),
        QStringLiteral("2026-10-20"));
    QDateTime parsed = AssignmentService::parseDeadline(assignment);
    QVERIFY(parsed.isValid());
    QCOMPARE(parsed.date(), QDate(2026, 10, 20));
    QCOMPARE(parsed.time(), QTime(23, 59, 59, 999));

    assignment.setDueTime(QStringLiteral("08:05"));
    parsed = AssignmentService::parseDeadline(assignment);
    QVERIFY(parsed.isValid());
    QCOMPARE(parsed.date(), QDate(2026, 10, 20));
    QCOMPARE(parsed.time(), QTime(8, 5));

    assignment.setDueDate(QStringLiteral("2026-2-20"));
    QVERIFY(!AssignmentService::parseDeadline(assignment).isValid());
    assignment.setDueDate(QStringLiteral("2026-10-20"));
    assignment.setDueTime(QStringLiteral("8:05"));
    QVERIFY(!AssignmentService::parseDeadline(assignment).isValid());
}

void AssignmentServiceTests::classifyDeadlineBoundariesAndCompletion()
{
    const QDateTime reference(QDate(2026, 10, 20), QTime(12, 0),
                              QTimeZone::systemTimeZone());
    Assignment assignment = makeAssignment(
        m_firstUserId, m_firstSubjectId, QStringLiteral("Date only"),
        QStringLiteral("2026-10-20"));
    QCOMPARE(AssignmentService::classifyDeadline(assignment, reference),
             AssignmentService::DeadlineClassification::Upcoming);

    assignment.setDueTime(QStringLiteral("12:00"));
    QCOMPARE(AssignmentService::classifyDeadline(assignment, reference),
             AssignmentService::DeadlineClassification::Upcoming);
    assignment.setDueTime(QStringLiteral("11:59"));
    QCOMPARE(AssignmentService::classifyDeadline(assignment, reference),
             AssignmentService::DeadlineClassification::Overdue);
    assignment.setDueDate(QStringLiteral("2026-10-19"));
    assignment.setDueTime(QString());
    QCOMPARE(AssignmentService::classifyDeadline(assignment, reference),
             AssignmentService::DeadlineClassification::Overdue);

    assignment.setStatus(QStringLiteral("Completed"));
    assignment.setDueDate(QStringLiteral("malformed"));
    QCOMPARE(AssignmentService::classifyDeadline(assignment, reference),
             AssignmentService::DeadlineClassification::Completed);

    assignment.setStatus(QStringLiteral("In Progress"));
    QCOMPARE(AssignmentService::classifyDeadline(assignment, reference),
             AssignmentService::DeadlineClassification::Invalid);
    QVERIFY(!AssignmentService::parseDeadline(assignment).isValid());
}

void AssignmentServiceTests::rejectMalformedStoredDeadlinesDuringClassification()
{
    Assignment malformed = makeAssignment(
        m_firstUserId, m_firstSubjectId, QStringLiteral("Malformed stored date"),
        QStringLiteral("2026-02-30"));
    QVERIFY(!AssignmentService::parseDeadline(malformed).isValid());
    QCOMPARE(AssignmentService::classifyDeadline(
                 malformed, QDateTime(QDate(2026, 10, 20), QTime(12, 0),
                           QTimeZone::systemTimeZone())),
             AssignmentService::DeadlineClassification::Invalid);
}

void AssignmentServiceTests::sortMalformedStoredDeadlinesLast()
{
    QString errorMessage;
    QVERIFY(m_assignmentService->addAssignment(
        makeAssignment(m_firstUserId, m_firstSubjectId,
                       QStringLiteral("Valid deadline"),
                       QStringLiteral("2026-10-20")), &errorMessage));

    QSqlQuery query(m_databaseManager->database());
    QVERIFY(query.prepare(QStringLiteral(
        "INSERT INTO assignments "
        "(user_id, subject_id, title, deadline, priority, status, created_at) "
        "VALUES (:user_id, :subject_id, :title, :deadline, :priority, :status, :created_at)")));
    query.bindValue(QStringLiteral(":user_id"), m_firstUserId);
    query.bindValue(QStringLiteral(":subject_id"), m_firstSubjectId);
    query.bindValue(QStringLiteral(":title"), QStringLiteral("Malformed deadline"));
    query.bindValue(QStringLiteral(":deadline"), QStringLiteral("not-a-date"));
    query.bindValue(QStringLiteral(":priority"), QStringLiteral("Low"));
    query.bindValue(QStringLiteral(":status"), QStringLiteral("Not Started"));
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    QVERIFY(query.exec());

    const QList<Assignment> assignments =
        m_assignmentService->getAssignments(m_firstUserId, 0, QString(),
                                            &errorMessage);
    QCOMPARE(assignments.size(), 2);
    QCOMPARE(assignments.first().title(), QStringLiteral("Valid deadline"));
    QCOMPARE(assignments.last().title(), QStringLiteral("Malformed deadline"));
    QCOMPARE(AssignmentService::classifyDeadline(
                 assignments.last(),
                 QDateTime(QDate(2026, 10, 19), QTime(12, 0),
                           QTimeZone::systemTimeZone())),
             AssignmentService::DeadlineClassification::Invalid);
}

void AssignmentServiceTests::reportUnavailableDatabase()
{
    QString errorMessage;
    QVERIFY(m_assignmentService->addAssignment(
        makeAssignment(m_firstUserId, m_firstSubjectId), &errorMessage));
    Assignment stored = m_assignmentService->getAssignments(
        m_firstUserId, 0, QString(), &errorMessage).first();

    m_databaseManager->close();
    QVERIFY(m_assignmentService->getAssignments(
                m_firstUserId, 0, QString(), &errorMessage).isEmpty());
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(m_assignmentService->getAssignment(m_firstUserId, 1, &errorMessage).id() == 0);
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(!m_assignmentService->addAssignment(
        makeAssignment(m_firstUserId, m_firstSubjectId), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    stored.setTitle(QStringLiteral("Updated while unavailable"));
    QVERIFY(!m_assignmentService->updateAssignment(stored, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(!m_assignmentService->deleteAssignment(m_firstUserId, 1, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(m_databaseManager->initialize());
}

QTEST_GUILESS_MAIN(AssignmentServiceTests)
#include "assignment_service_tests.moc"
