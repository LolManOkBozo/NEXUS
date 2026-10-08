// src/core/database_manager.h
#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QStandardPaths>
#include <QDir>

class DatabaseManager
{
public:
    explicit DatabaseManager(const QString& connectionName = QStringLiteral("nexus_connection"),
                             const QString& databasePath = QString());
    ~DatabaseManager();

    bool initialize();
    void close();

    bool isOpen() const;

    QString databasePath() const;
    QString lastError() const;

    QSqlDatabase database() const;

private:
    bool createSchema();
    bool enableForeignKeys();

    QString m_connectionName;
    QString m_customDatabasePath; // If non-empty, use this path instead of the standard location
    mutable QSqlDatabase m_db;
    QString m_lastError;
};
