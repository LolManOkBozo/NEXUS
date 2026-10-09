// src/ui/pages/timetable_page.h
#pragma once

#include <QWidget>

class TimetablePage : public QWidget
{
    Q_OBJECT
public:
    explicit TimetablePage(QWidget *parent = nullptr);
    ~TimetablePage() override = default;
};
