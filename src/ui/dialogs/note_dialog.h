#pragma once

#include <QDialog>

#include <QList>

#include "../../models/note.h"
#include "../../models/subject.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QTextEdit;

class NoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NoteDialog(const QList<Subject> &subjects,
                        QWidget *parent = nullptr,
                        const Note &note = Note());

    Note getNote() const;

signals:
    void saveRequested(const Note &note);

private:
    Note m_note;
    QLineEdit *m_titleEdit;
    QTextEdit *m_contentEdit;
    QComboBox *m_subjectCombo;
};
