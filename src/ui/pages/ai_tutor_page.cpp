// src/ui/pages/ai_tutor_page.cpp
#include "ai_tutor_page.h"

#include <QVBoxLayout>
#include <QLabel>

AITutorPage::AITutorPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);

    QLabel *title = new QLabel(QStringLiteral("AI Tutor"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    QLabel *description = new QLabel(QStringLiteral("Your future AI-powered study assistant."), this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addStretch();
}
