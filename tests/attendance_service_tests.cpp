#include <QtTest/QtTest>

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/database_manager.h"
#include "../src/data/attendance_dao.h"
#include "../src/models/attendance.h"
#include "../src/services/attendance_service.h"

class AttendanceServiceTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    DatabaseManager *m_databaseManager{nullptr};
    AttendanceDAO *m_attendanceDAO{nullptr};
    AttendanceService *m_attendanceService{nullptr};
    int m_firstUserId{0};
    int m_secondUserId{0};
    int m_firstSubjectId{0};
    int m_secondSubjectId{0};
    int m_otherUsersSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    Attendance makeAttendance(int userId, int subjectId, const QDate &date,
                              Attendance::Status status = Attendance::Status::Present) const;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void addLookupUpdateDeleteAndList();
    void validateIdsDatesStatusesAndRanges();
    void surfaceDuplicateAndOwnershipErrors();
    void isolateUsersAndSubjects();
    void calculateAttendancePercentages();
    void handleEmptyResultsAndDaoFailures();
};

void AttendanceServiceTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString connectionName = QStringLiteral("attendance_service_tests_%1")
                                       .arg(reinterpret_cast<quintptr>(this));
    m_databaseManager = new DatabaseManager(
        connectionName, m_temporaryDirectory.filePath(QStringLiteral("attendance.db")));
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    m_attendanceDAO = new AttendanceDAO(m_databaseManager);
    m_attendanceService = new AttendanceService(m_attendanceDAO);
}

void AttendanceServiceTests::init()
{
    QSqlQuery query(m_databaseManager->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM attendance")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM users")));

    m_firstUserId = insertUser(QStringLiteral("First User"),
                               QStringLiteral("attendance-service-first@example.test"));
    m_secondUserId = insertUser(QStringLiteral("Second User"),
                                QStringLiteral("attendance-service-second@example.test"));
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

void AttendanceServiceTests::cleanupTestCase()
{
    delete m_attendanceService;
    delete m_attendanceDAO;
    delete m_databaseManager;
    m_attendanceService = nullptr;
    m_attendanceDAO = nullptr;
    m_databaseManager = nullptr;
}

int AttendanceServiceTests::insertUser(const QString &name, const QString &email)
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

int AttendanceServiceTests::insertSubject(int userId, const QString &name)
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

Attendance AttendanceServiceTests::makeAttendance(
    int userId, int subjectId, const QDate &date, Attendance::Status status) const
{
    return Attendance(0, userId, subjectId, date, status);
}

void AttendanceServiceTests::addLookupUpdateDeleteAndList()
{
    QString errorMessage;
    const QDate firstDate(2026, 10, 19);
    const QDate futureDate(2026, 10, 21);
    QVERIFY2(m_attendanceService->addAttendance(
                 makeAttendance(m_firstUserId, m_firstSubjectId, firstDate),
                 &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY2(m_attendanceService->addAttendance(
                 makeAttendance(m_firstUserId, m_firstSubjectId, futureDate,
                                Attendance::Status::Absent),
                 &errorMessage),
             qPrintable(errorMessage));

    const Attendance found = m_attendanceService->getAttendance(
        m_firstUserId, m_firstSubjectId, firstDate, &errorMessage);
    QVERIFY(found.id() > 0);
    QCOMPARE(found.status(), Attendance::Status::Present);

    const QList<Attendance> userRecords =
        m_attendanceService->getAttendances(m_firstUserId, &errorMessage);
    QCOMPARE(userRecords.size(), 2);
    QCOMPARE(m_attendanceService->getAttendancesForSubject(
                 m_firstUserId, m_firstSubjectId, &errorMessage).size(),
             2);
    QCOMPARE(m_attendanceService->getAttendancesInDateRange(
                 m_firstUserId, firstDate, firstDate, &errorMessage).size(),
             1);
    QCOMPARE(m_attendanceService->getAttendancesForSubjectInDateRange(
                 m_firstUserId, m_firstSubjectId, firstDate, firstDate,
                 &errorMessage).size(),
             1);

    Attendance updated = m_attendanceService->getAttendance(
        m_firstUserId, m_firstSubjectId, futureDate, &errorMessage);
    updated.setStatus(Attendance::Status::Leave);
    QVERIFY2(m_attendanceService->updateAttendance(m_firstUserId, updated,
                                                   &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(m_attendanceService->getAttendance(
                 m_firstUserId, m_firstSubjectId, futureDate,
                 &errorMessage).status(),
             Attendance::Status::Leave);

    QVERIFY(m_attendanceService->deleteAttendance(m_firstUserId, found.id(),
                                                   &errorMessage));
    QCOMPARE(m_attendanceService->getAttendances(m_firstUserId,
                                                  &errorMessage).size(),
             1);
    QVERIFY(!m_attendanceService->deleteAttendance(m_firstUserId, found.id(),
                                                    &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("Attendance record not found."));
}

void AttendanceServiceTests::validateIdsDatesStatusesAndRanges()
{
    QString errorMessage;
    QVERIFY(!m_attendanceService->addAttendance(
        makeAttendance(0, m_firstSubjectId, QDate(2026, 10, 19)), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("user")));
    QVERIFY(!m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, 0, QDate(2026, 10, 19)), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject")));
    QVERIFY(!m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, {}), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("date")));

    Attendance invalid = makeAttendance(m_firstUserId, m_firstSubjectId,
                                         QDate(2026, 10, 19));
    invalid.setStatus(static_cast<Attendance::Status>(-1));
    QVERIFY(!m_attendanceService->addAttendance(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("status")));

    invalid = makeAttendance(m_firstUserId, m_firstSubjectId, QDate(2026, 10, 19));
    invalid.setId(1);
    QVERIFY(!m_attendanceService->addAttendance(invalid, &errorMessage));
    QVERIFY(!m_attendanceService->updateAttendance(m_firstUserId, invalid,
                                                   &errorMessage));
    invalid.setId(0);
    QVERIFY(!m_attendanceService->updateAttendance(m_firstUserId, invalid,
                                                   &errorMessage));
    QVERIFY(m_attendanceService->getAttendances(0, &errorMessage).isEmpty());
    QVERIFY(errorMessage.contains(QStringLiteral("user")));
    QVERIFY(m_attendanceService->getAttendance(
                m_firstUserId, m_firstSubjectId, {}, &errorMessage).id() == 0);
    QVERIFY(m_attendanceService->getAttendancesInDateRange(
                m_firstUserId, {}, QDate(2026, 10, 19), &errorMessage).isEmpty());
    QVERIFY(m_attendanceService->getAttendancesForSubjectInDateRange(
                m_firstUserId, m_firstSubjectId, QDate(2026, 10, 20),
                QDate(2026, 10, 19), &errorMessage).isEmpty());
    QVERIFY(!m_attendanceService->deleteAttendance(m_firstUserId, 0, &errorMessage));
    QVERIFY(!m_attendanceService->attendancePercentage(
                 m_firstUserId, m_firstSubjectId, QDate(2026, 10, 20),
                 QDate(2026, 10, 19), &errorMessage));
}

void AttendanceServiceTests::surfaceDuplicateAndOwnershipErrors()
{
    QString errorMessage;
    const QDate date(2026, 10, 19);
    const QDate otherDate(2026, 10, 20);
    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, date), &errorMessage));
    QVERIFY(!m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, date), &errorMessage));
    QCOMPARE(errorMessage,
             QStringLiteral("Attendance already exists for this subject and date."));

    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, otherDate), &errorMessage));
    Attendance update = m_attendanceService->getAttendance(
        m_firstUserId, m_firstSubjectId, otherDate, &errorMessage);
    update.setDate(date);
    QVERIFY(!m_attendanceService->updateAttendance(m_firstUserId, update,
                                                   &errorMessage));
    QCOMPARE(errorMessage,
             QStringLiteral("Attendance already exists for this subject and date."));

    QVERIFY(!m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_otherUsersSubjectId, QDate(2026, 10, 21)),
        &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject"), Qt::CaseInsensitive));
    update.setDate(otherDate);
    update.setSubjectId(m_otherUsersSubjectId);
    QVERIFY(!m_attendanceService->updateAttendance(m_firstUserId, update,
                                                   &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject"), Qt::CaseInsensitive));

    update.setSubjectId(m_firstSubjectId);
    QVERIFY(!m_attendanceService->updateAttendance(m_secondUserId, update,
                                                   &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("user"), Qt::CaseInsensitive));
}

void AttendanceServiceTests::isolateUsersAndSubjects()
{
    QString errorMessage;
    const QDate date(2026, 10, 19);
    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, date), &errorMessage));
    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_secondSubjectId, date,
                       Attendance::Status::Absent),
        &errorMessage));
    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_secondUserId, m_otherUsersSubjectId, date,
                       Attendance::Status::Leave),
        &errorMessage));

    QCOMPARE(m_attendanceService->getAttendances(m_firstUserId,
                                                  &errorMessage).size(),
             2);
    QCOMPARE(m_attendanceService->getAttendances(m_secondUserId,
                                                  &errorMessage).size(),
             1);
    QCOMPARE(m_attendanceService->getAttendancesForSubject(
                 m_firstUserId, m_firstSubjectId, &errorMessage).size(),
             1);
    QVERIFY(m_attendanceService->getAttendance(
                m_firstUserId, m_otherUsersSubjectId, date,
                &errorMessage).id() == 0);
    QVERIFY(errorMessage.contains(QStringLiteral("not found")));
}

void AttendanceServiceTests::calculateAttendancePercentages()
{
    QString errorMessage;
    const QDate startDate(2026, 10, 19);
    const QDate middleDate(2026, 10, 20);
    const QDate endDate(2026, 10, 21);
    const QDate outsideDate(2026, 10, 22);

    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, startDate,
                       Attendance::Status::Absent),
        &errorMessage));
    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, middleDate,
                       Attendance::Status::Present),
        &errorMessage));
    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, endDate,
                       Attendance::Status::Leave),
        &errorMessage));
    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, outsideDate,
                       Attendance::Status::Present),
        &errorMessage));
    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_secondSubjectId, middleDate,
                       Attendance::Status::Absent),
        &errorMessage));
    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_secondUserId, m_otherUsersSubjectId, middleDate,
                       Attendance::Status::Absent),
        &errorMessage));

    const std::optional<double> mixed = m_attendanceService->attendancePercentage(
        m_firstUserId, m_firstSubjectId, startDate, endDate, &errorMessage);
    QVERIFY(mixed.has_value());
    QCOMPARE(*mixed, 50.0);
    QVERIFY(errorMessage.isEmpty());
    const std::optional<double> leaveOnly =
        m_attendanceService->attendancePercentage(
            m_firstUserId, m_firstSubjectId, endDate, endDate, &errorMessage);
    QVERIFY(!leaveOnly.has_value());
    QVERIFY(errorMessage.isEmpty());

    QVERIFY(m_attendanceService->deleteAttendance(
        m_firstUserId,
        m_attendanceService->getAttendance(m_firstUserId, m_firstSubjectId,
                                           startDate, &errorMessage).id(),
        &errorMessage));
    const std::optional<double> allPresent = m_attendanceService->attendancePercentage(
        m_firstUserId, m_firstSubjectId, middleDate, middleDate, &errorMessage);
    QVERIFY(allPresent.has_value());
    QCOMPARE(*allPresent, 100.0);

    QVERIFY(m_attendanceService->addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, startDate,
                       Attendance::Status::Absent),
        &errorMessage));
    QVERIFY(m_attendanceService->deleteAttendance(
        m_firstUserId,
        m_attendanceService->getAttendance(m_firstUserId, m_firstSubjectId,
                                           middleDate, &errorMessage).id(),
        &errorMessage));
    const std::optional<double> allAbsent = m_attendanceService->attendancePercentage(
        m_firstUserId, m_firstSubjectId, startDate, startDate, &errorMessage);
    QVERIFY(allAbsent.has_value());
    QCOMPARE(*allAbsent, 0.0);
}

void AttendanceServiceTests::handleEmptyResultsAndDaoFailures()
{
    QString errorMessage;
    QVERIFY(m_attendanceService->getAttendances(m_firstUserId,
                                                 &errorMessage).isEmpty());
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(!m_attendanceService->attendancePercentage(
        m_firstUserId, m_firstSubjectId, QDate(2026, 10, 19),
        QDate(2026, 10, 20), &errorMessage).has_value());
    QVERIFY(errorMessage.isEmpty());

    m_databaseManager->close();
    QVERIFY(m_attendanceService->getAttendances(m_firstUserId,
                                                 &errorMessage).isEmpty());
    QCOMPARE(errorMessage, QStringLiteral("The database is not available."));
    QVERIFY(!m_attendanceService->attendancePercentage(
        m_firstUserId, m_firstSubjectId, QDate(2026, 10, 19),
        QDate(2026, 10, 20), &errorMessage).has_value());
    QCOMPARE(errorMessage, QStringLiteral("The database is not available."));
    QVERIFY2(m_databaseManager->initialize(), qPrintable(m_databaseManager->lastError()));

    QSqlQuery query(m_databaseManager->database());
    QVERIFY(query.exec(QStringLiteral("DROP TABLE attendance")));
    QVERIFY(m_attendanceService->getAttendances(m_firstUserId,
                                                 &errorMessage).isEmpty());
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(!m_attendanceService->attendancePercentage(
        m_firstUserId, m_firstSubjectId, QDate(2026, 10, 19),
        QDate(2026, 10, 20), &errorMessage).has_value());
    QVERIFY(!errorMessage.isEmpty());

    AttendanceService unavailableService(nullptr);
    QVERIFY(!unavailableService.addAttendance(
        makeAttendance(m_firstUserId, m_firstSubjectId, QDate(2026, 10, 19)),
        &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("unavailable")));
}

QTEST_GUILESS_MAIN(AttendanceServiceTests)
#include "attendance_service_tests.moc"
