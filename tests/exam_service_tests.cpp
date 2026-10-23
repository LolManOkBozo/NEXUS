#include <QtTest/QtTest>

#include <QDateTime>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/database_manager.h"
#include "../src/data/exam_dao.h"
#include "../src/data/subject_dao.h"
#include "../src/models/exam.h"
#include "../src/services/exam_service.h"

class ExamServiceTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    DatabaseManager *m_databaseManager{nullptr};
    ExamDAO *m_examDAO{nullptr};
    SubjectDAO *m_subjectDAO{nullptr};
    ExamService *m_examService{nullptr};
    int m_firstUserId{0};
    int m_secondUserId{0};
    int m_firstSubjectId{0};
    int m_otherUsersSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    Exam makeExam(int userId, int subjectId,
                  const QString &title = QStringLiteral("Final Exam")) const;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void addRetrieveUpdateDelete();
    void isolateUsersAndSubjects();
    void validateRequiredFieldsAndFormats();
    void daoScopesEveryOperationToUser();
    void reportUnavailableDatabase();
};

void ExamServiceTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString connectionName = QStringLiteral("exam_service_tests_%1")
                                       .arg(reinterpret_cast<quintptr>(this));
    m_databaseManager = new DatabaseManager(
        connectionName, m_temporaryDirectory.filePath(QStringLiteral("exams.db")));
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    m_examDAO = new ExamDAO(m_databaseManager);
    m_subjectDAO = new SubjectDAO(m_databaseManager);
    m_examService = new ExamService(m_examDAO, m_subjectDAO);
}

void ExamServiceTests::init()
{
    QSqlQuery query(m_databaseManager->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM exams")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM users")));

    m_firstUserId = insertUser(QStringLiteral("First User"),
                               QStringLiteral("first-exam@example.test"));
    m_secondUserId = insertUser(QStringLiteral("Second User"),
                                QStringLiteral("second-exam@example.test"));
    QVERIFY(m_firstUserId > 0);
    QVERIFY(m_secondUserId > 0);
    m_firstSubjectId = insertSubject(m_firstUserId, QStringLiteral("First Subject"));
    m_otherUsersSubjectId = insertSubject(m_secondUserId,
                                          QStringLiteral("Other User Subject"));
    QVERIFY(m_firstSubjectId > 0);
    QVERIFY(m_otherUsersSubjectId > 0);
}

void ExamServiceTests::cleanupTestCase()
{
    delete m_examService;
    delete m_subjectDAO;
    delete m_examDAO;
    delete m_databaseManager;
    m_examService = nullptr;
    m_subjectDAO = nullptr;
    m_examDAO = nullptr;
    m_databaseManager = nullptr;
}

int ExamServiceTests::insertUser(const QString &name, const QString &email)
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

int ExamServiceTests::insertSubject(int userId, const QString &name)
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
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

Exam ExamServiceTests::makeExam(int userId, int subjectId,
                                const QString &title) const
{
    return Exam(0, userId, subjectId, title, QStringLiteral("2026-12-01"),
                QStringLiteral("09:30"), QStringLiteral("Room 4"));
}

void ExamServiceTests::addRetrieveUpdateDelete()
{
    QString errorMessage;
    Exam exam = makeExam(m_firstUserId, m_firstSubjectId,
                         QStringLiteral("  Final Exam  "));
    QVERIFY2(m_examService->addExam(exam, &errorMessage),
             qPrintable(errorMessage));

    QList<Exam> exams = m_examService->getExams(m_firstUserId, &errorMessage);
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
    QCOMPARE(exams.size(), 1);
    QCOMPARE(exams.first().title(), QStringLiteral("Final Exam"));
    QCOMPARE(exams.first().startTime(), QStringLiteral("09:30"));
    QCOMPARE(exams.first().room(), QStringLiteral("Room 4"));

    exam = exams.first();
    exam.setTitle(QStringLiteral("Updated Final"));
    exam.setExamDate(QStringLiteral("2026-12-05"));
    exam.setStartTime(QString());
    exam.setRoom(QStringLiteral(" "));
    QVERIFY2(m_examService->updateExam(exam, &errorMessage),
             qPrintable(errorMessage));
    const Exam updated = m_examService->getExam(
        m_firstUserId, exam.id(), &errorMessage);
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
    QCOMPARE(updated.title(), QStringLiteral("Updated Final"));
    QCOMPARE(updated.examDate(), QStringLiteral("2026-12-05"));
    QVERIFY(updated.startTime().isEmpty());
    QVERIFY(updated.room().isEmpty());

    QVERIFY(m_examService->deleteExam(m_firstUserId, exam.id(), &errorMessage));
    QVERIFY(m_examService->getExams(m_firstUserId, &errorMessage).isEmpty());
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(!m_examService->deleteExam(m_firstUserId, exam.id(), &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("Exam not found."));
}

void ExamServiceTests::isolateUsersAndSubjects()
{
    QString errorMessage;
    QVERIFY(m_examService->addExam(makeExam(m_firstUserId, m_firstSubjectId),
                                   &errorMessage));
    QVERIFY(m_examService->getExams(m_secondUserId, &errorMessage).isEmpty());
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(m_examService->getExams(0, &errorMessage).isEmpty());
    QVERIFY(!errorMessage.isEmpty());

    QVERIFY(!m_examService->addExam(
        makeExam(m_firstUserId, m_otherUsersSubjectId), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("Subject not found")));
    QVERIFY(m_examService->getExams(m_firstUserId, &errorMessage).size() == 1);

    Exam unauthorized = makeExam(m_secondUserId, m_otherUsersSubjectId);
    QVERIFY(m_examService->addExam(unauthorized, &errorMessage));
    const int unauthorizedId =
        m_examService->getExams(m_secondUserId, &errorMessage).first().id();
    QVERIFY(m_examService->getExam(m_firstUserId, unauthorizedId,
                                   &errorMessage).id() == 0);
    QCOMPARE(errorMessage, QStringLiteral("Exam not found."));
    QVERIFY(!m_examService->deleteExam(m_firstUserId, unauthorizedId,
                                       &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("Exam not found."));

    Exam changedOwner = makeExam(m_firstUserId, m_firstSubjectId);
    changedOwner.setId(unauthorizedId);
    changedOwner.setTitle(QStringLiteral("Tampered"));
    QVERIFY(!m_examService->updateExam(changedOwner, &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("Exam not found or subject does not belong "
                                         "to the specified user."));
}

void ExamServiceTests::validateRequiredFieldsAndFormats()
{
    QString errorMessage;
    Exam invalid = makeExam(m_firstUserId, m_firstSubjectId, QStringLiteral(" \t "));
    QVERIFY(!m_examService->addExam(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("title")));

    const QStringList invalidDates{
        QString(), QStringLiteral("2026-02-30"), QStringLiteral("2026-2-03"),
        QStringLiteral("2026-12-01T09:30"), QStringLiteral("not-a-date")
    };
    for (const QString &date : invalidDates) {
        invalid = makeExam(m_firstUserId, m_firstSubjectId);
        invalid.setExamDate(date);
        QVERIFY(!m_examService->addExam(invalid, &errorMessage));
        QVERIFY(errorMessage.contains(QStringLiteral("exam date")));
    }

    const QStringList invalidTimes{
        QStringLiteral("9:30"), QStringLiteral("24:00"),
        QStringLiteral("12:60"), QStringLiteral("09:30:00"),
        QStringLiteral("bad")
    };
    for (const QString &time : invalidTimes) {
        invalid = makeExam(m_firstUserId, m_firstSubjectId);
        invalid.setStartTime(time);
        QVERIFY(!m_examService->addExam(invalid, &errorMessage));
        QVERIFY(errorMessage.contains(QStringLiteral("exam time")));
    }

    invalid = makeExam(0, m_firstSubjectId);
    QVERIFY(!m_examService->addExam(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("user")));
    invalid = makeExam(m_firstUserId, 0);
    QVERIFY(!m_examService->addExam(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject")));
}

void ExamServiceTests::daoScopesEveryOperationToUser()
{
    QString errorMessage;
    const Exam exam = makeExam(m_firstUserId, m_firstSubjectId);
    QVERIFY(m_examDAO->insertExam(exam, &errorMessage));
    const QList<Exam> exams = m_examDAO->getExamsByUserId(m_firstUserId,
                                                          &errorMessage);
    QCOMPARE(exams.size(), 1);
    const int examId = exams.first().id();
    QVERIFY(m_examDAO->getExamById(m_secondUserId, examId, &errorMessage).id() == 0);
    QCOMPARE(errorMessage, QStringLiteral("Exam not found."));
    QVERIFY(!m_examDAO->deleteExam(m_secondUserId, examId, &errorMessage));
    QVERIFY(m_examDAO->getExamById(m_firstUserId, examId, &errorMessage).id() > 0);

    Exam invalidSubject = makeExam(m_firstUserId, m_otherUsersSubjectId);
    QVERIFY(!m_examDAO->insertExam(invalidSubject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("specified user")));
    invalidSubject.setId(examId);
    QVERIFY(!m_examDAO->updateExam(invalidSubject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("specified user")));
}

void ExamServiceTests::reportUnavailableDatabase()
{
    QString errorMessage;
    m_databaseManager->close();
    QVERIFY(m_examService->getExams(m_firstUserId, &errorMessage).isEmpty());
    QCOMPARE(errorMessage, QStringLiteral("The database is not available."));
    QVERIFY(!m_examService->addExam(makeExam(m_firstUserId, m_firstSubjectId),
                                    &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("The database is not available."));
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
}

QTEST_GUILESS_MAIN(ExamServiceTests)
#include "exam_service_tests.moc"
