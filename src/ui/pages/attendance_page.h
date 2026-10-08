// src/ui/pages/attendance_page.h
#pragma once

#include <QWidget>

class AttendancePage : public QWidget
{
    Q_OBJECT
public:
    explicit AttendancePage(QWidget *parent = nullptr);
    ~AttendancePage() override = default;
};
