// tests/database_tests.cpp
#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryDir>
#include <QDateTime>
#include "../src/core/database_manager.h"

class DatabaseTests : public QObject
{
    Q_OBJECT

private:
    DatabaseManager* m_dbManager;
    QTemporaryDir m_temporaryDirectory;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testDatabaseOpens();
    void testDatabasePath();
    void testSchemaCreation();
    void testForeignKeysEnabled();
    void testInsertAndSelect();
    void testForeignKeyConstraint();
    void testPersistence();
};

void DatabaseTests::initTestCase()
{
    // Use a temporary database for testing
    if (!m_temporaryDirectory.isValid()) {
        qFatal("Could not create a temporary directory for database tests.");
    }
    const QString databaseFilePath =
        m_temporaryDirectory.filePath(QStringLiteral("nexus_test.db"));

    // Set up a database manager with a unique connection name for the test and the custom database path
    m_dbManager = new DatabaseManager(QStringLiteral("test_connection"), databaseFilePath);
}

void DatabaseTests::cleanupTestCase()
{
    delete m_dbManager;
    m_dbManager = nullptr;

}

void DatabaseTests::testDatabaseOpens()
{
    QVERIFY(m_dbManager->initialize());
}

void DatabaseTests::testDatabasePath()
{
    QVERIFY(!m_dbManager->databasePath().isEmpty());
    QFileInfo info(m_dbManager->databasePath());
    QVERIFY(info.exists());
}

void DatabaseTests::testSchemaCreation()
{
    QSqlDatabase db = m_dbManager->database();
    QStringList tables = db.tables(); // Returns only tables by default
    QVERIFY(tables.contains(QStringLiteral("users")));
    QVERIFY(tables.contains(QStringLiteral("student_profiles")));
    QVERIFY(tables.contains(QStringLiteral("subjects")));
    QVERIFY(tables.contains(QStringLiteral("timetable")));
    QVERIFY(tables.contains(QStringLiteral("assignments")));
    QVERIFY(tables.contains(QStringLiteral("attendance")));
    QVERIFY(tables.contains(QStringLiteral("exams")));
    QVERIFY(tables.contains(QStringLiteral("notes")));
}

void DatabaseTests::testForeignKeysEnabled()
{
    QSqlDatabase db = m_dbManager->database();
    QSqlQuery query(db);
    query.exec(QStringLiteral("PRAGMA foreign_keys;"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

void DatabaseTests::testInsertAndSelect()
{
    QSqlDatabase db = m_dbManager->database();
    QSqlQuery query(db);

    // Insert a test user
    query.prepare(QStringLiteral("INSERT INTO users (name, email, created_at) "
                                 "VALUES (:name, :email, :created_at)"));
    query.bindValue(QStringLiteral(":name"), QStringLiteral("Test User"));
    query.bindValue(QStringLiteral(":email"), QStringLiteral("test@example.com"));
    query.bindValue(QStringLiteral(":created_at"), QDateTime::currentDateTime().toString(Qt::ISODate));
    QVERIFY(query.exec());

    int userId = query.lastInsertId().toInt();
    QVERIFY(userId > 0);

    // Select the user
    query.prepare(QStringLiteral("SELECT name, email FROM users WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), userId);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Test User"));
    QCOMPARE(query.value(1).toString(), QStringLiteral("test@example.com"));
}

void DatabaseTests::testForeignKeyConstraint()
{
    QSqlDatabase db = m_dbManager->database();
    QSqlQuery query(db);

    // Try to insert a subject with a non-existent user_id -> should fail
    query.prepare(QStringLiteral("INSERT INTO subjects (user_id, name, created_at) "
                                 "VALUES (:user_id, :name, :created_at)"));
    query.bindValue(QStringLiteral(":user_id"), 9999); // non-existent user
    query.bindValue(QStringLiteral(":name"), QStringLiteral("Test Subject"));
    query.bindValue(QStringLiteral(":created_at"), QDateTime::currentDateTime().toString(Qt::ISODate));
    QVERIFY(!query.exec());
    QVERIFY(query.lastError().type() != QSqlError::NoError);
}

void DatabaseTests::testPersistence()
{
    QSqlDatabase db = m_dbManager->database();

    // Insert a test user
    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO users (name, email, created_at) "
                                 "VALUES (:name, :email, :created_at)"));
    query.bindValue(QStringLiteral(":name"), QStringLiteral("Persistence User"));
    query.bindValue(QStringLiteral(":email"), QStringLiteral("persistence@example.com"));
    query.bindValue(QStringLiteral(":created_at"), QDateTime::currentDateTime().toString(Qt::ISODate));
    QVERIFY(query.exec());
    int userId = query.lastInsertId().toInt();
    QVERIFY(userId > 0);

    // Close the database
    m_dbManager->close();

    // Reopen the database (same connection will reuse the same database file)
    QVERIFY(m_dbManager->initialize());

    // Query the user again
    query.prepare(QStringLiteral("SELECT name, email FROM users WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), userId);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Persistence User"));
    QCOMPARE(query.value(1).toString(), QStringLiteral("persistence@example.com"));
}

QTEST_MAIN(DatabaseTests)
#include "tests/database_tests.moc"
