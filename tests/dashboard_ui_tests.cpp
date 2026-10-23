#include <QtTest/QtTest>

#include <QApplication>
#include <QDate>
#include <QDateTime>
#include <QLabel>
#include <QListWidget>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "../src/core/application.h"
#include "../src/ui/pages/dashboard_page.h"

class DashboardUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    DashboardPage *m_page{nullptr};
    int m_userId{0};
    int m_otherUserId{0};
    int m_subjectId{0};
    int m_otherSubjectId{0};

    bool execute(const QString &sql);
    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    bool insertAssignment(int userId, int subjectId, const QString &title,
                          const QDate &dueDate, const QString &status,
                          const QString &dueTime = QString());
    bool insertAttendance(int userId, int subjectId, const QDate &date,
                          const QString &status);
    bool insertTimetable(int userId, int subjectId, int dayOfWeek,
                         const QString &start, const QString &end,
                         const QString &room);
    bool insertExam(int userId, int subjectId, const QString &title,
                    const QDate &date);
    QLabel *metric(const QString &name) const;

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void showsFriendlyEmptyState();
    void displaysCurrentUserMetricsAndSchedule();
    void filtersUpcomingAndOverdueAssignmentsAndExams();
    void calculatesOverallAttendanceExcludingLeaveAndOtherUsers();
    void refreshesWhenPageIsRevisited();
};

void DashboardUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("dashboard-tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    m_userId = m_application->currentUserId();
    m_otherUserId = insertUser(QStringLiteral("Other User"),
                               QStringLiteral("dashboard-other@example.test"));
    QVERIFY(m_otherUserId > 0);
}

void DashboardUiTests::init()
{
    QVERIFY(execute(QStringLiteral("DELETE FROM exams")));
    QVERIFY(execute(QStringLiteral("DELETE FROM attendance")));
    QVERIFY(execute(QStringLiteral("DELETE FROM assignments")));
    QVERIFY(execute(QStringLiteral("DELETE FROM timetable")));
    QVERIFY(execute(QStringLiteral("DELETE FROM subjects")));

    m_subjectId = insertSubject(m_userId, QStringLiteral("Biology"));
    m_otherSubjectId = insertSubject(m_otherUserId, QStringLiteral("Private Physics"));
    QVERIFY(m_subjectId > 0);
    QVERIFY(m_otherSubjectId > 0);

    m_page = new DashboardPage(m_application);
    m_page->show();
    QApplication::processEvents();
}

void DashboardUiTests::cleanup()
{
    delete m_page;
    m_page = nullptr;
}

void DashboardUiTests::cleanupTestCase()
{
    delete m_application;
    m_application = nullptr;
}

bool DashboardUiTests::execute(const QString &sql)
{
    QSqlQuery query(m_application->databaseManager()->database());
    return query.exec(sql);
}

int DashboardUiTests::insertUser(const QString &name, const QString &email)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO users (name, email, created_at) "
            "VALUES (:name, :email, :created_at)"))) {
        return 0;
    }
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":email"), email);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

int DashboardUiTests::insertSubject(int userId, const QString &name)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO subjects (user_id, name, created_at) "
            "VALUES (:user_id, :name, :created_at)"))) {
        return 0;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

bool DashboardUiTests::insertAssignment(int userId, int subjectId,
                                        const QString &title,
                                        const QDate &dueDate,
                                        const QString &status,
                                        const QString &dueTime)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO assignments "
            "(user_id, subject_id, title, description, deadline, priority, status, created_at) "
            "VALUES (:user_id, :subject_id, :title, '', :deadline, 'Medium', :status, :created_at)"))) {
        return false;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":title"), title);
    query.bindValue(QStringLiteral(":deadline"),
                    dueDate.toString(Qt::ISODate)
                        + (dueTime.isEmpty() ? QString() : QStringLiteral("T") + dueTime));
    query.bindValue(QStringLiteral(":status"), status);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec();
}

bool DashboardUiTests::insertAttendance(int userId, int subjectId,
                                        const QDate &date, const QString &status)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO attendance (user_id, subject_id, date, status) "
            "VALUES (:user_id, :subject_id, :date, :status)"))) {
        return false;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":status"), status);
    return query.exec();
}

bool DashboardUiTests::insertTimetable(int userId, int subjectId, int dayOfWeek,
                                       const QString &start, const QString &end,
                                       const QString &room)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO timetable "
            "(user_id, subject_id, day_of_week, start_time, end_time, room) "
            "VALUES (:user_id, :subject_id, :day, :start, :end, :room)"))) {
        return false;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":day"), dayOfWeek);
    query.bindValue(QStringLiteral(":start"), start);
    query.bindValue(QStringLiteral(":end"), end);
    query.bindValue(QStringLiteral(":room"), room);
    return query.exec();
}

bool DashboardUiTests::insertExam(int userId, int subjectId,
                                  const QString &title, const QDate &date)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO exams (user_id, subject_id, title, exam_date) "
            "VALUES (:user_id, :subject_id, :title, :date)"))) {
        return false;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":title"), title);
    query.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));
    return query.exec();
}

QLabel *DashboardUiTests::metric(const QString &name) const
{
    return m_page->findChild<QLabel *>(name);
}

void DashboardUiTests::showsFriendlyEmptyState()
{
    QVERIFY(execute(QStringLiteral("DELETE FROM subjects WHERE id = %1")
                        .arg(m_subjectId)));
    m_page->hide();
    m_page->show();
    QApplication::processEvents();

    QCOMPARE(metric(QStringLiteral("dashboardSubjectCountValue"))->text(),
             QStringLiteral("0"));
    QCOMPARE(metric(QStringLiteral("dashboardPendingAssignmentValue"))->text(),
             QStringLiteral("0"));
    QCOMPARE(metric(QStringLiteral("dashboardOverdueAssignmentValue"))->text(),
             QStringLiteral("0"));
    QCOMPARE(metric(QStringLiteral("dashboardUpcomingAssignmentValue"))->text(),
             QStringLiteral("0"));
    QCOMPARE(metric(QStringLiteral("dashboardUpcomingExamValue"))->text(),
             QStringLiteral("0"));
    QCOMPARE(metric(QStringLiteral("dashboardAttendanceValue"))->text(),
             QStringLiteral("No attendance recorded"));
    QVERIFY(m_page->findChild<QListWidget *>(
                QStringLiteral("dashboardTodaySchedule"))->item(0)->text()
                .contains(QStringLiteral("No classes")));
    QVERIFY(m_page->findChild<QListWidget *>(
                QStringLiteral("dashboardUpcomingItems"))->item(0)->text()
                .contains(QStringLiteral("No assignments or exams")));
    QVERIFY(metric(QStringLiteral("dashboardStatus"))->text().isEmpty());
}

void DashboardUiTests::displaysCurrentUserMetricsAndSchedule()
{
    const QDate today = QDate::currentDate();
    QVERIFY(insertAssignment(m_userId, m_subjectId, QStringLiteral("Due tomorrow"),
                             today.addDays(1), QStringLiteral("Not Started")));
    QVERIFY(insertAssignment(m_otherUserId, m_otherSubjectId,
                             QStringLiteral("Private assignment"),
                             today.addDays(1), QStringLiteral("Not Started")));
    QVERIFY(insertTimetable(m_userId, m_subjectId, today.dayOfWeek(),
                            QStringLiteral("09:00"), QStringLiteral("10:00"),
                            QStringLiteral("Room 1")));
    QVERIFY(insertTimetable(m_userId, m_subjectId,
                            today.dayOfWeek() % 7 + 1,
                            QStringLiteral("11:00"), QStringLiteral("12:00"),
                            QStringLiteral("Other day")));
    QVERIFY(insertTimetable(m_otherUserId, m_otherSubjectId, today.dayOfWeek(),
                            QStringLiteral("10:00"), QStringLiteral("11:00"),
                            QStringLiteral("Private room")));

    m_page->hide();
    m_page->show();
    QApplication::processEvents();

    QCOMPARE(metric(QStringLiteral("dashboardSubjectCountValue"))->text(),
             QStringLiteral("1"));
    QCOMPARE(metric(QStringLiteral("dashboardPendingAssignmentValue"))->text(),
             QStringLiteral("1"));
    QCOMPARE(m_page->findChild<QListWidget *>(
                 QStringLiteral("dashboardTodaySchedule"))->count(), 1);
    const QString scheduleText = m_page->findChild<QListWidget *>(
        QStringLiteral("dashboardTodaySchedule"))->item(0)->text();
    QVERIFY(scheduleText.contains(QStringLiteral("Biology")));
    QVERIFY(scheduleText.contains(QStringLiteral("Room 1")));
    QVERIFY(!scheduleText.contains(QStringLiteral("Private")));
    const QString upcomingText = m_page->findChild<QListWidget *>(
        QStringLiteral("dashboardUpcomingItems"))->item(0)->text();
    QVERIFY(upcomingText.contains(QStringLiteral("Due tomorrow")));
    QVERIFY(!upcomingText.contains(QStringLiteral("Private")));
}

void DashboardUiTests::filtersUpcomingAndOverdueAssignmentsAndExams()
{
    const QDate today = QDate::currentDate();
    QVERIFY(insertAssignment(m_userId, m_subjectId, QStringLiteral("Overdue"),
                             today.addDays(-1), QStringLiteral("In Progress")));
    QVERIFY(insertAssignment(m_userId, m_subjectId, QStringLiteral("Upcoming"),
                             today.addDays(5), QStringLiteral("Not Started")));
    QVERIFY(insertAssignment(m_userId, m_subjectId, QStringLiteral("Outside window"),
                             today.addDays(7), QStringLiteral("Not Started")));
    QVERIFY(insertAssignment(m_userId, m_subjectId, QStringLiteral("Completed"),
                             today.addDays(2), QStringLiteral("Completed")));
    QVERIFY(insertExam(m_userId, m_subjectId, QStringLiteral("Exam soon"),
                       today.addDays(2)));
    QVERIFY(insertExam(m_userId, m_subjectId, QStringLiteral("Exam outside"),
                       today.addDays(7)));
    QVERIFY(insertExam(m_otherUserId, m_otherSubjectId,
                       QStringLiteral("Private exam"), today.addDays(2)));

    m_page->hide();
    m_page->show();
    QApplication::processEvents();

    QCOMPARE(metric(QStringLiteral("dashboardPendingAssignmentValue"))->text(),
             QStringLiteral("3"));
    QCOMPARE(metric(QStringLiteral("dashboardOverdueAssignmentValue"))->text(),
             QStringLiteral("1"));
    QCOMPARE(metric(QStringLiteral("dashboardUpcomingAssignmentValue"))->text(),
             QStringLiteral("1"));
    QCOMPARE(metric(QStringLiteral("dashboardUpcomingExamValue"))->text(),
             QStringLiteral("1"));

    const auto *items = m_page->findChild<QListWidget *>(
        QStringLiteral("dashboardUpcomingItems"));
    QStringList itemTexts;
    for (int i = 0; i < items->count(); ++i) {
        itemTexts.append(items->item(i)->text());
    }
    QVERIFY(itemTexts.join(QLatin1Char('\n')).contains(QStringLiteral("Upcoming")));
    QVERIFY(itemTexts.join(QLatin1Char('\n')).contains(QStringLiteral("Exam soon")));
    QVERIFY(!itemTexts.join(QLatin1Char('\n')).contains(QStringLiteral("Overdue")));
    QVERIFY(!itemTexts.join(QLatin1Char('\n')).contains(
        QStringLiteral("Outside window"), Qt::CaseInsensitive));
    QVERIFY(!itemTexts.join(QLatin1Char('\n')).contains(
        QStringLiteral("Exam outside"), Qt::CaseInsensitive));
    QVERIFY(!itemTexts.join(QLatin1Char('\n')).contains(QStringLiteral("Private")));
}

void DashboardUiTests::calculatesOverallAttendanceExcludingLeaveAndOtherUsers()
{
    const QDate today = QDate::currentDate();
    QVERIFY(insertAttendance(m_userId, m_subjectId, today,
                             QStringLiteral("Present")));
    QVERIFY(insertAttendance(m_userId, m_subjectId, today.addDays(-1),
                             QStringLiteral("Absent")));
    QVERIFY(insertAttendance(m_userId, m_subjectId, today.addDays(-2),
                             QStringLiteral("Leave")));
    QVERIFY(insertAttendance(m_otherUserId, m_otherSubjectId, today,
                             QStringLiteral("Present")));

    m_page->hide();
    m_page->show();
    QApplication::processEvents();
    QCOMPARE(metric(QStringLiteral("dashboardAttendanceValue"))->text(),
             QStringLiteral("50.0%"));
}

void DashboardUiTests::refreshesWhenPageIsRevisited()
{
    QCOMPARE(metric(QStringLiteral("dashboardSubjectCountValue"))->text(),
             QStringLiteral("1"));
    const int addedSubject = insertSubject(m_userId, QStringLiteral("Chemistry"));
    QVERIFY(addedSubject > 0);
    QVERIFY(insertAssignment(m_userId, m_subjectId, QStringLiteral("New assignment"),
                             QDate::currentDate().addDays(3),
                             QStringLiteral("Not Started")));

    m_page->hide();
    m_page->show();
    QApplication::processEvents();
    QCOMPARE(metric(QStringLiteral("dashboardSubjectCountValue"))->text(),
             QStringLiteral("2"));
    QCOMPARE(metric(QStringLiteral("dashboardPendingAssignmentValue"))->text(),
             QStringLiteral("1"));
}

QTEST_MAIN(DashboardUiTests)
#include "dashboard_ui_tests.moc"
