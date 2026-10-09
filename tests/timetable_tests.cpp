#include <QtTest/QtTest>

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/database_manager.h"
#include "../src/data/timetable_dao.h"
#include "../src/models/timetable.h"

class TimetableTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    DatabaseManager *m_databaseManager{nullptr};
    TimetableDAO *m_timetableDAO{nullptr};
    int m_firstUserId{0};
    int m_secondUserId{0};
    int m_firstSubjectId{0};
    int m_secondSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    Timetable makeTimetable(int userId, int subjectId, int dayOfWeek,
                            const QString &startTime = QStringLiteral("09:00"),
                            const QString &endTime = QStringLiteral("10:00"),
                            const QString &room = QStringLiteral("A-101")) const;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void insertAndRetrieveTimetable();
    void listTimetablesByDay();
    void updateTimetable();
    void deleteTimetable();
    void scopeRecordsToUser();
    void rejectInvalidReferencesAndFields();
    void preserveTimetableAfterReopen();
    void reportUnavailableDatabase();
};

void TimetableTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString connectionName = QStringLiteral("timetable_tests_%1")
                                       .arg(reinterpret_cast<quintptr>(this));
    const QString databasePath = m_temporaryDirectory.filePath(QStringLiteral("timetable.db"));
    m_databaseManager = new DatabaseManager(connectionName, databasePath);
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    m_timetableDAO = new TimetableDAO(m_databaseManager);
}

void TimetableTests::init()
{
    QSqlQuery query(m_databaseManager->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM timetable")));
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

void TimetableTests::cleanupTestCase()
{
    delete m_timetableDAO;
    delete m_databaseManager;
    m_timetableDAO = nullptr;
    m_databaseManager = nullptr;
}

int TimetableTests::insertUser(const QString &name, const QString &email)
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

int TimetableTests::insertSubject(int userId, const QString &name)
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

Timetable TimetableTests::makeTimetable(int userId, int subjectId, int dayOfWeek,
                                        const QString &startTime, const QString &endTime,
                                        const QString &room) const
{
    return Timetable(0, userId, subjectId, dayOfWeek, startTime, endTime, room);
}

void TimetableTests::insertAndRetrieveTimetable()
{
    QString errorMessage;
    const Timetable timetable = makeTimetable(m_firstUserId, m_firstSubjectId, 2);
    QVERIFY2(m_timetableDAO->insertTimetable(timetable, &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const QList<Timetable> timetables =
        m_timetableDAO->getTimetablesByUserId(m_firstUserId, &errorMessage);
    QCOMPARE(timetables.size(), 1);
    QCOMPARE(timetables.first().userId(), m_firstUserId);
    QCOMPARE(timetables.first().subjectId(), m_firstSubjectId);
    QCOMPARE(timetables.first().dayOfWeek(), 2);
    QCOMPARE(timetables.first().startTime(), QStringLiteral("09:00"));
    QCOMPARE(timetables.first().endTime(), QStringLiteral("10:00"));
    QCOMPARE(timetables.first().room(), QStringLiteral("A-101"));

    const Timetable retrieved = m_timetableDAO->getTimetableById(
        m_firstUserId, timetables.first().id(), &errorMessage);
    QCOMPARE(retrieved.id(), timetables.first().id());
    QCOMPARE(retrieved.subjectId(), m_firstSubjectId);
}

void TimetableTests::listTimetablesByDay()
{
    QString errorMessage;
    QVERIFY(m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, m_firstSubjectId, 2, QStringLiteral("11:00")),
        &errorMessage));
    QVERIFY(m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, m_firstSubjectId, 4, QStringLiteral("08:00")),
        &errorMessage));
    QVERIFY(m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, m_firstSubjectId, 2, QStringLiteral("09:00")),
        &errorMessage));
    QVERIFY(m_timetableDAO->insertTimetable(
        makeTimetable(m_secondUserId, m_secondSubjectId, 2),
        &errorMessage));

    const QList<Timetable> dayEntries =
        m_timetableDAO->getTimetablesByUserIdAndDay(m_firstUserId, 2, &errorMessage);
    QCOMPARE(dayEntries.size(), 2);
    QCOMPARE(dayEntries.at(0).startTime(), QStringLiteral("09:00"));
    QCOMPARE(dayEntries.at(1).startTime(), QStringLiteral("11:00"));
    for (const Timetable &entry : dayEntries) {
        QCOMPARE(entry.userId(), m_firstUserId);
        QCOMPARE(entry.dayOfWeek(), 2);
    }
    QVERIFY(errorMessage.isEmpty());
}

void TimetableTests::updateTimetable()
{
    QString errorMessage;
    QVERIFY(m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, m_firstSubjectId, 2), &errorMessage));
    Timetable timetable =
        m_timetableDAO->getTimetablesByUserId(m_firstUserId).first();
    timetable.setSubjectId(m_firstSubjectId);
    timetable.setDayOfWeek(5);
    timetable.setStartTime(QStringLiteral("13:30"));
    timetable.setEndTime(QStringLiteral("14:45"));
    timetable.setRoom(QStringLiteral("B-202"));
    QVERIFY2(m_timetableDAO->updateTimetable(timetable, &errorMessage),
             qPrintable(errorMessage));

    const Timetable updated =
        m_timetableDAO->getTimetableById(m_firstUserId, timetable.id(), &errorMessage);
    QCOMPARE(updated.dayOfWeek(), 5);
    QCOMPARE(updated.startTime(), QStringLiteral("13:30"));
    QCOMPARE(updated.endTime(), QStringLiteral("14:45"));
    QCOMPARE(updated.room(), QStringLiteral("B-202"));

    Timetable invalid = timetable;
    invalid.setId(0);
    QVERIFY(!m_timetableDAO->updateTimetable(invalid, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("valid timetable")));
}

void TimetableTests::deleteTimetable()
{
    QString errorMessage;
    QVERIFY(m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, m_firstSubjectId, 2), &errorMessage));
    const int timetableId =
        m_timetableDAO->getTimetablesByUserId(m_firstUserId).first().id();

    QVERIFY2(m_timetableDAO->deleteTimetable(m_firstUserId, timetableId, &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(m_timetableDAO->getTimetablesByUserId(m_firstUserId).isEmpty());
    QVERIFY(!m_timetableDAO->deleteTimetable(m_firstUserId, timetableId, &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("Timetable entry not found."));
}

void TimetableTests::scopeRecordsToUser()
{
    QString errorMessage;
    QVERIFY(m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, m_firstSubjectId, 2), &errorMessage));
    QVERIFY(m_timetableDAO->insertTimetable(
        makeTimetable(m_secondUserId, m_secondSubjectId, 2), &errorMessage));

    const QList<Timetable> firstUsersEntries =
        m_timetableDAO->getTimetablesByUserId(m_firstUserId, &errorMessage);
    const QList<Timetable> secondUsersEntries =
        m_timetableDAO->getTimetablesByUserId(m_secondUserId, &errorMessage);
    QCOMPARE(firstUsersEntries.size(), 1);
    QCOMPARE(secondUsersEntries.size(), 1);
    QVERIFY(m_timetableDAO->getTimetableById(
                m_firstUserId, secondUsersEntries.first().id(), &errorMessage)
                .id() == 0);
    QVERIFY(!m_timetableDAO->deleteTimetable(
        m_firstUserId, secondUsersEntries.first().id(), &errorMessage));

    Timetable crossUserSubjectUpdate = firstUsersEntries.first();
    crossUserSubjectUpdate.setSubjectId(m_secondSubjectId);
    QVERIFY(!m_timetableDAO->updateTimetable(crossUserSubjectUpdate, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject")));
    QCOMPARE(m_timetableDAO->getTimetableById(
                 m_firstUserId, firstUsersEntries.first().id())
                 .subjectId(),
             m_firstSubjectId);

    Timetable crossUserSubject =
        makeTimetable(m_firstUserId, m_secondSubjectId, 3);
    QVERIFY(!m_timetableDAO->insertTimetable(crossUserSubject, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("does not belong")));

    Timetable crossUserUpdate = secondUsersEntries.first();
    crossUserUpdate.setUserId(m_firstUserId);
    QVERIFY(!m_timetableDAO->updateTimetable(crossUserUpdate, &errorMessage));
    QCOMPARE(m_timetableDAO->getTimetableById(
                 m_secondUserId, secondUsersEntries.first().id())
                 .userId(),
             m_secondUserId);
}

void TimetableTests::rejectInvalidReferencesAndFields()
{
    QString errorMessage;
    QVERIFY(!m_timetableDAO->insertTimetable(
        makeTimetable(0, m_firstSubjectId, 1), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("user")));

    QVERIFY(!m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, 0, 1), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject")));

    QVERIFY(!m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, 999999, 1), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("does not belong")));

    QVERIFY(!m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, m_firstSubjectId, 1, QString(), QStringLiteral("10:00")),
        &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("times are required")));

    QVERIFY(m_timetableDAO->getTimetablesByUserId(0, &errorMessage).isEmpty());
    QVERIFY(errorMessage.contains(QStringLiteral("user")));
    QVERIFY(m_timetableDAO->getTimetablesByUserIdAndDay(0, 1, &errorMessage).isEmpty());
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(m_timetableDAO->getTimetableById(0, 1, &errorMessage).id() == 0);
    QVERIFY(!errorMessage.isEmpty());
}

void TimetableTests::preserveTimetableAfterReopen()
{
    QString errorMessage;
    QVERIFY(m_timetableDAO->insertTimetable(
        makeTimetable(m_firstUserId, m_firstSubjectId, 6), &errorMessage));
    const Timetable original =
        m_timetableDAO->getTimetablesByUserId(m_firstUserId).first();

    m_databaseManager->close();
    QVERIFY2(m_databaseManager->initialize(), qPrintable(m_databaseManager->lastError()));
    const Timetable restored =
        m_timetableDAO->getTimetableById(m_firstUserId, original.id(), &errorMessage);
    QCOMPARE(restored.subjectId(), m_firstSubjectId);
    QCOMPARE(restored.dayOfWeek(), 6);
    QCOMPARE(restored.startTime(), QStringLiteral("09:00"));
}

void TimetableTests::reportUnavailableDatabase()
{
    m_databaseManager->close();
    QString errorMessage;
    QVERIFY(m_timetableDAO->getTimetablesByUserId(m_firstUserId, &errorMessage).isEmpty());
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(!m_timetableDAO->deleteTimetable(m_firstUserId, 1, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("database"), Qt::CaseInsensitive));
    QVERIFY(m_databaseManager->initialize());
}

QTEST_GUILESS_MAIN(TimetableTests)
#include "timetable_tests.moc"
