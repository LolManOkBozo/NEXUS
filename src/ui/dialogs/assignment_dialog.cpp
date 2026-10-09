#include "assignment_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

AssignmentDialog::AssignmentDialog(const QList<Subject> &subjects,
                                   QWidget *parent,
                                   const Assignment &assignment)
    : QDialog(parent)
    , m_assignment(assignment)
    , m_titleEdit(new QLineEdit(this))
    , m_subjectCombo(new QComboBox(this))
    , m_descriptionEdit(new QTextEdit(this))
    , m_dueDateEdit(new QDateEdit(this))
    , m_dueTimeEnabled(new QCheckBox(tr("Set due time"), this))
    , m_dueTimeEdit(new QTimeEdit(this))
    , m_priorityCombo(new QComboBox(this))
    , m_statusCombo(new QComboBox(this))
{
    setWindowTitle(assignment.id() > 0 ? tr("Edit Assignment")
                                        : tr("Add Assignment"));
    setModal(true);
    setMinimumWidth(460);

    m_titleEdit->setObjectName(QStringLiteral("assignmentTitleEdit"));
    m_titleEdit->setPlaceholderText(tr("Assignment title"));
    m_subjectCombo->setObjectName(QStringLiteral("assignmentSubjectCombo"));
    for (const Subject &subject : subjects) {
        m_subjectCombo->addItem(subject.name(), subject.id());
    }
    m_descriptionEdit->setObjectName(QStringLiteral("assignmentDescriptionEdit"));
    m_descriptionEdit->setPlaceholderText(tr("Optional instructions"));
    m_descriptionEdit->setMaximumHeight(110);
    m_dueDateEdit->setObjectName(QStringLiteral("assignmentDueDateEdit"));
    m_dueDateEdit->setCalendarPopup(true);
    m_dueDateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_dueDateEdit->setDate(QDate::currentDate());
    m_dueTimeEnabled->setObjectName(QStringLiteral("assignmentDueTimeEnabled"));
    m_dueTimeEdit->setObjectName(QStringLiteral("assignmentDueTimeEdit"));
    m_dueTimeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    m_dueTimeEdit->setTime(QTime(23, 59));
    m_dueTimeEdit->setEnabled(false);

    m_priorityCombo->setObjectName(QStringLiteral("assignmentPriorityCombo"));
    m_priorityCombo->addItems({tr("Low"), tr("Medium"), tr("High")});
    m_statusCombo->setObjectName(QStringLiteral("assignmentStatusCombo"));
    m_statusCombo->addItems({tr("Not Started"), tr("In Progress"), tr("Completed")});

    m_titleEdit->setText(assignment.title());
    m_subjectCombo->setCurrentIndex(m_subjectCombo->findData(assignment.subjectId()));
    m_descriptionEdit->setPlainText(assignment.description());
    if (assignment.dueDate().isEmpty()) {
        m_dueDateEdit->setDate(QDate::currentDate());
    } else {
        m_dueDateEdit->setDate(QDate::fromString(assignment.dueDate(), Qt::ISODate));
    }
    if (!assignment.dueTime().isEmpty()) {
        m_dueTimeEnabled->setChecked(true);
        m_dueTimeEdit->setEnabled(true);
        m_dueTimeEdit->setTime(QTime::fromString(assignment.dueTime(),
                                                 QStringLiteral("HH:mm")));
    }
    const int priorityIndex = m_priorityCombo->findText(assignment.priority());
    m_priorityCombo->setCurrentIndex(priorityIndex >= 0 ? priorityIndex : 1);
    const int statusIndex = m_statusCombo->findText(assignment.status());
    m_statusCombo->setCurrentIndex(statusIndex >= 0 ? statusIndex : 0);

    auto *deadlineWidget = new QWidget(this);
    auto *deadlineLayout = new QVBoxLayout(deadlineWidget);
    deadlineLayout->setContentsMargins(0, 0, 0, 0);
    deadlineLayout->setSpacing(8);
    deadlineLayout->addWidget(m_dueDateEdit);
    auto *timeLayout = new QHBoxLayout;
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->addWidget(m_dueTimeEnabled);
    timeLayout->addWidget(m_dueTimeEdit);
    timeLayout->addStretch();
    deadlineLayout->addLayout(timeLayout);

    auto *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(14);
    auto createFieldLabel = [this](const QString &text) {
        auto *label = new QLabel(text, this);
        label->setObjectName(QStringLiteral("assignmentFieldLabel"));
        return label;
    };
    formLayout->addRow(createFieldLabel(tr("Title")), m_titleEdit);
    formLayout->addRow(createFieldLabel(tr("Subject")), m_subjectCombo);
    formLayout->addRow(createFieldLabel(tr("Description")), m_descriptionEdit);
    formLayout->addRow(createFieldLabel(tr("Due date")), deadlineWidget);
    formLayout->addRow(createFieldLabel(tr("Priority")), m_priorityCombo);
    formLayout->addRow(createFieldLabel(tr("Status")), m_statusCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    buttons->button(QDialogButtonBox::Save)->setObjectName(
        QStringLiteral("saveAssignmentButton"));
    buttons->button(QDialogButtonBox::Cancel)->setObjectName(
        QStringLiteral("cancelAssignmentButton"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(24);
    layout->addLayout(formLayout);
    layout->addWidget(buttons);

    connect(m_dueTimeEnabled, &QCheckBox::toggled,
            m_dueTimeEdit, &QTimeEdit::setEnabled);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (m_titleEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Title Required"),
                                 tr("Enter an assignment title."));
            m_titleEdit->setFocus();
            return;
        }
        emit saveRequested(getAssignment());
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &AssignmentDialog::reject);
}

Assignment AssignmentDialog::getAssignment() const
{
    Assignment assignment = m_assignment;
    assignment.setTitle(m_titleEdit->text().trimmed());
    assignment.setSubjectId(m_subjectCombo->currentData().toInt());
    assignment.setDescription(m_descriptionEdit->toPlainText().trimmed());
    assignment.setDueDate(m_dueDateEdit->date().toString(Qt::ISODate));
    assignment.setDueTime(m_dueTimeEnabled->isChecked()
                              ? m_dueTimeEdit->time().toString(QStringLiteral("HH:mm"))
                              : QString());
    assignment.setPriority(m_priorityCombo->currentText());
    assignment.setStatus(m_statusCombo->currentText());
    return assignment;
}
