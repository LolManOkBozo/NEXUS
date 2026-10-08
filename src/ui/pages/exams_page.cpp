// src/ui/pages/exams_page.cpp
#include "exams_page.h"

#include <QVBoxLayout>
#include <QLabel>

ExamsPage::ExamsPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);

    QLabel *title = new QLabel(QStringLiteral("Exams"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    QLabel *description = new QLabel(QStringLiteral("Keep track of upcoming examinations."), this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addStretch();
}
