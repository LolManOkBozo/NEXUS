// src/ui/pages/notes_page.cpp
#include "notes_page.h"

#include <QVBoxLayout>
#include <QLabel>

NotesPage::NotesPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);

    QLabel *title = new QLabel(QStringLiteral("Notes"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    QLabel *description = new QLabel(QStringLiteral("Organize your academic notes."), this);
    description->setObjectName(QStringLiteral("pageDescription"));
    description->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addStretch();
}
