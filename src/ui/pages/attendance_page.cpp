#include "attendance_page.h"

#include <algorithm>

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include "../../core/application.h"
#include "../../data/attendance_dao.h"
#include "../../data/subject_dao.h"
#include "../../models/subject.h"
#include "../../services/attendance_service.h"
#include "../../services/subject_service.h"

AttendancePage::AttendancePage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
    , m_attendanceDAO(std::make_unique<AttendanceDAO>(
          application ? application->databaseManager() : nullptr))
    , m_attendanceService(std::make_unique<AttendanceService>(m_attendanceDAO.get()))
    , m_subjectDAO(std::make_unique<SubjectDAO>(
          application ? application->databaseManager() : nullptr))
    , m_subjectService(std::make_unique<SubjectService>(m_subjectDAO.get()))
    , m_subjectCombo(new QComboBox(this))
    , m_attendanceDate(new QDateEdit(QDate::currentDate(), this))
    , m_statusCombo(new QComboBox(this))
    , m_saveButton(new QPushButton(tr("Mark Attendance"), this))
    , m_actionHint(new QLabel(this))
    , m_rangeStart(new QDateEdit(this))
    , m_rangeEnd(new QDateEdit(this))
    , m_percentageLabel(new QLabel(this))
    , m_historyTable(new QTableWidget(this))
    , m_emptyState(new QWidget(this))
    , m_emptyMessage(new QLabel(this))
    , m_feedbackLabel(new QLabel(this))
{
    setObjectName(QStringLiteral("attendancePage"));

    auto *title = new QLabel(tr("Attendance"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *description = new QLabel(
        tr("Record attendance and review your subject history."), this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    m_subjectCombo->setObjectName(QStringLiteral("attendanceSubjectCombo"));
    m_subjectCombo->setMinimumWidth(180);

    m_attendanceDate->setObjectName(QStringLiteral("attendanceDateEdit"));
    m_attendanceDate->setCalendarPopup(true);
    m_attendanceDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));

    m_statusCombo->setObjectName(QStringLiteral("attendanceStatusCombo"));
    m_statusCombo->addItem(tr("Present"), static_cast<int>(Attendance::Status::Present));
    m_statusCombo->addItem(tr("Absent"), static_cast<int>(Attendance::Status::Absent));
    m_statusCombo->addItem(tr("Leave"), static_cast<int>(Attendance::Status::Leave));

    m_saveButton->setObjectName(QStringLiteral("saveAttendanceButton"));
    m_saveButton->setDefault(true);

    auto *entryLayout = new QHBoxLayout;
    entryLayout->setSpacing(12);
    entryLayout->addWidget(new QLabel(tr("Subject:"), this));
    entryLayout->addWidget(m_subjectCombo, 1);
    entryLayout->addWidget(new QLabel(tr("Date:"), this));
    entryLayout->addWidget(m_attendanceDate);
    entryLayout->addWidget(new QLabel(tr("Status:"), this));
    entryLayout->addWidget(m_statusCombo);
    entryLayout->addWidget(m_saveButton);

    m_actionHint->setObjectName(QStringLiteral("attendanceActionHint"));
    m_actionHint->setWordWrap(true);

    const QDate today = QDate::currentDate();
    m_rangeStart->setObjectName(QStringLiteral("attendanceRangeStart"));
    m_rangeStart->setCalendarPopup(true);
    m_rangeStart->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_rangeStart->setDate(QDate(today.year(), today.month(), 1));
    m_rangeEnd->setObjectName(QStringLiteral("attendanceRangeEnd"));
    m_rangeEnd->setCalendarPopup(true);
    m_rangeEnd->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_rangeEnd->setDate(today);

    m_percentageLabel->setObjectName(QStringLiteral("attendancePercentageLabel"));
    m_percentageLabel->setWordWrap(true);
    auto *rangeLayout = new QHBoxLayout;
    rangeLayout->setSpacing(12);
    rangeLayout->addWidget(new QLabel(tr("Percentage range:"), this));
    rangeLayout->addWidget(m_rangeStart);
    rangeLayout->addWidget(new QLabel(tr("through"), this));
    rangeLayout->addWidget(m_rangeEnd);
    rangeLayout->addStretch();
    rangeLayout->addWidget(m_percentageLabel);

    m_historyTable->setObjectName(QStringLiteral("attendanceHistoryTable"));
    m_historyTable->setColumnCount(4);
    m_historyTable->setHorizontalHeaderLabels(
        {tr("Date"), tr("Status"), tr("Edit"), tr("Delete")});
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->verticalHeader()->hide();
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_historyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_historyTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_historyTable->setColumnWidth(2, 100);
    m_historyTable->setColumnWidth(3, 110);

    m_emptyState->setObjectName(QStringLiteral("attendanceEmptyState"));
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setContentsMargins(32, 32, 32, 32);
    emptyLayout->setAlignment(Qt::AlignCenter);
    m_emptyMessage->setObjectName(QStringLiteral("attendanceEmptyMessage"));
    m_emptyMessage->setAlignment(Qt::AlignCenter);
    m_emptyMessage->setWordWrap(true);
    emptyLayout->addWidget(m_emptyMessage);

    m_feedbackLabel->setObjectName(QStringLiteral("attendanceFeedbackLabel"));
    m_feedbackLabel->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addLayout(entryLayout);
    layout->addWidget(m_actionHint);
    layout->addLayout(rangeLayout);
    layout->addWidget(m_feedbackLabel);
    layout->addWidget(m_historyTable, 1);
    layout->addWidget(m_emptyState, 1);

    connect(m_subjectCombo, &QComboBox::currentIndexChanged,
            this, [this] { refreshPage(); });
    connect(m_saveButton, &QPushButton::clicked,
            this, &AttendancePage::saveAttendance);
    connect(m_rangeStart, &QDateEdit::dateChanged,
            this, &AttendancePage::refreshPage);
    connect(m_rangeEnd, &QDateEdit::dateChanged,
            this, &AttendancePage::refreshPage);

    refreshPage();
}

AttendancePage::~AttendancePage() = default;

void AttendancePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshPage();
    QTimer::singleShot(50, this, [this] { window()->repaint(); });
}

bool AttendancePage::selectedSubjectId(int *subjectId) const
{
    if (!subjectId || m_subjectCombo->currentIndex() < 0) {
        return false;
    }
    *subjectId = m_subjectCombo->currentData().toInt();
    return *subjectId > 0;
}

void AttendancePage::refreshPage()
{
    const int previousSubjectId = m_subjectCombo->currentData().toInt();
    if (!m_application || m_application->currentUserId() <= 0) {
        m_subjectCombo->clear();
        m_subjectCombo->setEnabled(false);
        m_saveButton->setEnabled(false);
        m_historyTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("The current user is unavailable."));
        m_percentageLabel->setText(tr("Attendance percentage unavailable."));
        m_actionHint->setText(tr("The current user is unavailable."));
        return;
    }

    const int userId = m_application->currentUserId();
    QString errorMessage;
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        m_subjectCombo->setEnabled(false);
        m_saveButton->setEnabled(false);
        m_historyTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load subjects.\n%1").arg(errorMessage));
        m_percentageLabel->setText(tr("Attendance percentage unavailable."));
        m_actionHint->setText(tr("Attendance cannot be saved until subjects load."));
        return;
    }

    {
        const QSignalBlocker blocker(m_subjectCombo);
        m_subjectCombo->clear();
        for (const Subject &subject : subjects) {
            m_subjectCombo->addItem(subject.name(), subject.id());
        }
        const int previousIndex = m_subjectCombo->findData(previousSubjectId);
        if (previousIndex >= 0) {
            m_subjectCombo->setCurrentIndex(previousIndex);
        } else if (!subjects.isEmpty()) {
            m_subjectCombo->setCurrentIndex(0);
        }
    }

    m_subjectCombo->setEnabled(!subjects.isEmpty());
    if (subjects.isEmpty()) {
        m_editingAttendance.reset();
        m_saveButton->setEnabled(false);
        m_saveButton->setText(tr("Mark Attendance"));
        m_historyTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("No subjects are available. Add a subject before recording attendance."));
        m_percentageLabel->setText(tr("Attendance percentage unavailable."));
        m_actionHint->setText(tr("Add a subject to enable attendance recording."));
        return;
    }

    m_saveButton->setEnabled(true);
    m_actionHint->setText(m_editingAttendance
        ? tr("Editing the selected record. Save to apply your changes.")
        : tr("Selecting a date with an existing record updates that record."));
    m_saveButton->setText(m_editingAttendance ? tr("Update Attendance")
                                               : tr("Mark Attendance"));
    int subjectId = 0;
    if (!selectedSubjectId(&subjectId)) {
        m_historyTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Select a subject to see attendance history."));
        m_percentageLabel->setText(tr("Select a subject to calculate attendance."));
        return;
    }

    QString attendanceError;
    const QList<Attendance> allRecords =
        m_attendanceService->getAttendancesForSubject(userId, subjectId,
                                                      &attendanceError);
    if (!attendanceError.isEmpty()) {
        m_historyTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load attendance.\n%1").arg(attendanceError));
        m_percentageLabel->setText(tr("Attendance percentage unavailable."));
        return;
    }

    QString rangeError;
    const QList<Attendance> rangeRecords =
        m_attendanceService->getAttendancesForSubjectInDateRange(
            userId, subjectId, m_rangeStart->date(), m_rangeEnd->date(),
            &rangeError);
    if (!rangeError.isEmpty()) {
        m_historyTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load attendance history.\n%1").arg(rangeError));
        m_percentageLabel->setText(tr("Attendance percentage unavailable."));
        return;
    }

    m_historyTable->clearContents();
    m_historyTable->setRowCount(rangeRecords.size());
    for (qsizetype row = 0; row < rangeRecords.size(); ++row) {
        const Attendance &attendance = rangeRecords.at(row);
        auto *dateItem = new QTableWidgetItem(
            Attendance::dateToString(attendance.date()));
        dateItem->setData(Qt::UserRole, attendance.id());
        m_historyTable->setItem(row, 0, dateItem);
        QString statusText;
        switch (attendance.status()) {
        case Attendance::Status::Present:
            statusText = tr("Present");
            break;
        case Attendance::Status::Absent:
            statusText = tr("Absent");
            break;
        case Attendance::Status::Leave:
            statusText = tr("Leave");
            break;
        }
        m_historyTable->setItem(row, 1, new QTableWidgetItem(statusText));

        auto *editButton = new QPushButton(tr("Edit"), m_historyTable);
        editButton->setObjectName(QStringLiteral("editAttendanceButton"));
        editButton->setMinimumWidth(76);
        auto *deleteButton = new QPushButton(tr("Delete"), m_historyTable);
        deleteButton->setObjectName(QStringLiteral("deleteAttendanceButton"));
        deleteButton->setMinimumWidth(82);
        m_historyTable->setCellWidget(row, 2, editButton);
        m_historyTable->setCellWidget(row, 3, deleteButton);
        m_historyTable->setRowHeight(row, 56);
        connect(editButton, &QPushButton::clicked, this,
                [this, attendance] { editAttendance(attendance); });
        connect(deleteButton, &QPushButton::clicked, this,
                [this, attendanceId = attendance.id()] {
                    deleteAttendance(attendanceId);
                });
    }

    updatePercentage();
    m_historyTable->setVisible(!rangeRecords.isEmpty());
    m_emptyState->setVisible(rangeRecords.isEmpty());
    if (rangeRecords.isEmpty()) {
        m_emptyMessage->setText(allRecords.isEmpty()
            ? tr("No attendance has been recorded for this subject yet.")
            : tr("No attendance records match the selected date range."));
    }
}

void AttendancePage::saveAttendance()
{
    int subjectId = 0;
    if (!m_application || m_application->currentUserId() <= 0) {
        setFeedback(tr("The current user is unavailable. Attendance was not saved."), true);
        return;
    }
    if (!selectedSubjectId(&subjectId)) {
        setFeedback(tr("Select a subject before saving attendance."), true);
        return;
    }
    if (!m_attendanceDate->date().isValid()) {
        setFeedback(tr("Choose a valid attendance date."), true);
        return;
    }

    const int userId = m_application->currentUserId();
    const QDate date = m_attendanceDate->date();
    const auto status = static_cast<Attendance::Status>(
        m_statusCombo->currentData().toInt());
    QString errorMessage;
    bool saved = false;

    if (m_editingAttendance) {
        Attendance updated = *m_editingAttendance;
        updated.setSubjectId(subjectId);
        updated.setDate(date);
        updated.setStatus(status);
        saved = m_attendanceService->updateAttendance(userId, updated, &errorMessage);
    } else {
        const QList<Attendance> existingRecords =
            m_attendanceService->getAttendancesForSubject(
                userId, subjectId, &errorMessage);
        if (errorMessage.isEmpty()) {
            const auto existing = std::find_if(
                existingRecords.cbegin(), existingRecords.cend(),
                [date](const Attendance &record) { return record.date() == date; });
            if (existing != existingRecords.cend()) {
                Attendance updated = *existing;
                updated.setStatus(status);
                saved = m_attendanceService->updateAttendance(
                    userId, updated, &errorMessage);
            } else {
                saved = m_attendanceService->addAttendance(
                    Attendance(0, userId, subjectId, date, status), &errorMessage);
            }
        }
    }

    if (!saved) {
        setFeedback(tr("Could not save attendance.\n%1")
                        .arg(errorMessage.isEmpty()
                                 ? tr("The operation did not complete.")
                                 : errorMessage),
                    true);
        refreshPage();
        return;
    }

    resetEditor();
    setFeedback(tr("Attendance saved."));
    refreshPage();
}

void AttendancePage::updatePercentage()
{
    int subjectId = 0;
    if (!m_application || !selectedSubjectId(&subjectId)) {
        m_percentageLabel->setText(tr("Select a subject to calculate attendance."));
        return;
    }
    if (m_rangeStart->date() > m_rangeEnd->date()) {
        m_percentageLabel->setText(tr("Choose a valid inclusive date range."));
        return;
    }

    QString errorMessage;
    const std::optional<double> percentage =
        m_attendanceService->attendancePercentage(
            m_application->currentUserId(), subjectId, m_rangeStart->date(),
            m_rangeEnd->date(), &errorMessage);
    if (!errorMessage.isEmpty()) {
        m_percentageLabel->setText(tr("Could not calculate attendance: %1")
                                       .arg(errorMessage));
    } else if (!percentage.has_value()) {
        m_percentageLabel->setText(tr("No attendance recorded"));
    } else {
        m_percentageLabel->setText(tr("Attendance: %1%")
                                       .arg(*percentage, 0, 'f', 1));
    }
}

void AttendancePage::editAttendance(const Attendance &attendance)
{
    if (!m_application || attendance.userId() != m_application->currentUserId()) {
        setFeedback(tr("This attendance record is no longer available for this user."), true);
        refreshPage();
        return;
    }
    const int subjectIndex = m_subjectCombo->findData(attendance.subjectId());
    if (subjectIndex < 0) {
        setFeedback(tr("The subject for this attendance record is no longer available."), true);
        refreshPage();
        return;
    }
    m_editingAttendance = attendance;
    m_subjectCombo->setCurrentIndex(subjectIndex);
    m_attendanceDate->setDate(attendance.date());
    m_statusCombo->setCurrentIndex(
        m_statusCombo->findData(static_cast<int>(attendance.status())));
    m_saveButton->setText(tr("Update Attendance"));
    m_actionHint->setText(tr("Editing the selected record. Save to apply your changes."));
    setFeedback(tr("Update the date or status, then save."));
}

void AttendancePage::deleteAttendance(int attendanceId)
{
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Delete Attendance"),
        tr("Delete this attendance record? This action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    if (!m_application || m_application->currentUserId() <= 0) {
        setFeedback(tr("The current user is unavailable. Attendance was not deleted."), true);
        return;
    }

    QString errorMessage;
    if (!m_attendanceService->deleteAttendance(
            m_application->currentUserId(), attendanceId, &errorMessage)) {
        setFeedback(tr("Could not delete attendance.\n%1").arg(errorMessage), true);
        refreshPage();
        return;
    }
    if (m_editingAttendance && m_editingAttendance->id() == attendanceId) {
        resetEditor();
    }
    setFeedback(tr("Attendance deleted."));
    refreshPage();
}

void AttendancePage::resetEditor()
{
    m_editingAttendance.reset();
    m_attendanceDate->setDate(QDate::currentDate());
    m_statusCombo->setCurrentIndex(0);
    m_saveButton->setText(tr("Mark Attendance"));
    m_actionHint->setText(
        tr("Selecting a date with an existing record updates that record."));
}

void AttendancePage::setFeedback(const QString &message, bool isError)
{
    m_feedbackLabel->setText(message);
    m_feedbackLabel->setStyleSheet(isError
        ? QStringLiteral("color: #ff8a80;")
        : QStringLiteral("color: #b9d7b9;"));
}
