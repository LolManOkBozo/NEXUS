#include <QtTest/QtTest>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/database_manager.h"

namespace
{
int nextConnectionId()
{
    static int id = 0;
    return ++id;
}

bool createLegacyDatabase(const QString &path, int duplicateCount,
                          QString *errorMessage = nullptr,
                          bool createIndexNameCollision = false)
{
    const QString connectionName =
        QStringLiteral("attendance_migration_seed_%1").arg(nextConnectionId());
    QSqlDatabase database =
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(path);
    if (!database.open()) {
        if (errorMessage) {
            *errorMessage = database.lastError().text();
        }
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    bool succeeded = true;
    {
        QSqlQuery query(database);
        const QStringList schema{
            QStringLiteral(
                "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT, "
                "created_at TEXT)"),
            QStringLiteral(
                "CREATE TABLE subjects (id INTEGER PRIMARY KEY, user_id INTEGER, "
                "name TEXT, created_at TEXT)"),
            QStringLiteral(
                "CREATE TABLE attendance (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "user_id INTEGER NOT NULL, subject_id INTEGER NOT NULL, "
                "date TEXT NOT NULL, status TEXT NOT NULL)")
        };
        for (const QString &statement : schema) {
            if (!query.exec(statement)) {
                succeeded = false;
                break;
            }
        }
        if (succeeded) {
            succeeded = query.exec(QStringLiteral(
                "INSERT INTO users (id, name, email, created_at) "
                "VALUES (1, 'Test User', 'migration@example.test', '2026-01-01'), "
                "(2, 'Other User', 'other-migration@example.test', '2026-01-01')"));
        }
        if (succeeded) {
            succeeded = query.exec(QStringLiteral(
                "INSERT INTO subjects (id, user_id, name, created_at) VALUES "
                "(10, 1, 'First Subject', '2026-01-01'), "
                "(11, 1, 'Second Subject', '2026-01-01'), "
                "(20, 2, 'Other User Subject', '2026-01-01')"));
        }
        for (int i = 0; succeeded && i < duplicateCount; ++i) {
            succeeded = query.exec(QStringLiteral(
                "INSERT INTO attendance (user_id, subject_id, date, status) "
                "VALUES (1, 10, '2026-10-19', 'Present')"));
        }
        if (succeeded && duplicateCount > 1) {
            succeeded = query.exec(QStringLiteral(
                "INSERT INTO attendance (user_id, subject_id, date, status) "
                "VALUES (1, 11, '2026-10-19', 'Absent'), "
                "(2, 10, '2026-10-19', 'Leave')"));
        }
        if (succeeded && createIndexNameCollision) {
            succeeded = query.exec(QStringLiteral(
                "CREATE TABLE idx_attendance_user_subject_date (id INTEGER)"));
        }
        if (!succeeded && errorMessage) {
            *errorMessage = query.lastError().text();
        }
    }

    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
    return succeeded;
}

bool scalarInt(const QString &path, const QString &sql, int *value,
               QString *errorMessage = nullptr)
{
    const QString connectionName =
        QStringLiteral("attendance_migration_verify_%1").arg(nextConnectionId());
    QSqlDatabase database =
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(path);
    if (!database.open()) {
        if (errorMessage) {
            *errorMessage = database.lastError().text();
        }
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }
    bool succeeded = false;
    {
        QSqlQuery query(database);
        succeeded = query.exec(sql) && query.next();
        if (succeeded) {
            *value = query.value(0).toInt();
        } else if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
    }
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
    return succeeded;
}
}

class AttendanceMigrationTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;

private slots:
    void initTestCase();
    void createsUniqueIndexWithoutChangingExistingRows();
    void reportsDuplicateGroupsAndPreservesAllRows();
    void safelyRepeatsMigrationOnReopen();
    void rollsBackWhenIndexCreationFails();
};

void AttendanceMigrationTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
}

void AttendanceMigrationTests::createsUniqueIndexWithoutChangingExistingRows()
{
    const QString path = m_temporaryDirectory.filePath(QStringLiteral("clean.db"));
    QString errorMessage;
    QVERIFY2(createLegacyDatabase(path, 1, &errorMessage), qPrintable(errorMessage));

    DatabaseManager databaseManager(QStringLiteral("attendance_migration_clean"), path);
    QVERIFY2(databaseManager.initialize(), qPrintable(databaseManager.lastError()));

    QSqlQuery indexQuery(databaseManager.database());
    QVERIFY(indexQuery.exec(QStringLiteral("PRAGMA index_list('attendance')")));
    bool foundExpectedIndex = false;
    while (indexQuery.next()) {
        if (indexQuery.value(1).toString()
            == QStringLiteral("idx_attendance_user_subject_date")) {
            foundExpectedIndex = true;
            QCOMPARE(indexQuery.value(2).toInt(), 1);
        }
    }
    QVERIFY(foundExpectedIndex);

    int rowCount = 0;
    QVERIFY2(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                       &rowCount, &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(rowCount, 1);

    QSqlQuery duplicateInsert(databaseManager.database());
    QVERIFY(duplicateInsert.prepare(QStringLiteral(
        "INSERT INTO attendance (user_id, subject_id, date, status) "
        "VALUES (:user_id, :subject_id, :date, :status)")));
    duplicateInsert.bindValue(QStringLiteral(":user_id"), 1);
    duplicateInsert.bindValue(QStringLiteral(":subject_id"), 10);
    duplicateInsert.bindValue(QStringLiteral(":date"), QStringLiteral("2026-10-19"));
    duplicateInsert.bindValue(QStringLiteral(":status"), QStringLiteral("Absent"));
    QVERIFY(!duplicateInsert.exec());
    QVERIFY(duplicateInsert.lastError().text().contains(
        QStringLiteral("UNIQUE constraint failed")));
}

void AttendanceMigrationTests::reportsDuplicateGroupsAndPreservesAllRows()
{
    const QString path = m_temporaryDirectory.filePath(QStringLiteral("duplicates.db"));
    QString errorMessage;
    QVERIFY2(createLegacyDatabase(path, 2, &errorMessage), qPrintable(errorMessage));

    DatabaseManager databaseManager(QStringLiteral("attendance_migration_duplicates"), path);
    QVERIFY(!databaseManager.initialize());
    const QString migrationError = databaseManager.lastError();
    QVERIFY(migrationError.contains(QStringLiteral("user_id=1")));
    QVERIFY(migrationError.contains(QStringLiteral("subject_id=10")));
    QVERIFY(migrationError.contains(QStringLiteral("date='2026-10-19'")));
    QVERIFY(migrationError.contains(QStringLiteral("record_ids=1,2")));
    QVERIFY(migrationError.contains(QStringLiteral("without deleting or merging")));
    QVERIFY(!databaseManager.isOpen());

    int rowCount = 0;
    QVERIFY2(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                       &rowCount, &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(rowCount, 4);
    int indexCount = 0;
    QVERIFY2(scalarInt(
                 path,
                 QStringLiteral(
                     "SELECT COUNT(*) FROM sqlite_master WHERE type='index' "
                     "AND name='idx_attendance_user_subject_date'"),
                 &indexCount, &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(indexCount, 0);
}

void AttendanceMigrationTests::safelyRepeatsMigrationOnReopen()
{
    const QString path = m_temporaryDirectory.filePath(QStringLiteral("repeat.db"));
    QString errorMessage;
    QVERIFY2(createLegacyDatabase(path, 0, &errorMessage), qPrintable(errorMessage));

    DatabaseManager databaseManager(QStringLiteral("attendance_migration_repeat"), path);
    QVERIFY2(databaseManager.initialize(), qPrintable(databaseManager.lastError()));
    databaseManager.close();
    QVERIFY2(databaseManager.initialize(), qPrintable(databaseManager.lastError()));

    QSqlQuery indexQuery(databaseManager.database());
    QVERIFY(indexQuery.exec(QStringLiteral("PRAGMA index_list('attendance')")));
    int matchingIndexes = 0;
    while (indexQuery.next()) {
        if (indexQuery.value(1).toString()
            == QStringLiteral("idx_attendance_user_subject_date")) {
            ++matchingIndexes;
        }
    }
    QCOMPARE(matchingIndexes, 1);
}

void AttendanceMigrationTests::rollsBackWhenIndexCreationFails()
{
    const QString path = m_temporaryDirectory.filePath(QStringLiteral("index_failure.db"));
    QString errorMessage;
    QVERIFY2(createLegacyDatabase(path, 1, &errorMessage, true), qPrintable(errorMessage));

    DatabaseManager databaseManager(QStringLiteral("attendance_migration_failure"), path);
    QVERIFY(!databaseManager.initialize());
    QVERIFY(databaseManager.lastError().contains(
        QStringLiteral("could not create the unique index")));
    QVERIFY(!databaseManager.isOpen());

    int rowCount = 0;
    QVERIFY2(scalarInt(path, QStringLiteral("SELECT COUNT(*) FROM attendance"),
                       &rowCount, &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(rowCount, 1);
    int rolledBackTableCount = 0;
    QVERIFY2(scalarInt(
                 path,
                 QStringLiteral(
                     "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='notes'"),
                 &rolledBackTableCount, &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(rolledBackTableCount, 0);
}

QTEST_GUILESS_MAIN(AttendanceMigrationTests)
#include "attendance_migration_tests.moc"
