// src/core/theme_manager.h
#pragma once

#include <QObject>
#include <QString>

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum class Theme
    {
        Dark,
        Light
    };

    static ThemeManager &instance();

    Theme currentTheme() const;
    void setTheme(Theme theme);
    void applyTheme(Theme theme);
    void applyStoredTheme();

    QString themeToString(Theme theme) const;
    Theme stringToTheme(const QString &str) const;

signals:
    void themeChanged(Theme theme);

private:
    ThemeManager() = default;
    ~ThemeManager() override = default;

    Theme m_currentTheme{Theme::Dark};
};
