// src/core/database_manager.cpp
#include "database_manager.h"

#include "attendance_uniqueness_migration.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStringList>
#include <QStandardPaths>
#include <QDir>

bool ensureAttendanceUniquenessMigration(const QSqlDatabase &database,
                                         QString *errorMessage)
{
    const auto fail = [errorMessage](const QString &message) {
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    };
    const QString indexName = QStringLiteral("idx_attendance_user_subject_date");
    QSqlQuery duplicateQuery(database);
    if (!duplicateQuery.exec(QStringLiteral(
            "SELECT user_id, subject_id, date, GROUP_CONCAT(id, ',') "
            "FROM attendance "
            "GROUP BY user_id, subject_id, date "
            "HAVING COUNT(*) > 1 "
            "ORDER BY user_id, subject_id, date"))) {
        return fail(QStringLiteral(
                        "Attendance uniqueness migration could not check existing records: ")
                    + duplicateQuery.lastError().text());
    }

    QStringList conflicts;
    while (duplicateQuery.next()) {
        conflicts.append(
            QStringLiteral("(user_id=%1, subject_id=%2, date='%3', record_ids=%4)")
                .arg(duplicateQuery.value(0).toString(),
                     duplicateQuery.value(1).toString(),
                     duplicateQuery.value(2).toString(),
                     duplicateQuery.value(3).toString()));
    }
    if (duplicateQuery.lastError().isValid()) {
        return fail(QStringLiteral(
                        "Attendance uniqueness migration could not read duplicate records: ")
                    + duplicateQuery.lastError().text());
    }
    if (!conflicts.isEmpty()) {
        return fail(QStringLiteral(
                        "Attendance uniqueness migration was aborted. Resolve these "
                        "duplicate records without deleting or merging data, then retry: %1")
                        .arg(conflicts.join(QStringLiteral("; "))));
    }

    QSqlQuery indexListQuery(database);
    if (!indexListQuery.exec(QStringLiteral("PRAGMA index_list('attendance')"))) {
        return fail(QStringLiteral(
                        "Attendance uniqueness migration could not inspect existing indexes: ")
                    + indexListQuery.lastError().text());
    }
    bool indexAlreadyExists = false;
    while (indexListQuery.next()) {
        if (indexListQuery.value(1).toString() == indexName) {
            indexAlreadyExists = true;
            if (indexListQuery.value(2).toInt() != 1
                || indexListQuery.value(4).toInt() != 0) {
                return fail(QStringLiteral(
                    "Attendance uniqueness migration found an incompatible index named %1.")
                                .arg(indexName));
            }
            break;
        }
    }
    indexListQuery.finish();
    if (indexListQuery.lastError().isValid()) {
        return fail(QStringLiteral(
                        "Attendance uniqueness migration could not inspect existing indexes: ")
                    + indexListQuery.lastError().text());
    }

    if (!indexAlreadyExists) {
        QSqlQuery createIndexQuery(database);
        if (!createIndexQuery.exec(
                QStringLiteral("CREATE UNIQUE INDEX %1 ON attendance "
                               "(user_id, subject_id, date)")
                    .arg(indexName))) {
            return fail(QStringLiteral(
                            "Attendance uniqueness migration could not create the unique index: ")
                        + createIndexQuery.lastError().text());
        }
    }

    QSqlQuery indexInfoQuery(database);
    if (!indexInfoQuery.exec(QStringLiteral("PRAGMA index_info(%1)").arg(indexName))) {
        return fail(QStringLiteral(
                        "Attendance uniqueness migration could not verify the unique index: ")
                    + indexInfoQuery.lastError().text());
    }
    const QStringList expectedColumns{
        QStringLiteral("user_id"), QStringLiteral("subject_id"), QStringLiteral("date")
    };
    QStringList actualColumns;
    while (indexInfoQuery.next()) {
        actualColumns.append(indexInfoQuery.value(2).toString());
    }
    if (indexInfoQuery.lastError().isValid()) {
        return fail(QStringLiteral(
                        "Attendance uniqueness migration could not verify the unique index: ")
                    + indexInfoQuery.lastError().text());
    }
    if (actualColumns != expectedColumns) {
        return fail(QStringLiteral(
                        "Attendance uniqueness migration found an index named %1 with "
                        "unexpected columns.")
                        .arg(indexName));
    }
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

DatabaseManager::DatabaseManager(const QString& connectionName, const QString& databasePath)
    : m_connectionName(connectionName)
    , m_customDatabasePath(databasePath)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::initialize()
{
    // If already open, return true
    if (isOpen()) {
        return true;
    }

    QString databaseFilePath;

    if (!m_customDatabasePath.isEmpty()) {
        // Use the provided custom path
        databaseFilePath = m_customDatabasePath;
        // Ensure the directory exists
        QFileInfo fileInfo(databaseFilePath);
        QDir dir = fileInfo.absoluteDir();
        if (!dir.exists() && !dir.mkpath(".")) {
            m_lastError = QStringLiteral("Failed to create directory for custom database path: ") + dir.absolutePath();
            return false;
        }
    } else {
        // Determine the database directory using standard location
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (appDataDir.isEmpty()) {
            m_lastError = QStringLiteral("Unable to determine application data location.");
            return false;
        }

        // Create the directory if it doesn't exist
        QDir dir(appDataDir);
        if (!dir.exists() && !dir.mkpath(".")) {
            m_lastError = QStringLiteral("Failed to create application data directory: ") + appDataDir;
            return false;
        }

        QString primaryPath = dir.filePath(QStringLiteral("nexus.db"));
        databaseFilePath = primaryPath;

        QDir parentDir = dir;
        if (parentDir.cdUp()) {
            QString legacyPath = parentDir.filePath(QStringLiteral("nexus.db"));
            if (QFileInfo::exists(legacyPath) && !QFileInfo::exists(primaryPath)) {
                databaseFilePath = legacyPath;
            } else if (QFileInfo::exists(legacyPath) && QFileInfo::exists(primaryPath)) {
                QSqlDatabase tempDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName + QStringLiteral("_chk"));
                tempDb.setDatabaseName(legacyPath);
                if (tempDb.open()) {
                    QSqlQuery chkQ(tempDb);
                    if (chkQ.exec(QStringLiteral("SELECT COUNT(*) FROM users WHERE email != 'demo@nexus.local'")) && chkQ.next() && chkQ.value(0).toInt() > 0) {
                        databaseFilePath = legacyPath;
                    }
                    tempDb.close();
                }
                QSqlDatabase::removeDatabase(m_connectionName + QStringLiteral("_chk"));
            }
        }
    }

    // Add or retrieve the database connection
    if (QSqlDatabase::contains(m_connectionName)) {
        m_db = QSqlDatabase::database(m_connectionName);
    } else {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    }

    m_db.setDatabaseName(databaseFilePath);

    // Open the database
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    // Enable foreign keys
    if (!enableForeignKeys()) {
        close();
        return false;
    }

    // Create the schema
    if (!createSchema()) {
        close();
        return false;
    }

    m_lastError.clear();
    return true;
}

void DatabaseManager::close()
{
    if (isOpen()) {
        m_db.close();
    }
    // Note: We do not remove the connection here because it might be used elsewhere.
    // In a more complex application, we might manage the connection lifetime differently.
    // For simplicity, we leave the connection added.
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
}

QString DatabaseManager::databasePath() const
{
    if (isOpen()) {
        return m_db.databaseName();
    }
    return QString();
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

QSqlDatabase DatabaseManager::database() const
{
    return m_db;
}

bool DatabaseManager::enableForeignKeys()
{
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        m_lastError = QStringLiteral("Failed to enable foreign keys: ") + query.lastError().text();
        return false;
    }

    if (!query.exec(QStringLiteral("PRAGMA foreign_keys;"))) {
        m_lastError = QStringLiteral("Failed to verify SQLite foreign keys: ")
                      + query.lastError().text();
        return false;
    }
    if (!query.next()) {
        m_lastError = QStringLiteral(
            "Failed to verify SQLite foreign keys: the query returned no result.");
        return false;
    }
    if (query.value(0).toInt() != 1) {
        m_lastError = QStringLiteral("Foreign keys are not enabled.");
        return false;
    }

    return true;
}

bool DatabaseManager::createSchema()
{
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("BEGIN IMMEDIATE TRANSACTION;"))) {
        m_lastError = QStringLiteral("Failed to begin transaction: ") + query.lastError().text();
        return false;
    }

    // Helper lambda to execute a SQL statement and check for errors.
    auto executeSql = [&](const QString& sql) -> bool {
        if (!query.exec(sql)) {
            m_lastError = QStringLiteral("Failed to execute SQL: %1").arg(sql) + QStringLiteral("\nError: ") + query.lastError().text();
            return false;
        }
        return true;
    };

    // Create tables
    const QStringList tableCreationStatements = {
        // users table
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL, "
            "email TEXT NOT NULL UNIQUE, "
            "password_hash TEXT, "
            "created_at TEXT NOT NULL"
            ");"
        ),
        // student_profiles table
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS student_profiles ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL UNIQUE, "
            "student_id TEXT, "
            "college TEXT, "
            "course TEXT, "
            "semester INTEGER, "
            "department TEXT, "
            "academic_year TEXT, "
            "profile_image TEXT, "
            "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE"
            ");"
        ),
        // subjects table
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS subjects ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "
            "name TEXT NOT NULL, "
            "code TEXT, "
            "teacher TEXT, "
            "credits INTEGER, "
            "semester INTEGER, "
            "created_at TEXT NOT NULL, "
            "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE"
            ");"
        ),
        // timetable table
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS timetable ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "
            "subject_id INTEGER NOT NULL, "
            "day_of_week INTEGER NOT NULL, "
            "start_time TEXT NOT NULL, "
            "end_time TEXT NOT NULL, "
            "room TEXT, "
            "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE, "
            "FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE CASCADE"
            ");"
        ),
        // assignments table
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS assignments ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "
            "subject_id INTEGER NOT NULL, "
            "title TEXT NOT NULL, "
            "description TEXT, "
            "deadline TEXT, "
            "priority TEXT, "
            "status TEXT, "
            "created_at TEXT NOT NULL, "
            "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE, "
            "FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE CASCADE"
            ");"
        ),
        // attendance table
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS attendance ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "
            "subject_id INTEGER NOT NULL, "
            "date TEXT NOT NULL, "
            "status TEXT NOT NULL, "
            "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE, "
            "FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE CASCADE"
            ");"
        ),
        // exams table
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS exams ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "
            "subject_id INTEGER NOT NULL, "
            "title TEXT NOT NULL, "
            "exam_date TEXT NOT NULL, "
            "start_time TEXT, "
            "room TEXT, "
            "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE, "
            "FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE CASCADE"
            ");"
        ),
        // notes table
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS notes ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "
            "subject_id INTEGER, "
            "title TEXT NOT NULL, "
            "content TEXT, "
            "created_at TEXT NOT NULL, "
            "updated_at TEXT NOT NULL, "
            "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE, "
            "FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE SET NULL"
            ");"
        )
    };

    for (const QString& statement : tableCreationStatements) {
        if (!executeSql(statement)) {
            query.exec(QStringLiteral("ROLLBACK;"));
            return false;
        }
    }

    if (!migrateAttendanceUniqueness()) {
        query.exec(QStringLiteral("ROLLBACK;"));
        return false;
    }

    if (!query.exec(QStringLiteral("COMMIT;"))) {
        const QString commitError = query.lastError().text();
        query.exec(QStringLiteral("ROLLBACK;"));
        m_lastError = QStringLiteral("Failed to commit transaction: ") + commitError;
        return false;
    }

    return true;
}

bool DatabaseManager::migrateAttendanceUniqueness()
{
    return ensureAttendanceUniquenessMigration(m_db, &m_lastError);
}
