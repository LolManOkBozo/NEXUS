#include <QtTest/QtTest>

#include "../src/models/exam.h"

class ExamModelTests : public QObject
{
    Q_OBJECT

private slots:
    void defaultExamHasNoIdentity();
    void storesExamFields();
};

void ExamModelTests::defaultExamHasNoIdentity()
{
    const Exam exam;
    QCOMPARE(exam.id(), 0);
    QCOMPARE(exam.userId(), 0);
    QCOMPARE(exam.subjectId(), 0);
    QVERIFY(exam.title().isEmpty());
    QVERIFY(exam.examDate().isEmpty());
    QVERIFY(exam.startTime().isEmpty());
    QVERIFY(exam.room().isEmpty());
}

void ExamModelTests::storesExamFields()
{
    Exam exam(7, 11, 13, QStringLiteral("Final Exam"),
              QStringLiteral("2026-12-01"), QStringLiteral("09:30"),
              QStringLiteral("Room 4"));
    QCOMPARE(exam.id(), 7);
    QCOMPARE(exam.userId(), 11);
    QCOMPARE(exam.subjectId(), 13);
    QCOMPARE(exam.title(), QStringLiteral("Final Exam"));
    QCOMPARE(exam.examDate(), QStringLiteral("2026-12-01"));
    QCOMPARE(exam.startTime(), QStringLiteral("09:30"));
    QCOMPARE(exam.room(), QStringLiteral("Room 4"));

    exam.setTitle(QStringLiteral("Updated Exam"));
    exam.setStartTime(QString());
    QCOMPARE(exam.title(), QStringLiteral("Updated Exam"));
    QVERIFY(exam.startTime().isEmpty());
}

QTEST_GUILESS_MAIN(ExamModelTests)
#include "exam_model_tests.moc"
