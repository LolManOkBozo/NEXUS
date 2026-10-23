// src/ui/pages/dashboard_page.h
#pragma once

#include <QWidget>

class Application;
class QLabel;
class QListWidget;
class QShowEvent;

class DashboardPage : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardPage(Application *application = nullptr,
                           QWidget *parent = nullptr);
    ~DashboardPage() override = default;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void refreshData();

    Application *m_application;
    QLabel *m_subjectCountValue{nullptr};
    QLabel *m_pendingAssignmentValue{nullptr};
    QLabel *m_overdueAssignmentValue{nullptr};
    QLabel *m_upcomingAssignmentValue{nullptr};
    QLabel *m_upcomingExamValue{nullptr};
    QLabel *m_attendanceValue{nullptr};
    QLabel *m_statusLabel{nullptr};
    QListWidget *m_todaySchedule{nullptr};
    QListWidget *m_upcomingItems{nullptr};
};
