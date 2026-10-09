#pragma once

#include <QWidget>

#include <memory>

class Application;
class AssignmentDAO;
class AssignmentService;
class SubjectDAO;
class SubjectService;
class QComboBox;
class QLabel;
class QTableWidget;

class AssignmentsPage : public QWidget
{
    Q_OBJECT

public:
    explicit AssignmentsPage(Application *application, QWidget *parent = nullptr);
    ~AssignmentsPage() override;

private slots:
    void refreshAssignments();
    void addAssignment();

private:
    void editAssignment(int assignmentId);
    void deleteAssignment(int assignmentId);

    Application *m_application;
    std::unique_ptr<AssignmentDAO> m_assignmentDAO;
    std::unique_ptr<SubjectDAO> m_subjectDAO;
    std::unique_ptr<AssignmentService> m_assignmentService;
    std::unique_ptr<SubjectService> m_subjectService;
    QComboBox *m_subjectFilter;
    QComboBox *m_statusFilter;
    QTableWidget *m_assignmentTable;
    QWidget *m_emptyState;
    QLabel *m_emptyMessage;
};
