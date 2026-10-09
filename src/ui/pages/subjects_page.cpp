#include "subjects_page.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "../../core/application.h"
#include "../../data/subject_dao.h"
#include "../../models/subject.h"
#include "../../services/subject_service.h"
#include "../dialogs/subject_dialog.h"

SubjectsPage::SubjectsPage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
    , m_subjectDAO(std::make_unique<SubjectDAO>(
          application ? application->databaseManager() : nullptr))
    , m_subjectService(std::make_unique<SubjectService>(m_subjectDAO.get()))
    , m_searchEdit(new QLineEdit(this))
    , m_subjectsTable(new QTableWidget(this))
    , m_emptyState(new QWidget(this))
    , m_emptyMessage(new QLabel(this))
{
    setupUi();
    refreshSubjects();
}

SubjectsPage::~SubjectsPage() = default;

void SubjectsPage::setupUi()
{
    auto *title = new QLabel(tr("Subjects"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *description = new QLabel(tr("Manage your academic subjects."), this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    auto *addButton = new QPushButton(tr("Add Subject"), this);
    addButton->setObjectName(QStringLiteral("addSubjectButton"));
    addButton->setDefault(true);
    m_searchEdit->setObjectName(QStringLiteral("subjectSearchEdit"));
    m_searchEdit->setPlaceholderText(tr("Search name, code, or teacher"));
    m_searchEdit->setClearButtonEnabled(true);

    auto *actionBar = new QHBoxLayout;
    actionBar->setSpacing(12);
    actionBar->addWidget(addButton);
    actionBar->addStretch();
    actionBar->addWidget(m_searchEdit, 1);

    m_subjectsTable->setObjectName(QStringLiteral("subjectsTable"));
    m_subjectsTable->setColumnCount(6);
    m_subjectsTable->setHorizontalHeaderLabels(
        {tr("Subject"), tr("Code"), tr("Teacher"), tr("Credits"), tr("Semester"), tr("Actions")});
    m_subjectsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_subjectsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_subjectsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_subjectsTable->setAlternatingRowColors(true);
    m_subjectsTable->verticalHeader()->hide();
    m_subjectsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_subjectsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_subjectsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_subjectsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_subjectsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_subjectsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);

    m_emptyState->setObjectName(QStringLiteral("subjectsEmptyState"));
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setContentsMargins(24, 24, 24, 24);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(12);
    m_emptyMessage->setObjectName(QStringLiteral("subjectsEmptyMessage"));
    m_emptyMessage->setAlignment(Qt::AlignCenter);
    auto *emptyAddButton = new QPushButton(tr("Add Subject"), m_emptyState);
    emptyAddButton->setObjectName(QStringLiteral("emptyAddSubjectButton"));
    emptyLayout->addWidget(m_emptyMessage);
    emptyLayout->addWidget(emptyAddButton, 0, Qt::AlignCenter);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addLayout(actionBar);
    layout->addWidget(m_subjectsTable, 1);
    layout->addWidget(m_emptyState, 1);

    connect(addButton, &QPushButton::clicked, this, &SubjectsPage::addSubject);
    connect(emptyAddButton, &QPushButton::clicked, this, &SubjectsPage::addSubject);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SubjectsPage::searchSubjects);
}

void SubjectsPage::refreshSubjects()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        m_subjectsTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("The current user is unavailable."));
        return;
    }

    QString errorMessage;
    const QString searchTerm = m_searchEdit->text().trimmed();
    const QList<Subject> subjects = searchTerm.isEmpty()
        ? m_subjectService->getSubjects(m_application->currentUserId(), &errorMessage)
        : m_subjectService->searchSubjects(m_application->currentUserId(), searchTerm,
                                          &errorMessage);

    if (!errorMessage.isEmpty()) {
        m_subjectsTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load subjects.\n%1").arg(errorMessage));
        return;
    }

    m_subjectsTable->clearContents();
    m_subjectsTable->setRowCount(subjects.size());
    for (qsizetype row = 0; row < subjects.size(); ++row) {
        const Subject &subject = subjects.at(row);
        auto *nameItem = new QTableWidgetItem(subject.name());
        nameItem->setData(Qt::UserRole, subject.id());
        m_subjectsTable->setItem(row, 0, nameItem);
        m_subjectsTable->setItem(row, 1, new QTableWidgetItem(subject.code()));
        m_subjectsTable->setItem(row, 2, new QTableWidgetItem(subject.teacher()));
        m_subjectsTable->setItem(row, 3, new QTableWidgetItem(QString::number(subject.credits())));
        m_subjectsTable->setItem(row, 4, new QTableWidgetItem(QString::number(subject.semester())));

        auto *actionWidget = new QWidget(m_subjectsTable);
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(4, 2, 4, 2);
        actionLayout->setSpacing(6);
        auto *editButton = new QPushButton(tr("Edit"), actionWidget);
        editButton->setObjectName(QStringLiteral("editSubjectButton"));
        auto *deleteButton = new QPushButton(tr("Delete"), actionWidget);
        deleteButton->setObjectName(QStringLiteral("deleteSubjectButton"));
        actionLayout->addWidget(editButton);
        actionLayout->addWidget(deleteButton);
        m_subjectsTable->setCellWidget(row, 5, actionWidget);
        connect(editButton, &QPushButton::clicked, this,
                [this, subjectId = subject.id()] { editSubject(subjectId); });
        connect(deleteButton, &QPushButton::clicked, this,
                [this, subjectId = subject.id()] { deleteSubject(subjectId); });
    }

    const bool hasSubjects = !subjects.isEmpty();
    m_subjectsTable->setVisible(hasSubjects);
    m_emptyState->setVisible(!hasSubjects);
    if (!hasSubjects) {
        m_emptyMessage->setText(searchTerm.isEmpty()
                                    ? tr("No subjects added yet.")
                                    : tr("No subjects match your search."));
    }
}

void SubjectsPage::addSubject()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }

    SubjectDialog dialog(this);
    connect(&dialog, &SubjectDialog::saveRequested, this,
            [this, &dialog](Subject subject) {
                subject.setUserId(m_application->currentUserId());
                QString errorMessage;
                if (!m_subjectService->addSubject(subject, &errorMessage)) {
                    QMessageBox::warning(&dialog, tr("Could Not Add Subject"), errorMessage);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_searchEdit->clear();
    refreshSubjects();
}

void SubjectsPage::editSubject(int subjectId)
{
    if (!m_application) {
        return;
    }

    QString errorMessage;
    const int userId = m_application->currentUserId();
    const Subject subject = m_subjectService->getSubject(userId, subjectId, &errorMessage);
    if (subject.id() <= 0) {
        QMessageBox::warning(this, tr("Could Not Load Subject"),
                             errorMessage.isEmpty() ? tr("Subject not found.") : errorMessage);
        return;
    }

    SubjectDialog dialog(this, true, subject);
    connect(&dialog, &SubjectDialog::saveRequested, this,
            [this, &dialog](const Subject &updatedSubject) {
                QString saveError;
                if (!m_subjectService->updateSubject(updatedSubject, &saveError)) {
                    QMessageBox::warning(&dialog, tr("Could Not Update Subject"), saveError);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    refreshSubjects();
}

void SubjectsPage::deleteSubject(int subjectId)
{
    if (!m_application) {
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Delete Subject"),
        tr("Are you sure you want to delete this subject? Its associated timetable entries will also be deleted."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString errorMessage;
    if (!m_subjectService->deleteSubject(m_application->currentUserId(), subjectId,
                                         &errorMessage)) {
        QMessageBox::warning(this, tr("Could Not Delete Subject"), errorMessage);
        return;
    }
    refreshSubjects();
}

void SubjectsPage::searchSubjects(const QString &text)
{
    Q_UNUSED(text);
    refreshSubjects();
}
