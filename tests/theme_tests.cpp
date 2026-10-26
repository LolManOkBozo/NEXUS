#include <QtTest/QtTest>

#include <QApplication>
#include <QComboBox>
#include <QTemporaryDir>
#include <QSettings>

#include "../src/core/application.h"
#include "../src/core/theme_manager.h"
#include "../src/ui/pages/settings_page.h"

class ThemeTests : public QObject
{
    Q_OBJECT

private slots:
    void testThemeManagerSwitchingAndPersistence();
};

void ThemeTests::testThemeManagerSwitchingAndPersistence()
{
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS"));
    settings.setValue(QStringLiteral("theme"), QStringLiteral("dark"));

    ThemeManager::instance().applyStoredTheme();
    QCOMPARE(ThemeManager::instance().currentTheme(), ThemeManager::Theme::Dark);

    ThemeManager::instance().setTheme(ThemeManager::Theme::Light);
    QCOMPARE(ThemeManager::instance().currentTheme(), ThemeManager::Theme::Light);
    QCOMPARE(settings.value(QStringLiteral("theme")).toString(), QStringLiteral("light"));

    ThemeManager::instance().applyStoredTheme();
    QCOMPARE(ThemeManager::instance().currentTheme(), ThemeManager::Theme::Light);

    // Reset back to dark
    ThemeManager::instance().setTheme(ThemeManager::Theme::Dark);
}

QTEST_MAIN(ThemeTests)
#include "theme_tests.moc"
