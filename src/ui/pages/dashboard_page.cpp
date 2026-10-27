#include "dashboard_page.h"

#include <QDate>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QShowEvent>
#include <QStringList>
#include <QVBoxLayout>

#include "../../core/application.h"
#include "../../data/assignment_dao.h"
#include "../../data/attendance_dao.h"
#include "../../data/exam_dao.h"
#include "../../data/subject_dao.h"
#include "../../data/timetable_dao.h"
#include "../../models/assignment.h"
#include "../../models/exam.h"
#include "../../models/subject.h"
#include "../../models/timetable.h"
#include "../../services/assignment_service.h"
#include "../../services/attendance_service.h"
#include "../../services/exam_service.h"
#include "../../services/subject_service.h"
#include "../../services/timetable_service.h"

namespace
{
QFrame *createHeroCard(const QString &title, const QString &valueObjectName,
                       QLabel **valueLabel, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("dashboardHeroCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(28, 28, 28, 28);
    layout->setSpacing(12);

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("dashboardHeroTitle"));
    auto *badgeLabel = new QLabel(QCoreApplication::translate("DashboardPage", "KEY METRIC"), card);
    badgeLabel->setObjectName(QStringLiteral("dashboardHeroBadge"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(badgeLabel);

    *valueLabel = new QLabel(QStringLiteral("—"), card);
    (*valueLabel)->setObjectName(valueObjectName);
    (*valueLabel)->setStyleSheet(QStringLiteral("font-size: 32pt; font-weight: 800; color: #ff6b00;"));

    layout->addLayout(headerLayout);
    layout->addWidget(*valueLabel);
    return card;
}

QFrame *createListPanel(const QString &title, const QString &indexNum, QListWidget **listWidget,
                        const QString &objectName, QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("dashboardPanel"));
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(28, 28, 28, 28);
    layout->setSpacing(16);

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto *titleLabel = new QLabel(title, frame);
    titleLabel->setObjectName(QStringLiteral("dashboardPanelTitle"));
    auto *indexLabel = new QLabel(indexNum, frame);
    indexLabel->setObjectName(QStringLiteral("dashboardPanelIndex"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(indexLabel);

    *listWidget = new QListWidget(frame);
    (*listWidget)->setObjectName(objectName);
    (*listWidget)->setMinimumHeight(260);
    layout->addLayout(headerLayout);
    layout->addWidget(*listWidget, 1);
    return frame;
}

QFrame *createMetricCard(const QString &title, const QString &valueObjectName,
                         QLabel **valueLabel, int index, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("dashboardMetricCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(6);

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("dashboardMetricTitle"));
    auto *indexLabel = new QLabel(QStringLiteral("0%1").arg(index), card);
    indexLabel->setObjectName(QStringLiteral("dashboardMetricIndex"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(indexLabel);

    *valueLabel = new QLabel(QStringLiteral("—"), card);
    (*valueLabel)->setObjectName(valueObjectName);
    (*valueLabel)->setStyleSheet(QStringLiteral("font-size: 22pt; font-weight: bold;"));

    layout->addLayout(headerLayout);
    layout->addWidget(*valueLabel);
    return card;
}

QString subjectName(const QHash<int, QString> &subjectNames, int subjectId)
{
    return subjectNames.value(subjectId, QStringLiteral("Subject unavailable"));
}

QString examDateFromDatabase(const QString &dateText)
{
    const QDate date = QDate::fromString(dateText, Qt::ISODate);
    return date.isValid() && date.toString(Qt::ISODate) == dateText
        ? date.toString(QStringLiteral("ddd, MMM d"))
        : dateText;
}
}

DashboardPage::DashboardPage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
{
    setObjectName(QStringLiteral("dashboardPage"));

    auto *welcomeTitle = new QLabel(tr("Welcome to NEXUS"), this);
    welcomeTitle->setObjectName(QStringLiteral("welcomeTitle"));
    auto *welcomeSubtitle = new QLabel(
        tr("Your academic command center."), this);
    welcomeSubtitle->setObjectName(QStringLiteral("welcomeSubtitle"));

    auto *welcomeLayout = new QVBoxLayout;
    welcomeLayout->addWidget(welcomeTitle);
    welcomeLayout->addWidget(welcomeSubtitle);
    welcomeLayout->setSpacing(4);

    auto *topRowLayout = new QHBoxLayout;
    topRowLayout->setSpacing(20);

    auto *metricsWidget = new QWidget(this);
    auto *cardsLayout = new QGridLayout(metricsWidget);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(16);
    cardsLayout->addWidget(createMetricCard(
        tr("Subjects"), QStringLiteral("dashboardSubjectCountValue"),
        &m_subjectCountValue, 1, metricsWidget), 0, 0);
    cardsLayout->addWidget(createMetricCard(
        tr("Open Assignments"), QStringLiteral("dashboardPendingAssignmentValue"),
        &m_pendingAssignmentValue, 2, metricsWidget), 0, 1);
    cardsLayout->addWidget(createMetricCard(
        tr("Overdue Assignments"), QStringLiteral("dashboardOverdueAssignmentValue"),
        &m_overdueAssignmentValue, 3, metricsWidget), 0, 2);
    cardsLayout->addWidget(createMetricCard(
        tr("Assignments: Next 7 Days"),
        QStringLiteral("dashboardUpcomingAssignmentValue"),
        &m_upcomingAssignmentValue, 4, metricsWidget), 1, 0);
    cardsLayout->addWidget(createMetricCard(
        tr("Exams: Next 7 Days"), QStringLiteral("dashboardUpcomingExamValue"),
        &m_upcomingExamValue, 5, metricsWidget), 1, 1);

    auto *heroCard = createHeroCard(tr("Overall Attendance"),
                                      QStringLiteral("dashboardAttendanceValue"),
                                      &m_attendanceValue, this);

    topRowLayout->addWidget(metricsWidget, 2);
    topRowLayout->addWidget(heroCard, 1);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("dashboardStatus"));
    m_statusLabel->setWordWrap(true);

    auto *panelsLayout = new QHBoxLayout;
    panelsLayout->setSpacing(20);
    panelsLayout->addWidget(createListPanel(
        tr("Today's Schedule"), QStringLiteral("01"), &m_todaySchedule,
        QStringLiteral("dashboardTodaySchedule"), this), 1);
    panelsLayout->addWidget(createListPanel(
        tr("Upcoming Assignments & Exams"), QStringLiteral("02"), &m_upcomingItems,
        QStringLiteral("dashboardUpcomingItems"), this), 1);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 36, 36, 36);
    mainLayout->setSpacing(24);
    mainLayout->addLayout(welcomeLayout);
    mainLayout->addLayout(topRowLayout);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(panelsLayout, 1);

    refreshData();
}

void DashboardPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshData();
}

void DashboardPage::refreshData()
{
    m_subjectCountValue->setText(QStringLiteral("—"));
    m_pendingAssignmentValue->setText(QStringLiteral("—"));
    m_overdueAssignmentValue->setText(QStringLiteral("—"));
    m_upcomingAssignmentValue->setText(QStringLiteral("—"));
    m_upcomingExamValue->setText(QStringLiteral("—"));
    m_attendanceValue->setText(tr("No attendance recorded"));
    m_todaySchedule->clear();
    m_upcomingItems->clear();
    m_statusLabel->clear();

    if (!m_application || !m_application->databaseManager()
        || !m_application->databaseManager()->isOpen()
        || m_application->currentUserId() <= 0) {
        m_statusLabel->setText(tr("Dashboard data is unavailable because no current user "
                                  "database connection is open."));
        m_todaySchedule->addItem(tr("Today's schedule is unavailable."));
        m_upcomingItems->addItem(tr("Upcoming items are unavailable."));
        return;
    }

    const int userId = m_application->currentUserId();
    QStringList errors;

    SubjectDAO subjectDAO(m_application->databaseManager());
    SubjectService subjectService(&subjectDAO);
    QString subjectError;
    const QList<Subject> subjects = subjectService.getSubjects(userId, &subjectError);
    if (!subjectError.isEmpty()) {
        errors.append(tr("Subjects: %1").arg(subjectError));
    } else {
        m_subjectCountValue->setText(QString::number(subjects.size()));
    }
    QHash<int, QString> subjectNames;
    for (const Subject &subject : subjects) {
        subjectNames.insert(subject.id(), subject.name());
    }

    const QDate today = QDate::currentDate();
    const QDate lastUpcomingDay = today.addDays(6);
    const QDateTime now = QDateTime::currentDateTime();

    AssignmentDAO assignmentDAO(m_application->databaseManager());
    AssignmentService assignmentService(&assignmentDAO, &subjectDAO);
    QString assignmentError;
    const QList<Assignment> assignments =
        assignmentService.getAssignments(userId, 0, QString(), &assignmentError);
    if (!assignmentError.isEmpty()) {
        errors.append(tr("Assignments: %1").arg(assignmentError));
    } else {
        int openCount = 0;
        int overdueCount = 0;
        int upcomingCount = 0;
        for (const Assignment &assignment : assignments) {
            const auto classification =
                AssignmentService::classifyDeadline(assignment, now);
            if (assignment.status() != QStringLiteral("Completed")
                && (assignment.status() == QStringLiteral("Not Started")
                    || assignment.status() == QStringLiteral("In Progress"))) {
                ++openCount;
            }
            if (classification == AssignmentService::DeadlineClassification::Overdue) {
                ++overdueCount;
            }
            if (classification == AssignmentService::DeadlineClassification::Upcoming) {
                const QDate dueDate = QDate::fromString(assignment.dueDate(), Qt::ISODate);
                if (dueDate.isValid() && dueDate <= lastUpcomingDay) {
                    ++upcomingCount;
                    QString item = tr("Assignment · %1 · %2")
                        .arg(assignment.title(),
                             subjectName(subjectNames, assignment.subjectId()));
                    if (!assignment.dueTime().isEmpty()) {
                        item += QStringLiteral(" · %1").arg(assignment.dueTime());
                    }
                    m_upcomingItems->addItem(
                        QStringLiteral("%1 — %2")
                            .arg(assignment.dueDate(), item));
                }
            }
        }
        m_pendingAssignmentValue->setText(QString::number(openCount));
        m_overdueAssignmentValue->setText(QString::number(overdueCount));
        m_upcomingAssignmentValue->setText(QString::number(upcomingCount));
    }

    TimetableDAO timetableDAO(m_application->databaseManager());
    TimetableService timetableService(&timetableDAO);
    QString timetableError;
    const QList<Timetable> todayEntries =
        timetableService.getTimetablesForDay(userId, today.dayOfWeek(), &timetableError);
    if (!timetableError.isEmpty()) {
        errors.append(tr("Today's timetable: %1").arg(timetableError));
    } else if (todayEntries.isEmpty()) {
        m_todaySchedule->addItem(tr("No classes scheduled for today."));
    } else {
        for (const Timetable &entry : todayEntries) {
            QString item = QStringLiteral("%1–%2 · %3")
                .arg(entry.startTime(), entry.endTime(),
                     subjectName(subjectNames, entry.subjectId()));
            if (!entry.room().trimmed().isEmpty()) {
                item += QStringLiteral(" · %1").arg(entry.room().trimmed());
            }
            m_todaySchedule->addItem(item);
        }
    }

    ExamDAO examDAO(m_application->databaseManager());
    ExamService examService(&examDAO, &subjectDAO);
    QString examError;
    const QList<Exam> exams = examService.getExams(userId, &examError);
    if (!examError.isEmpty()) {
        errors.append(tr("Exams: %1").arg(examError));
    } else {
        int examCount = 0;
        for (const Exam &exam : exams) {
            const QDate examDate = QDate::fromString(exam.examDate(), Qt::ISODate);
            if (!examDate.isValid()
                || examDate.toString(Qt::ISODate) != exam.examDate()
                || examDate < today || examDate > lastUpcomingDay) {
                continue;
            }
            ++examCount;
            QString item = tr("Exam · %1 · %2")
                .arg(exam.title(), subjectName(subjectNames, exam.subjectId()));
            if (!exam.startTime().isEmpty()) {
                item += QStringLiteral(" · %1").arg(exam.startTime());
            }
            if (!exam.room().trimmed().isEmpty()) {
                item += QStringLiteral(" · %1").arg(exam.room().trimmed());
            }
            m_upcomingItems->addItem(
                QStringLiteral("%1 — %2")
                    .arg(examDateFromDatabase(exam.examDate()), item));
        }
        m_upcomingExamValue->setText(QString::number(examCount));
    }

    if (m_upcomingItems->count() == 0) {
        m_upcomingItems->addItem(tr("No assignments or exams due in the next 7 days."));
    }

    AttendanceDAO attendanceDAO(m_application->databaseManager());
    AttendanceService attendanceService(&attendanceDAO);
    QString attendanceError;
    const std::optional<double> attendance =
        attendanceService.overallAttendancePercentage(userId, &attendanceError);
    if (!attendanceError.isEmpty()) {
        errors.append(tr("Attendance: %1").arg(attendanceError));
    } else if (attendance.has_value()) {
        m_attendanceValue->setText(tr("%1%").arg(*attendance, 0, 'f', 1));
    }

    m_statusLabel->setText(errors.join(QLatin1Char('\n')));
}
