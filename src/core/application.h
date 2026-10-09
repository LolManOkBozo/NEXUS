// src/core/application.h
#pragma once

#include <QObject>
#include <QString>

#include "database_manager.h"

class Application : public QObject
{
    Q_OBJECT
public:
    explicit Application(QObject *parent = nullptr);
    explicit Application(const QString &databasePath, QObject *parent = nullptr);
    ~Application() override = default;

    bool initialize();
    int currentUserId() const;
    DatabaseManager *databaseManager();
    QString lastError() const;

private:
    DatabaseManager m_databaseManager;
    int m_currentUserId{0};
    QString m_lastError;
};
