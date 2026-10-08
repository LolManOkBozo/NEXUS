// src/ui/main_window.h
#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>

// Forward declarations of pages
class DashboardPage;
class SubjectsPage;
class TimetablePage;
class AssignmentsPage;
class AttendancePage;
class ExamsPage;
class NotesPage;
class AITutorPage;
class AnalyticsPage;
class SettingsPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupUI();
    void createSidebar();
    void createContent();

    QListWidget *sidebar;
    QStackedWidget *stackedWidget;
};
