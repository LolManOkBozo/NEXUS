#include <QtTest/QtTest>

#include <QApplication>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>

#include "../src/core/application.h"
#include "../src/ui/pages/attendance_page.h"

class AttendanceUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    AttendancePage *m_page{nullptr};
    int m_firstSubjectId{0};
    int m_secondSubjectId{0};
    int m_otherUserId{0};
    int m_otherSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    int insertAttendance(int userId, int subjectId, const QDate &date,
                         const QString &status);
    QComboBox *subjectCombo() const;
    QDateEdit *attendanceDate() const;
    QComboBox *statusCombo() const;
    QPushButton *saveButton() const;
    QDateEdit *rangeStart() const;
    QDateEdit *rangeEnd() const;
    QTableWidget *historyTable() const;
    void setDateRange(const QDate &startDate, const QDate &endDate);
    void scheduleConfirmation(QMessageBox::StandardButton answer);
    int countAttendance(int userId, int subjectId) const;

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void showsEmptyStatesAndOnlyCurrentUsersSubjects();
    void marksAllStatusesAndUpdatesAnExistingSubjectDate();
    void editsDeletesAndCancelsDeletion();
    void calculatesInclusivePercentageAndRefreshesOnVisit();
    void preservesSelectionsOnSaveFailure();
    void refreshesAfterStaleRecordAndReportsDatabaseErrors();
};

void AttendanceUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("attendance_ui_tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    m_otherUserId = insertUser(QStringLiteral("Other Attendance User"),
                               QStringLiteral("other-attendance@example.test"));
    QVERIFY(m_otherUserId > 0);
}

void AttendanceUiTests::init()
{
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM attendance")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM timetable")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));

    m_firstSubjectId = insertSubject(m_application->currentUserId(),
                                     QStringLiteral("Biology"));
    m_secondSubjectId = insertSubject(m_application->currentUserId(),
                                      QStringLiteral("Chemistry"));
    m_otherSubjectId = insertSubject(m_otherUserId, QStringLiteral("Private Subject"));
    QVERIFY(m_firstSubjectId > 0);
    QVERIFY(m_secondSubjectId > 0);
    QVERIFY(m_otherSubjectId > 0);

    m_page = new AttendancePage(m_application);
    m_page->show();
    QApplication::processEvents();
    setDateRange(QDate(2026, 10, 1), QDate(2026, 10, 31));
    attendanceDate()->setDate(QDate(2026, 10, 19));
    statusCombo()->setCurrentIndex(0);
}

void AttendanceUiTests::cleanup()
{
    delete m_page;
    m_page = nullptr;
}

void AttendanceUiTests::cleanupTestCase()
{
    delete m_application;
    m_application = nullptr;
}

int AttendanceUiTests::insertUser(const QString &name, const QString &email)
{
    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral(
        "INSERT INTO users (name, email, created_at) "
        "VALUES (:name, :email, :created_at)"));
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":email"), email);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

int AttendanceUiTests::insertSubject(int userId, const QString &name)
{
    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral(
        "INSERT INTO subjects (user_id, name, created_at) "
        "VALUES (:user_id, :name, :created_at)"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

int AttendanceUiTests::insertAttendance(int userId, int subjectId,
                                        const QDate &date,
                                        const QString &status)
{
    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral(
        "INSERT INTO attendance (user_id, subject_id, date, status) "
        "VALUES (:user_id, :subject_id, :date, :status)"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":status"), status);
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

QComboBox *AttendanceUiTests::subjectCombo() const
{
    return m_page->findChild<QComboBox *>(QStringLiteral("attendanceSubjectCombo"));
}

QDateEdit *AttendanceUiTests::attendanceDate() const
{
    return m_page->findChild<QDateEdit *>(QStringLiteral("attendanceDateEdit"));
}

QComboBox *AttendanceUiTests::statusCombo() const
{
    return m_page->findChild<QComboBox *>(QStringLiteral("attendanceStatusCombo"));
}

QPushButton *AttendanceUiTests::saveButton() const
{
    return m_page->findChild<QPushButton *>(QStringLiteral("saveAttendanceButton"));
}

QDateEdit *AttendanceUiTests::rangeStart() const
{
    return m_page->findChild<QDateEdit *>(QStringLiteral("attendanceRangeStart"));
}

QDateEdit *AttendanceUiTests::rangeEnd() const
{
    return m_page->findChild<QDateEdit *>(QStringLiteral("attendanceRangeEnd"));
}

QTableWidget *AttendanceUiTests::historyTable() const
{
    return m_page->findChild<QTableWidget *>(QStringLiteral("attendanceHistoryTable"));
}

void AttendanceUiTests::setDateRange(const QDate &startDate, const QDate &endDate)
{
    rangeStart()->setDate(startDate);
    rangeEnd()->setDate(endDate);
    QApplication::processEvents();
}

void AttendanceUiTests::scheduleConfirmation(QMessageBox::StandardButton answer)
{
    QTimer::singleShot(0, this, [answer] {
        auto *messageBox =
            qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (messageBox) {
            messageBox->button(answer)->click();
        }
    });
}

int AttendanceUiTests::countAttendance(int userId, int subjectId) const
{
    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM attendance WHERE user_id = :user_id "
        "AND subject_id = :subject_id"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    if (!query.exec() || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

void AttendanceUiTests::showsEmptyStatesAndOnlyCurrentUsersSubjects()
{
    QCOMPARE(subjectCombo()->count(), 2);
    QVERIFY(subjectCombo()->findText(QStringLiteral("Private Subject")) < 0);
    QVERIFY(saveButton()->isEnabled());
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendanceEmptyMessage"))->text()
                .contains(QStringLiteral("No attendance has been recorded")));

    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    m_page->hide();
    m_page->show();
    QApplication::processEvents();
    QCOMPARE(subjectCombo()->count(), 0);
    QVERIFY(!saveButton()->isEnabled());
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendanceEmptyMessage"))->text()
                .contains(QStringLiteral("No subjects are available")));
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendanceActionHint"))->text()
                .contains(QStringLiteral("Add a subject")));
}

void AttendanceUiTests::marksAllStatusesAndUpdatesAnExistingSubjectDate()
{
    const int subjectId = subjectCombo()->currentData().toInt();
    attendanceDate()->setDate(QDate(2026, 10, 19));
    QCOMPARE(countAttendance(m_application->currentUserId(), subjectId), 0);
    statusCombo()->setCurrentIndex(
        statusCombo()->findData(static_cast<int>(Attendance::Status::Present)));
    saveButton()->click();
    QCOMPARE(countAttendance(m_application->currentUserId(), subjectId), 1);

    attendanceDate()->setDate(QDate(2026, 10, 19));
    statusCombo()->setCurrentIndex(
        statusCombo()->findData(static_cast<int>(Attendance::Status::Absent)));
    saveButton()->click();
    QCOMPARE(countAttendance(m_application->currentUserId(), subjectId), 1);

    attendanceDate()->setDate(QDate(2026, 10, 19));
    statusCombo()->setCurrentIndex(
        statusCombo()->findData(static_cast<int>(Attendance::Status::Leave)));
    saveButton()->click();
    QCOMPARE(countAttendance(m_application->currentUserId(), subjectId), 1);

    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral(
        "SELECT status FROM attendance WHERE user_id = :user_id "
        "AND subject_id = :subject_id AND date = :date"));
    query.bindValue(QStringLiteral(":user_id"), m_application->currentUserId());
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":date"), QStringLiteral("2026-10-19"));
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Leave"));

    attendanceDate()->setDate(QDate(2026, 10, 20));
    statusCombo()->setCurrentIndex(
        statusCombo()->findData(static_cast<int>(Attendance::Status::Absent)));
    saveButton()->click();
    QCOMPARE(countAttendance(m_application->currentUserId(), subjectId), 2);
    QCOMPARE(historyTable()->rowCount(), 2);
}

void AttendanceUiTests::editsDeletesAndCancelsDeletion()
{
    attendanceDate()->setDate(QDate(2026, 10, 19));
    statusCombo()->setCurrentIndex(
        statusCombo()->findData(static_cast<int>(Attendance::Status::Present)));
    saveButton()->click();
    QCOMPARE(historyTable()->rowCount(), 1);

    auto *editButton = qobject_cast<QPushButton *>(historyTable()->cellWidget(0, 2));
    QVERIFY(editButton);
    editButton->click();
    QCOMPARE(saveButton()->text(), QStringLiteral("Update Attendance"));
    statusCombo()->setCurrentIndex(
        statusCombo()->findData(static_cast<int>(Attendance::Status::Absent)));
    saveButton()->click();

    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral(
        "SELECT status FROM attendance WHERE user_id = :user_id AND subject_id = :subject_id"));
    query.bindValue(QStringLiteral(":user_id"), m_application->currentUserId());
    query.bindValue(QStringLiteral(":subject_id"), m_firstSubjectId);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Absent"));

    auto *deleteButton = qobject_cast<QPushButton *>(historyTable()->cellWidget(0, 3));
    QVERIFY(deleteButton);
    scheduleConfirmation(QMessageBox::No);
    deleteButton->click();
    QCOMPARE(countAttendance(m_application->currentUserId(), m_firstSubjectId), 1);

    deleteButton = qobject_cast<QPushButton *>(historyTable()->cellWidget(0, 3));
    scheduleConfirmation(QMessageBox::Yes);
    deleteButton->click();
    QCOMPARE(countAttendance(m_application->currentUserId(), m_firstSubjectId), 0);
    QVERIFY(historyTable()->isHidden());
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendanceEmptyMessage"))->text()
                .contains(QStringLiteral("No attendance has been recorded")));
}

void AttendanceUiTests::calculatesInclusivePercentageAndRefreshesOnVisit()
{
    const int userId = m_application->currentUserId();
    QVERIFY(insertAttendance(userId, m_firstSubjectId, QDate(2026, 10, 19),
                             QStringLiteral("Present")) > 0);
    QVERIFY(insertAttendance(userId, m_firstSubjectId, QDate(2026, 10, 20),
                             QStringLiteral("Absent")) > 0);
    QVERIFY(insertAttendance(userId, m_firstSubjectId, QDate(2026, 10, 21),
                             QStringLiteral("Present")) > 0);
    setDateRange(QDate(2026, 10, 20), QDate(2026, 10, 21));
    QCOMPARE(historyTable()->rowCount(), 2);
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendancePercentageLabel"))->text()
                .contains(QStringLiteral("50.0%")));

    m_page->hide();
    const int newSubjectId = insertSubject(userId, QStringLiteral("Physics"));
    QVERIFY(newSubjectId > 0);
    QVERIFY(insertAttendance(userId, m_firstSubjectId, QDate(2026, 10, 22),
                             QStringLiteral("Leave")) > 0);
    m_page->show();
    QApplication::processEvents();
    QCOMPARE(subjectCombo()->count(), 3);

    setDateRange(QDate(2026, 10, 19), QDate(2026, 10, 22));
    QCOMPARE(historyTable()->rowCount(), 4);
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendancePercentageLabel"))->text()
                .contains(QStringLiteral("66.7%")));
}

void AttendanceUiTests::preservesSelectionsOnSaveFailure()
{
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TRIGGER reject_attendance_insert BEFORE INSERT ON attendance "
        "BEGIN SELECT RAISE(ABORT, 'forced test failure'); END")));

    const int selectedSubjectId = subjectCombo()->currentData().toInt();
    const QDate selectedDate(2026, 10, 23);
    attendanceDate()->setDate(selectedDate);
    const int absentIndex = statusCombo()->findData(
        static_cast<int>(Attendance::Status::Absent));
    statusCombo()->setCurrentIndex(absentIndex);
    saveButton()->click();

    QCOMPARE(subjectCombo()->currentData().toInt(), selectedSubjectId);
    QCOMPARE(attendanceDate()->date(), selectedDate);
    QCOMPARE(statusCombo()->currentIndex(), absentIndex);
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendanceFeedbackLabel"))->text()
                .contains(QStringLiteral("Could not save attendance")));
    QVERIFY(query.exec(QStringLiteral("DROP TRIGGER reject_attendance_insert")));
}

void AttendanceUiTests::refreshesAfterStaleRecordAndReportsDatabaseErrors()
{
    attendanceDate()->setDate(QDate(2026, 10, 19));
    saveButton()->click();
    QCOMPARE(historyTable()->rowCount(), 1);
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM attendance")));

    auto *deleteButton = qobject_cast<QPushButton *>(historyTable()->cellWidget(0, 3));
    QVERIFY(deleteButton);
    scheduleConfirmation(QMessageBox::Yes);
    deleteButton->click();
    QCOMPARE(historyTable()->rowCount(), 0);
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendanceFeedbackLabel"))->text()
                .contains(QStringLiteral("Attendance record not found")));

    QVERIFY(query.exec(QStringLiteral("DROP TABLE attendance")));
    m_page->hide();
    m_page->show();
    QApplication::processEvents();
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendanceEmptyMessage"))->text()
                .contains(QStringLiteral("Could not load attendance")));
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("attendancePercentageLabel"))->text()
                .contains(QStringLiteral("unavailable")));
}

QTEST_MAIN(AttendanceUiTests)
#include "attendance_ui_tests.moc"
