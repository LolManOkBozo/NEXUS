// src/ui/pages/analytics_page.h
#pragma once

#include <QWidget>

class AnalyticsPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalyticsPage(QWidget *parent = nullptr);
    ~AnalyticsPage() override = default;
};
