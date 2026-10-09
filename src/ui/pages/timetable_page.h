#pragma once

#include <QWidget>

#include <memory>

class Application;
class QLabel;
class QComboBox;
class QTableWidget;
class TimetableDAO;
class TimetableService;
class SubjectDAO;
class SubjectService;
class QShowEvent;

class TimetablePage : public QWidget
{
    Q_OBJECT

public:
    explicit TimetablePage(Application *application, QWidget *parent = nullptr);
    ~TimetablePage() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void refreshTimetable();
    void addTimetable();

private:
    void setupUi();
    void editTimetable(int timetableId);
    void deleteTimetable(int timetableId);

    Application *m_application;
    std::unique_ptr<TimetableDAO> m_timetableDAO;
    std::unique_ptr<TimetableService> m_timetableService;
    std::unique_ptr<SubjectDAO> m_subjectDAO;
    std::unique_ptr<SubjectService> m_subjectService;
    QComboBox *m_weekdayCombo;
    QTableWidget *m_timetableTable;
    QWidget *m_emptyState;
    QLabel *m_emptyMessage;
};
