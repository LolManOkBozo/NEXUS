// src/ui/pages/subjects_page.h
#pragma once

#include <QWidget>

class SubjectsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SubjectsPage(QWidget *parent = nullptr);
    ~SubjectsPage() override = default;
};
