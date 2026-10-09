#include <QtTest/QtTest>

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/database_manager.h"
#include "../src/data/timetable_dao.h"
#include "../src/models/timetable.h"
#include "../src/services/timetable_service.h"

class TimetableServiceTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    DatabaseManager *m_databaseManager{nullptr};
    TimetableDAO *m_timetableDAO{nullptr};
    TimetableService *m_timetableService{nullptr};
    int m_firstUserId{0};
    int m_secondUserId{0};
    int m_firstSubjectId{0};
    int m_secondSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    Timetable makeTimetable(int userId, int subjectId, int dayOfWeek,
                            const QString &startTime = QStringLiteral("09:00"),
                            const QString &endTime = QStringLiteral("10:00")) const;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void addAndRetrieveValidTimetable();
    void acceptWeekdayBoundariesAndRejectInvalidWeekdays();
    void rejectMalformedTimes();
    void rejectReversedOrEqualTimeRanges();
    void rejectOverlappingEntries();
    void excludeEditedEntryFromConflictCheck();
    void updateAndDeleteTimetable();
    void enforceSubjectOwnershipAndUserScoping();
};

void TimetableServiceTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString connectionName = QStringLiteral("timetable_service_tests_%1")
                                       .arg(reinterpret_cast<quintptr>(this));
    const QString databasePath =
        m_temporaryDirectory.filePath(QStringLiteral("timetable_service.db"));
    m_databaseManager = new DatabaseManager(connectionName, databasePath);
    QVERIFY2(m_databaseManager->initialize(),
             qPrintable(m_databaseManager->lastError()));
    m_timetableDAO = new TimetableDAO(m_databaseManager);
    m_timetableService = new TimetableService(m_timetableDAO);
}

void TimetableServiceTests::init()
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

void TimetableServiceTests::cleanupTestCase()
{
    delete m_timetableService;
    delete m_timetableDAO;
    delete m_databaseManager;
    m_timetableService = nullptr;
    m_timetableDAO = nullptr;
    m_databaseManager = nullptr;
}

int TimetableServiceTests::insertUser(const QString &name, const QString &email)
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

int TimetableServiceTests::insertSubject(int userId, const QString &name)
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

Timetable TimetableServiceTests::makeTimetable(int userId, int subjectId,
                                                int dayOfWeek,
                                                const QString &startTime,
                                                const QString &endTime) const
{
    return Timetable(0, userId, subjectId, dayOfWeek, startTime, endTime,
                     QStringLiteral("A-101"));
}

void TimetableServiceTests::addAndRetrieveValidTimetable()
{
    QString errorMessage;
    Timetable timetable = makeTimetable(m_firstUserId, m_firstSubjectId, 1,
                                        QStringLiteral(" 09:05 "),
                                        QStringLiteral(" 10:30 "));
    timetable.setRoom(QStringLiteral(" Room 12 "));
    QVERIFY2(m_timetableService->addTimetable(timetable, &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const QList<Timetable> entries =
        m_timetableService->getTimetables(m_firstUserId, &errorMessage);
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().dayOfWeek(), 1);
    QCOMPARE(entries.first().startTime(), QStringLiteral("09:05"));
    QCOMPARE(entries.first().endTime(), QStringLiteral("10:30"));
    QCOMPARE(entries.first().room(), QStringLiteral("Room 12"));
    QCOMPARE(m_timetableService->getTimetable(
                 m_firstUserId, entries.first().id(), &errorMessage)
                 .id(),
             entries.first().id());
    QCOMPARE(m_timetableService->getTimetablesForDay(m_firstUserId, 1,
                                                      &errorMessage).size(),
             1);
}

void TimetableServiceTests::acceptWeekdayBoundariesAndRejectInvalidWeekdays()
{
    QString errorMessage;
    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_firstUserId, m_firstSubjectId, 7),
                 &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(m_timetableService->getTimetablesForDay(m_firstUserId, 7,
                                                      &errorMessage).size(),
             1);

    for (const int dayOfWeek : {0, 8}) {
        QVERIFY(!m_timetableService->addTimetable(
            makeTimetable(m_firstUserId, m_firstSubjectId, dayOfWeek),
            &errorMessage));
        QVERIFY(errorMessage.contains(QStringLiteral("Weekday")));
        QVERIFY(m_timetableService->getTimetablesForDay(
                    m_firstUserId, dayOfWeek, &errorMessage).isEmpty());
        QVERIFY(errorMessage.contains(QStringLiteral("Weekday")));
    }
}

void TimetableServiceTests::rejectMalformedTimes()
{
    QString errorMessage;
    const QStringList invalidTimes{
        QStringLiteral("9:00"),
        QStringLiteral("24:00"),
        QStringLiteral("12:60"),
        QStringLiteral("ab:cd"),
        QStringLiteral("09:00:00"),
        QString()
    };
    for (const QString &invalidTime : invalidTimes) {
        Timetable invalidStart = makeTimetable(
            m_firstUserId, m_firstSubjectId, 2, invalidTime, QStringLiteral("10:00"));
        QVERIFY(!m_timetableService->addTimetable(invalidStart, &errorMessage));
        QVERIFY(errorMessage.contains(QStringLiteral("HH:mm")));

        Timetable invalidEnd = makeTimetable(
            m_firstUserId, m_firstSubjectId, 2, QStringLiteral("09:00"), invalidTime);
        QVERIFY(!m_timetableService->addTimetable(invalidEnd, &errorMessage));
        QVERIFY(errorMessage.contains(QStringLiteral("HH:mm")));
    }
}

void TimetableServiceTests::rejectReversedOrEqualTimeRanges()
{
    QString errorMessage;
    for (const auto &range : {
             qMakePair(QStringLiteral("10:00"), QStringLiteral("09:59")),
             qMakePair(QStringLiteral("09:00"), QStringLiteral("09:00"))}) {
        QVERIFY(!m_timetableService->addTimetable(
            makeTimetable(m_firstUserId, m_firstSubjectId, 3,
                          range.first, range.second),
            &errorMessage));
        QVERIFY(errorMessage.contains(QStringLiteral("earlier")));
    }
}

void TimetableServiceTests::rejectOverlappingEntries()
{
    QString errorMessage;
    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_firstUserId, m_firstSubjectId, 2,
                               QStringLiteral("10:00"), QStringLiteral("11:00")),
                 &errorMessage),
             qPrintable(errorMessage));

    const QList<QPair<QString, QString>> overlaps{
        {QStringLiteral("10:00"), QStringLiteral("11:00")},
        {QStringLiteral("10:30"), QStringLiteral("11:30")},
        {QStringLiteral("10:15"), QStringLiteral("10:45")},
        {QStringLiteral("09:30"), QStringLiteral("11:30")}
    };
    for (const auto &range : overlaps) {
        QVERIFY(!m_timetableService->addTimetable(
            makeTimetable(m_firstUserId, m_firstSubjectId, 2,
                          range.first, range.second),
            &errorMessage));
        QCOMPARE(errorMessage,
                 QStringLiteral("This timetable entry overlaps another entry on "
                                "the selected day."));
        QCOMPARE(m_timetableService->getTimetables(m_firstUserId).size(), 1);
    }

    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_firstUserId, m_firstSubjectId, 2,
                               QStringLiteral("11:00"), QStringLiteral("12:00")),
                 &errorMessage),
             qPrintable(errorMessage));
    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_firstUserId, m_firstSubjectId, 2,
                               QStringLiteral("13:00"), QStringLiteral("14:00")),
                 &errorMessage),
             qPrintable(errorMessage));
    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_firstUserId, m_firstSubjectId, 3,
                               QStringLiteral("10:00"), QStringLiteral("11:00")),
                 &errorMessage),
             qPrintable(errorMessage));
    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_secondUserId, m_secondSubjectId, 2,
                               QStringLiteral("10:00"), QStringLiteral("11:00")),
                 &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(m_timetableService->getTimetables(m_firstUserId).size(), 4);
    QCOMPARE(m_timetableService->getTimetables(m_secondUserId).size(), 1);
}

void TimetableServiceTests::excludeEditedEntryFromConflictCheck()
{
    QString errorMessage;
    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_firstUserId, m_firstSubjectId, 2,
                               QStringLiteral("10:00"), QStringLiteral("11:00")),
                 &errorMessage),
             qPrintable(errorMessage));
    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_firstUserId, m_firstSubjectId, 2,
                               QStringLiteral("12:00"), QStringLiteral("13:00")),
                 &errorMessage),
             qPrintable(errorMessage));

    QList<Timetable> entries = m_timetableService->getTimetables(m_firstUserId);
    QCOMPARE(entries.size(), 2);
    Timetable edited = entries.first();
    QVERIFY2(m_timetableService->updateTimetable(edited, &errorMessage),
             qPrintable(errorMessage));

    edited.setStartTime(QStringLiteral("12:30"));
    edited.setEndTime(QStringLiteral("13:30"));
    QVERIFY(!m_timetableService->updateTimetable(edited, &errorMessage));
    QCOMPARE(errorMessage,
             QStringLiteral("This timetable entry overlaps another entry on "
                            "the selected day."));
    const Timetable unchanged =
        m_timetableService->getTimetable(m_firstUserId, edited.id(), &errorMessage);
    QCOMPARE(unchanged.startTime(), QStringLiteral("10:00"));
    QCOMPARE(unchanged.endTime(), QStringLiteral("11:00"));
    QCOMPARE(m_timetableService->getTimetables(m_firstUserId).size(), 2);
}

void TimetableServiceTests::updateAndDeleteTimetable()
{
    QString errorMessage;
    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_firstUserId, m_firstSubjectId, 2),
                 &errorMessage),
             qPrintable(errorMessage));
    Timetable timetable = m_timetableService->getTimetables(m_firstUserId).first();
    timetable.setDayOfWeek(7);
    timetable.setStartTime(QStringLiteral(" 00:00 "));
    timetable.setEndTime(QStringLiteral(" 23:59 "));
    QVERIFY2(m_timetableService->updateTimetable(timetable, &errorMessage),
             qPrintable(errorMessage));

    const Timetable updated =
        m_timetableService->getTimetable(m_firstUserId, timetable.id(), &errorMessage);
    QCOMPARE(updated.dayOfWeek(), 7);
    QCOMPARE(updated.startTime(), QStringLiteral("00:00"));
    QCOMPARE(updated.endTime(), QStringLiteral("23:59"));

    QVERIFY2(m_timetableService->deleteTimetable(m_firstUserId, timetable.id(),
                                                  &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(m_timetableService->getTimetables(m_firstUserId).isEmpty());
}

void TimetableServiceTests::enforceSubjectOwnershipAndUserScoping()
{
    QString errorMessage;
    QVERIFY(!m_timetableService->addTimetable(
        makeTimetable(m_firstUserId, m_secondSubjectId, 2), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject")));

    QVERIFY2(m_timetableService->addTimetable(
                 makeTimetable(m_firstUserId, m_firstSubjectId, 2),
                 &errorMessage),
             qPrintable(errorMessage));
    const Timetable firstUsersEntry =
        m_timetableService->getTimetables(m_firstUserId).first();

    Timetable invalidUpdate = firstUsersEntry;
    invalidUpdate.setSubjectId(m_secondSubjectId);
    QVERIFY(!m_timetableService->updateTimetable(invalidUpdate, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("subject")));
    QCOMPARE(m_timetableService->getTimetable(
                 m_firstUserId, firstUsersEntry.id())
                 .subjectId(),
             m_firstSubjectId);

    QVERIFY(m_timetableService->getTimetables(m_secondUserId).isEmpty());
    QVERIFY(m_timetableService->getTimetable(
                m_secondUserId, firstUsersEntry.id(), &errorMessage)
                .id() == 0);
    QVERIFY(!errorMessage.isEmpty());
}

QTEST_GUILESS_MAIN(TimetableServiceTests)
#include "timetable_service_tests.moc"
