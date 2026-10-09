#include "timetable_page.h"

#include <QComboBox>
#include <QHash>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include "../../core/application.h"
#include "../../data/subject_dao.h"
#include "../../data/timetable_dao.h"
#include "../../models/subject.h"
#include "../../models/timetable.h"
#include "../../services/subject_service.h"
#include "../../services/timetable_service.h"
#include "../dialogs/timetable_entry_dialog.h"

TimetablePage::TimetablePage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
    , m_timetableDAO(std::make_unique<TimetableDAO>(
          application ? application->databaseManager() : nullptr))
    , m_timetableService(std::make_unique<TimetableService>(m_timetableDAO.get()))
    , m_subjectDAO(std::make_unique<SubjectDAO>(
          application ? application->databaseManager() : nullptr))
    , m_subjectService(std::make_unique<SubjectService>(m_subjectDAO.get()))
    , m_weekdayCombo(new QComboBox(this))
    , m_timetableTable(new QTableWidget(this))
    , m_emptyState(new QWidget(this))
    , m_emptyMessage(new QLabel(this))
{
    setObjectName(QStringLiteral("timetablePage"));
    setupUi();
    refreshTimetable();
}

TimetablePage::~TimetablePage() = default;

void TimetablePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(50, this, [this] { window()->repaint(); });
}

void TimetablePage::setupUi()
{
    auto *title = new QLabel(tr("Timetable"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *description = new QLabel(
        tr("View and manage your weekly class schedule."), this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    m_weekdayCombo->setObjectName(QStringLiteral("timetableWeekdayCombo"));
    const QStringList weekdays{
        tr("Monday"), tr("Tuesday"), tr("Wednesday"), tr("Thursday"),
        tr("Friday"), tr("Saturday"), tr("Sunday")
    };
    for (qsizetype index = 0; index < weekdays.size(); ++index) {
        m_weekdayCombo->addItem(weekdays.at(index), index + 1);
    }

    auto *addButton = new QPushButton(tr("Add Entry"), this);
    addButton->setObjectName(QStringLiteral("addTimetableButton"));

    auto *actionBar = new QHBoxLayout;
    actionBar->setSpacing(12);
    actionBar->addWidget(new QLabel(tr("Weekday:"), this));
    actionBar->addWidget(m_weekdayCombo);
    actionBar->addStretch();
    actionBar->addWidget(addButton);

    m_timetableTable->setObjectName(QStringLiteral("timetableTable"));
    m_timetableTable->setColumnCount(4);
    m_timetableTable->setHorizontalHeaderLabels(
        {tr("Subject"), tr("Time"), tr("Room"), tr("Actions")});
    m_timetableTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_timetableTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_timetableTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_timetableTable->setAlternatingRowColors(true);
    m_timetableTable->verticalHeader()->hide();
    m_timetableTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_timetableTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_timetableTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_timetableTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_timetableTable->setColumnWidth(3, 194);

    m_emptyState->setObjectName(QStringLiteral("timetableEmptyState"));
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setContentsMargins(24, 24, 24, 24);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(12);
    m_emptyMessage->setObjectName(QStringLiteral("timetableEmptyMessage"));
    m_emptyMessage->setAlignment(Qt::AlignCenter);
    m_emptyMessage->setWordWrap(true);
    auto *emptyAddButton = new QPushButton(tr("Add Entry"), m_emptyState);
    emptyAddButton->setObjectName(QStringLiteral("emptyAddTimetableButton"));
    emptyLayout->addWidget(m_emptyMessage);
    emptyLayout->addWidget(emptyAddButton, 0, Qt::AlignCenter);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addLayout(actionBar);
    layout->addWidget(m_timetableTable, 1);
    layout->addWidget(m_emptyState, 1);

    connect(m_weekdayCombo, &QComboBox::currentIndexChanged,
            this, &TimetablePage::refreshTimetable);
    connect(addButton, &QPushButton::clicked, this, &TimetablePage::addTimetable);
    connect(emptyAddButton, &QPushButton::clicked, this, &TimetablePage::addTimetable);
}

void TimetablePage::refreshTimetable()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        m_timetableTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("The current user is unavailable."));
        return;
    }

    const int userId = m_application->currentUserId();
    QString errorMessage;
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        m_timetableTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load subjects.\n%1").arg(errorMessage));
        return;
    }

    QHash<int, QString> subjectNames;
    for (const Subject &subject : subjects) {
        subjectNames.insert(subject.id(), subject.name());
    }

    const int dayOfWeek = m_weekdayCombo->currentData().toInt();
    const QList<Timetable> entries =
        m_timetableService->getTimetablesForDay(userId, dayOfWeek, &errorMessage);
    if (!errorMessage.isEmpty()) {
        m_timetableTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load timetable entries.\n%1").arg(errorMessage));
        return;
    }

    m_timetableTable->clearContents();
    m_timetableTable->setRowCount(entries.size());
    for (qsizetype row = 0; row < entries.size(); ++row) {
        const Timetable &entry = entries.at(row);
        auto *subjectItem = new QTableWidgetItem(
            subjectNames.value(entry.subjectId(), tr("Unknown subject")));
        subjectItem->setData(Qt::UserRole, entry.id());
        m_timetableTable->setItem(row, 0, subjectItem);
        m_timetableTable->setItem(
            row, 1, new QTableWidgetItem(
                        QStringLiteral("%1 - %2").arg(entry.startTime(), entry.endTime())));
        m_timetableTable->setItem(row, 2, new QTableWidgetItem(entry.room()));

        auto *actionWidget = new QWidget(m_timetableTable);
        actionWidget->setObjectName(QStringLiteral("timetableActionWidget"));
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(8, 5, 8, 5);
        actionLayout->setSpacing(8);
        auto *editButton = new QPushButton(tr("Edit"), actionWidget);
        editButton->setObjectName(QStringLiteral("editTimetableButton"));
        editButton->setMinimumWidth(76);
        auto *deleteButton = new QPushButton(tr("Delete"), actionWidget);
        deleteButton->setObjectName(QStringLiteral("deleteTimetableButton"));
        deleteButton->setMinimumWidth(82);
        actionLayout->addWidget(editButton);
        actionLayout->addWidget(deleteButton);
        actionWidget->setMinimumWidth(182);
        m_timetableTable->setCellWidget(row, 3, actionWidget);
        m_timetableTable->setRowHeight(row, 50);
        connect(editButton, &QPushButton::clicked, this,
                [this, timetableId = entry.id()] { editTimetable(timetableId); });
        connect(deleteButton, &QPushButton::clicked, this,
                [this, timetableId = entry.id()] { deleteTimetable(timetableId); });
    }

    const bool hasEntries = !entries.isEmpty();
    m_timetableTable->setVisible(hasEntries);
    m_emptyState->setVisible(!hasEntries);
    if (!hasEntries) {
        m_emptyMessage->setText(
            subjects.isEmpty()
                ? tr("Add a subject before creating a timetable entry.")
                : tr("No timetable entries for %1.").arg(m_weekdayCombo->currentText()));
    }
}

void TimetablePage::addTimetable()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }

    QString errorMessage;
    const int userId = m_application->currentUserId();
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Could Not Load Subjects"), errorMessage);
        return;
    }
    if (subjects.isEmpty()) {
        QMessageBox::information(this, tr("Subject Required"),
                                 tr("Add a subject before creating a timetable entry."));
        return;
    }

    TimetableEntryDialog dialog(subjects, m_weekdayCombo->currentData().toInt(),
                                this);
    connect(&dialog, &TimetableEntryDialog::saveRequested, this,
            [this, &dialog, userId](Timetable entry) {
                entry.setUserId(userId);
                QString saveError;
                if (!m_timetableService->addTimetable(entry, &saveError)) {
                    QMessageBox::warning(&dialog, tr("Could Not Add Timetable Entry"),
                                         saveError);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    refreshTimetable();
}

void TimetablePage::editTimetable(int timetableId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }

    const int userId = m_application->currentUserId();
    QString errorMessage;
    const Timetable entry =
        m_timetableService->getTimetable(userId, timetableId, &errorMessage);
    if (entry.id() <= 0) {
        QMessageBox::warning(this, tr("Could Not Load Timetable Entry"),
                             errorMessage.isEmpty() ? tr("Timetable entry not found.")
                                                    : errorMessage);
        return;
    }
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Could Not Load Subjects"), errorMessage);
        return;
    }
    if (subjects.isEmpty()) {
        QMessageBox::information(this, tr("Subject Required"),
                                 tr("Add a subject before editing a timetable entry."));
        return;
    }

    TimetableEntryDialog dialog(subjects, entry.dayOfWeek(), this, true, entry);
    connect(&dialog, &TimetableEntryDialog::saveRequested, this,
            [this, &dialog, userId, timetableId](Timetable updated) {
                updated.setId(timetableId);
                updated.setUserId(userId);
                QString saveError;
                if (!m_timetableService->updateTimetable(updated, &saveError)) {
                    QMessageBox::warning(&dialog, tr("Could Not Update Timetable Entry"),
                                         saveError);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_weekdayCombo->setCurrentIndex(
        m_weekdayCombo->findData(dialog.getTimetable().dayOfWeek()));
    refreshTimetable();
}

void TimetablePage::deleteTimetable(int timetableId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Delete Timetable Entry"),
        tr("Are you sure you want to delete this timetable entry?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString errorMessage;
    if (!m_timetableService->deleteTimetable(m_application->currentUserId(),
                                              timetableId, &errorMessage)) {
        QMessageBox::warning(this, tr("Could Not Delete Timetable Entry"), errorMessage);
        return;
    }
    refreshTimetable();
}
