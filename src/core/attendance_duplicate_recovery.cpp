#include "attendance_duplicate_recovery.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>
#include <QUuid>

#include <algorithm>
#include <utility>

#include "attendance_uniqueness_migration.h"
#include "../models/attendance.h"

namespace
{
class ScopedSqliteConnection
{
public:
    explicit ScopedSqliteConnection(const QString &path)
        : m_connectionName(QStringLiteral("nexus_attendance_recovery_%1")
                               .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
        , m_database(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                               m_connectionName))
    {
        m_database.setDatabaseName(path);
        m_database.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
    }

    ~ScopedSqliteConnection()
    {
        m_database.close();
        m_database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    QSqlDatabase &database() { return m_database; }

private:
    QString m_connectionName;
    QSqlDatabase m_database;
};

bool fail(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

bool openDatabase(const QString &path, ScopedSqliteConnection &connection,
                  QString *errorMessage)
{
    if (!QFileInfo::exists(path) || !QFileInfo(path).isFile()) {
        return fail(errorMessage,
                    QStringLiteral("The database file does not exist: %1").arg(path));
    }
    if (!connection.database().open()) {
        return fail(errorMessage,
                    QStringLiteral("Could not open the recovery database: %1")
                        .arg(connection.database().lastError().text()));
    }

    QSqlQuery query(connection.database());
    if (!query.exec(QStringLiteral("PRAGMA foreign_keys = ON"))
        || !query.exec(QStringLiteral("PRAGMA foreign_keys"))
        || !query.next() || query.value(0).toInt() != 1) {
        return fail(errorMessage,
                    QStringLiteral("Could not enable and verify SQLite foreign keys: %1")
                        .arg(query.lastError().text()));
    }
    if (!query.exec(QStringLiteral("PRAGMA busy_timeout = 5000"))) {
        return fail(errorMessage,
                    QStringLiteral("Could not configure SQLite busy timeout: %1")
                        .arg(query.lastError().text()));
    }
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool loadDuplicateGroups(const QSqlDatabase &database,
                         QList<AttendanceDuplicateGroup> *groups,
                         QString *errorMessage)
{
    groups->clear();
    QSqlQuery keysQuery(database);
    if (!keysQuery.exec(QStringLiteral(
            "SELECT user_id, subject_id, date FROM attendance "
            "GROUP BY user_id, subject_id, date HAVING COUNT(*) > 1 "
            "ORDER BY user_id, subject_id, date"))) {
        return fail(errorMessage,
                    QStringLiteral("Could not inspect duplicate attendance keys: %1")
                        .arg(keysQuery.lastError().text()));
    }

    while (keysQuery.next()) {
        AttendanceDuplicateGroup group;
        group.userId = keysQuery.value(0).toInt();
        group.subjectId = keysQuery.value(1).toInt();
        group.date = keysQuery.value(2).toString();

        QSqlQuery recordsQuery(database);
        if (!recordsQuery.prepare(QStringLiteral(
                "SELECT a.id, a.user_id, a.subject_id, a.date, a.status, "
                "COALESCE(u.name, ''), COALESCE(s.name, '') "
                "FROM attendance a "
                "LEFT JOIN users u ON u.id = a.user_id "
                "LEFT JOIN subjects s ON s.id = a.subject_id "
                "WHERE a.user_id = :user_id AND a.subject_id = :subject_id "
                "AND a.date = :date ORDER BY a.id"))) {
            return fail(errorMessage,
                        QStringLiteral("Could not prepare duplicate details query: %1")
                            .arg(recordsQuery.lastError().text()));
        }
        recordsQuery.bindValue(QStringLiteral(":user_id"), group.userId);
        recordsQuery.bindValue(QStringLiteral(":subject_id"), group.subjectId);
        recordsQuery.bindValue(QStringLiteral(":date"), group.date);
        if (!recordsQuery.exec()) {
            return fail(errorMessage,
                        QStringLiteral("Could not read duplicate attendance details: %1")
                            .arg(recordsQuery.lastError().text()));
        }
        while (recordsQuery.next()) {
            AttendanceDuplicateRecord record;
            record.id = recordsQuery.value(0).toInt();
            record.userId = recordsQuery.value(1).toInt();
            record.subjectId = recordsQuery.value(2).toInt();
            record.date = recordsQuery.value(3).toString();
            record.status = recordsQuery.value(4).toString();
            record.userName = recordsQuery.value(5).toString();
            record.subjectName = recordsQuery.value(6).toString();
            if (group.records.isEmpty()) {
                group.userName = record.userName;
                group.subjectName = record.subjectName;
            }
            group.records.append(record);
        }
        if (recordsQuery.lastError().isValid()) {
            return fail(errorMessage,
                        QStringLiteral("Could not finish reading duplicate details: %1")
                            .arg(recordsQuery.lastError().text()));
        }
        if (group.records.size() < 2) {
            return fail(errorMessage,
                        QStringLiteral("A duplicate group changed while it was being inspected."));
        }
        groups->append(group);
    }
    if (keysQuery.lastError().isValid()) {
        return fail(errorMessage,
                    QStringLiteral("Could not finish checking duplicate keys: %1")
                        .arg(keysQuery.lastError().text()));
    }
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool checkForeignKeys(const QSqlDatabase &database, QString *errorMessage)
{
    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA foreign_key_check"))) {
        return fail(errorMessage,
                    QStringLiteral("Could not validate database foreign keys: %1")
                        .arg(query.lastError().text()));
    }
    if (query.next()) {
        return fail(errorMessage,
                    QStringLiteral(
                        "Recovery is unsafe because the database already contains a "
                        "foreign-key violation in table '%1' at row %2.")
                        .arg(query.value(0).toString(), query.value(1).toString()));
    }
    if (query.lastError().isValid()) {
        return fail(errorMessage,
                    QStringLiteral("Could not finish foreign-key validation: %1")
                        .arg(query.lastError().text()));
    }
    return true;
}

bool validateAttendanceGroups(const QSqlDatabase &database,
                              const QList<AttendanceDuplicateGroup> &groups,
                              QString *errorMessage)
{
    if (!checkForeignKeys(database, errorMessage)) {
        return false;
    }

    for (const AttendanceDuplicateGroup &group : groups) {
        QSqlQuery ownershipQuery(database);
        if (!ownershipQuery.prepare(QStringLiteral(
                "SELECT 1 FROM subjects WHERE id = :subject_id AND user_id = :user_id"))) {
            return fail(errorMessage,
                        QStringLiteral("Could not prepare attendance ownership check: %1")
                            .arg(ownershipQuery.lastError().text()));
        }
        ownershipQuery.bindValue(QStringLiteral(":subject_id"), group.subjectId);
        ownershipQuery.bindValue(QStringLiteral(":user_id"), group.userId);
        if (!ownershipQuery.exec() || !ownershipQuery.next()) {
            return fail(errorMessage,
                        QStringLiteral(
                            "Recovery is unsafe: subject %1 is not owned by user %2.")
                            .arg(group.subjectId).arg(group.userId));
        }

        for (const AttendanceDuplicateRecord &record : group.records) {
            const QDate date = Attendance::stringToDate(record.date);
            Attendance::Status status;
            if (record.status == QStringLiteral("Present")) {
                status = Attendance::Status::Present;
            } else if (record.status == QStringLiteral("Absent")) {
                status = Attendance::Status::Absent;
            } else if (record.status == QStringLiteral("Leave")) {
                status = Attendance::Status::Leave;
            } else {
                return fail(errorMessage,
                            QStringLiteral(
                                "Recovery is unsafe: attendance record %1 has invalid status '%2'.")
                                .arg(record.id).arg(record.status));
            }
            const Attendance attendance(record.id, record.userId, record.subjectId,
                                        date, status);
            if (!attendance.isValid()
                || record.userId != group.userId
                || record.subjectId != group.subjectId
                || record.date != group.date) {
                return fail(errorMessage,
                            QStringLiteral(
                                "Recovery is unsafe: attendance record %1 does not satisfy "
                                "the Attendance model's identity and date rules.")
                                .arg(record.id));
            }
        }
    }
    return true;
}

bool readDataVersion(const QSqlDatabase &database, int *version,
                     QString *errorMessage)
{
    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA data_version")) || !query.next()) {
        return fail(errorMessage,
                    QStringLiteral("Could not read SQLite data version: %1")
                        .arg(query.lastError().text()));
    }
    *version = query.value(0).toInt();
    return true;
}

QString quoteIdentifier(const QString &identifier)
{
    QString quoted = identifier;
    quoted.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(quoted);
}

bool readTableCounts(const QSqlDatabase &database, QMap<QString, qint64> *counts,
                     QString *errorMessage)
{
    counts->clear();
    QSqlQuery tablesQuery(database);
    if (!tablesQuery.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type = 'table' "
            "AND name NOT LIKE 'sqlite_%' ORDER BY name"))) {
        return fail(errorMessage,
                    QStringLiteral("Could not inspect database tables for backup: %1")
                        .arg(tablesQuery.lastError().text()));
    }
    QStringList tableNames;
    while (tablesQuery.next()) {
        tableNames.append(tablesQuery.value(0).toString());
    }
    if (tablesQuery.lastError().isValid()) {
        return fail(errorMessage,
                    QStringLiteral("Could not finish inspecting database tables: %1")
                        .arg(tablesQuery.lastError().text()));
    }
    for (const QString &tableName : tableNames) {
        QSqlQuery countQuery(database);
        if (!countQuery.exec(QStringLiteral("SELECT COUNT(*) FROM %1")
                                 .arg(quoteIdentifier(tableName)))
            || !countQuery.next()) {
            return fail(errorMessage,
                        QStringLiteral("Could not verify table '%1' for backup: %2")
                            .arg(tableName, countQuery.lastError().text()));
        }
        counts->insert(tableName, countQuery.value(0).toLongLong());
    }
    return true;
}

bool verifyBackup(const QString &backupPath,
                  const QMap<QString, qint64> &sourceCounts,
                  QString *errorMessage)
{
    ScopedSqliteConnection backup(backupPath);
    if (!backup.database().open()) {
        return fail(errorMessage,
                    QStringLiteral("Could not open the generated backup for verification: %1")
                        .arg(backup.database().lastError().text()));
    }
    QSqlQuery integrityQuery(backup.database());
    if (!integrityQuery.exec(QStringLiteral("PRAGMA integrity_check"))
        || !integrityQuery.next()
        || integrityQuery.value(0).toString() != QStringLiteral("ok")) {
        return fail(errorMessage,
                    QStringLiteral("The generated backup failed SQLite integrity_check: %1")
                        .arg(integrityQuery.value(0).toString()));
    }
    QMap<QString, qint64> backupCounts;
    if (!readTableCounts(backup.database(), &backupCounts, errorMessage)) {
        return false;
    }
    if (backupCounts != sourceCounts) {
        return fail(errorMessage,
                    QStringLiteral(
                        "The generated backup does not contain the same table row counts "
                        "as the source database."));
    }
    return true;
}

bool sameGroupAsResolution(const AttendanceDuplicateGroup &group,
                           const AttendanceDuplicateResolution &resolution)
{
    if (group.userId != resolution.userId
        || group.subjectId != resolution.subjectId
        || group.date != resolution.date) {
        return false;
    }
    QList<AttendanceDuplicateRecord> actualRecords = group.records;
    QList<AttendanceDuplicateRecord> inspectedRecords = resolution.inspectedRecords;
    const auto byId = [](const auto &left, const auto &right) {
        return left.id < right.id;
    };
    std::sort(actualRecords.begin(), actualRecords.end(), byId);
    std::sort(inspectedRecords.begin(), inspectedRecords.end(), byId);
    if (actualRecords.size() != inspectedRecords.size()) {
        return false;
    }
    bool keepRecordExists = false;
    for (qsizetype i = 0; i < actualRecords.size(); ++i) {
        const auto &actual = actualRecords.at(i);
        const auto &inspected = inspectedRecords.at(i);
        if (actual.id != inspected.id || actual.userId != inspected.userId
            || actual.subjectId != inspected.subjectId || actual.date != inspected.date
            || actual.status != inspected.status || actual.userName != inspected.userName
            || actual.subjectName != inspected.subjectName) {
            return false;
        }
        keepRecordExists = keepRecordExists || actual.id == resolution.keepRecordId;
    }
    return keepRecordExists;
}

QString makeBackupPath(const QString &databasePath, const QString &backupDirectory)
{
    const QString suffix = QStringLiteral(".attendance-recovery-%1-%2.bak")
                               .arg(QDateTime::currentDateTimeUtc()
                                        .toString(QStringLiteral("yyyyMMdd'T'HHmmsszzz'Z'")),
                                    QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString fileName = QFileInfo(databasePath).fileName() + suffix;
    return QDir(backupDirectory).filePath(fileName);
}

bool createVerifiedBackup(const QSqlDatabase &database, const QString &databasePath,
                          const QString &backupDirectory, QString *backupPath,
                          QString *errorMessage)
{
    QMap<QString, qint64> sourceTableCounts;
    if (!readTableCounts(database, &sourceTableCounts, errorMessage)) {
        return false;
    }
    const QString generatedBackupPath = makeBackupPath(databasePath, backupDirectory);
    QString sqlPath = generatedBackupPath;
    sqlPath.replace(QLatin1Char('\''), QStringLiteral("''"));
    QSqlQuery backupQuery(database);
    if (!backupQuery.exec(QStringLiteral("VACUUM INTO '%1'").arg(sqlPath))) {
        return fail(errorMessage,
                    QStringLiteral(
                        "Could not create the required SQLite-consistent backup; no attendance "
                        "records were changed: %1")
                        .arg(backupQuery.lastError().text()));
    }
    backupQuery.finish();
    if (backupPath) {
        *backupPath = generatedBackupPath;
    }
    if (!verifyBackup(generatedBackupPath, sourceTableCounts, errorMessage)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 Unverified backup retained at: %2")
                                .arg(*errorMessage, generatedBackupPath);
        }
        return false;
    }
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}
}

AttendanceDuplicateRecovery::AttendanceDuplicateRecovery(QString databasePath,
                                                         QString backupDirectory)
    : m_databasePath(std::move(databasePath))
    , m_backupDirectory(backupDirectory.isEmpty()
                            ? QFileInfo(m_databasePath).absolutePath()
                            : std::move(backupDirectory))
{
}

QList<AttendanceDuplicateGroup> AttendanceDuplicateRecovery::findDuplicateGroups(
    QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    ScopedSqliteConnection connection(m_databasePath);
    if (!openDatabase(m_databasePath, connection, errorMessage)) {
        return {};
    }
    QList<AttendanceDuplicateGroup> groups;
    if (!loadDuplicateGroups(connection.database(), &groups, errorMessage)) {
        return {};
    }
    return groups;
}

bool AttendanceDuplicateRecovery::resolveDuplicates(
    const QList<AttendanceDuplicateResolution> &resolutions,
    bool explicitlyConfirmed, QString *backupPath, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (backupPath) {
        backupPath->clear();
    }
    if (!explicitlyConfirmed) {
        return fail(errorMessage,
                    QStringLiteral("Attendance duplicate recovery was cancelled; no data was changed."));
    }

    ScopedSqliteConnection connection(m_databasePath);
    if (!openDatabase(m_databasePath, connection, errorMessage)) {
        return false;
    }

    int dataVersionBeforeBackup = 0;
    if (!readDataVersion(connection.database(), &dataVersionBeforeBackup, errorMessage)) {
        return false;
    }

    QList<AttendanceDuplicateGroup> groups;
    if (!loadDuplicateGroups(connection.database(), &groups, errorMessage)) {
        return false;
    }
    if (groups.isEmpty()) {
        if (!resolutions.isEmpty()) {
            return fail(errorMessage,
                        QStringLiteral("The inspected duplicate records are no longer present."));
        }
        QString generatedBackupPath;
        if (!createVerifiedBackup(connection.database(), m_databasePath,
                                  m_backupDirectory, &generatedBackupPath,
                                  errorMessage)) {
            if (backupPath) {
                *backupPath = generatedBackupPath;
            }
            return false;
        }
        if (backupPath) {
            *backupPath = generatedBackupPath;
        }
        QSqlQuery beginQuery(connection.database());
        if (!beginQuery.exec(QStringLiteral("BEGIN IMMEDIATE TRANSACTION"))) {
            return fail(errorMessage,
                        QStringLiteral("Could not begin migration retry; backup retained at %1: %2")
                            .arg(generatedBackupPath, beginQuery.lastError().text()));
        }
        int dataVersionAfterBackup = 0;
        if (!readDataVersion(connection.database(), &dataVersionAfterBackup, errorMessage)) {
            connection.database().rollback();
            if (errorMessage) {
                *errorMessage += QStringLiteral(" Migration retry rolled back. Backup retained at: %1")
                                     .arg(generatedBackupPath);
            }
            return false;
        }
        if (dataVersionAfterBackup != dataVersionBeforeBackup) {
            connection.database().rollback();
            return fail(errorMessage,
                        QStringLiteral(
                            "The database changed while the backup was being created. "
                            "Migration retry was rolled back; backup retained at: %1")
                            .arg(generatedBackupPath));
        }
        if (!ensureAttendanceUniquenessMigration(connection.database(), errorMessage)) {
            connection.database().rollback();
            if (errorMessage) {
                *errorMessage += QStringLiteral(" Backup retained at: %1").arg(generatedBackupPath);
            }
            return false;
        }
        if (!connection.database().commit()) {
            const QString commitError = connection.database().lastError().text();
            connection.database().rollback();
            return fail(errorMessage,
                        QStringLiteral("Could not commit migration retry; backup retained at %1: %2")
                            .arg(generatedBackupPath, commitError));
        }
        return true;
    }
    if (resolutions.size() != groups.size()) {
        return fail(errorMessage,
                    QStringLiteral("Choose exactly one record to retain in every duplicate group."));
    }
    if (!validateAttendanceGroups(connection.database(), groups, errorMessage)) {
        return false;
    }

    QList<AttendanceDuplicateResolution> orderedResolutions = resolutions;
    std::sort(orderedResolutions.begin(), orderedResolutions.end(),
              [](const auto &left, const auto &right) {
                  if (left.userId != right.userId) {
                      return left.userId < right.userId;
                  }
                  if (left.subjectId != right.subjectId) {
                      return left.subjectId < right.subjectId;
                  }
                  return left.date < right.date;
              });
    for (qsizetype i = 0; i < groups.size(); ++i) {
        if (!sameGroupAsResolution(groups.at(i), orderedResolutions.at(i))) {
            return fail(errorMessage,
                        QStringLiteral(
                            "The duplicate records changed after inspection. Refresh the list "
                            "and explicitly choose a record to retain in each group."));
        }
    }

    QString generatedBackupPath;
    if (!createVerifiedBackup(connection.database(), m_databasePath,
                              m_backupDirectory, &generatedBackupPath,
                              errorMessage)) {
        if (backupPath) {
            *backupPath = generatedBackupPath;
        }
        return false;
    }
    if (backupPath) {
        *backupPath = generatedBackupPath;
    }

    QSqlQuery beginQuery(connection.database());
    if (!beginQuery.exec(QStringLiteral("BEGIN IMMEDIATE TRANSACTION"))) {
        return fail(errorMessage,
                    QStringLiteral("Could not begin recovery transaction; backup retained at %1: %2")
                        .arg(generatedBackupPath, beginQuery.lastError().text()));
    }
    auto rollback = [&connection]() -> QString {
        QSqlQuery rollbackQuery(connection.database());
        if (!rollbackQuery.exec(QStringLiteral("ROLLBACK"))) {
            return rollbackQuery.lastError().text();
        }
        return {};
    };
    auto rollbackWithBackup = [&](const QString &message) {
        const QString rollbackError = rollback();
        const QString rollbackDetails = rollbackError.isEmpty()
            ? QString()
            : QStringLiteral(" Rollback reported an error: %1").arg(rollbackError);
        return fail(errorMessage,
                    QStringLiteral("%1 Recovery transaction rollback attempted.%2 "
                                   "Backup retained at: %3")
                        .arg(message, rollbackDetails, generatedBackupPath));
    };

    int dataVersionAfterLock = 0;
    if (!readDataVersion(connection.database(), &dataVersionAfterLock, errorMessage)) {
        return rollbackWithBackup(errorMessage ? *errorMessage
                                               : QStringLiteral("Could not recheck database state."));
    }
    if (dataVersionAfterLock != dataVersionBeforeBackup) {
        return rollbackWithBackup(
            QStringLiteral(
                "The database changed while the backup was being created. No records "
                "were changed; refresh the conflicts and retry."));
    }
    QList<AttendanceDuplicateGroup> currentGroups;
    if (!loadDuplicateGroups(connection.database(), &currentGroups, errorMessage)) {
        return rollbackWithBackup(errorMessage ? *errorMessage
                                               : QStringLiteral("Could not recheck duplicate groups."));
    }
    if (currentGroups.size() != orderedResolutions.size()) {
        return rollbackWithBackup(
            QStringLiteral(
                "Duplicate groups changed before recovery. No records were changed; "
                "refresh the conflicts and retry."));
    }
    if (!validateAttendanceGroups(connection.database(), currentGroups, errorMessage)) {
        return rollbackWithBackup(errorMessage ? *errorMessage
                                               : QStringLiteral("Attendance validation failed."));
    }

    for (qsizetype i = 0; i < currentGroups.size(); ++i) {
        const AttendanceDuplicateGroup &group = currentGroups.at(i);
        const AttendanceDuplicateResolution &resolution = orderedResolutions.at(i);
        if (!sameGroupAsResolution(group, resolution)) {
            return rollbackWithBackup(
                QStringLiteral(
                    "A duplicate group changed before recovery. No records were changed; "
                    "refresh the conflicts and retry."));
        }
        QSqlQuery deleteQuery(connection.database());
        if (!deleteQuery.prepare(QStringLiteral(
                "DELETE FROM attendance WHERE user_id = :user_id "
                "AND subject_id = :subject_id AND date = :date AND id <> :keep_id"))) {
            return rollbackWithBackup(
                QStringLiteral("Could not prepare conflict resolution: %1")
                    .arg(deleteQuery.lastError().text()));
        }
        deleteQuery.bindValue(QStringLiteral(":user_id"), group.userId);
        deleteQuery.bindValue(QStringLiteral(":subject_id"), group.subjectId);
        deleteQuery.bindValue(QStringLiteral(":date"), group.date);
        deleteQuery.bindValue(QStringLiteral(":keep_id"), resolution.keepRecordId);
        if (!deleteQuery.exec()
            || deleteQuery.numRowsAffected() != group.records.size() - 1) {
            const QString deleteError = deleteQuery.lastError().text();
            return rollbackWithBackup(
                QStringLiteral(
                    "Could not resolve duplicate attendance group for user %1, subject %2, "
                    "date '%3': %4")
                    .arg(group.userId).arg(group.subjectId).arg(group.date, deleteError));
        }
    }

    QList<AttendanceDuplicateGroup> remainingGroups;
    if (!loadDuplicateGroups(connection.database(), &remainingGroups, errorMessage)) {
        return rollbackWithBackup(errorMessage ? *errorMessage
                                               : QStringLiteral("Could not validate resolved groups."));
    }
    if (!remainingGroups.isEmpty()) {
        return rollbackWithBackup(
            QStringLiteral("Duplicate validation failed after resolution."));
    }
    if (!checkForeignKeys(connection.database(), errorMessage)
        || !ensureAttendanceUniquenessMigration(connection.database(), errorMessage)) {
        return rollbackWithBackup(errorMessage ? *errorMessage
                                               : QStringLiteral("Final integrity validation failed."));
    }

    QSqlQuery commitQuery(connection.database());
    if (!commitQuery.exec(QStringLiteral("COMMIT"))) {
        const QString commitError = commitQuery.lastError().text();
        return rollbackWithBackup(
            QStringLiteral("Could not commit attendance recovery: %1").arg(commitError));
    }
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}
