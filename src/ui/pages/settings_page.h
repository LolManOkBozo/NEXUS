// src/ui/pages/settings_page.h
#pragma once

#include <QWidget>

class Application;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(Application *application, QWidget *parent = nullptr);
    ~SettingsPage() override = default;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void loadSettings();
    void saveSettings();

    Application *m_application{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QLineEdit *m_emailEdit{nullptr};
    QComboBox *m_themeCombo{nullptr};
    QPushButton *m_saveButton{nullptr};
    QLabel *m_statusLabel{nullptr};
};

