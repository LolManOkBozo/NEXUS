// src/ui/pages/notes_page.h
#pragma once

#include <QWidget>

#include <memory>

class Application;
class NotesDAO;
class NotesService;
class SubjectDAO;
class SubjectService;
class StudyFileDAO;
class StudyFileService;
class QLabel;
class QShowEvent;
class QTableWidget;
class QWidget;
class QTabWidget;
class QLineEdit;
class QComboBox;

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
    void importFiles();
    void refreshLibrary();
    void openLibraryFile(int fileId);
    void deleteLibraryFile(int fileId);

private:
    void editNote(int noteId);

    Application *m_application;
    std::unique_ptr<NotesDAO> m_notesDAO;
    std::unique_ptr<SubjectDAO> m_subjectDAO;
    std::unique_ptr<NotesService> m_notesService;
    std::unique_ptr<SubjectService> m_subjectService;
    std::unique_ptr<StudyFileDAO> m_studyFileDAO;
    std::unique_ptr<StudyFileService> m_studyFileService;

    QTabWidget *m_tabWidget;

    // Notes tab widgets
    QTableWidget *m_noteTable;
    QWidget *m_emptyState;
    QLabel *m_emptyMessage;

    // Study Library tab widgets
    QTableWidget *m_libraryTable;
    QWidget *m_libraryEmptyState;
    QLabel *m_libraryEmptyMessage;
    QLineEdit *m_librarySearchEdit;
    QComboBox *m_libraryTypeFilter;
    QComboBox *m_librarySubjectFilter;
};
