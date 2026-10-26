// src/ui/pages/settings_page.cpp
#include "settings_page.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFrame>
#include <QShowEvent>
#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>

#include "../../core/application.h"

SettingsPage::SettingsPage(Application *application, QWidget *parent)
    : QWidget(parent)
    , m_application(application)
    , m_nameEdit(new QLineEdit(this))
    , m_emailEdit(new QLineEdit(this))
    , m_themeCombo(new QComboBox(this))
    , m_saveButton(new QPushButton(tr("Save Settings"), this))
    , m_statusLabel(new QLabel(this))
{
    setObjectName(QStringLiteral("settingsPage"));

    auto *title = new QLabel(tr("Settings"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *description = new QLabel(
        tr("Manage your user profile details and application preferences."),
        this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    m_nameEdit->setObjectName(QStringLiteral("settingsNameEdit"));
    m_emailEdit->setObjectName(QStringLiteral("settingsEmailEdit"));
    
    m_themeCombo->setObjectName(QStringLiteral("settingsThemeCombo"));
    m_themeCombo->addItem(tr("Dark Theme (Default)"), QStringLiteral("dark"));
    m_themeCombo->addItem(tr("Classic Dark"), QStringLiteral("classic"));

    m_saveButton->setObjectName(QStringLiteral("settingsSaveButton"));

    auto *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(16);
    formLayout->addRow(tr("User Name"), m_nameEdit);
    formLayout->addRow(tr("Email Address"), m_emailEdit);
    formLayout->addRow(tr("Color Theme"), m_themeCombo);

    auto *panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("settingsPanel"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(24, 24, 24, 24);
    panelLayout->setSpacing(20);
    panelLayout->addLayout(formLayout);

    auto *actionBar = new QHBoxLayout;
    actionBar->addStretch();
    actionBar->addWidget(m_saveButton);
    panelLayout->addLayout(actionBar);

    m_statusLabel->setObjectName(QStringLiteral("settingsStatus"));
    m_statusLabel->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(20);
    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(panel);
    layout->addWidget(m_statusLabel);
    layout->addStretch(1);

    connect(m_saveButton, &QPushButton::clicked, this, &SettingsPage::saveSettings);

    loadSettings();
}

void SettingsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    loadSettings();
}

void SettingsPage::loadSettings()
{
    m_statusLabel->clear();
    if (!m_application || !m_application->databaseManager()
        || !m_application->databaseManager()->isOpen()
        || m_application->currentUserId() <= 0) {
        m_statusLabel->setText(tr("Settings data is unavailable because no user database is open."));
        return;
    }

    const int userId = m_application->currentUserId();
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral("SELECT name, email FROM users WHERE id = :id"))) {
        m_statusLabel->setText(query.lastError().text());
        return;
    }
    query.bindValue(QStringLiteral(":id"), userId);
    if (query.exec() && query.next()) {
        m_nameEdit->setText(query.value(0).toString());
        m_emailEdit->setText(query.value(1).toString());
    }

    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS"));
    const QString theme = settings.value(QStringLiteral("theme"), QStringLiteral("dark")).toString();
    const int idx = m_themeCombo->findData(theme);
    if (idx >= 0) {
        m_themeCombo->setCurrentIndex(idx);
    }
}

void SettingsPage::saveSettings()
{
    m_statusLabel->clear();
    if (!m_application || !m_application->databaseManager()
        || !m_application->databaseManager()->isOpen()
        || m_application->currentUserId() <= 0) {
        m_statusLabel->setText(tr("Cannot save settings: database is not available."));
        return;
    }

    const QString name = m_nameEdit->text().trimmed();
    const QString email = m_emailEdit->text().trimmed();
    if (name.isEmpty() || email.isEmpty()) {
        m_statusLabel->setText(tr("Name and email address cannot be empty."));
        return;
    }

    const int userId = m_application->currentUserId();
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral("UPDATE users SET name = :name, email = :email WHERE id = :id"))) {
        m_statusLabel->setText(query.lastError().text());
        return;
    }
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":email"), email);
    query.bindValue(QStringLiteral(":id"), userId);

    if (!query.exec()) {
        m_statusLabel->setText(tr("Could not update profile: %1").arg(query.lastError().text()));
        return;
    }

    QSettings settings(QStringLiteral("NEXUS"), QStringLiteral("NEXUS"));
    settings.setValue(QStringLiteral("theme"), m_themeCombo->currentData().toString());

    m_statusLabel->setText(tr("Settings successfully saved."));
}

