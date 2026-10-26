#include "notes_page.h"

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
#include "../../data/notes_dao.h"
#include "../../data/subject_dao.h"
#include "../../models/note.h"
#include "../../models/subject.h"
#include "../../services/notes_service.h"
#include "../../services/subject_service.h"
#include "../dialogs/note_dialog.h"

NotesPage::NotesPage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
    , m_notesDAO(std::make_unique<NotesDAO>(
          application ? application->databaseManager() : nullptr))
    , m_subjectDAO(std::make_unique<SubjectDAO>(
          application ? application->databaseManager() : nullptr))
    , m_notesService(std::make_unique<NotesService>(m_notesDAO.get()))
    , m_subjectService(std::make_unique<SubjectService>(m_subjectDAO.get()))
    , m_noteTable(new QTableWidget(this))
    , m_emptyState(new QWidget(this))
    , m_emptyMessage(new QLabel(this))
{
    setObjectName(QStringLiteral("notesPage"));

    auto *title = new QLabel(tr("Notes"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *description = new QLabel(
        tr("Capture and organize your academic notes, optionally linked to a subject."),
        this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    auto *addButton = new QPushButton(tr("Add Note"), this);
    addButton->setObjectName(QStringLiteral("addNoteButton"));
    auto *actionBar = new QHBoxLayout;
    actionBar->addStretch();
    actionBar->addWidget(addButton);

    m_noteTable->setObjectName(QStringLiteral("notesTable"));
    m_noteTable->setColumnCount(5);
    m_noteTable->setHorizontalHeaderLabels(
        {tr("Title"), tr("Subject"), tr("Content"), tr("Updated"), tr("Actions")});
    m_noteTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_noteTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_noteTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_noteTable->setAlternatingRowColors(true);
    m_noteTable->verticalHeader()->hide();
    m_noteTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_noteTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_noteTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_noteTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_noteTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_noteTable->setColumnWidth(4, 194);

    m_emptyState->setObjectName(QStringLiteral("notesEmptyState"));
    auto *emptyLayout = new QVBoxLayout(m_emptyState);
    emptyLayout->setContentsMargins(32, 32, 32, 32);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(12);
    m_emptyMessage->setObjectName(QStringLiteral("notesEmptyMessage"));
    m_emptyMessage->setAlignment(Qt::AlignCenter);
    m_emptyMessage->setWordWrap(true);
    m_emptyMessage->setText(tr("No notes added yet."));
    auto *emptyAddButton = new QPushButton(tr("Add Note"), m_emptyState);
    emptyAddButton->setObjectName(QStringLiteral("emptyAddNoteButton"));
    emptyLayout->addWidget(m_emptyMessage);
    emptyLayout->addWidget(emptyAddButton, 0, Qt::AlignCenter);

    auto *contentLayout = new QVBoxLayout(this);
    contentLayout->setContentsMargins(40, 40, 40, 40);
    contentLayout->setSpacing(16);
    contentLayout->addWidget(title);
    contentLayout->addWidget(description);
    contentLayout->addLayout(actionBar);
    contentLayout->addWidget(m_noteTable, 1);
    contentLayout->addWidget(m_emptyState, 1);

    connect(addButton, &QPushButton::clicked, this, &NotesPage::addNote);
    connect(emptyAddButton, &QPushButton::clicked, this, &NotesPage::addNote);

    refreshNotes();
}

NotesPage::~NotesPage() = default;

void NotesPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshNotes();
}

void NotesPage::refreshNotes()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        m_noteTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("The current user is not available."));
        return;
    }

    const int userId = m_application->currentUserId();
    QString errorMessage;
    const QList<Note> notes =
        m_notesService->getNotes(userId, &errorMessage);

    if (!errorMessage.isEmpty()) {
        m_noteTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load notes: %1").arg(errorMessage));
        return;
    }

    m_noteTable->setRowCount(0);
    if (notes.isEmpty()) {
        m_noteTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("No notes added yet."));
        return;
    }

    QHash<int, QString> subjectNames;
    QString subjectError;
    const QList<Subject> subjects =
        m_subjectService->getSubjects(userId, &subjectError);
    if (!subjectError.isEmpty()) {
        m_noteTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("Could not load subjects: %1").arg(subjectError));
        return;
    }
    for (const Subject &subject : subjects) {
        subjectNames.insert(subject.id(), subject.name());
    }

    m_noteTable->setRowCount(static_cast<int>(notes.size()));
    int row = 0;
    for (const Note &note : notes) {
        auto *titleItem = new QTableWidgetItem(note.title());
        titleItem->setData(Qt::UserRole, note.id());
        m_noteTable->setItem(row, 0, titleItem);

        QString subjectDisplay = note.subjectId() > 0
                                     ? subjectNames.value(note.subjectId(),
                                                          tr("Subject unavailable"))
                                     : tr("None");
        auto *subjectItem = new QTableWidgetItem(subjectDisplay);
        m_noteTable->setItem(row, 1, subjectItem);

        QString contentPreview = note.content();
        if (contentPreview.length() > 80) {
            contentPreview.truncate(80);
            contentPreview.append(QStringLiteral("…"));
        }
        auto *contentItem = new QTableWidgetItem(contentPreview);
        m_noteTable->setItem(row, 2, contentItem);

        QString updatedDisplay = note.updatedAt().isEmpty()
                                     ? note.createdAt()
                                     : note.updatedAt();
        if (!updatedDisplay.isEmpty()) {
            const QDate date = QDate::fromString(updatedDisplay, Qt::ISODate);
            if (date.isValid()) {
                updatedDisplay = date.toString(QStringLiteral("ddd, MMM d"));
            }
        }
        auto *updatedItem = new QTableWidgetItem(updatedDisplay);
        m_noteTable->setItem(row, 3, updatedItem);

        auto *actionsWidget = new QWidget(this);
        auto *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(4, 2, 4, 2);
        actionsLayout->setSpacing(6);

        auto *editButton = new QPushButton(tr("Edit"), actionsWidget);
        editButton->setObjectName(QStringLiteral("editNoteButton"));
        auto *deleteButton = new QPushButton(tr("Delete"), actionsWidget);
        deleteButton->setObjectName(QStringLiteral("deleteNoteButton"));

        actionsLayout->addWidget(editButton);
        actionsLayout->addWidget(deleteButton);
        actionsLayout->addStretch();
        m_noteTable->setCellWidget(row, 4, actionsWidget);

        connect(editButton, &QPushButton::clicked, this,
                [this, noteId = note.id()] { editNote(noteId); });
        connect(deleteButton, &QPushButton::clicked, this,
                [this, noteId = note.id()] { deleteNote(noteId); });

        ++row;
    }

    m_emptyState->hide();
    m_noteTable->show();
    m_noteTable->resizeRowsToContents();
}

void NotesPage::addNote()
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

    NoteDialog dialog(subjects, this);
    connect(&dialog, &NoteDialog::saveRequested, this,
            [this, &dialog, userId](Note note) {
                note.setUserId(userId);
                QString saveError;
                if (!m_notesService->addNote(note, &saveError)) {
                    QMessageBox::warning(&dialog, tr("Could Not Add Note"), saveError);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() == QDialog::Accepted) {
        refreshNotes();
    }
}

void NotesPage::editNote(int noteId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }
    const int userId = m_application->currentUserId();
    QString errorMessage;
    const Note note = m_notesService->getNote(userId, noteId, &errorMessage);
    if (!errorMessage.isEmpty() || note.id() <= 0) {
        if (errorMessage == QStringLiteral("Note not found.")) {
            refreshNotes();
            return;
        }
        QMessageBox::warning(this, tr("Could Not Load Note"),
                             errorMessage.isEmpty() ? tr("The note no longer exists.")
                                                    : errorMessage);
        refreshNotes();
        return;
    }
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Could Not Load Subjects"), errorMessage);
        return;
    }

    NoteDialog dialog(subjects, this, note);
    connect(&dialog, &NoteDialog::saveRequested, this,
            [this, &dialog, userId](Note updated) {
                updated.setUserId(userId);
                QString saveError;
                if (!m_notesService->updateNote(updated, &saveError)) {
                    QMessageBox::warning(&dialog, tr("Could Not Update Note"), saveError);
                    return;
                }
                dialog.accept();
            });
    if (dialog.exec() == QDialog::Accepted) {
        refreshNotes();
    }
}

void NotesPage::deleteNote(int noteId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        QMessageBox::warning(this, tr("User Unavailable"),
                             tr("The current user is not available. Restart NEXUS and try again."));
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Delete Note"),
        tr("Are you sure you want to delete this note?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    QString errorMessage;
    if (!m_notesService->deleteNote(m_application->currentUserId(), noteId,
                                    &errorMessage)) {
        if (errorMessage == QStringLiteral("Note not found.")) {
            refreshNotes();
            return;
        }
        QMessageBox::warning(this, tr("Could Not Delete Note"), errorMessage);
        return;
    }
    refreshNotes();
}