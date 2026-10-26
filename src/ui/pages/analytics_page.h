// src/ui/pages/analytics_page.h
#pragma once

#include <QWidget>

class Application;
class QLabel;
class QProgressBar;
class QListWidget;

class AnalyticsPage : public QWidget
{
    Q_OBJECT
public:
    explicit AnalyticsPage(Application *application, QWidget *parent = nullptr);
    ~AnalyticsPage() override = default;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void refreshAnalytics();

    Application *m_application{nullptr};
    QLabel *m_overallAttendanceLabel{nullptr};
    QProgressBar *m_attendanceBar{nullptr};
    QLabel *m_assignmentCompletionLabel{nullptr};
    QProgressBar *m_assignmentBar{nullptr};
    QLabel *m_examCountLabel{nullptr};
    QLabel *m_subjectCountLabel{nullptr};
    QListWidget *m_subjectBreakdownList{nullptr};
    QLabel *m_statusLabel{nullptr};
};

