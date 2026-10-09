#pragma once

#include <QWidget>

#include <memory>

class Application;
class QLabel;
class QLineEdit;
class QTableWidget;
class SubjectDAO;
class SubjectService;

class SubjectsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SubjectsPage(Application *application, QWidget *parent = nullptr);
    ~SubjectsPage() override;

private slots:
    void addSubject();
    void searchSubjects(const QString &text);

private:
    void setupUi();
    void refreshSubjects();
    void editSubject(int subjectId);
    void deleteSubject(int subjectId);

    Application *m_application;
    std::unique_ptr<SubjectDAO> m_subjectDAO;
    std::unique_ptr<SubjectService> m_subjectService;
    QLineEdit *m_searchEdit;
    QTableWidget *m_subjectsTable;
    QWidget *m_emptyState;
    QLabel *m_emptyMessage;
};
