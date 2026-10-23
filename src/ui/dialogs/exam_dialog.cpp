#include "exam_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

ExamDialog::ExamDialog(const QList<Subject> &subjects, QWidget *parent,
                       const Exam &exam)
    : QDialog(parent)
    , m_exam(exam)
    , m_titleEdit(new QLineEdit(this))
    , m_subjectCombo(new QComboBox(this))
    , m_dateEdit(new QDateEdit(this))
    , m_timeEnabled(new QCheckBox(tr("Set start time"), this))
    , m_timeEdit(new QTimeEdit(this))
    , m_roomEdit(new QLineEdit(this))
{
    setWindowTitle(exam.id() > 0 ? tr("Edit Exam") : tr("Add Exam"));
    setModal(true);
    setMinimumWidth(440);

    m_titleEdit->setObjectName(QStringLiteral("examTitleEdit"));
    m_titleEdit->setPlaceholderText(tr("e.g. Database Systems Final"));
    m_subjectCombo->setObjectName(QStringLiteral("examSubjectCombo"));
    for (const Subject &subject : subjects) {
        m_subjectCombo->addItem(subject.name(), subject.id());
    }
    m_dateEdit->setObjectName(QStringLiteral("examDateEdit"));
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_dateEdit->setDate(QDate::currentDate());
    m_timeEnabled->setObjectName(QStringLiteral("examTimeEnabled"));
    m_timeEdit->setObjectName(QStringLiteral("examTimeEdit"));
    m_timeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    m_timeEdit->setTime(QTime(9, 0));
    m_timeEdit->setEnabled(false);
    m_roomEdit->setObjectName(QStringLiteral("examRoomEdit"));
    m_roomEdit->setPlaceholderText(tr("Optional"));

    m_titleEdit->setText(exam.title());
    const int subjectIndex = m_subjectCombo->findData(exam.subjectId());
    if (subjectIndex >= 0) {
        m_subjectCombo->setCurrentIndex(subjectIndex);
    }
    if (!exam.examDate().isEmpty()) {
        const QDate date = QDate::fromString(exam.examDate(), Qt::ISODate);
        if (date.isValid()) {
            m_dateEdit->setDate(date);
        }
    }
    if (!exam.startTime().isEmpty()) {
        const QTime time = QTime::fromString(exam.startTime(),
                                             QStringLiteral("HH:mm"));
        if (time.isValid()) {
            m_timeEnabled->setChecked(true);
            m_timeEdit->setEnabled(true);
            m_timeEdit->setTime(time);
        }
    }
    m_roomEdit->setText(exam.room());

    auto *timeRow = new QWidget(this);
    auto *timeLayout = new QHBoxLayout(timeRow);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->addWidget(m_timeEnabled);
    timeLayout->addWidget(m_timeEdit);
    timeLayout->addStretch();

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(14);
    form->addRow(tr("Exam title"), m_titleEdit);
    form->addRow(tr("Subject"), m_subjectCombo);
    form->addRow(tr("Date"), m_dateEdit);
    form->addRow(tr("Time"), timeRow);
    form->addRow(tr("Room"), m_roomEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    buttons->button(QDialogButtonBox::Save)->setObjectName(
        QStringLiteral("saveExamButton"));
    buttons->button(QDialogButtonBox::Cancel)->setObjectName(
        QStringLiteral("cancelExamButton"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 28, 28, 28);
    layout->setSpacing(20);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(m_timeEnabled, &QCheckBox::toggled,
            m_timeEdit, &QTimeEdit::setEnabled);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (m_titleEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Title Required"),
                                 tr("Enter an exam title."));
            m_titleEdit->setFocus();
            return;
        }
        if (m_subjectCombo->currentData().toInt() <= 0) {
            QMessageBox::warning(this, tr("Subject Required"),
                                 tr("Select a subject for this exam."));
            m_subjectCombo->setFocus();
            return;
        }
        emit saveRequested(getExam());
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &ExamDialog::reject);
}

Exam ExamDialog::getExam() const
{
    Exam exam = m_exam;
    exam.setTitle(m_titleEdit->text().trimmed());
    exam.setSubjectId(m_subjectCombo->currentData().toInt());
    exam.setExamDate(m_dateEdit->date().toString(Qt::ISODate));
    exam.setStartTime(m_timeEnabled->isChecked()
                          ? m_timeEdit->time().toString(QStringLiteral("HH:mm"))
                          : QString());
    exam.setRoom(m_roomEdit->text().trimmed());
    return exam;
}
