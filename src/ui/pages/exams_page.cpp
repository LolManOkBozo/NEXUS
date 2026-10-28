#include "exams_page.h"

#include <QDate>
#include <QHash>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "../../core/application.h"
#include "../../data/exam_dao.h"
#include "../../data/subject_dao.h"
#include "../../models/exam.h"
#include "../../models/subject.h"
#include "../../services/exam_service.h"
#include "../../services/subject_service.h"
#include "../dialogs/exam_dialog.h"

ExamsPage::ExamsPage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
    , m_examDAO(std::make_unique<ExamDAO>(
          application ? application->databaseManager() : nullptr))
    , m_subjectDAO(std::make_unique<SubjectDAO>(
          application ? application->databaseManager() : nullptr))
    , m_examService(std::make_unique<ExamService>(
          m_examDAO.get(), m_subjectDAO.get()))
    , m_subjectService(std::make_unique<SubjectService>(m_subjectDAO.get()))
    , m_examTable(new QTableWidget(this))
    , m_emptyState(new QWidget(this))
    , m_emptyMessage(new QLabel(this))
{
    setObjectName(QStringLiteral("examsPage"));

    auto *title = new QLabel(tr("Exams"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *description = new QLabel(
        tr("Plan examinations and keep their dates, times, and rooms in one place."),
        this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    auto *addButton = new QPushButton(tr("Add Exam"), this);
    addButton->setObjectName(QStringLiteral("addExamButton"));
    auto *actionBar = new QHBoxLayout;
    actionBar->addStretch();
    actionBar->addWidget(addButton);

    m_examTable->setObjectName(QStringLiteral("examsTable"));
    m_examTable->setColumnCount(6);
    m_examTable->setHorizontalHeaderLabels(
        {tr("Exam"), tr("Subject"), tr("Date"), tr("Time"),
         tr("Room"), tr("Actions")});
    m_examTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_examTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_examTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_examTable->setAlternatingRowColors(true);
    m_examTable->verticalHeader()->hide();
    m_examTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_examTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_examTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_examTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_examTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_examTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_examTable->setColumnWidth(5, 194);

    m_emptyState->setObjectName(QStringLiteral("examsEmptyState"));
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setContentsMargins(32, 32, 32, 32);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(12);
    m_emptyMessage->setObjectName(QStringLiteral("examsEmptyMessage"));
    m_emptyMessage->setAlignment(Qt::AlignCenter);
    m_emptyMessage->setWordWrap(true);
    auto *emptyAddButton = new QPushButton(tr("Add Exam"), m_emptyState);
    emptyAddButton->setObjectName(QStringLiteral("emptyAddExamButton"));
    emptyLayout->addWidget(m_emptyMessage);
    emptyLayout->addWidget(emptyAddButton, 0, Qt::AlignCenter);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addLayout(actionBar);
    layout->addWidget(m_examTable, 1);
    layout->addWidget(m_emptyState, 1);

    connect(addButton, &QPushButton::clicked, this, &ExamsPage::addExam);
    connect(emptyAddButton, &QPushButton::clicked, this, &ExamsPage::addExam);
    refreshExams();
}

ExamsPage::~ExamsPage() = default;

void ExamsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshExams();
}

void ExamsPage::refreshExams()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        m_examTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("The current user is unavailable."));
        return;
    }

    const int userId = m_application->currentUserId();
    QString errorMessage;
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        m_examTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load subjects.\n%1").arg(errorMessage));
        return;
    }
    const QList<Exam> exams = m_examService->getExams(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        m_examTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load exams.\n%1").arg(errorMessage));
        return;
    }

    QHash<int, QString> subjectNames;
    for (const Subject &subject : subjects) {
        subjectNames.insert(subject.id(), subject.name());
    }

    m_examTable->clearContents();
    m_examTable->setRowCount(exams.size());
    for (qsizetype row = 0; row < exams.size(); ++row) {
        const Exam &exam = exams.at(row);
        auto *titleItem = new QTableWidgetItem(exam.title());
        titleItem->setData(Qt::UserRole, exam.id());
        m_examTable->setItem(row, 0, titleItem);
        m_examTable->setItem(
            row, 1, new QTableWidgetItem(
                        subjectNames.value(exam.subjectId(), tr("Subject unavailable"))));

        const QDate examDate = QDate::fromString(exam.examDate(), Qt::ISODate);
        const QString dateText = examDate.isValid()
            && examDate.toString(Qt::ISODate) == exam.examDate()
            ? examDate.toString(QStringLiteral("ddd, MMM d, yyyy"))
            : exam.examDate();
        m_examTable->setItem(row, 2, new QTableWidgetItem(dateText));
        m_examTable->setItem(
            row, 3, new QTableWidgetItem(
                        exam.startTime().isEmpty() ? tr("Not set") : exam.startTime()));
        m_examTable->setItem(
            row, 4, new QTableWidgetItem(
                        exam.room().isEmpty() ? tr("Not set") : exam.room()));

        auto *actionWidget = new QWidget(m_examTable);
        actionWidget->setObjectName(QStringLiteral("examActionWidget"));
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(8, 2, 8, 2);
        actionLayout->setSpacing(8);
        auto *editButton = new QPushButton(tr("Edit"), actionWidget);
        editButton->setObjectName(QStringLiteral("editExamButton"));
        editButton->setMinimumWidth(76);
        auto *deleteButton = new QPushButton(tr("Delete"), actionWidget);
        deleteButton->setObjectName(QStringLiteral("deleteExamButton"));
        deleteButton->setMinimumWidth(82);
        actionLayout->addWidget(editButton);
        actionLayout->addWidget(deleteButton);
        actionWidget->setMinimumWidth(178);
        m_examTable->setCellWidget(row, 5, actionWidget);
        m_examTable->setRowHeight(row, 56);
        connect(editButton, &QPushButton::clicked, this,
                [this, examId = exam.id()] { editExam(examId); });
        connect(deleteButton, &QPushButton::clicked, this,
                [this, examId = exam.id()] { deleteExam(examId); });
    }

    const bool hasExams = !exams.isEmpty();
    m_examTable->setVisible(hasExams);
    m_emptyState->setVisible(!hasExams);
    if (!hasExams) {
        m_emptyMessage->setText(
            subjects.isEmpty()
                ? tr("Add a subject before creating an exam.")
                : tr("No exams added yet."));
    }
}

void ExamsPage::addExam()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }
    const int userId = m_application->currentUserId();
    QString errorMessage;
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Could Not Load Subjects"), errorMessage);
        return;
    }
    if (subjects.isEmpty()) {
        QMessageBox::information(this, tr("No Subjects"),
                                 tr("Add a subject before creating an exam."));
        return;
    }

    ExamDialog dialog(subjects, this);
    connect(&dialog, &ExamDialog::saveRequested, this,
            [this, &dialog, userId](Exam exam) {
                exam.setUserId(userId);
                QString saveError;
                if (!m_examService->addExam(exam, &saveError)) {
                    QMessageBox::warning(&dialog, tr("Could Not Add Exam"), saveError);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() == QDialog::Accepted) {
        refreshExams();
    }
}

void ExamsPage::editExam(int examId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }
    const int userId = m_application->currentUserId();
    QString errorMessage;
    const Exam exam = m_examService->getExam(userId, examId, &errorMessage);
    if (!errorMessage.isEmpty() || exam.id() <= 0) {
        if (errorMessage == QStringLiteral("Exam not found.")) {
            refreshExams();
            return;
        }
        QMessageBox::warning(this, tr("Could Not Load Exam"),
                             errorMessage.isEmpty() ? tr("The exam no longer exists.")
                                                    : errorMessage);
        refreshExams();
        return;
    }
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Could Not Load Subjects"), errorMessage);
        return;
    }

    ExamDialog dialog(subjects, this, exam);
    connect(&dialog, &ExamDialog::saveRequested, this,
            [this, &dialog, userId](Exam updated) {
                updated.setUserId(userId);
                QString saveError;
                if (!m_examService->updateExam(updated, &saveError)) {
                    QMessageBox::warning(&dialog, tr("Could Not Update Exam"), saveError);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() == QDialog::Accepted) {
        refreshExams();
    }
}

void ExamsPage::deleteExam(int examId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Delete Exam"),
        tr("Are you sure you want to delete this exam?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    QString errorMessage;
    if (!m_examService->deleteExam(m_application->currentUserId(),
                                   examId, &errorMessage)) {
        if (errorMessage == QStringLiteral("Exam not found.")) {
            refreshExams();
            return;
        }
        QMessageBox::warning(this, tr("Could Not Delete Exam"), errorMessage);
        return;
    }
    refreshExams();
}
