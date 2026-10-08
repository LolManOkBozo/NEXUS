// src/ui/pages/assignments_page.h
#pragma once

#include <QWidget>

class AssignmentsPage : public QWidget
{
    Q_OBJECT
public:
    explicit AssignmentsPage(QWidget *parent = nullptr);
    ~AssignmentsPage() override = default;
};
