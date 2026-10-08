// src/ui/pages/settings_page.cpp
#include "settings_page.h"

#include <QVBoxLayout>
#include <QLabel>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);

    QLabel *title = new QLabel(QStringLiteral("Settings"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    QLabel *description = new QLabel(QStringLiteral("Configure your NEXUS preferences."), this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addStretch();
}
