// src/ui/pages/notes_page.h
#pragma once

#include <QWidget>

#include <memory>

class Application;
class NotesDAO;
class NotesService;
class SubjectDAO;
class SubjectService;
class QLabel;
class QShowEvent;
class QTableWidget;
class QWidget;

class NotesPage : public QWidget
{
    Q_OBJECT
public:
    explicit NotesPage(Application *application, QWidget *parent = nullptr);
    ~NotesPage() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void refreshNotes();
    void addNote();
    void deleteNote(int noteId);

private:
    void editNote(int noteId);

    Application *m_application;
    std::unique_ptr<NotesDAO> m_notesDAO;
    std::unique_ptr<SubjectDAO> m_subjectDAO;
    std::unique_ptr<NotesService> m_notesService;
    std::unique_ptr<SubjectService> m_subjectService;
    QTableWidget *m_noteTable;
    QWidget *m_emptyState;
    QLabel *m_emptyMessage;
};
