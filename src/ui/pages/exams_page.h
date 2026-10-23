#pragma once

#include <QWidget>

#include <memory>

class Application;
class ExamDAO;
class ExamService;
class SubjectDAO;
class SubjectService;
class QLabel;
class QShowEvent;
class QTableWidget;
class QWidget;

class ExamsPage : public QWidget
{
    Q_OBJECT

public:
    explicit ExamsPage(Application *application, QWidget *parent = nullptr);
    ~ExamsPage() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void refreshExams();
    void addExam();
    void deleteExam(int examId);

private:
    void editExam(int examId);

    Application *m_application;
    std::unique_ptr<ExamDAO> m_examDAO;
    std::unique_ptr<SubjectDAO> m_subjectDAO;
    std::unique_ptr<ExamService> m_examService;
    std::unique_ptr<SubjectService> m_subjectService;
    QTableWidget *m_examTable;
    QWidget *m_emptyState;
    QLabel *m_emptyMessage;
};
