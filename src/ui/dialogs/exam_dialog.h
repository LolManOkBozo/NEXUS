#pragma once

#include <QDialog>

#include <QList>

#include "../../models/exam.h"
#include "../../models/subject.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLineEdit;
class QTimeEdit;

class ExamDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExamDialog(const QList<Subject> &subjects,
                        QWidget *parent = nullptr,
                        const Exam &exam = Exam());

    Exam getExam() const;

signals:
    void saveRequested(const Exam &exam);

private:
    Exam m_exam;
    QLineEdit *m_titleEdit;
    QComboBox *m_subjectCombo;
    QDateEdit *m_dateEdit;
    QCheckBox *m_timeEnabled;
    QTimeEdit *m_timeEdit;
    QLineEdit *m_roomEdit;
};
