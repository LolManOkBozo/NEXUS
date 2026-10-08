// src/core/application.h
#pragma once

#include <QObject>

class Application : public QObject
{
    Q_OBJECT
public:
    explicit Application(QObject *parent = nullptr);
    ~Application() override = default;
};
