// src/ui/main_window.cpp
#include "main_window.h"
#include "../core/application.h"
#include "pages/dashboard_page.h"
#include "pages/subjects_page.h"
#include "pages/timetable_page.h"
#include "pages/assignments_page.h"
#include "pages/attendance_page.h"
#include "pages/exams_page.h"
#include "pages/notes_page.h"
#include "pages/ai_tutor_page.h"
#include "pages/analytics_page.h"
#include "pages/settings_page.h"

#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : MainWindow(static_cast<Application *>(nullptr), parent)
{
}

MainWindow::MainWindow(Application *application, QWidget *parent)
    : QMainWindow(parent)
    , m_application(application)
{
    setupUI();
    setWindowTitle(QStringLiteral("NEXUS"));
    resize(1600, 900);
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    createSidebar();
    createContent();

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(stackedWidget);

    setCentralWidget(centralWidget);

    // Load stylesheet from resources
    QFile styleSheet(QStringLiteral(":/styles/theme.qss"));
    if (styleSheet.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleSheet.readAll());
        qApp->setStyleSheet(style);
        styleSheet.close();
    }
}

void MainWindow::createSidebar()
{
    sidebar = new QListWidget(this);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(240);
    sidebar->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // Add items in the specified order
    QStringList items = {
        QStringLiteral("Dashboard"),
        QStringLiteral("Subjects"),
        QStringLiteral("Timetable"),
        QStringLiteral("Assignments"),
        QStringLiteral("Attendance"),
        QStringLiteral("Exams"),
        QStringLiteral("Notes"),
        QStringLiteral("AI Tutor"),
        QStringLiteral("Analytics"),
        QStringLiteral("Settings")
    };

    sidebar->addItems(items);
    sidebar->setCurrentRow(0); // Select Dashboard by default
}

void MainWindow::createContent()
{
    stackedWidget = new QStackedWidget(this);

    // Create pages
    DashboardPage *dashboardPage = new DashboardPage();
    SubjectsPage *subjectsPage = new SubjectsPage(m_application);
    TimetablePage *timetablePage = new TimetablePage();
    AssignmentsPage *assignmentsPage = new AssignmentsPage();
    AttendancePage *attendancePage = new AttendancePage();
    ExamsPage *examsPage = new ExamsPage();
    NotesPage *notesPage = new NotesPage();
    AITutorPage *aiTutorPage = new AITutorPage();
    AnalyticsPage *analyticsPage = new AnalyticsPage();
    SettingsPage *settingsPage = new SettingsPage();

    // Add pages to stacked widget
    stackedWidget->addWidget(dashboardPage);
    stackedWidget->addWidget(subjectsPage);
    stackedWidget->addWidget(timetablePage);
    stackedWidget->addWidget(assignmentsPage);
    stackedWidget->addWidget(attendancePage);
    stackedWidget->addWidget(examsPage);
    stackedWidget->addWidget(notesPage);
    stackedWidget->addWidget(aiTutorPage);
    stackedWidget->addWidget(analyticsPage);
    stackedWidget->addWidget(settingsPage);

    // Connect sidebar to stacked widget
    connect(sidebar, &QListWidget::currentRowChanged,
            stackedWidget, &QStackedWidget::setCurrentIndex);
}
