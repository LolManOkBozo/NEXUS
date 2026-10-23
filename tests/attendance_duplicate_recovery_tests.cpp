#include <QtTest/QtTest>

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>

#include <functional>

#include "../src/core/attendance_duplicate_recovery.h"
#include "../src/core/database_manager.h"

namespace
{
QString uniqueConnectionName(const QString &prefix)
{
    return QStringLiteral("%1_%2")
        .arg(prefix, QUuid::createUuid().toString(QUuid::WithoutBraces));
}

bool withDatabase(const QString &path, const std::function<bool(QSqlDatabase &)> &action,
                  QString *errorMessage = nullptr)
{
    const QString connectionName = uniqueConnectionName(QStringLiteral("recovery_test"));
    QSqlDatabase database =
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(path);
    database.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=300"));
    if (!database.open()) {
        if (errorMessage) {
            *errorMessage = database.lastError().text();
        }
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }
    const bool result = action(database);
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
    return result;
}

bool createLegacyDatabase(const QString &path, QString *errorMessage = nullptr,
                          bool indexNameCollision = false)
{
    return withDatabase(path, [&](QSqlDatabase &database) {
        QSqlQuery query(database);
        const QStringList statements{
            QStringLiteral("PRAGMA foreign_keys = ON"),
            QStringLiteral(
                "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL, "
                "email TEXT NOT NULL, created_at TEXT NOT NULL)"),
            QStringLiteral(
                "CREATE TABLE subjects (id INTEGER PRIMARY KEY, user_id INTEGER NOT NULL, "
                "name TEXT NOT NULL, created_at TEXT NOT NULL, "
                "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE)"),
            QStringLiteral(
                "CREATE TABLE attendance (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "user_id INTEGER NOT NULL, subject_id INTEGER NOT NULL, "
                "date TEXT NOT NULL, status TEXT NOT NULL, "
                "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE, "
                "FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE CASCADE)")
        };
        for (const QString &statement : statements) {
            if (!query.exec(statement)) {
                if (errorMessage) {
                    *errorMessage = query.lastError().text();
                }
                return false;
            }
        }
        if (!query.exec(QStringLiteral(
                "INSERT INTO users VALUES "
                "(1, 'User One', 'one@example.test', '2026-01-01'), "
                "(2, 'User Two', 'two@example.test', '2026-01-01')"))
            || !query.exec(QStringLiteral(
                "INSERT INTO subjects VALUES "
                "(10, 1, 'Biology', '2026-01-01'), "
                "(20, 2, 'Physics', '2026-01-01')"))
            || !query.exec(QStringLiteral(
                "INSERT INTO attendance (id, user_id, subject_id, date, status) VALUES "
                "(1, 1, 10, '2026-10-19', 'Present'), "
                "(2, 1, 10, '2026-10-19', 'Absent'), "
                "(3, 1, 10, '2026-10-19', 'Leave'), "
                "(4, 2, 20, '2026-10-19', 'Present'), "
                "(5, 2, 20, '2026-10-19', 'Absent'), "
                "(6, 1, 10, '2026-10-20', 'Present')"))) {
            if (errorMessage) {
                *errorMessage = query.lastError().text();
            }
            return false;
        }
        if (indexNameCollision
            && !query.exec(QStringLiteral(
                "CREATE TABLE idx_attendance_user_subject_date (id INTEGER)"))) {
            if (errorMessage) {
                *errorMessage = query.lastError().text();
            }
            return false;
        }
        return true;
    }, errorMessage);
}

bool scalarInt(const QString &path, const QString &sql, int *value,
               QString *errorMessage = nullptr)
{
    return withDatabase(path, [&](QSqlDatabase &database) {
        QSqlQuery query(database);
        if (!query.exec(sql) || !query.next()) {
            if (errorMessage) {
                *errorMessage = query.lastError().text();
            }
            return false;
        }
        *value = query.value(0).toInt();
        return true;
    }, errorMessage);
}

QList<AttendanceDuplicateResolution> resolutionsFor(
    const QList<AttendanceDuplicateGroup> &groups, const QList<int> &keepIds)
{
    QList<AttendanceDuplicateResolution> resolutions;
    if (groups.size() != keepIds.size()) {
        return resolutions;
    }
    for (qsizetype groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
        const AttendanceDuplicateGroup &group = groups.at(groupIndex);
        AttendanceDuplicateResolution resolution;
        resolution.userId = group.userId;
        resolution.subjectId = group.subjectId;
        resolution.date = group.date;
        resolution.keepRecordId = keepIds.at(groupIndex);
        for (const AttendanceDuplicateRecord &record : group.records) {
            resolution.inspectedRecords.append(record);
        }
        resolutions.append(resolution);
    }
    return resolutions;
}

class TestConnection
{
public:
    explicit TestConnection(const QString &path)
        : name(uniqueConnectionName(QStringLiteral("recovery_lock")))
        , database(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name))
    {
        database.setDatabaseName(path);
        database.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=300"));
        database.open();
    }

    ~TestConnection()
    {
        database.close();
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
    }

    QString name;
    QSqlDatabase database;
};
}

class AttendanceDuplicateRecoveryTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    QString databasePath(const QString &name) const
    {
        return m_temporaryDirectory.filePath(name);
    }
    bool makeDatabase(const QString &name, bool collision = false)
    {
        QString error;
        return createLegacyDatabase(databasePath(name), &error, collision);
    }

private slots:
    void initTestCase();
    void reportsEveryDuplicateGroupAndRecordDetail();
    void backsUpAndResolvesOnlyExplicitlySelectedRecords();
    void failedBackupPreventsAllDeletes();
    void lockAfterBackupPreventsRecoveryDeletes();
    void cancellationAndInvalidResolutionLeaveDataUntouched();
    void rejectsInvalidModelDataAndUnauthorizedOwnership();
    void rollsBackDeletesIfUniquenessMigrationFails();
    void rechecksChangedGroupsBeforeApplyingPlan();
    void rechecksChangedRecordDetailsBeforeApplyingPlan();
    void repeatedRecoveryIsIdempotent();
};

void AttendanceDuplicateRecoveryTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
}

void AttendanceDuplicateRecoveryTests::reportsEveryDuplicateGroupAndRecordDetail()
{
    QVERIFY(makeDatabase(QStringLiteral("inspect.db")));
    AttendanceDuplicateRecovery recovery(databasePath(QStringLiteral("inspect.db")));
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(groups.size(), 2);
    QCOMPARE(groups.at(0).userId, 1);
    QCOMPARE(groups.at(0).subjectId, 10);
    QCOMPARE(groups.at(0).date, QStringLiteral("2026-10-19"));
    QCOMPARE(groups.at(0).userName, QStringLiteral("User One"));
    QCOMPARE(groups.at(0).subjectName, QStringLiteral("Biology"));
    QCOMPARE(groups.at(0).records.size(), 3);
    QCOMPARE(groups.at(0).records.at(0).id, 1);
    QCOMPARE(groups.at(0).records.at(1).id, 2);
    QCOMPARE(groups.at(0).records.at(2).id, 3);
    QCOMPARE(groups.at(0).records.at(2).status, QStringLiteral("Leave"));
    QCOMPARE(groups.at(1).records.size(), 2);
    QCOMPARE(groups.at(1).records.at(0).id, 4);
    QCOMPARE(groups.at(1).records.at(1).id, 5);
}

void AttendanceDuplicateRecoveryTests::backsUpAndResolvesOnlyExplicitlySelectedRecords()
{
    QVERIFY(makeDatabase(QStringLiteral("success.db")));
    const QString path = databasePath(QStringLiteral("success.db"));
    AttendanceDuplicateRecovery recovery(path);
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QString backupPath;
    QVERIFY2(recovery.resolveDuplicates(resolutionsFor(groups, {2, 5}), true,
                                        &backupPath, &error),
             qPrintable(error));
    QVERIFY(QFileInfo::exists(backupPath));
    QVERIFY(QFileInfo(backupPath).size() > 0);
    QVERIFY(withDatabase(backupPath, [](QSqlDatabase &database) {
        QSqlQuery query(database);
        return query.exec(QStringLiteral("PRAGMA integrity_check"))
            && query.next() && query.value(0).toString() == QStringLiteral("ok");
    }));

    int count = 0;
    QVERIFY(scalarInt(backupPath, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                      &count, &error));
    QCOMPARE(count, 6);
    QVERIFY(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                      &count, &error));
    QCOMPARE(count, 3);
    QVERIFY(scalarInt(path, QStringLiteral(
                          "SELECT COUNT(*) FROM attendance WHERE id IN (2, 5, 6)"),
                      &count, &error));
    QCOMPARE(count, 3);
    QVERIFY(scalarInt(path, QStringLiteral(
                          "SELECT COUNT(*) FROM attendance WHERE id IN (1, 3, 4)"),
                      &count, &error));
    QCOMPARE(count, 0);
    QVERIFY(withDatabase(path, [](QSqlDatabase &database) {
        QSqlQuery query(database);
        if (!query.exec(QStringLiteral(
                "SELECT id, status FROM attendance ORDER BY id"))) {
            return false;
        }
        const QList<QPair<int, QString>> expected{
            {2, QStringLiteral("Absent")},
            {5, QStringLiteral("Absent")},
            {6, QStringLiteral("Present")}
        };
        int index = 0;
        while (query.next()) {
            if (index >= expected.size()
                || query.value(0).toInt() != expected.at(index).first
                || query.value(1).toString() != expected.at(index).second) {
                return false;
            }
            ++index;
        }
        return index == expected.size();
    }));
    QVERIFY(withDatabase(path, [](QSqlDatabase &database) {
        QSqlQuery query(database);
        return query.exec(QStringLiteral("PRAGMA foreign_key_check"))
            && !query.next();
    }));
    QVERIFY(scalarInt(path, QStringLiteral(
                          "SELECT COUNT(*) FROM sqlite_master WHERE type = 'index' "
                          "AND name = 'idx_attendance_user_subject_date'"),
                      &count, &error));
    QCOMPARE(count, 1);

    DatabaseManager databaseManager(QStringLiteral("recovery_migration_retry"), path);
    QVERIFY2(databaseManager.initialize(), qPrintable(databaseManager.lastError()));
}

void AttendanceDuplicateRecoveryTests::failedBackupPreventsAllDeletes()
{
    QVERIFY(makeDatabase(QStringLiteral("backup_failure.db")));
    const QString path = databasePath(QStringLiteral("backup_failure.db"));
    AttendanceDuplicateRecovery recovery(
        path, m_temporaryDirectory.filePath(QStringLiteral("missing-backup-directory")));
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QString backupPath;
    QVERIFY(!recovery.resolveDuplicates(resolutionsFor(groups, {1, 4}), true,
                                       &backupPath, &error));
    QVERIFY(error.contains(QStringLiteral("backup"), Qt::CaseInsensitive));
    QVERIFY2(backupPath.isEmpty(), qPrintable(error));

    int count = 0;
    QVERIFY(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                      &count, &error));
    QCOMPARE(count, 6);
}

void AttendanceDuplicateRecoveryTests::lockAfterBackupPreventsRecoveryDeletes()
{
    QVERIFY(makeDatabase(QStringLiteral("locked_recovery.db")));
    const QString path = databasePath(QStringLiteral("locked_recovery.db"));
    AttendanceDuplicateRecovery recovery(path);
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    TestConnection lock(path);
    QVERIFY(lock.database.isOpen());
    QSqlQuery lockQuery(lock.database);
    QVERIFY(lockQuery.exec(QStringLiteral("BEGIN IMMEDIATE TRANSACTION")));

    QString backupPath;
    QVERIFY(!recovery.resolveDuplicates(resolutionsFor(groups, {1, 4}), true,
                                       &backupPath, &error));
    QVERIFY(QFileInfo::exists(backupPath));
    QVERIFY(error.contains(QStringLiteral("backup retained")));

    lockQuery.exec(QStringLiteral("ROLLBACK"));
    int count = 0;
    QVERIFY(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                      &count, &error));
    QCOMPARE(count, 6);
}

void AttendanceDuplicateRecoveryTests::cancellationAndInvalidResolutionLeaveDataUntouched()
{
    QVERIFY(makeDatabase(QStringLiteral("cancel.db")));
    const QString path = databasePath(QStringLiteral("cancel.db"));
    AttendanceDuplicateRecovery recovery(path);
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    const auto choices = resolutionsFor(groups, {1, 4});

    QString backupPath;
    QVERIFY(!recovery.resolveDuplicates(choices, false, &backupPath, &error));
    QVERIFY(error.contains(QStringLiteral("cancelled")));
    QVERIFY(backupPath.isEmpty());
    QCOMPARE(QDir(m_temporaryDirectory.path()).entryList(
                 {QStringLiteral("cancel.db.attendance-recovery-*.bak")},
                 QDir::Files).size(), 0);

    auto invalidChoices = choices;
    invalidChoices[0].keepRecordId = 999;
    QVERIFY(!recovery.resolveDuplicates(invalidChoices, true, &backupPath, &error));
    QVERIFY(error.contains(QStringLiteral("Choose exactly one"))
            || error.contains(QStringLiteral("changed after inspection")));
    int count = 0;
    QVERIFY(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                      &count, &error));
    QCOMPARE(count, 6);
}

void AttendanceDuplicateRecoveryTests::rejectsInvalidModelDataAndUnauthorizedOwnership()
{
    QVERIFY(makeDatabase(QStringLiteral("invalid_data.db")));
    const QString path = databasePath(QStringLiteral("invalid_data.db"));
    QVERIFY(withDatabase(path, [](QSqlDatabase &database) {
        QSqlQuery query(database);
        return query.exec(QStringLiteral(
            "UPDATE attendance SET status = 'Excused' WHERE id = 2"));
    }));

    AttendanceDuplicateRecovery recovery(path);
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QString backupPath;
    QVERIFY(!recovery.resolveDuplicates(resolutionsFor(groups, {1, 4}), true,
                                        &backupPath, &error));
    QVERIFY(error.contains(QStringLiteral("invalid status")));
    QVERIFY(backupPath.isEmpty());

    QVERIFY(makeDatabase(QStringLiteral("invalid_date.db")));
    const QString invalidDatePath = databasePath(QStringLiteral("invalid_date.db"));
    QVERIFY(withDatabase(invalidDatePath, [](QSqlDatabase &database) {
        QSqlQuery query(database);
        return query.exec(QStringLiteral(
            "UPDATE attendance SET date = 'not-a-date' WHERE id IN (1, 2, 3)"));
    }));
    AttendanceDuplicateRecovery invalidDateRecovery(invalidDatePath);
    const auto invalidDateGroups = invalidDateRecovery.findDuplicateGroups(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!invalidDateRecovery.resolveDuplicates(
        resolutionsFor(invalidDateGroups, {1, 4}), true, &backupPath, &error));
    QVERIFY(error.contains(QStringLiteral("Attendance model")));
    QVERIFY(backupPath.isEmpty());

    QVERIFY(makeDatabase(QStringLiteral("wrong_owner.db")));
    const QString wrongOwnerPath = databasePath(QStringLiteral("wrong_owner.db"));
    QVERIFY(withDatabase(wrongOwnerPath, [](QSqlDatabase &database) {
        QSqlQuery query(database);
        return query.exec(QStringLiteral(
            "INSERT INTO attendance (user_id, subject_id, date, status) "
            "VALUES (1, 20, '2026-10-19', 'Present'), "
            "(1, 20, '2026-10-19', 'Absent')"));
    }));
    AttendanceDuplicateRecovery wrongOwnerRecovery(wrongOwnerPath);
    const auto wrongOwnerGroups = wrongOwnerRecovery.findDuplicateGroups(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!wrongOwnerRecovery.resolveDuplicates(
        resolutionsFor(wrongOwnerGroups, {1, 7, 4}), true, &backupPath, &error));
    QVERIFY(error.contains(QStringLiteral("not owned")));
}

void AttendanceDuplicateRecoveryTests::rollsBackDeletesIfUniquenessMigrationFails()
{
    QVERIFY(makeDatabase(QStringLiteral("index_collision.db"), true));
    const QString path = databasePath(QStringLiteral("index_collision.db"));
    AttendanceDuplicateRecovery recovery(path);
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QString backupPath;
    QVERIFY(!recovery.resolveDuplicates(resolutionsFor(groups, {1, 4}), true,
                                       &backupPath, &error));
    QVERIFY(error.contains(QStringLiteral("incompatible index"))
            || error.contains(QStringLiteral("unique index")));
    QVERIFY(QFileInfo::exists(backupPath));
    int count = 0;
    QVERIFY(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                      &count, &error));
    QCOMPARE(count, 6);
    QVERIFY(scalarInt(path, QStringLiteral(
                          "SELECT COUNT(*) FROM attendance WHERE id IN (1, 2, 3, 4, 5)"),
                      &count, &error));
    QCOMPARE(count, 5);
}

void AttendanceDuplicateRecoveryTests::rechecksChangedGroupsBeforeApplyingPlan()
{
    QVERIFY(makeDatabase(QStringLiteral("stale_plan.db")));
    const QString path = databasePath(QStringLiteral("stale_plan.db"));
    AttendanceDuplicateRecovery recovery(path);
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    auto choices = resolutionsFor(groups, {1, 4});

    QVERIFY(withDatabase(path, [](QSqlDatabase &database) {
        QSqlQuery query(database);
        return query.exec(QStringLiteral(
            "INSERT INTO attendance (user_id, subject_id, date, status) "
            "VALUES (1, 10, '2026-10-19', 'Present')"));
    }));
    QString backupPath;
    QVERIFY(!recovery.resolveDuplicates(choices, true, &backupPath, &error));
    QVERIFY(error.contains(QStringLiteral("changed after inspection")));
    int count = 0;
    QVERIFY(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                      &count, &error));
    QCOMPARE(count, 7);
}

void AttendanceDuplicateRecoveryTests::rechecksChangedRecordDetailsBeforeApplyingPlan()
{
    QVERIFY(makeDatabase(QStringLiteral("stale_details.db")));
    const QString path = databasePath(QStringLiteral("stale_details.db"));
    AttendanceDuplicateRecovery recovery(path);
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    const auto choices = resolutionsFor(groups, {1, 4});

    QVERIFY(withDatabase(path, [](QSqlDatabase &database) {
        QSqlQuery query(database);
        return query.exec(QStringLiteral(
            "UPDATE attendance SET status = 'Leave' WHERE id = 2"));
    }));
    QString backupPath;
    QVERIFY(!recovery.resolveDuplicates(choices, true, &backupPath, &error));
    QVERIFY(error.contains(QStringLiteral("changed after inspection")));
    QVERIFY(backupPath.isEmpty());
    int count = 0;
    QVERIFY(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                      &count, &error));
    QCOMPARE(count, 6);
}

void AttendanceDuplicateRecoveryTests::repeatedRecoveryIsIdempotent()
{
    QVERIFY(makeDatabase(QStringLiteral("repeat.db")));
    const QString path = databasePath(QStringLiteral("repeat.db"));
    AttendanceDuplicateRecovery recovery(path);
    QString error;
    const auto groups = recovery.findDuplicateGroups(&error);
    QString backupPath;
    QVERIFY2(recovery.resolveDuplicates(resolutionsFor(groups, {2, 5}), true,
                                       &backupPath, &error),
             qPrintable(error));

    QVERIFY(recovery.findDuplicateGroups(&error).isEmpty());
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY2(recovery.resolveDuplicates({}, true, nullptr, &error), qPrintable(error));

    int count = 0;
    QVERIFY(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                      &count, &error));
    QCOMPARE(count, 3);
}

QTEST_GUILESS_MAIN(AttendanceDuplicateRecoveryTests)
#include "attendance_duplicate_recovery_tests.moc"
