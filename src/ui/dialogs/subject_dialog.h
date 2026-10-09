#pragma once

#include <QDialog>

#include "../../models/subject.h"

class QLineEdit;
class QSpinBox;

class SubjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubjectDialog(QWidget *parent = nullptr, bool editMode = false,
                           const Subject &subject = Subject());

    Subject getSubject() const;

signals:
    void saveRequested(const Subject &subject);

private slots:
    void saveSubject();

private:
    Subject m_subject;
    QLineEdit *m_nameEdit;
    QLineEdit *m_codeEdit;
    QLineEdit *m_teacherEdit;
    QSpinBox *m_creditsSpinBox;
    QSpinBox *m_semesterSpinBox;
};
