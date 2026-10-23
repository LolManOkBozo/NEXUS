#include <QtTest/QtTest>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QUuid>

#include "../src/ui/dialogs/attendance_duplicate_recovery_dialog.h"

namespace
{
bool createDatabase(const QString &path)
{
    const QString connectionName =
        QStringLiteral("attendance_recovery_ui_%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase database =
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(path);
    if (!database.open()) {
        return false;
    }
    bool succeeded = false;
    {
        QSqlQuery query(database);
        succeeded = query.exec(QStringLiteral("PRAGMA foreign_keys = ON"))
            && query.exec(QStringLiteral(
                "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT, "
                "created_at TEXT)"))
            && query.exec(QStringLiteral(
                "CREATE TABLE subjects (id INTEGER PRIMARY KEY, user_id INTEGER, "
                "name TEXT, created_at TEXT, "
                "FOREIGN KEY(user_id) REFERENCES users(id))"))
            && query.exec(QStringLiteral(
                "CREATE TABLE attendance (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "user_id INTEGER NOT NULL, subject_id INTEGER NOT NULL, date TEXT NOT NULL, "
                "status TEXT NOT NULL, FOREIGN KEY(user_id) REFERENCES users(id), "
                "FOREIGN KEY(subject_id) REFERENCES subjects(id))"))
            && query.exec(QStringLiteral(
                "INSERT INTO users VALUES (1, 'User', 'ui@example.test', '2026-01-01')"))
            && query.exec(QStringLiteral(
                "INSERT INTO subjects VALUES (10, 1, 'Chemistry', '2026-01-01')"))
            && query.exec(QStringLiteral(
                "INSERT INTO attendance (id, user_id, subject_id, date, status) VALUES "
                "(1, 1, 10, '2026-10-19', 'Present'), "
                "(2, 1, 10, '2026-10-19', 'Absent'), "
                "(3, 1, 10, '2026-10-20', 'Leave')"));
    }
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
    return succeeded;
}

int attendanceCount(const QString &path)
{
    const QString connectionName =
        QStringLiteral("attendance_recovery_ui_verify_%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase database =
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(path);
    if (!database.open()) {
        return -1;
    }
    int count = -1;
    {
        QSqlQuery query(database);
        if (query.exec(QStringLiteral("SELECT COUNT(*) FROM attendance"))
            && query.next()) {
            count = query.value(0).toInt();
        }
    }
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
    return count;
}

void clickConfirmation(QMessageBox::StandardButton answer)
{
    QTimer::singleShot(0, [answer]() {
        auto *messageBox = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (messageBox) {
            if (QAbstractButton *button = messageBox->button(answer)) {
                button->click();
            }
        }
    });
}
}

class AttendanceDuplicateRecoveryUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;

private slots:
    void initTestCase();
    void displaysDetailedGroupsAndRequiresAChoice();
    void cancellationLeavesTheDatabaseUnchanged();
    void confirmedSelectionRunsRecoveryAndShowsBackup();
};

void AttendanceDuplicateRecoveryUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
}

void AttendanceDuplicateRecoveryUiTests::displaysDetailedGroupsAndRequiresAChoice()
{
    const QString path = m_temporaryDirectory.filePath(QStringLiteral("no-choice.db"));
    QVERIFY(createDatabase(path));
    AttendanceDuplicateRecoveryDialog dialog(path);
    auto *table = dialog.findChild<QTableWidget *>(
        QStringLiteral("duplicateAttendanceTable"));
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 2);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("User"));
    QCOMPARE(table->item(0, 2)->text(), QStringLiteral("Chemistry"));
    QCOMPARE(table->item(0, 4)->text(), QStringLiteral("2026-10-19"));
    QCOMPARE(table->item(1, 6)->text(), QStringLiteral("Absent"));

    auto *button = dialog.findChild<QPushButton *>(
        QStringLiteral("resolveAttendanceDuplicatesButton"));
    QVERIFY(button);
    QTest::mouseClick(button, Qt::LeftButton);
    auto *feedback = dialog.findChild<QLabel *>(QStringLiteral("recoveryFeedback"));
    QVERIFY(feedback);
    QVERIFY(feedback->text().contains(QStringLiteral("Choose one record")));
    QCOMPARE(attendanceCount(path), 3);
}

void AttendanceDuplicateRecoveryUiTests::cancellationLeavesTheDatabaseUnchanged()
{
    const QString path = m_temporaryDirectory.filePath(QStringLiteral("cancel.db"));
    QVERIFY(createDatabase(path));
    AttendanceDuplicateRecoveryDialog dialog(path);
    auto *selector = dialog.findChild<QComboBox *>(
        QStringLiteral("keepRecordSelector_1"));
    QVERIFY(selector);
    selector->setCurrentIndex(1);

    clickConfirmation(QMessageBox::Cancel);
    auto *button = dialog.findChild<QPushButton *>(
        QStringLiteral("resolveAttendanceDuplicatesButton"));
    QVERIFY(button);
    QTest::mouseClick(button, Qt::LeftButton);
    QCOMPARE(dialog.result(), 0);
    QCOMPARE(attendanceCount(path), 3);
}

void AttendanceDuplicateRecoveryUiTests::confirmedSelectionRunsRecoveryAndShowsBackup()
{
    const QString path = m_temporaryDirectory.filePath(QStringLiteral("confirm.db"));
    QVERIFY(createDatabase(path));
    AttendanceDuplicateRecoveryDialog dialog(path);
    auto *selector = dialog.findChild<QComboBox *>(
        QStringLiteral("keepRecordSelector_1"));
    QVERIFY(selector);
    selector->setCurrentIndex(2);

    clickConfirmation(QMessageBox::Yes);
    auto *button = dialog.findChild<QPushButton *>(
        QStringLiteral("resolveAttendanceDuplicatesButton"));
    QVERIFY(button);
    QTest::mouseClick(button, Qt::LeftButton);
    QCOMPARE(dialog.result(), QDialog::Accepted);
    QVERIFY(QFileInfo::exists(dialog.backupPath()));
    QCOMPARE(attendanceCount(path), 2);
}

QTEST_MAIN(AttendanceDuplicateRecoveryUiTests)
#include "attendance_duplicate_recovery_ui_tests.moc"
