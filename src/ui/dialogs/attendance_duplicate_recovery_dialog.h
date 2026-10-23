#pragma once

#include <QDialog>

#include "../../core/attendance_duplicate_recovery.h"

class QLabel;
class QTableWidget;

class AttendanceDuplicateRecoveryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttendanceDuplicateRecoveryDialog(const QString &databasePath,
                                               QWidget *parent = nullptr);

    QString backupPath() const;

private slots:
    void resolveSelectedRecords();

private:
    void populateConflicts();

    AttendanceDuplicateRecovery m_recovery;
    QList<AttendanceDuplicateGroup> m_groups;
    QTableWidget *m_table{nullptr};
    QLabel *m_feedback{nullptr};
    QString m_backupPath;
};
