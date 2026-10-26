#include <QtTest/QtTest>

#include <QApplication>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <QDateTime>

#include "../src/core/application.h"
#include "../src/ui/pages/analytics_page.h"

class AnalyticsUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    AnalyticsPage *m_page{nullptr};

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void displaysAnalyticsMetricsAndBreakdown();
};

void AnalyticsUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("analytics_ui_tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
}

void AnalyticsUiTests::init()
{
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM exams")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM attendance")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM assignments")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));

    m_page = new AnalyticsPage(m_application);
    m_page->show();
    QApplication::processEvents();
}

void AnalyticsUiTests::cleanup()
{
    delete m_page;
    m_page = nullptr;
}

void AnalyticsUiTests::cleanupTestCase()
{
    delete m_application;
    m_application = nullptr;
}

void AnalyticsUiTests::displaysAnalyticsMetricsAndBreakdown()
{
    auto *subjectCount = m_page->findChild<QLabel *>(QStringLiteral("analyticsCardValue"));
    QVERIFY(subjectCount);
    auto *breakdownList = m_page->findChild<QListWidget *>(QStringLiteral("analyticsSubjectList"));
    QVERIFY(breakdownList);
    QVERIFY(breakdownList->item(0)->text().contains(QStringLiteral("No subjects registered yet")));
}

QTEST_MAIN(AnalyticsUiTests)
#include "analytics_ui_tests.moc"
