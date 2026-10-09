#pragma once

#include <QDialog>

#include <QList>

#include "../../models/subject.h"
#include "../../models/timetable.h"

class QComboBox;
class QLineEdit;
class QTimeEdit;

class TimetableEntryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TimetableEntryDialog(const QList<Subject> &subjects, int dayOfWeek,
                                  QWidget *parent = nullptr, bool editMode = false,
                                  const Timetable &timetable = Timetable());

    Timetable getTimetable() const;

signals:
    void saveRequested(const Timetable &timetable);

private:
    QComboBox *m_subjectCombo;
    QComboBox *m_weekdayCombo;
    QTimeEdit *m_startTimeEdit;
    QTimeEdit *m_endTimeEdit;
    QLineEdit *m_roomEdit;
    int m_timetableId;
};
