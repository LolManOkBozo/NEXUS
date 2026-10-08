// src/core/database_manager.cpp
#include "database_manager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

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

        databaseFilePath = dir.filePath(QStringLiteral("nexus.db"));
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

    // Verify that foreign keys are enabled
    query.exec(QStringLiteral("PRAGMA foreign_keys;"));
    if (query.next() && query.value(0).toInt() != 1) {
        m_lastError = QStringLiteral("Foreign keys are not enabled.");
        return false;
    }

    return true;
}

bool DatabaseManager::createSchema()
{
    // We'll use a transaction for creating tables to ensure atomicity.
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("BEGIN TRANSACTION;"))) {
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

    if (!query.exec(QStringLiteral("COMMIT;"))) {
        m_lastError = QStringLiteral("Failed to commit transaction: ") + query.lastError().text();
        return false;
    }

    return true;
}
