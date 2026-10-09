#pragma once

#include <QDialog>

#include <QList>

#include "../../models/assignment.h"
#include "../../models/subject.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLineEdit;
class QTextEdit;
class QTimeEdit;

class AssignmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AssignmentDialog(const QList<Subject> &subjects,
                              QWidget *parent = nullptr,
                              const Assignment &assignment = Assignment());

    Assignment getAssignment() const;

signals:
    void saveRequested(const Assignment &assignment);

private:
    Assignment m_assignment;
    QLineEdit *m_titleEdit;
    QComboBox *m_subjectCombo;
    QTextEdit *m_descriptionEdit;
    QDateEdit *m_dueDateEdit;
    QCheckBox *m_dueTimeEnabled;
    QTimeEdit *m_dueTimeEdit;
    QComboBox *m_priorityCombo;
    QComboBox *m_statusCombo;
};
