#pragma once

#include <QWidget>

#include <memory>
#include <optional>

#include "../../models/attendance.h"

class Application;
class AttendanceDAO;
class AttendanceService;
class SubjectDAO;
class SubjectService;
class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QShowEvent;
class QTableWidget;

class AttendancePage : public QWidget
{
    Q_OBJECT

public:
    explicit AttendancePage(Application *application, QWidget *parent = nullptr);
    ~AttendancePage() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void refreshPage();
    void saveAttendance();
    void updatePercentage();

private:
    void editAttendance(const Attendance &attendance);
    void deleteAttendance(int attendanceId);
    void resetEditor();
    void setFeedback(const QString &message, bool isError = false);
    bool selectedSubjectId(int *subjectId) const;

    Application *m_application;
    std::unique_ptr<AttendanceDAO> m_attendanceDAO;
    std::unique_ptr<AttendanceService> m_attendanceService;
    std::unique_ptr<SubjectDAO> m_subjectDAO;
    std::unique_ptr<SubjectService> m_subjectService;
    QComboBox *m_subjectCombo;
    QDateEdit *m_attendanceDate;
    QComboBox *m_statusCombo;
    QPushButton *m_saveButton;
    QLabel *m_actionHint;
    QDateEdit *m_rangeStart;
    QDateEdit *m_rangeEnd;
    QLabel *m_percentageLabel;
    QTableWidget *m_historyTable;
    QWidget *m_emptyState;
    QLabel *m_emptyMessage;
    QLabel *m_feedbackLabel;
    std::optional<Attendance> m_editingAttendance;
};
