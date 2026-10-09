#include <QtTest/QtTest>

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/database_manager.h"
#include "../src/data/subject_dao.h"
#include "../src/models/subject.h"
#include "../src/services/subject_service.h"

class SubjectTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    DatabaseManager *m_databaseManager{nullptr};
    SubjectDAO *m_subjectDAO{nullptr};
    SubjectService *m_subjectService{nullptr};
    int m_firstUserId{0};
    int m_secondUserId{0};

    int insertUser(const QString &name, const QString &email);
    Subject makeSubject(int userId, const QString &name = QStringLiteral("Data Structures"),
                        const QString &code = QStringLiteral("CSIT-214"),
                        const QString &teacher = QStringLiteral("Alex Sharma")) const;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void insertAndRetrieveSubject();
    void updateSubject();
    void deleteSubject();
    void searchSubjects();
    void isolateUsers();
    void rejectInvalidUserForeignKey();
    void preserveSubjectsAfterReopen();
    void validateAndNormalizeSubject();
    void reportUnavailableDatabase();
};

void SubjectTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString connectionName = QStringLiteral("subject_tests_%1")
                                       .arg(reinterpret_cast<quintptr>(this));
    const QString databasePath = m_temporaryDirectory.filePath(QStringLiteral("subjects.db"));
    m_databaseManager = new DatabaseManager(connectionName, databasePath);
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    m_subjectDAO = new SubjectDAO(m_databaseManager);
    m_subjectService = new SubjectService(m_subjectDAO);
}

void SubjectTests::init()
{
    QSqlQuery query(m_databaseManager->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM users")));
    m_firstUserId = insertUser(QStringLiteral("First Test User"),
                               QStringLiteral("first@example.test"));
    m_secondUserId = insertUser(QStringLiteral("Second Test User"),
                                QStringLiteral("second@example.test"));
    QVERIFY(m_firstUserId > 0);
    QVERIFY(m_secondUserId > 0);
}

void SubjectTests::cleanupTestCase()
{
    delete m_subjectService;
    delete m_subjectDAO;
    delete m_databaseManager;
    m_subjectService = nullptr;
    m_subjectDAO = nullptr;
    m_databaseManager = nullptr;
}

int SubjectTests::insertUser(const QString &name, const QString &email)
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

Subject SubjectTests::makeSubject(int userId, const QString &name, const QString &code,
                                  const QString &teacher) const
{
    Subject subject;
    subject.setUserId(userId);
    subject.setName(name);
    subject.setCode(code);
    subject.setTeacher(teacher);
    subject.setCredits(3);
    subject.setSemester(2);
    return subject;
}

void SubjectTests::insertAndRetrieveSubject()
{
    QString errorMessage;
    const Subject subject = makeSubject(m_firstUserId);
    QVERIFY2(m_subjectService->addSubject(subject, &errorMessage), qPrintable(errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const QList<Subject> subjects = m_subjectService->getSubjects(m_firstUserId, &errorMessage);
    QCOMPARE(subjects.size(), 1);
    QCOMPARE(subjects.first().name(), QStringLiteral("Data Structures"));
    QCOMPARE(subjects.first().code(), QStringLiteral("CSIT-214"));
    QCOMPARE(subjects.first().teacher(), QStringLiteral("Alex Sharma"));
    QCOMPARE(subjects.first().credits(), 3);
    QCOMPARE(subjects.first().semester(), 2);
    QVERIFY(!subjects.first().createdAt().isEmpty());

    const Subject retrieved = m_subjectService->getSubject(m_firstUserId, subjects.first().id(),
                                                           &errorMessage);
    QCOMPARE(retrieved.id(), subjects.first().id());
    QCOMPARE(retrieved.userId(), m_firstUserId);
}

void SubjectTests::updateSubject()
{
    QString errorMessage;
    QVERIFY(m_subjectService->addSubject(makeSubject(m_firstUserId), &errorMessage));
    Subject subject = m_subjectService->getSubjects(m_firstUserId).first();
    const QString createdAt = subject.createdAt();
    subject.setName(QStringLiteral("  Advanced Data Structures  "));
    subject.setCode(QStringLiteral("  CSIT-315 "));
    subject.setTeacher(QStringLiteral("  Priya  "));
    subject.setCredits(4);
    subject.setSemester(3);
    QVERIFY2(m_subjectService->updateSubject(subject, &errorMessage), qPrintable(errorMessage));

    const Subject updated = m_subjectService->getSubject(m_firstUserId, subject.id(),
                                                         &errorMessage);
    QCOMPARE(updated.name(), QStringLiteral("Advanced Data Structures"));
    QCOMPARE(updated.code(), QStringLiteral("CSIT-315"));
    QCOMPARE(updated.teacher(), QStringLiteral("Priya"));
    QCOMPARE(updated.credits(), 4);
    QCOMPARE(updated.semester(), 3);
    QCOMPARE(updated.createdAt(), createdAt);
}

void SubjectTests::deleteSubject()
{
    QString errorMessage;
    QVERIFY(m_subjectService->addSubject(makeSubject(m_firstUserId), &errorMessage));
    const int subjectId = m_subjectService->getSubjects(m_firstUserId).first().id();

    QVERIFY2(m_subjectService->deleteSubject(m_firstUserId, subjectId, &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(m_subjectService->getSubjects(m_firstUserId).isEmpty());
    QVERIFY(!m_subjectService->deleteSubject(m_firstUserId, subjectId, &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("Subject not found."));
}

void SubjectTests::searchSubjects()
{
    QString errorMessage;
    QVERIFY(m_subjectService->addSubject(
        makeSubject(m_firstUserId, QStringLiteral("Advanced Algorithms"),
                    QStringLiteral("CSIT-330"), QStringLiteral("Rita Karki")),
        &errorMessage));
    QVERIFY(m_subjectService->addSubject(
        makeSubject(m_firstUserId, QStringLiteral("100% Coverage"),
                    QStringLiteral("CSIT-331"), QStringLiteral("Samir")),
        &errorMessage));
    QVERIFY(m_subjectService->addSubject(
        makeSubject(m_secondUserId, QStringLiteral("Advanced Algebra"),
                    QStringLiteral("MATH-201"), QStringLiteral("Rita Karki")),
        &errorMessage));

    QCOMPARE(m_subjectService->searchSubjects(m_firstUserId, QStringLiteral("advanced"),
                                              &errorMessage).size(), 1);
    QCOMPARE(m_subjectService->searchSubjects(m_firstUserId, QStringLiteral("CSIT-330"),
                                              &errorMessage).size(), 1);
    QCOMPARE(m_subjectService->searchSubjects(m_firstUserId, QStringLiteral("Rita Karki"),
                                              &errorMessage).size(), 1);
    const QList<Subject> literalPercentMatches =
        m_subjectService->searchSubjects(m_firstUserId, QStringLiteral("%"), &errorMessage);
    QCOMPARE(literalPercentMatches.size(), 1);
    QCOMPARE(literalPercentMatches.first().name(), QStringLiteral("100% Coverage"));
    QVERIFY(errorMessage.isEmpty());
}

void SubjectTests::isolateUsers()
{
    QString errorMessage;
    QVERIFY(m_subjectService->addSubject(makeSubject(m_firstUserId), &errorMessage));
    QVERIFY(m_subjectService->addSubject(
        makeSubject(m_secondUserId, QStringLiteral("Other User Subject"),
                    QStringLiteral("OTHER-101")),
        &errorMessage));

    const QList<Subject> firstUsersSubjects = m_subjectService->getSubjects(m_firstUserId);
    const QList<Subject> secondUsersSubjects = m_subjectService->getSubjects(m_secondUserId);
    QCOMPARE(firstUsersSubjects.size(), 1);
    QCOMPARE(secondUsersSubjects.size(), 1);

    const int otherUsersSubjectId = secondUsersSubjects.first().id();
    QVERIFY(m_subjectService->getSubject(m_firstUserId, otherUsersSubjectId,
                                         &errorMessage).id() == 0);
    Subject unauthorizedUpdate = secondUsersSubjects.first();
    unauthorizedUpdate.setUserId(m_firstUserId);
    unauthorizedUpdate.setName(QStringLiteral("Should Not Update"));
    QVERIFY(!m_subjectService->updateSubject(unauthorizedUpdate, &errorMessage));
    QVERIFY(!m_subjectService->deleteSubject(m_firstUserId, otherUsersSubjectId, &errorMessage));
    QCOMPARE(m_subjectService->getSubject(m_secondUserId, otherUsersSubjectId).name(),
             QStringLiteral("Other User Subject"));
}

void SubjectTests::rejectInvalidUserForeignKey()
{
    QString errorMessage;
    const Subject subject = makeSubject(999999);
    QVERIFY(!m_subjectDAO->insertSubject(subject, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());
}

void SubjectTests::preserveSubjectsAfterReopen()
{
    QString errorMessage;
    QVERIFY(m_subjectService->addSubject(makeSubject(m_firstUserId), &errorMessage));
    const int subjectId = m_subjectService->getSubjects(m_firstUserId).first().id();

    m_databaseManager->close();
    QVERIFY2(m_databaseManager->initialize(), qPrintable(m_databaseManager->lastError()));
    const Subject restored = m_subjectService->getSubject(m_firstUserId, subjectId, &errorMessage);
    QCOMPARE(restored.name(), QStringLiteral("Data Structures"));
    QCOMPARE(restored.code(), QStringLiteral("CSIT-214"));
}

void SubjectTests::validateAndNormalizeSubject()
{
    QString errorMessage;
    Subject subject = makeSubject(m_firstUserId, QStringLiteral("  Operating Systems "),
                                  QStringLiteral(" OS-301  "), QStringLiteral("  "));
    QVERIFY2(m_subjectService->addSubject(subject, &errorMessage), qPrintable(errorMessage));
    const Subject stored = m_subjectService->getSubjects(m_firstUserId).first();
    QCOMPARE(stored.name(), QStringLiteral("Operating Systems"));
    QCOMPARE(stored.code(), QStringLiteral("OS-301"));
    QVERIFY(stored.teacher().isEmpty());

    subject.setName(QStringLiteral("   "));
    QVERIFY(!m_subjectService->addSubject(subject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("name")));

    subject.setName(QStringLiteral("Valid Name"));
    subject.setCode(QStringLiteral("   "));
    QVERIFY(!m_subjectService->addSubject(subject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("code")));

    subject.setCode(QStringLiteral("CODE"));
    subject.setCredits(31);
    QVERIFY(!m_subjectService->addSubject(subject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("Credits")));

    subject.setCredits(3);
    subject.setSemester(13);
    QVERIFY(!m_subjectService->addSubject(subject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("Semester")));
}

void SubjectTests::reportUnavailableDatabase()
{
    m_databaseManager->close();
    QString errorMessage;
    QVERIFY(m_subjectService->getSubjects(m_firstUserId, &errorMessage).isEmpty());
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(m_databaseManager->initialize());
}

QTEST_GUILESS_MAIN(SubjectTests)
#include "subject_tests.moc"
