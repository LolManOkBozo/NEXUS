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
};

void SettingsUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("settings_ui_tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
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
}

QTEST_MAIN(SettingsUiTests)
#include "settings_ui_tests.moc"
