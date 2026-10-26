// src/ui/pages/analytics_page.cpp
#include "analytics_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QFrame>
#include <QShowEvent>

#include "../../core/application.h"
#include "../../data/subject_dao.h"
#include "../../data/assignment_dao.h"
#include "../../data/attendance_dao.h"
#include "../../data/exam_dao.h"
#include "../../models/subject.h"
#include "../../models/assignment.h"
#include "../../models/attendance.h"
#include "../../models/exam.h"
#include "../../services/subject_service.h"
#include "../../services/assignment_service.h"
#include "../../services/attendance_service.h"
#include "../../services/exam_service.h"

namespace
{
QFrame *createSummaryCard(const QString &title, QLabel **valueLabel, QProgressBar **progressBar, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("analyticsSummaryCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(8);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("analyticsCardTitle"));
    *valueLabel = new QLabel(QStringLiteral("—"), card);
    (*valueLabel)->setObjectName(QStringLiteral("analyticsCardValue"));
    (*valueLabel)->setStyleSheet(QStringLiteral("font-size: 22pt; font-weight: bold;"));

    layout->addWidget(titleLabel);
    layout->addWidget(*valueLabel);

    if (progressBar) {
        *progressBar = new QProgressBar(card);
        (*progressBar)->setObjectName(QStringLiteral("analyticsCardProgressBar"));
        (*progressBar)->setRange(0, 100);
        (*progressBar)->setValue(0);
        (*progressBar)->setTextVisible(true);
        layout->addWidget(*progressBar);
    }
    return card;
}

QFrame *createStatCard(const QString &title, QLabel **valueLabel, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("analyticsSummaryCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(8);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("analyticsCardTitle"));
    *valueLabel = new QLabel(QStringLiteral("—"), card);
    (*valueLabel)->setObjectName(QStringLiteral("analyticsCardValue"));
    (*valueLabel)->setStyleSheet(QStringLiteral("font-size: 22pt; font-weight: bold;"));

    layout->addWidget(titleLabel);
    layout->addWidget(*valueLabel);
    return card;
}
}

AnalyticsPage::AnalyticsPage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
{
    setObjectName(QStringLiteral("analyticsPage"));

    auto *title = new QLabel(tr("Analytics"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *description = new QLabel(
        tr("Understand your academic performance across attendance, assignments, and exams."),
        this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    auto *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(16);
    cardsLayout->addWidget(createSummaryCard(tr("Overall Attendance"), &m_overallAttendanceLabel, &m_attendanceBar, this));
    cardsLayout->addWidget(createSummaryCard(tr("Assignment Completion"), &m_assignmentCompletionLabel, &m_assignmentBar, this));
    cardsLayout->addWidget(createStatCard(tr("Total Exams"), &m_examCountLabel, this));
    cardsLayout->addWidget(createStatCard(tr("Registered Subjects"), &m_subjectCountLabel, this));

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("analyticsStatus"));
    m_statusLabel->setWordWrap(true);

    auto *breakdownFrame = new QFrame(this);
    breakdownFrame->setObjectName(QStringLiteral("analyticsPanel"));
    auto *breakdownLayout = new QVBoxLayout(breakdownFrame);
    breakdownLayout->setContentsMargins(20, 20, 20, 20);
    breakdownLayout->setSpacing(8);

    auto *breakdownTitle = new QLabel(tr("Subject Summary Breakdown"), breakdownFrame);
    breakdownTitle->setObjectName(QStringLiteral("analyticsPanelTitle"));
    m_subjectBreakdownList = new QListWidget(breakdownFrame);
    m_subjectBreakdownList->setObjectName(QStringLiteral("analyticsSubjectList"));
    m_subjectBreakdownList->setMinimumHeight(240);
    breakdownLayout->addWidget(breakdownTitle);
    breakdownLayout->addWidget(m_subjectBreakdownList, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addLayout(cardsLayout);
    layout->addWidget(m_statusLabel);
    layout->addWidget(breakdownFrame, 1);

    refreshAnalytics();
}

void AnalyticsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshAnalytics();
}

void AnalyticsPage::refreshAnalytics()
{
    m_overallAttendanceLabel->setText(QStringLiteral("—"));
    if (m_attendanceBar) m_attendanceBar->setValue(0);
    m_assignmentCompletionLabel->setText(QStringLiteral("—"));
    if (m_assignmentBar) m_assignmentBar->setValue(0);
    m_examCountLabel->setText(QStringLiteral("—"));
    m_subjectCountLabel->setText(QStringLiteral("—"));
    m_subjectBreakdownList->clear();
    m_statusLabel->clear();

    if (!m_application || !m_application->databaseManager()
        || !m_application->databaseManager()->isOpen()
        || m_application->currentUserId() <= 0) {
        m_statusLabel->setText(tr("Analytics data is unavailable because no current user "
                                  "database connection is open."));
        m_subjectBreakdownList->addItem(tr("Subject summary breakdown is unavailable."));
        return;
    }

    const int userId = m_application->currentUserId();

    SubjectDAO subjectDAO(m_application->databaseManager());
    SubjectService subjectService(&subjectDAO);
    QString subjectError;
    const QList<Subject> subjects = subjectService.getSubjects(userId, &subjectError);
    if (!subjectError.isEmpty()) {
        m_statusLabel->setText(tr("Could not load subjects: %1").arg(subjectError));
        return;
    }
    m_subjectCountLabel->setText(QString::number(subjects.size()));

    AssignmentDAO assignmentDAO(m_application->databaseManager());
    AssignmentService assignmentService(&assignmentDAO, &subjectDAO);
    QString assignmentError;
    const QList<Assignment> assignments = assignmentService.getAssignments(userId, 0, QString(), &assignmentError);
    int completedAssignments = 0;
    for (const Assignment &assignment : assignments) {
        if (assignment.status() == QStringLiteral("Completed")) {
            ++completedAssignments;
        }
    }
    if (!assignments.isEmpty()) {
        const int completionRate = qRound((static_cast<double>(completedAssignments) / assignments.size()) * 100.0);
        m_assignmentCompletionLabel->setText(QStringLiteral("%1% (%2/%3)")
            .arg(completionRate).arg(completedAssignments).arg(assignments.size()));
        if (m_assignmentBar) m_assignmentBar->setValue(completionRate);
    } else {
        m_assignmentCompletionLabel->setText(tr("No assignments"));
        if (m_assignmentBar) m_assignmentBar->setValue(0);
    }

    AttendanceDAO attendanceDAO(m_application->databaseManager());
    AttendanceService attendanceService(&attendanceDAO);
    QString attendanceError;
    const auto overallAttendance = attendanceService.overallAttendancePercentage(userId, &attendanceError);
    if (overallAttendance.has_value()) {
        const int pct = qRound(overallAttendance.value());
        m_overallAttendanceLabel->setText(QStringLiteral("%1%").arg(pct));
        if (m_attendanceBar) m_attendanceBar->setValue(pct);
    } else {
        m_overallAttendanceLabel->setText(tr("No attendance"));
        if (m_attendanceBar) m_attendanceBar->setValue(0);
    }

    ExamDAO examDAO(m_application->databaseManager());
    ExamService examService(&examDAO, &subjectDAO);
    QString examError;
    const QList<Exam> exams = examService.getExams(userId, &examError);
    m_examCountLabel->setText(QString::number(exams.size()));

    if (subjects.isEmpty()) {
        m_subjectBreakdownList->addItem(tr("No subjects registered yet. Add subjects to view individual breakdowns."));
        return;
    }

    for (const Subject &subject : subjects) {
        const QList<Assignment> subjectAssignments = assignmentService.getAssignments(userId, subject.id());
        int subCompleted = 0;
        for (const Assignment &a : subjectAssignments) {
            if (a.status() == QStringLiteral("Completed")) {
                ++subCompleted;
            }
        }
        const auto subAttendance = attendanceService.attendancePercentage(userId, subject.id(), QDate(), QDate());
        QString attText = subAttendance.has_value() ? QStringLiteral("%1%").arg(qRound(subAttendance.value())) : QStringLiteral("N/A");
        QString assignmentText = subjectAssignments.isEmpty() ? QStringLiteral("No assignments") : QStringLiteral("%1/%2 completed").arg(subCompleted).arg(subjectAssignments.size());

        m_subjectBreakdownList->addItem(
            QStringLiteral("• %1 (%2) — Attendance: %3 | Assignments: %4")
                .arg(subject.name())
                .arg(subject.code())
                .arg(attText)
                .arg(assignmentText)
        );
    }
}

