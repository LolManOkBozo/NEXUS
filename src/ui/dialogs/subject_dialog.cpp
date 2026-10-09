#include "subject_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

SubjectDialog::SubjectDialog(QWidget *parent, bool editMode, const Subject &subject)
    : QDialog(parent)
    , m_subject(subject)
    , m_nameEdit(new QLineEdit(this))
    , m_codeEdit(new QLineEdit(this))
    , m_teacherEdit(new QLineEdit(this))
    , m_creditsSpinBox(new QSpinBox(this))
    , m_semesterSpinBox(new QSpinBox(this))
{
    setWindowTitle(editMode ? tr("Edit Subject") : tr("Add Subject"));
    setModal(true);
    setMinimumWidth(420);

    m_nameEdit->setObjectName(QStringLiteral("subjectNameEdit"));
    m_codeEdit->setObjectName(QStringLiteral("subjectCodeEdit"));
    m_teacherEdit->setObjectName(QStringLiteral("subjectTeacherEdit"));
    m_creditsSpinBox->setObjectName(QStringLiteral("subjectCreditsSpinBox"));
    m_semesterSpinBox->setObjectName(QStringLiteral("subjectSemesterSpinBox"));

    m_nameEdit->setPlaceholderText(tr("e.g. Data Structures"));
    m_codeEdit->setPlaceholderText(tr("e.g. CSIT-214"));
    m_teacherEdit->setPlaceholderText(tr("Optional"));
    m_creditsSpinBox->setRange(1, 30);
    m_creditsSpinBox->setValue(3);
    m_semesterSpinBox->setRange(1, 12);
    m_semesterSpinBox->setValue(1);

    if (editMode) {
        m_nameEdit->setText(subject.name());
        m_codeEdit->setText(subject.code());
        m_teacherEdit->setText(subject.teacher());
        m_creditsSpinBox->setValue(subject.credits());
        m_semesterSpinBox->setValue(subject.semester());
    }

    auto *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(12);
    formLayout->addRow(tr("Subject Name"), m_nameEdit);
    formLayout->addRow(tr("Subject Code"), m_codeEdit);
    formLayout->addRow(tr("Teacher"), m_teacherEdit);
    formLayout->addRow(tr("Credits"), m_creditsSpinBox);
    formLayout->addRow(tr("Semester"), m_semesterSpinBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("saveSubjectButton"));
    buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("cancelSubjectButton"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);
    layout->addLayout(formLayout);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &SubjectDialog::saveSubject);
    connect(buttons, &QDialogButtonBox::rejected, this, &SubjectDialog::reject);
}

Subject SubjectDialog::getSubject() const
{
    Subject subject = m_subject;
    subject.setName(m_nameEdit->text().trimmed());
    subject.setCode(m_codeEdit->text().trimmed());
    subject.setTeacher(m_teacherEdit->text().trimmed());
    subject.setCredits(m_creditsSpinBox->value());
    subject.setSemester(m_semesterSpinBox->value());
    return subject;
}

void SubjectDialog::saveSubject()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Subject Required"), tr("Enter a subject name."));
        m_nameEdit->setFocus();
        return;
    }
    if (m_codeEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Subject Code Required"), tr("Enter a subject code."));
        m_codeEdit->setFocus();
        return;
    }
    emit saveRequested(getSubject());
}
