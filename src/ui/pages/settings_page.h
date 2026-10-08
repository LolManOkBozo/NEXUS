// src/ui/pages/settings_page.h
#pragma once

#include <QWidget>

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);
    ~SettingsPage() override = default;
};
