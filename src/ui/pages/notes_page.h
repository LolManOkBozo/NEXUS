// src/ui/pages/notes_page.h
#pragma once

#include <QWidget>

class NotesPage : public QWidget
{
    Q_OBJECT
public:
    explicit NotesPage(QWidget *parent = nullptr);
    ~NotesPage() override = default;
};
