#include <QtTest/QtTest>

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/database_manager.h"
#include "../src/data/attendance_dao.h"

class AttendanceDAOTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    DatabaseManager *m_databaseManager{nullptr};
    AttendanceDAO *m_attendanceDAO{nullptr};
    int m_firstUserId{0};
    int m_secondUserId{0};
    int m_firstSubjectId{0};
    int m_secondSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    Attendance makeAttendance(int userId, int subjectId, const QDate &date,
                              Attendance::Status status = Attendance::Status::Present) const;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void insertFindUpdateDeleteAndPersistence();
    void rejectDuplicateInsertAndUpdate();
    void reportConcurrentDuplicateInsertClearly();
    void scopeEveryOperationToUser();
    void rejectInvalidIdsDatesAndRanges();
    void retrieveInclusiveDateRangesAndStableOrder();
    void returnEmptyResultsAndCascadeAfterSubjectDeletion();
    void reportDatabaseAndQueryErrors();
};

void AttendanceDAOTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString connectionName = QStringLiteral("attendance_dao_tests_%1")
                                       .arg(reinterpret_cast<quintptr>(this));
    m_databaseManager = new DatabaseManager(
        connectionName, m_temporaryDirectory.filePath(QStringLiteral("attendance.db")));
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    m_attendanceDAO = new AttendanceDAO(m_databaseManager);
}

void AttendanceDAOTests::init()
{
    QSqlQuery query(m_databaseManager->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM attendance")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM users")));

    m_firstUserId = insertUser(QStringLiteral("First User"),
                               QStringLiteral("attendance-first@example.test"));
    m_secondUserId = insertUser(QStringLiteral("Second User"),
                                QStringLiteral("attendance-second@example.test"));
    QVERIFY(m_firstUserId > 0);
    QVERIFY(m_secondUserId > 0);
    m_firstSubjectId = insertSubject(m_firstUserId, QStringLiteral("First Subject"));
    m_secondSubjectId = insertSubject(m_secondUserId, QStringLiteral("Second Subject"));
    QVERIFY(m_firstSubjectId > 0);
    QVERIFY(m_secondSubjectId > 0);
}

void AttendanceDAOTests::cleanupTestCase()
{
    delete m_attendanceDAO;
    delete m_databaseManager;
    m_attendanceDAO = nullptr;
    m_databaseManager = nullptr;
}

int AttendanceDAOTests::insertUser(const QString &name, const QString &email)
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

int AttendanceDAOTests::insertSubject(int userId, const QString &name)
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

Attendance AttendanceDAOTests::makeAttendance(int userId, int subjectId,
                                               const QDate &date,
                                               Attendance::Status status) const
{
    return Attendance(0, userId, subjectId, date, status);
}

void AttendanceDAOTests::insertFindUpdateDeleteAndPersistence()
{
    QString errorMessage;
    const QDate date(2026, 4, 15);
    QVERIFY2(m_attendanceDAO->insertAttendance(
                 makeAttendance(m_firstUserId, m_firstSubjectId, date), &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(errorMessage.isEmpty());

    Attendance stored = m_attendanceDAO->getAttendanceByUserSubjectAndDate(
        m_firstUserId, m_firstSubjectId, date, &errorMessage);
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
    QVERIFY(stored.id() > 0);
    QCOMPARE(stored.userId(), m_firstUserId);
    QCOMPARE(stored.subjectId(), m_firstSubjectId);
    QCOMPARE(stored.date(), date);
    QCOMPARE(stored.status(), Attendance::Status::Present);

    m_databaseManager->close();
    QVERIFY2(m_databaseManager->initialize(), qPrintable(m_databaseManager->lastError()));
    stored = m_attendanceDAO->getAttendanceByUserSubjectAndDate(
        m_firstUserId, m_firstSubjectId, date, &errorMessage);
    QCOMPARE(stored.id() > 0, true);

    stored.setStatus(Attendance::Status::Leave);
    QVERIFY2(m_attendanceDAO->updateAttendance(m_firstUserId, stored, &errorMessage),
             qPrintable(errorMessage));
    const Attendance updated = m_attendanceDAO->getAttendanceByUserSubjectAndDate(
        m_firstUserId, m_firstSubjectId, date, &errorMessage);
    QCOMPARE(updated.status(), Attendance::Status::Leave);

    QVERIFY2(m_attendanceDAO->deleteAttendance(m_firstUserId, stored.id(), &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(m_attendanceDAO->getAttendanceByUserId(m_firstUserId, &errorMessage).isEmpty());
    QVERIFY(!m_attendanceDAO->deleteAttendance(m_firstUserId, stored.id(), &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("Attendance record not found."));
}

void AttendanceDAOTests::rejectDuplicateInsertAndUpdate()
{
    QString errorMessage;
    const QDate firstDate(2026, 4, 15);
    const QDate secondDate(2026, 4, 16);
    QVERIFY(m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, firstDate), &errorMessage));
    QVERIFY(m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, secondDate), &errorMessage));
    QVERIFY(!m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, firstDate), &errorMessage));
    QCOMPARE(errorMessage,
             QStringLiteral("Attendance already exists for this subject and date."));

    QList<Attendance> records = m_attendanceDAO->getAttendanceByUserId(
        m_firstUserId, &errorMessage);
    QCOMPARE(records.size(), 2);
    Attendance second = records.first();
    second.setDate(firstDate);
    QVERIFY(!m_attendanceDAO->updateAttendance(m_firstUserId, second, &errorMessage));
    QCOMPARE(errorMessage,
             QStringLiteral("Attendance already exists for this subject and date."));
    records = m_attendanceDAO->getAttendanceByUserId(m_firstUserId, &errorMessage);
    QCOMPARE(records.size(), 2);
    QCOMPARE(records.first().date(), secondDate);
}

void AttendanceDAOTests::reportConcurrentDuplicateInsertClearly()
{
    QString errorMessage;
    QSqlQuery trigger(m_databaseManager->database());
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER simulate_attendance_insert_race BEFORE INSERT ON attendance "
        "BEGIN "
        "INSERT INTO attendance (user_id, subject_id, date, status) "
        "VALUES (NEW.user_id, NEW.subject_id, NEW.date, NEW.status); "
        "END")));

    const bool inserted = m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, QDate(2026, 4, 15)),
        &errorMessage);
    QVERIFY(!inserted);
    QCOMPARE(errorMessage,
             QStringLiteral("Attendance already exists for this subject and date."));
    QVERIFY(trigger.exec(
        QStringLiteral("DROP TRIGGER simulate_attendance_insert_race")));
    QVERIFY(m_attendanceDAO->getAttendanceByUserId(
                m_firstUserId, &errorMessage).isEmpty());
}

void AttendanceDAOTests::scopeEveryOperationToUser()
{
    QString errorMessage;
    const QDate date(2026, 4, 15);
    QVERIFY(m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, date), &errorMessage));
    const Attendance firstUserRecord = m_attendanceDAO->getAttendanceByUserId(
        m_firstUserId, &errorMessage).first();

    QVERIFY(m_attendanceDAO->getAttendanceByUserId(m_secondUserId, &errorMessage).isEmpty());
    QVERIFY(m_attendanceDAO->getAttendanceByUserIdAndSubjectId(
                m_secondUserId, m_firstSubjectId, &errorMessage).isEmpty());
    QVERIFY(m_attendanceDAO->getAttendanceByUserIdInDateRange(
                m_secondUserId, date, date, &errorMessage).isEmpty());
    QVERIFY(m_attendanceDAO->getAttendanceByUserIdAndSubjectIdInDateRange(
                m_secondUserId, m_firstSubjectId, date, date, &errorMessage).isEmpty());
    QVERIFY(m_attendanceDAO->getAttendanceByUserSubjectAndDate(
                m_secondUserId, m_firstSubjectId, date, &errorMessage).id() == 0);
    QCOMPARE(errorMessage, QStringLiteral("Attendance record not found."));

    Attendance tampered = firstUserRecord;
    tampered.setUserId(m_secondUserId);
    QVERIFY(!m_attendanceDAO->updateAttendance(m_secondUserId, tampered, &errorMessage));
    QVERIFY(!m_attendanceDAO->deleteAttendance(m_secondUserId, firstUserRecord.id(),
                                               &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("Attendance record not found."));
    QVERIFY(m_attendanceDAO->getAttendanceByUserId(m_firstUserId, &errorMessage).size() == 1);

    Attendance foreignSubject = firstUserRecord;
    foreignSubject.setSubjectId(m_secondSubjectId);
    QVERIFY(!m_attendanceDAO->updateAttendance(m_firstUserId, foreignSubject,
                                               &errorMessage));
    QCOMPARE(errorMessage,
             QStringLiteral("The subject does not belong to the specified user."));
    QVERIFY(!m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_secondSubjectId, date), &errorMessage));
    QCOMPARE(errorMessage,
             QStringLiteral("The subject does not belong to the specified user."));
}

void AttendanceDAOTests::rejectInvalidIdsDatesAndRanges()
{
    QString errorMessage;
    Attendance invalid = makeAttendance(0, m_firstSubjectId, QDate(2026, 4, 15));
    QVERIFY(!m_attendanceDAO->insertAttendance(invalid, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    invalid = makeAttendance(m_firstUserId, 0, QDate(2026, 4, 15));
    QVERIFY(!m_attendanceDAO->insertAttendance(invalid, &errorMessage));
    invalid = makeAttendance(m_firstUserId, m_firstSubjectId, {});
    QVERIFY(!m_attendanceDAO->insertAttendance(invalid, &errorMessage));
    invalid = makeAttendance(m_firstUserId, m_firstSubjectId, QDate(2026, 4, 15));
    invalid.setStatus(static_cast<Attendance::Status>(99));
    QVERIFY(!m_attendanceDAO->insertAttendance(invalid, &errorMessage));
    invalid = makeAttendance(m_firstUserId, m_firstSubjectId, QDate(2026, 4, 15));
    invalid.setId(10);
    QVERIFY(!m_attendanceDAO->insertAttendance(invalid, &errorMessage));

    QVERIFY(m_attendanceDAO->getAttendanceByUserId(0, &errorMessage).isEmpty());
    QVERIFY(m_attendanceDAO->getAttendanceByUserIdAndSubjectId(
                m_firstUserId, 0, &errorMessage).isEmpty());
    QVERIFY(m_attendanceDAO->getAttendanceByUserIdInDateRange(
                m_firstUserId, {}, QDate(2026, 4, 15), &errorMessage).isEmpty());
    QVERIFY(m_attendanceDAO->getAttendanceByUserIdAndSubjectIdInDateRange(
                m_firstUserId, m_firstSubjectId, QDate(2026, 4, 16),
                QDate(2026, 4, 15), &errorMessage).isEmpty());
    QVERIFY(!m_attendanceDAO->deleteAttendance(m_firstUserId, 0, &errorMessage));
}

void AttendanceDAOTests::retrieveInclusiveDateRangesAndStableOrder()
{
    QString errorMessage;
    const QDate startDate(2026, 4, 15);
    const QDate middleDate(2026, 4, 16);
    const QDate endDate(2026, 4, 17);
    QVERIFY(m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, startDate), &errorMessage));
    QVERIFY(m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, middleDate), &errorMessage));
    QVERIFY(m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, endDate), &errorMessage));
    const int anotherFirstUserSubjectId =
        insertSubject(m_firstUserId, QStringLiteral("Another First User Subject"));
    QVERIFY(anotherFirstUserSubjectId > 0);
    QVERIFY(m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, anotherFirstUserSubjectId, middleDate,
                       Attendance::Status::Absent), &errorMessage));

    const QList<Attendance> inRange = m_attendanceDAO->getAttendanceByUserIdInDateRange(
        m_firstUserId, startDate, endDate, &errorMessage);
    QCOMPARE(inRange.size(), 4);
    QCOMPARE(inRange.at(0).date(), endDate);
    QCOMPARE(inRange.at(1).date(), middleDate);
    QCOMPARE(inRange.at(2).date(), middleDate);
    QCOMPARE(inRange.at(3).date(), startDate);
    QVERIFY(inRange.at(1).id() > inRange.at(2).id());

    const QList<Attendance> subjectRange =
        m_attendanceDAO->getAttendanceByUserIdAndSubjectIdInDateRange(
            m_firstUserId, m_firstSubjectId, startDate, endDate, &errorMessage);
    QCOMPARE(subjectRange.size(), 3);
    QCOMPARE(subjectRange.first().date(), endDate);
    QCOMPARE(subjectRange.last().date(), startDate);
}

void AttendanceDAOTests::returnEmptyResultsAndCascadeAfterSubjectDeletion()
{
    QString errorMessage;
    QVERIFY(m_attendanceDAO->getAttendanceByUserId(m_firstUserId, &errorMessage).isEmpty());
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(m_attendanceDAO->insertAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, QDate(2026, 4, 15)),
        &errorMessage));

    QSqlQuery query(m_databaseManager->database());
    query.prepare(QStringLiteral("DELETE FROM subjects WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), m_firstSubjectId);
    QVERIFY(query.exec());
    QVERIFY(m_attendanceDAO->getAttendanceByUserId(m_firstUserId, &errorMessage).isEmpty());
    QVERIFY(errorMessage.isEmpty());
}

void AttendanceDAOTests::reportDatabaseAndQueryErrors()
{
    QString errorMessage;
    m_databaseManager->close();
    QVERIFY(m_attendanceDAO->getAttendanceByUserId(m_firstUserId, &errorMessage).isEmpty());
    QCOMPARE(errorMessage, QStringLiteral("The database is not available."));
    QVERIFY2(m_databaseManager->initialize(), qPrintable(m_databaseManager->lastError()));

    QTemporaryDir brokenDatabaseDirectory;
    QVERIFY(brokenDatabaseDirectory.isValid());
    DatabaseManager brokenDatabase(
        QStringLiteral("attendance_dao_broken_%1")
            .arg(reinterpret_cast<quintptr>(this)),
        brokenDatabaseDirectory.filePath(QStringLiteral("broken.db")));
    QVERIFY2(brokenDatabase.initialize(), qPrintable(brokenDatabase.lastError()));
    QSqlQuery query(brokenDatabase.database());
    QVERIFY(query.exec(QStringLiteral("DROP TABLE attendance")));
    AttendanceDAO brokenDAO(&brokenDatabase);
    QVERIFY(brokenDAO.getAttendanceByUserId(1, &errorMessage).isEmpty());
    QVERIFY(!errorMessage.isEmpty());
}

QTEST_GUILESS_MAIN(AttendanceDAOTests)
#include "attendance_dao_tests.moc"
