#include "timetable_entry_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTime>
#include <QTimeEdit>
#include <QStringList>
#include <QVBoxLayout>

TimetableEntryDialog::TimetableEntryDialog(const QList<Subject> &subjects,
                                           int dayOfWeek, QWidget *parent,
                                           bool editMode,
                                           const Timetable &timetable)
    : QDialog(parent)
    , m_subjectCombo(new QComboBox(this))
    , m_weekdayCombo(new QComboBox(this))
    , m_startTimeEdit(new QTimeEdit(this))
    , m_endTimeEdit(new QTimeEdit(this))
    , m_roomEdit(new QLineEdit(this))
    , m_timetableId(editMode ? timetable.id() : 0)
{
    setWindowTitle(editMode ? tr("Edit Timetable Entry")
                            : tr("Add Timetable Entry"));
    setModal(true);
    setMinimumWidth(420);

    m_subjectCombo->setObjectName(QStringLiteral("timetableSubjectCombo"));
    for (const Subject &subject : subjects) {
        m_subjectCombo->addItem(subject.name(), subject.id());
    }

    m_weekdayCombo->setObjectName(QStringLiteral("timetableEntryWeekdayCombo"));
    const QStringList weekdays{
        tr("Monday"), tr("Tuesday"), tr("Wednesday"), tr("Thursday"),
        tr("Friday"), tr("Saturday"), tr("Sunday")
    };
    for (qsizetype index = 0; index < weekdays.size(); ++index) {
        m_weekdayCombo->addItem(weekdays.at(index), index + 1);
    }
    const int weekdayIndex = m_weekdayCombo->findData(
        editMode ? timetable.dayOfWeek() : dayOfWeek);
    if (weekdayIndex >= 0) {
        m_weekdayCombo->setCurrentIndex(weekdayIndex);
    }

    m_startTimeEdit->setObjectName(QStringLiteral("timetableStartTimeEdit"));
    m_startTimeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    m_startTimeEdit->setTime(QTime(9, 0));
    m_endTimeEdit->setObjectName(QStringLiteral("timetableEndTimeEdit"));
    m_endTimeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    m_endTimeEdit->setTime(QTime(10, 0));
    m_roomEdit->setObjectName(QStringLiteral("timetableRoomEdit"));
    m_roomEdit->setPlaceholderText(tr("Optional"));

    if (editMode) {
        m_subjectCombo->setCurrentIndex(
            m_subjectCombo->findData(timetable.subjectId()));
        m_startTimeEdit->setTime(QTime::fromString(timetable.startTime(),
                                                   QStringLiteral("HH:mm")));
        m_endTimeEdit->setTime(QTime::fromString(timetable.endTime(),
                                                 QStringLiteral("HH:mm")));
        m_roomEdit->setText(timetable.room());
    }

    auto *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(12);
    formLayout->addRow(tr("Subject"), m_subjectCombo);
    formLayout->addRow(tr("Weekday"), m_weekdayCombo);
    formLayout->addRow(tr("Start Time"), m_startTimeEdit);
    formLayout->addRow(tr("End Time"), m_endTimeEdit);
    formLayout->addRow(tr("Room"), m_roomEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    buttons->button(QDialogButtonBox::Save)->setObjectName(
        QStringLiteral("saveTimetableButton"));
    buttons->button(QDialogButtonBox::Cancel)->setObjectName(
        QStringLiteral("cancelTimetableButton"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);
    layout->addLayout(formLayout);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        emit saveRequested(getTimetable());
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &TimetableEntryDialog::reject);
}

Timetable TimetableEntryDialog::getTimetable() const
{
    return Timetable(m_timetableId, 0, m_subjectCombo->currentData().toInt(),
                     m_weekdayCombo->currentData().toInt(),
                     m_startTimeEdit->time().toString(QStringLiteral("HH:mm")),
                     m_endTimeEdit->time().toString(QStringLiteral("HH:mm")),
                     m_roomEdit->text().trimmed());
}
