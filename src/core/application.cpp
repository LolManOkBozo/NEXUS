// src/core/application.cpp
#include "application.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

Application::Application(QObject *parent)
    : Application(QString(), parent)
{
}

Application::Application(const QString &databasePath, QObject *parent)
    : QObject(parent)
    , m_databaseManager(QStringLiteral("nexus_connection"), databasePath)
{
}

bool Application::initialize()
{
    if (!m_databaseManager.initialize()) {
        m_lastError = m_databaseManager.lastError();
        return false;
    }

    QSqlQuery query(m_databaseManager.database());
    if (!query.exec(QStringLiteral("SELECT id FROM users WHERE email != 'demo@nexus.local' ORDER BY id ASC LIMIT 1"))) {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.next()) {
        m_currentUserId = query.value(0).toInt();
    } else {
        if (!query.exec(QStringLiteral("SELECT id FROM users WHERE email = 'demo@nexus.local'"))) {
            m_lastError = query.lastError().text();
            return false;
        }
        if (query.next()) {
            m_currentUserId = query.value(0).toInt();
        } else {
            if (!query.exec(QStringLiteral("SELECT id FROM users ORDER BY id ASC LIMIT 1"))) {
                m_lastError = query.lastError().text();
                return false;
            }
            if (query.next()) {
                m_currentUserId = query.value(0).toInt();
            } else {
                if (!query.prepare(QStringLiteral(
                        "INSERT INTO users (name, email, created_at) "
                        "VALUES (:name, :email, :created_at)"))) {
                    m_lastError = query.lastError().text();
                    return false;
                }
                query.bindValue(QStringLiteral(":name"), QStringLiteral("NEXUS Demo User"));
                query.bindValue(QStringLiteral(":email"), QStringLiteral("demo@nexus.local"));
                query.bindValue(QStringLiteral(":created_at"),
                                QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                if (!query.exec()) {
                    m_lastError = query.lastError().text();
                    return false;
                }
                m_currentUserId = query.lastInsertId().toInt();
            }
        }
    }

    if (m_currentUserId <= 0) {
        m_lastError = QStringLiteral("The current user has an invalid database ID.");
        return false;
    }
    m_lastError.clear();
    return true;
}

int Application::currentUserId() const
{
    return m_currentUserId;
}

DatabaseManager *Application::databaseManager()
{
    return &m_databaseManager;
}

QString Application::lastError() const
{
    return m_lastError;
}
