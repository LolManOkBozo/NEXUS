// src/ui/pages/exams_page.h
#pragma once

#include <QWidget>

class ExamsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ExamsPage(QWidget *parent = nullptr);
    ~ExamsPage() override = default;
};
