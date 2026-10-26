#include <QtTest/QtTest>

#include <QApplication>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <QSettings>

#include "../src/core/application.h"
#include "../src/ui/pages/settings_page.h"

class SettingsUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    SettingsPage *m_page{nullptr};

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void loadsAndPersistsSettings();
    void preservesExistingUserOnRestart();
};

void SettingsUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    const QString dbPath = m_temporaryDirectory.filePath(QStringLiteral("settings_ui_tests.db"));
    {
        Application setupApp(dbPath);
        QVERIFY(setupApp.initialize());
    }
    m_application = new Application(dbPath);
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    QSqlQuery q(m_application->databaseManager()->database());
    q.exec(QStringLiteral("UPDATE users SET name = 'NEXUS Demo User', email = 'demo@nexus.local' WHERE id = ") + QString::number(m_application->currentUserId()));
}

void SettingsUiTests::init()
{
    m_page = new SettingsPage(m_application);
    m_page->show();
    QApplication::processEvents();
}

void SettingsUiTests::cleanup()
{
    delete m_page;
    m_page = nullptr;
}

void SettingsUiTests::cleanupTestCase()
{
    delete m_application;
    m_application = nullptr;
}

void SettingsUiTests::loadsAndPersistsSettings()
{
    auto *nameEdit = m_page->findChild<QLineEdit *>(QStringLiteral("settingsNameEdit"));
    auto *emailEdit = m_page->findChild<QLineEdit *>(QStringLiteral("settingsEmailEdit"));
    auto *themeCombo = m_page->findChild<QComboBox *>(QStringLiteral("settingsThemeCombo"));
    auto *saveButton = m_page->findChild<QPushButton *>(QStringLiteral("settingsSaveButton"));

    QVERIFY(nameEdit);
    QVERIFY(emailEdit);
    QVERIFY(themeCombo);
    QVERIFY(saveButton);

    QCOMPARE(nameEdit->text(), QStringLiteral("NEXUS Demo User"));

    nameEdit->setText(QStringLiteral("Updated Demo User"));
    emailEdit->setText(QStringLiteral("updated-demo@nexus.local"));
    themeCombo->setCurrentIndex(1);
    saveButton->click();
    QApplication::processEvents();

    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS"));
    QCOMPARE(settings.value(QStringLiteral("theme")).toString(), QStringLiteral("classic"));

    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("SELECT name, email FROM users WHERE email = 'updated-demo@nexus.local'")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("Updated Demo User"));

    // Test saving with the same email (should succeed without unique constraint failure)
    saveButton->click();
    QApplication::processEvents();
    QCOMPARE(m_page->findChild<QLabel *>(QStringLiteral("settingsStatus"))->text(),
             QStringLiteral("Settings successfully saved."));
}

void SettingsUiTests::preservesExistingUserOnRestart()
{
    const QString dbPath = m_temporaryDirectory.filePath(QStringLiteral("restart_test.db"));
    {
        Application app1(dbPath);
        QVERIFY(app1.initialize());
        QSqlQuery q(app1.databaseManager()->database());
        QVERIFY(q.exec(QStringLiteral("INSERT INTO users (id, name, email, created_at) VALUES (99, 'Existing User', 'existing@example.test', '2026-01-01T00:00:00Z')")));
    }

    {
        Application app2(dbPath);
        QVERIFY(app2.initialize());
        QCOMPARE(app2.currentUserId(), 99);
    }
}

QTEST_MAIN(SettingsUiTests)
#include "settings_ui_tests.moc"
