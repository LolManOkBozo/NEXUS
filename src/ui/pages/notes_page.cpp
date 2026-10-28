#include "notes_page.h"

#include <QDate>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHash>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

#include "../../core/application.h"
#include "../../data/notes_dao.h"
#include "../../data/study_file_dao.h"
#include "../../data/subject_dao.h"
#include "../../models/note.h"
#include "../../models/study_file.h"
#include "../../models/subject.h"
#include "../../services/notes_service.h"
#include "../../services/study_file_service.h"
#include "../../services/subject_service.h"
#include "../dialogs/note_dialog.h"

namespace
{
QString formatFileSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    }
    if (bytes < 1024 * 1024 * 1024) {
        return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    }
    return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}
}

NotesPage::NotesPage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
    , m_notesDAO(std::make_unique<NotesDAO>(
          application ? application->databaseManager() : nullptr))
    , m_subjectDAO(std::make_unique<SubjectDAO>(
          application ? application->databaseManager() : nullptr))
    , m_notesService(std::make_unique<NotesService>(m_notesDAO.get()))
    , m_subjectService(std::make_unique<SubjectService>(m_subjectDAO.get()))
    , m_studyFileDAO(std::make_unique<StudyFileDAO>(
          application ? application->databaseManager() : nullptr))
    , m_studyFileService(std::make_unique<StudyFileService>(m_studyFileDAO.get()))
    , m_tabWidget(new QTabWidget(this))
    , m_noteTable(new QTableWidget(this))
    , m_emptyState(new QWidget(this))
    , m_emptyMessage(new QLabel(this))
    , m_libraryTable(new QTableWidget(this))
    , m_libraryEmptyState(new QWidget(this))
    , m_libraryEmptyMessage(new QLabel(this))
    , m_librarySearchEdit(new QLineEdit(this))
    , m_libraryTypeFilter(new QComboBox(this))
    , m_librarySubjectFilter(new QComboBox(this))
{
    setObjectName(QStringLiteral("notesPage"));

    auto *title = new QLabel(tr("Notes & Study Library"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *description = new QLabel(
        tr("Capture academic notes and manage imported study materials."),
        this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    // --- Notes Tab Setup ---
    auto *notesTabWidget = new QWidget(this);
    auto *addButton = new QPushButton(tr("Add Note"), this);
    addButton->setObjectName(QStringLiteral("addNoteButton"));
    auto *notesActionBar = new QHBoxLayout;
    notesActionBar->setContentsMargins(0, 0, 0, 0);
    notesActionBar->addStretch();
    notesActionBar->addWidget(addButton);

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
    m_noteTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

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

    auto *notesTabLayout = new QVBoxLayout(notesTabWidget);
    notesTabLayout->setContentsMargins(16, 16, 16, 16);
    notesTabLayout->setSpacing(16);
    notesTabLayout->addLayout(notesActionBar);
    notesTabLayout->addWidget(m_noteTable, 1);
    notesTabLayout->addWidget(m_emptyState, 1);

    // --- Study Library Tab Setup ---
    auto *libraryTabWidget = new QWidget(this);
    auto *importButton = new QPushButton(tr("Import Files"), this);
    importButton->setObjectName(QStringLiteral("importFilesButton"));
    m_librarySearchEdit->setObjectName(QStringLiteral("librarySearchEdit"));
    m_librarySearchEdit->setPlaceholderText(tr("Search files by name"));
    m_librarySearchEdit->setClearButtonEnabled(true);

    m_libraryTypeFilter->setObjectName(QStringLiteral("libraryTypeFilter"));
    m_libraryTypeFilter->addItem(tr("All Types"), QString());
    m_libraryTypeFilter->addItem(tr("PDF"), QStringLiteral("PDF"));
    m_libraryTypeFilter->addItem(tr("DOCX"), QStringLiteral("DOCX"));
    m_libraryTypeFilter->addItem(tr("PPTX"), QStringLiteral("PPTX"));
    m_libraryTypeFilter->addItem(tr("TXT"), QStringLiteral("TXT"));
    m_libraryTypeFilter->addItem(tr("Image"), QStringLiteral("Image"));

    m_librarySubjectFilter->setObjectName(QStringLiteral("librarySubjectFilter"));
    m_librarySubjectFilter->addItem(tr("All Subjects"), 0);

    auto *libraryActionBar = new QHBoxLayout;
    libraryActionBar->setContentsMargins(0, 0, 0, 0);
    libraryActionBar->setSpacing(12);
    libraryActionBar->addWidget(importButton);
    libraryActionBar->addStretch();
    libraryActionBar->addWidget(new QLabel(tr("Type:"), this));
    libraryActionBar->addWidget(m_libraryTypeFilter);
    libraryActionBar->addWidget(new QLabel(tr("Subject:"), this));
    libraryActionBar->addWidget(m_librarySubjectFilter);
    libraryActionBar->addWidget(m_librarySearchEdit, 1);

    m_libraryTable->setObjectName(QStringLiteral("libraryTable"));
    m_libraryTable->setColumnCount(6);
    m_libraryTable->setHorizontalHeaderLabels(
        {tr("Filename"), tr("Type"), tr("Size"), tr("Import Date"), tr("Subject"), tr("Actions")});
    m_libraryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_libraryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_libraryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_libraryTable->setAlternatingRowColors(true);
    m_libraryTable->verticalHeader()->hide();
    m_libraryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_libraryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_libraryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_libraryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_libraryTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_libraryTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_libraryTable->setColumnWidth(5, 194);

    m_libraryEmptyState->setObjectName(QStringLiteral("libraryEmptyState"));
    auto *libEmptyLayout = new QVBoxLayout(m_libraryEmptyState);
    libEmptyLayout->setContentsMargins(32, 32, 32, 32);
    libEmptyLayout->setAlignment(Qt::AlignCenter);
    libEmptyLayout->setSpacing(12);
    m_libraryEmptyMessage->setObjectName(QStringLiteral("libraryEmptyMessage"));
    m_libraryEmptyMessage->setAlignment(Qt::AlignCenter);
    m_libraryEmptyMessage->setWordWrap(true);
    m_libraryEmptyMessage->setText(tr("No study files imported yet."));
    auto *emptyImportButton = new QPushButton(tr("Import Files"), m_libraryEmptyState);
    emptyImportButton->setObjectName(QStringLiteral("emptyImportFilesButton"));
    libEmptyLayout->addWidget(m_libraryEmptyMessage);
    libEmptyLayout->addWidget(emptyImportButton, 0, Qt::AlignCenter);

    auto *libraryTabLayout = new QVBoxLayout(libraryTabWidget);
    libraryTabLayout->setContentsMargins(16, 16, 16, 16);
    libraryTabLayout->setSpacing(16);
    libraryTabLayout->addLayout(libraryActionBar);
    libraryTabLayout->addWidget(m_libraryTable, 1);
    libraryTabLayout->addWidget(m_libraryEmptyState, 1);

    m_tabWidget->setObjectName(QStringLiteral("notesTabWidget"));
    m_tabWidget->addTab(notesTabWidget, tr("Notes"));
    m_tabWidget->addTab(libraryTabWidget, tr("Study Library"));

    auto *contentLayout = new QVBoxLayout(this);
    contentLayout->setContentsMargins(40, 40, 40, 40);
    contentLayout->setSpacing(16);
    contentLayout->addWidget(title);
    contentLayout->addWidget(description);
    contentLayout->addWidget(m_tabWidget, 1);

    connect(addButton, &QPushButton::clicked, this, &NotesPage::addNote);
    connect(emptyAddButton, &QPushButton::clicked, this, &NotesPage::addNote);
    connect(importButton, &QPushButton::clicked, this, &NotesPage::importFiles);
    connect(emptyImportButton, &QPushButton::clicked, this, &NotesPage::importFiles);
    connect(m_librarySearchEdit, &QLineEdit::textChanged, this, &NotesPage::refreshLibrary);
    connect(m_libraryTypeFilter, &QComboBox::currentIndexChanged, this, &NotesPage::refreshLibrary);
    connect(m_librarySubjectFilter, &QComboBox::currentIndexChanged, this, &NotesPage::refreshLibrary);

    refreshNotes();
    refreshLibrary();
}

NotesPage::~NotesPage() = default;

void NotesPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshNotes();
    refreshLibrary();
}

void NotesPage::refreshNotes()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        m_noteTable->hide();
        m_emptyState->show();
        m_emptyMessage->setText(tr("The current user is unavailable."));
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

        auto *actionsWidget = new QWidget(m_noteTable);
        actionsWidget->setObjectName(QStringLiteral("noteActionWidget"));
        auto *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(8, 2, 8, 2);
        actionsLayout->setSpacing(8);

        auto *editButton = new QPushButton(tr("Edit"), actionsWidget);
        editButton->setObjectName(QStringLiteral("editNoteButton"));
        editButton->setMinimumWidth(76);
        auto *deleteButton = new QPushButton(tr("Delete"), actionsWidget);
        deleteButton->setObjectName(QStringLiteral("deleteNoteButton"));
        deleteButton->setMinimumWidth(82);

        actionsLayout->addWidget(editButton);
        actionsLayout->addWidget(deleteButton);
        actionsWidget->setMinimumWidth(178);
        m_noteTable->setCellWidget(row, 4, actionsWidget);
        m_noteTable->setRowHeight(row, 56);

        connect(editButton, &QPushButton::clicked, this,
                [this, noteId = note.id()] { editNote(noteId); });
        connect(deleteButton, &QPushButton::clicked, this,
                [this, noteId = note.id()] { deleteNote(noteId); });

        ++row;
    }

    m_emptyState->hide();
    m_noteTable->show();
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
                             errorMessage.isEmpty() ? tr("The note no longer exists.") : errorMessage);
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

    const int userId = m_application->currentUserId();
    QString errorMessage;
    if (!m_notesService->deleteNote(userId, noteId, &errorMessage)) {
        if (errorMessage == QStringLiteral("Note not found.")) {
            refreshNotes();
            return;
        }
        QMessageBox::warning(this, tr("Could Not Delete Note"), errorMessage);
        return;
    }
    refreshNotes();
}

void NotesPage::importFiles()
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

    QStringList filePaths = QFileDialog::getOpenFileNames(
        this, tr("Import Study Materials"), QString(),
        tr("All Supported Files (*.pdf *.docx *.doc *.pptx *.ppt *.txt *.md *.csv *.png *.jpg *.jpeg *.gif *.webp *.bmp);;PDF Files (*.pdf);;Word Documents (*.docx *.doc);;Presentations (*.pptx *.ppt);;Text & Data (*.txt *.md *.csv);;Images (*.png *.jpg *.jpeg *.gif *.webp *.bmp);;All Files (*.*)"));

    if (filePaths.isEmpty()) {
        return;
    }

    int selectedSubjectId = 0;
    if (!subjects.isEmpty()) {
        QStringList subjectItems;
        subjectItems.append(tr("None (Unassigned)"));
        for (const Subject &s : subjects) {
            subjectItems.append(s.name());
        }
        bool ok = false;
        QString chosen = QInputDialog::getItem(
            this, tr("Associate Subject"), tr("Link imported files to a subject (optional):"),
            subjectItems, 0, false, &ok);
        if (ok) {
            int idx = subjectItems.indexOf(chosen);
            if (idx > 0 && idx <= subjects.size()) {
                selectedSubjectId = subjects.at(idx - 1).id();
            }
        }
    }

    int successCount = 0;
    QStringList failures;
    for (const QString &path : filePaths) {
        QString importError;
        if (m_studyFileService->importFile(userId, selectedSubjectId, path, &importError)) {
            successCount++;
        } else {
            failures.append(QStringLiteral("%1: %2").arg(QFileInfo(path).fileName(), importError));
        }
    }

    refreshLibrary();

    if (!failures.isEmpty()) {
        QMessageBox::warning(
            this, tr("Import Completed with Errors"),
            tr("Successfully imported %1 file(s).\n\nFailed to import %2 file(s):\n%3")
                .arg(successCount)
                .arg(failures.size())
                .arg(failures.join(QStringLiteral("\n"))));
    } else {
        QMessageBox::information(
            this, tr("Import Successful"),
            tr("Successfully imported %1 file(s) into your study library.").arg(successCount));
    }
}

void NotesPage::refreshLibrary()
{
    if (!m_application || m_application->currentUserId() <= 0) {
        m_libraryTable->hide();
        m_libraryEmptyState->show();
        m_libraryEmptyMessage->setText(tr("The current user is unavailable."));
        return;
    }

    const int userId = m_application->currentUserId();
    QString errorMessage;
    const QList<StudyFile> files = m_studyFileService->getStudyFiles(userId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        m_libraryTable->hide();
        m_libraryEmptyState->show();
        m_libraryEmptyMessage->setText(tr("Could not load study library: %1").arg(errorMessage));
        return;
    }

    const int prevSubjectId = m_librarySubjectFilter->currentData().toInt();
    const QString prevType = m_libraryTypeFilter->currentData().toString();
    const QString searchQuery = m_librarySearchEdit->text().trimmed().toLower();

    QString subjectError;
    const QList<Subject> subjects = m_subjectService->getSubjects(userId, &subjectError);
    if (subjectError.isEmpty()) {
        m_librarySubjectFilter->blockSignals(true);
        m_librarySubjectFilter->clear();
        m_librarySubjectFilter->addItem(tr("All Subjects"), 0);
        for (const Subject &s : subjects) {
            m_librarySubjectFilter->addItem(s.name(), s.id());
        }
        int idx = m_librarySubjectFilter->findData(prevSubjectId);
        m_librarySubjectFilter->setCurrentIndex(idx >= 0 ? idx : 0);
        m_librarySubjectFilter->blockSignals(false);
    }

    QHash<int, QString> subjectNames;
    for (const Subject &s : subjects) {
        subjectNames.insert(s.id(), s.name());
    }

    QList<StudyFile> filteredFiles;
    for (const StudyFile &file : files) {
        if (!searchQuery.isEmpty() && !file.originalFilename().toLower().contains(searchQuery)) {
            continue;
        }
        if (!prevType.isEmpty() && file.fileType() != prevType) {
            continue;
        }
        if (prevSubjectId > 0 && file.subjectId() != prevSubjectId) {
            continue;
        }
        filteredFiles.append(file);
    }

    m_libraryTable->setRowCount(0);
    if (filteredFiles.isEmpty()) {
        m_libraryTable->hide();
        m_libraryEmptyState->show();
        m_libraryEmptyMessage->setText(
            files.isEmpty()
                ? tr("No study files imported yet. Click 'Import Files' to add documents.")
                : tr("No study files match your search or filter."));
        return;
    }

    m_libraryTable->setRowCount(static_cast<int>(filteredFiles.size()));
    int row = 0;
    for (const StudyFile &file : filteredFiles) {
        auto *nameItem = new QTableWidgetItem(file.originalFilename());
        nameItem->setData(Qt::UserRole, file.id());
        nameItem->setToolTip(file.originalFilename());
        m_libraryTable->setItem(row, 0, nameItem);

        m_libraryTable->setItem(row, 1, new QTableWidgetItem(file.fileType()));
        m_libraryTable->setItem(row, 2, new QTableWidgetItem(formatFileSize(file.fileSize())));

        QString dateDisplay = file.importedAt();
        const QDate date = QDate::fromString(dateDisplay, Qt::ISODate);
        if (date.isValid()) {
            dateDisplay = date.toString(QStringLiteral("ddd, MMM d, yyyy"));
        }
        m_libraryTable->setItem(row, 3, new QTableWidgetItem(dateDisplay));

        QString subjectDisplay = file.subjectId() > 0
            ? subjectNames.value(file.subjectId(), tr("Subject unavailable"))
            : tr("None");
        m_libraryTable->setItem(row, 4, new QTableWidgetItem(subjectDisplay));

        auto *actionsWidget = new QWidget(m_libraryTable);
        actionsWidget->setObjectName(QStringLiteral("libraryActionWidget"));
        auto *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(8, 2, 8, 2);
        actionsLayout->setSpacing(8);

        auto *openButton = new QPushButton(tr("Open"), actionsWidget);
        openButton->setObjectName(QStringLiteral("openStudyFileButton"));
        openButton->setMinimumWidth(76);
        auto *deleteButton = new QPushButton(tr("Delete"), actionsWidget);
        deleteButton->setObjectName(QStringLiteral("deleteStudyFileButton"));
        deleteButton->setMinimumWidth(82);

        actionsLayout->addWidget(openButton);
        actionsLayout->addWidget(deleteButton);
        actionsWidget->setMinimumWidth(178);
        m_libraryTable->setCellWidget(row, 5, actionsWidget);
        m_libraryTable->setRowHeight(row, 56);

        connect(openButton, &QPushButton::clicked, this,
                [this, fileId = file.id()] { openLibraryFile(fileId); });
        connect(deleteButton, &QPushButton::clicked, this,
                [this, fileId = file.id()] { deleteLibraryFile(fileId); });

        ++row;
    }

    m_libraryEmptyState->hide();
    m_libraryTable->show();
}

void NotesPage::openLibraryFile(int fileId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        return;
    }
    QString errorMessage;
    const StudyFile file = m_studyFileService->getStudyFile(m_application->currentUserId(), fileId, &errorMessage);
    if (!errorMessage.isEmpty() || file.id() <= 0) {
        QMessageBox::warning(this, tr("File Not Found"), tr("Could not locate the study file."));
        refreshLibrary();
        return;
    }

    if (!QFileInfo::exists(file.managedFilePath())) {
        QMessageBox::warning(this, tr("File Missing on Disk"),
                             tr("The managed file could not be found on disk. It may have been moved or deleted."));
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(file.managedFilePath()));
}

void NotesPage::deleteLibraryFile(int fileId)
{
    if (!m_application || m_application->currentUserId() <= 0) {
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Delete Study File"),
        tr("Remove this file from your study library? The managed copy will be deleted, but your original source file will remain untouched."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString errorMessage;
    if (!m_studyFileService->removeStudyFile(m_application->currentUserId(), fileId, &errorMessage)) {
        QMessageBox::warning(this, tr("Could Not Remove File"), errorMessage);
        refreshLibrary();
        return;
    }
    refreshLibrary();
}
