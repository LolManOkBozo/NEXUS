#pragma once

#include <QList>
#include <QString>

struct AttendanceDuplicateRecord
{
    int id{0};
    int userId{0};
    int subjectId{0};
    QString date;
    QString status;
    QString userName;
    QString subjectName;
};

struct AttendanceDuplicateGroup
{
    int userId{0};
    int subjectId{0};
    QString date;
    QString userName;
    QString subjectName;
    QList<AttendanceDuplicateRecord> records;
};

struct AttendanceDuplicateResolution
{
    int userId{0};
    int subjectId{0};
    QString date;
    int keepRecordId{0};
    QList<AttendanceDuplicateRecord> inspectedRecords;
};

class AttendanceDuplicateRecovery
{
public:
    explicit AttendanceDuplicateRecovery(QString databasePath,
                                         QString backupDirectory = QString());

    QList<AttendanceDuplicateGroup> findDuplicateGroups(
        QString *errorMessage = nullptr) const;
    bool resolveDuplicates(const QList<AttendanceDuplicateResolution> &resolutions,
                           bool explicitlyConfirmed, QString *backupPath = nullptr,
                           QString *errorMessage = nullptr) const;

private:
    QString m_databasePath;
    QString m_backupDirectory;
};
