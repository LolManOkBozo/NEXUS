// src/core/theme_manager.cpp
#include "theme_manager.h"

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QPalette>
#include <QColor>

ThemeManager &ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::Theme themeManagerCurrentTheme()
{
    return ThemeManager::instance().currentTheme();
}

ThemeManager::Theme ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

QString ThemeManager::themeToString(Theme theme) const
{
    switch (theme) {
    case Theme::Light:
        return QStringLiteral("light");
    case Theme::Dark:
    default:
        return QStringLiteral("dark");
    }
}

ThemeManager::Theme ThemeManager::stringToTheme(const QString &str) const
{
    if (str.toLower() == QStringLiteral("light")) {
        return Theme::Light;
    }
    return Theme::Dark;
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_currentTheme == theme) {
        return;
    }
    m_currentTheme = theme;
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS"));
    settings.setValue(QStringLiteral("theme"), themeToString(theme));

    applyTheme(theme);
    emit themeChanged(theme);
}

void ThemeManager::applyStoredTheme()
{
    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS"));
    QString themeStr = settings.value(QStringLiteral("theme"), QStringLiteral("dark")).toString();
    // Map legacy 'classic' to dark
    if (themeStr == QStringLiteral("classic")) {
        themeStr = QStringLiteral("dark");
    }
    Theme theme = stringToTheme(themeStr);
    m_currentTheme = theme;
    applyTheme(theme);
}

void ThemeManager::applyTheme(Theme theme)
{
    QString qssPath;
    QPalette palette;

    if (theme == Theme::Light) {
        qssPath = QStringLiteral(":/styles/theme_light.qss");
        palette.setColor(QPalette::Window, QColor("#F5F0E6"));
        palette.setColor(QPalette::WindowText, QColor("#29251F"));
        palette.setColor(QPalette::Base, QColor("#FFFCF5"));
        palette.setColor(QPalette::AlternateBase, QColor("#E7DED0"));
        palette.setColor(QPalette::Text, QColor("#29251F"));
        palette.setColor(QPalette::Button, QColor("#FFFCF5"));
        palette.setColor(QPalette::ButtonText, QColor("#29251F"));
        palette.setColor(QPalette::Highlight, QColor("#B94E25"));
        palette.setColor(QPalette::HighlightedText, QColor("#FFFCF5"));
    } else {
        qssPath = QStringLiteral(":/styles/theme.qss");
        palette.setColor(QPalette::Window, QColor("#252622"));
        palette.setColor(QPalette::WindowText, QColor("#F1EADF"));
        palette.setColor(QPalette::Base, QColor("#2D2E29"));
        palette.setColor(QPalette::AlternateBase, QColor("#363732"));
        palette.setColor(QPalette::Text, QColor("#F1EADF"));
        palette.setColor(QPalette::Button, QColor("#2D2E29"));
        palette.setColor(QPalette::ButtonText, QColor("#F1EADF"));
        palette.setColor(QPalette::Highlight, QColor("#FF985F"));
        palette.setColor(QPalette::HighlightedText, QColor("#252622"));
    }

    qApp->setPalette(palette);

    QFile styleSheet(qssPath);
    if (styleSheet.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleSheet.readAll());
        qApp->setStyleSheet(style);
        styleSheet.close();
    }
}
