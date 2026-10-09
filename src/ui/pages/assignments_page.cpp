#include "assignments_page.h"

#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "../../core/application.h"
#include "../../data/assignment_dao.h"
#include "../../data/subject_dao.h"
#include "../../models/assignment.h"
#include "../../models/subject.h"
#include "../../services/assignment_service.h"
#include "../../services/subject_service.h"
#include "../dialogs/assignment_dialog.h"

AssignmentsPage::AssignmentsPage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
    , m_assignmentDAO(std::make_unique<AssignmentDAO>(
          application ? application->databaseManager() : nullptr))
    , m_subjectDAO(std::make_unique<SubjectDAO>(
          application ? application->databaseManager() : nullptr))
    , m_assignmentService(std::make_unique<AssignmentService>(
          m_assignmentDAO.get(), m_subjectDAO.get()))
    , m_subjectService(std::make_unique<SubjectService>(m_subjectDAO.get()))
    , m_subjectFilter(new QComboBox(this))
    , m_statusFilter(new QComboBox(this))
    , m_assignmentTable(new QTableWidget(this))
    , m_emptyState(new QWidget(this))
    , m_emptyMessage(new QLabel(this))
{
    setObjectName(QStringLiteral("assignmentsPage"));

    auto *title = new QLabel(tr("Assignments"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *description = new QLabel(
        tr("Organize your coursework and keep track of upcoming deadlines."), this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    auto *addButton = new QPushButton(tr("Add Assignment"), this);
    addButton->setObjectName(QStringLiteral("addAssignmentButton"));
    m_subjectFilter->setObjectName(QStringLiteral("assignmentSubjectFilter"));
    m_subjectFilter->addItem(tr("All subjects"), 0);
    m_statusFilter->setObjectName(QStringLiteral("assignmentStatusFilter"));
    m_statusFilter->addItem(tr("All statuses"), QString());
    for (const QString &status : {QStringLiteral("Not Started"),
                                  QStringLiteral("In Progress"),
                                  QStringLiteral("Completed")}) {
        m_statusFilter->addItem(status, status);
    }

    auto *actionBar = new QHBoxLayout;
    actionBar->setSpacing(12);
    actionBar->addWidget(new QLabel(tr("Subject:"), this));
    actionBar->addWidget(m_subjectFilter);
    actionBar->addWidget(new QLabel(tr("Status:"), this));
    actionBar->addWidget(m_statusFilter);
    actionBar->addStretch();
    actionBar->addWidget(addButton);

    m_assignmentTable->setObjectName(QStringLiteral("assignmentsTable"));
    m_assignmentTable->setColumnCount(6);
    m_assignmentTable->setHorizontalHeaderLabels(
        {tr("Assignment"), tr("Subject"), tr("Deadline"), tr("Priority"),
         tr("Status"), tr("Actions")});
    m_assignmentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_assignmentTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_assignmentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_assignmentTable->setAlternatingRowColors(true);
    m_assignmentTable->verticalHeader()->hide();
    m_assignmentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_assignmentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_assignmentTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_assignmentTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_assignmentTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_assignmentTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_assignmentTable->setColumnWidth(5, 194);

    m_emptyState->setObjectName(QStringLiteral("assignmentsEmptyState"));
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setContentsMargins(32, 32, 32, 32);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(12);
    m_emptyMessage->setObjectName(QStringLiteral("assignmentsEmptyMessage"));
    m_emptyMessage->setAlignment(Qt::AlignCenter);
    m_emptyMessage->setWordWrap(true);
    auto *emptyAddButton = new QPushButton(tr("Add Assignment"), m_emptyState);
    emptyAddButton->setObjectName(QStringLiteral("emptyAddAssignmentButton"));
    emptyLayout->addWidget(m_emptyMessage);
    emptyLayout->addWidget(emptyAddButton, 0, Qt::AlignCenter);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addLayout(actionBar);
    layout->addWidget(m_assignmentTable, 1);
    layout->addWidget(m_emptyState, 1);

    connect(addButton, &QPushButton::clicked, this, &AssignmentsPage::addAssignment);
    connect(emptyAddButton, &QPushButton::clicked, this, &AssignmentsPage::addAssignment);
    connect(m_subjectFilter, &QComboBox::currentIndexChanged,
            this, &AssignmentsPage::refreshAssignments);
    connect(m_statusFilter, &QComboBox::currentIndexChanged,
            this, &AssignmentsPage::refreshAssignments);
    refreshAssignments();
}

AssignmentsPage::~AssignmentsPage() = default;

void AssignmentsPage::refreshAssignments()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        m_assignmentTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("The current user is unavailable."));
        return;
    }

    const int userId = m_application->currentUserId();
    QString errorMessage;
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        m_assignmentTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load subjects.\n%1").arg(errorMessage));
        return;
    }

    const int selectedSubjectId = m_subjectFilter->currentData().toInt();
    const QString selectedStatus = m_statusFilter->currentData().toString();
    const QSignalBlocker subjectBlocker(m_subjectFilter);
    m_subjectFilter->clear();
    m_subjectFilter->addItem(tr("All subjects"), 0);
    for (const Subject &subject : subjects) {
        m_subjectFilter->addItem(subject.name(), subject.id());
    }
    const int subjectIndex = m_subjectFilter->findData(selectedSubjectId);
    m_subjectFilter->setCurrentIndex(subjectIndex >= 0 ? subjectIndex : 0);

    QString assignmentError;
    const QList<Assignment> assignments = m_assignmentService->getAssignments(
        userId, m_subjectFilter->currentData().toInt(), selectedStatus, &assignmentError);
    if (!assignmentError.isEmpty()) {
        m_assignmentTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load assignments.\n%1").arg(assignmentError));
        return;
    }

    QHash<int, QString> subjectNames;
    for (const Subject &subject : subjects) {
        subjectNames.insert(subject.id(), subject.name());
    }

    m_assignmentTable->clearContents();
    m_assignmentTable->setRowCount(assignments.size());
    const QDateTime reference = QDateTime::currentDateTime();
    for (qsizetype row = 0; row < assignments.size(); ++row) {
        const Assignment &assignment = assignments.at(row);
        auto *titleItem = new QTableWidgetItem(assignment.title());
        titleItem->setData(Qt::UserRole, assignment.id());
        titleItem->setToolTip(assignment.description());
        m_assignmentTable->setItem(row, 0, titleItem);
        m_assignmentTable->setItem(
            row, 1, new QTableWidgetItem(
                        subjectNames.value(assignment.subjectId(), tr("Unknown subject"))));

        QString deadlineText;
        const QDateTime deadline = AssignmentService::parseDeadline(assignment);
        const auto classification =
            AssignmentService::classifyDeadline(assignment, reference);
        if (!deadline.isValid()) {
            deadlineText = tr("Invalid deadline");
        } else {
            const QString dateText = assignment.dueDate();
            const QString detail = assignment.dueTime().isEmpty()
                ? tr("%1 (end of day)").arg(dateText)
                : tr("%1 at %2").arg(dateText, assignment.dueTime());
            switch (classification) {
            case AssignmentService::DeadlineClassification::Overdue:
                deadlineText = tr("Overdue: %1").arg(detail);
                break;
            case AssignmentService::DeadlineClassification::Upcoming:
                deadlineText = tr("Upcoming: %1").arg(detail);
                break;
            case AssignmentService::DeadlineClassification::Completed:
                deadlineText = detail;
                break;
            case AssignmentService::DeadlineClassification::Invalid:
                deadlineText = tr("Invalid deadline");
                break;
            }
        }
        m_assignmentTable->setItem(row, 2, new QTableWidgetItem(deadlineText));
        m_assignmentTable->setItem(row, 3, new QTableWidgetItem(assignment.priority()));
        m_assignmentTable->setItem(row, 4, new QTableWidgetItem(assignment.status()));

        auto *actionWidget = new QWidget(m_assignmentTable);
        actionWidget->setObjectName(QStringLiteral("assignmentActionWidget"));
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(8, 5, 8, 5);
        actionLayout->setSpacing(8);
        auto *editButton = new QPushButton(tr("Edit"), actionWidget);
        editButton->setObjectName(QStringLiteral("editAssignmentButton"));
        editButton->setMinimumWidth(76);
        auto *deleteButton = new QPushButton(tr("Delete"), actionWidget);
        deleteButton->setObjectName(QStringLiteral("deleteAssignmentButton"));
        deleteButton->setMinimumWidth(82);
        actionLayout->addWidget(editButton);
        actionLayout->addWidget(deleteButton);
        actionWidget->setMinimumWidth(178);
        m_assignmentTable->setCellWidget(row, 5, actionWidget);
        m_assignmentTable->setRowHeight(row, 50);
        connect(editButton, &QPushButton::clicked, this,
                [this, assignmentId = assignment.id()] { editAssignment(assignmentId); });
        connect(deleteButton, &QPushButton::clicked, this,
                [this, assignmentId = assignment.id()] { deleteAssignment(assignmentId); });
    }

    const bool hasAssignments = !assignments.isEmpty();
    m_assignmentTable->setVisible(hasAssignments);
    m_emptyState->setVisible(!hasAssignments);
    if (!hasAssignments) {
        if (subjects.isEmpty()) {
            m_emptyMessage->setText(tr("Add a subject before creating an assignment."));
        } else {
            m_emptyMessage->setText(
                selectedSubjectId > 0 || !selectedStatus.isEmpty()
                    ? tr("No assignments match these filters.")
                    : tr("No assignments added yet."));
        }
    }
}

void AssignmentsPage::addAssignment()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }

    QString errorMessage;
    const QList<Subject> subjects =
        m_subjectService->getSubjects(m_application->currentUserId(), &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Could Not Load Subjects"), errorMessage);
        return;
    }
    if (subjects.isEmpty()) {
        QMessageBox::information(this, tr("No Subjects"),
                                 tr("Add a subject before creating an assignment."));
        return;
    }

    AssignmentDialog dialog(subjects, this);
    connect(&dialog, &AssignmentDialog::saveRequested, this,
            [this, &dialog](Assignment assignment) {
                assignment.setUserId(m_application->currentUserId());
                QString saveError;
                if (!m_assignmentService->addAssignment(assignment, &saveError)) {
                    QMessageBox::warning(&dialog, tr("Could Not Add Assignment"), saveError);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() == QDialog::Accepted) {
        refreshAssignments();
    }
}

void AssignmentsPage::editAssignment(int assignmentId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }

    const int userId = m_application->currentUserId();
    QString errorMessage;
    const Assignment assignment =
        m_assignmentService->getAssignment(userId, assignmentId, &errorMessage);
    if (!errorMessage.isEmpty() || assignment.id() <= 0) {
        QMessageBox::warning(this, tr("Could Not Load Assignment"),
                             errorMessage.isEmpty() ? tr("The assignment no longer exists.")
                                                    : errorMessage);
        refreshAssignments();
        return;
    }
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Could Not Load Subjects"), errorMessage);
        return;
    }

    AssignmentDialog dialog(subjects, this, assignment);
    connect(&dialog, &AssignmentDialog::saveRequested, this,
            [this, &dialog, userId](Assignment updated) {
                updated.setUserId(userId);
                QString saveError;
                if (!m_assignmentService->updateAssignment(updated, &saveError)) {
                    QMessageBox::warning(&dialog, tr("Could Not Update Assignment"), saveError);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() == QDialog::Accepted) {
        refreshAssignments();
    }
}

void AssignmentsPage::deleteAssignment(int assignmentId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Delete Assignment"),
        tr("Are you sure you want to delete this assignment?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString errorMessage;
    if (!m_assignmentService->deleteAssignment(
            m_application->currentUserId(), assignmentId, &errorMessage)) {
        QMessageBox::warning(this, tr("Could Not Delete Assignment"), errorMessage);
        return;
    }
    refreshAssignments();
}
