// src/ui/pages/dashboard_page.h
#pragma once

#include <QWidget>

class DashboardPage : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardPage(QWidget *parent = nullptr);
    ~DashboardPage() override = default;
};
