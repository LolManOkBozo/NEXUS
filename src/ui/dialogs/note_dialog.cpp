#include "note_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

NoteDialog::NoteDialog(const QList<Subject> &subjects, QWidget *parent,
                       const Note &note)
    : QDialog(parent)
    , m_note(note)
    , m_titleEdit(new QLineEdit(this))
    , m_contentEdit(new QTextEdit(this))
    , m_subjectCombo(new QComboBox(this))
{
    setWindowTitle(note.id() > 0 ? tr("Edit Note") : tr("Add Note"));
    setModal(true);
    setMinimumWidth(520);

    m_titleEdit->setObjectName(QStringLiteral("noteTitleEdit"));
    m_titleEdit->setPlaceholderText(tr("e.g. Lecture summary"));

    m_contentEdit->setObjectName(QStringLiteral("noteContentEdit"));
    m_contentEdit->setPlaceholderText(tr("Write your note here..."));
    m_contentEdit->setAcceptRichText(false);

    m_subjectCombo->setObjectName(QStringLiteral("noteSubjectCombo"));
    m_subjectCombo->addItem(tr("None (no subject)"), 0);
    for (const Subject &subject : subjects) {
        m_subjectCombo->addItem(subject.name(), subject.id());
    }

    m_titleEdit->setText(note.title());
    const int subjectIndex = m_subjectCombo->findData(note.subjectId());
    if (subjectIndex >= 0) {
        m_subjectCombo->setCurrentIndex(subjectIndex);
    }
    m_contentEdit->setPlainText(note.content());

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(14);
    form->addRow(tr("Note title"), m_titleEdit);
    form->addRow(tr("Subject"), m_subjectCombo);
    form->addRow(tr("Content"), m_contentEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    buttons->button(QDialogButtonBox::Save)->setObjectName(
        QStringLiteral("saveNoteButton"));
    buttons->button(QDialogButtonBox::Cancel)->setObjectName(
        QStringLiteral("cancelNoteButton"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 28, 28, 28);
    layout->setSpacing(20);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (m_titleEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Title Required"),
                                 tr("Enter a note title."));
            m_titleEdit->setFocus();
            return;
        }
        emit saveRequested(getNote());
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &NoteDialog::reject);
}

Note NoteDialog::getNote() const
{
    Note note = m_note;
    note.setTitle(m_titleEdit->text().trimmed());
    note.setContent(m_contentEdit->toPlainText().trimmed());
    note.setSubjectId(m_subjectCombo->currentData().toInt());
    return note;
}
