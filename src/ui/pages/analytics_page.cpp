// src/ui/pages/analytics_page.cpp
#include "analytics_page.h"

#include <QVBoxLayout>
#include <QLabel>

AnalyticsPage::AnalyticsPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);

    QLabel *title = new QLabel(QStringLiteral("Analytics"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    QLabel *description = new QLabel(QStringLiteral("Understand your academic and study performance."), this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addStretch();
}
