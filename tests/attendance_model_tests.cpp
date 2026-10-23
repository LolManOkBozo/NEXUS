#include <QtTest/QtTest>

#include <QDate>

#include "../src/models/attendance.h"

class AttendanceModelTests : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultConstruction();
    void testParameterizedConstruction();
    void testGettersAndSetters();
    void testIsValid();
    void testDateConversion();
    void testRejectInvalidStatusAndIdentifiers();
};

void AttendanceModelTests::testDefaultConstruction()
{
    Attendance a;
    QCOMPARE(a.id(), 0);
    QCOMPARE(a.userId(), 0);
    QCOMPARE(a.subjectId(), 0);
    QVERIFY(!a.date().isValid());
    QCOMPARE(a.status(), Attendance::Status::Present);
}

void AttendanceModelTests::testParameterizedConstruction()
{
    QDate date(2026, 10, 19);
    Attendance a(1, 2, 3, date, Attendance::Status::Absent);
    QCOMPARE(a.id(), 1);
    QCOMPARE(a.userId(), 2);
    QCOMPARE(a.subjectId(), 3);
    QCOMPARE(a.date(), date);
    QCOMPARE(a.status(), Attendance::Status::Absent);
}

void AttendanceModelTests::testGettersAndSetters()
{
    Attendance a;
    a.setId(10);
    a.setUserId(20);
    a.setSubjectId(30);
    QDate date(2026, 1, 1);
    a.setDate(date);
    a.setStatus(Attendance::Status::Leave);

    QCOMPARE(a.id(), 10);
    QCOMPARE(a.userId(), 20);
    QCOMPARE(a.subjectId(), 30);
    QCOMPARE(a.date(), date);
    QCOMPARE(a.status(), Attendance::Status::Leave);
}

void AttendanceModelTests::testIsValid()
{
    Attendance a;
    QVERIFY(!a.isValid());

    QDate validDate(2026, 10, 19);
    a.setUserId(1);
    a.setSubjectId(2);
    a.setDate(validDate);
    QVERIFY(a.isValid());

    a.setDate(QDate());
    QVERIFY(!a.isValid());
}

void AttendanceModelTests::testDateConversion()
{
    const QDate original(2026, 10, 19);
    const QString str = Attendance::dateToString(original);
    QCOMPARE(str, QStringLiteral("2026-10-19"));

    const QDate parsed = Attendance::stringToDate(str);
    QCOMPARE(parsed, original);

    const QDate invalid = Attendance::stringToDate(QStringLiteral("not-a-date"));
    QVERIFY(!invalid.isValid());

    const QDate empty = Attendance::stringToDate(QString());
    QVERIFY(!empty.isValid());
    QVERIFY(Attendance::dateToString(QDate()).isEmpty());
    QVERIFY(!Attendance::stringToDate(QStringLiteral("2026-2-3")).isValid());
}

void AttendanceModelTests::testRejectInvalidStatusAndIdentifiers()
{
    const QDate date(2026, 10, 19);
    Attendance attendance(0, 1, 2, date, Attendance::Status::Present);
    QVERIFY(attendance.isValid());

    attendance.setStatus(static_cast<Attendance::Status>(-1));
    QVERIFY(!attendance.isValid());
    attendance.setStatus(Attendance::Status::Absent);
    QVERIFY(attendance.isValid());
    attendance.setStatus(Attendance::Status::Leave);
    QVERIFY(attendance.isValid());

    attendance.setUserId(0);
    QVERIFY(!attendance.isValid());
    attendance.setUserId(1);
    attendance.setSubjectId(0);
    QVERIFY(!attendance.isValid());
    attendance.setSubjectId(2);
    attendance.setId(-1);
    QVERIFY(!attendance.isValid());
}

QTEST_MAIN(AttendanceModelTests)
#include "tests/attendance_model_tests.moc"
