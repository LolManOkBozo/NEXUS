// src/ui/pages/dashboard_page.cpp
#include "dashboard_page.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QGridLayout>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(24);

    // Welcome header
    QLabel *welcomeTitle = new QLabel(QStringLiteral("Welcome to NEXUS"), this);
    welcomeTitle->setObjectName(QStringLiteral("welcomeTitle"));
    QLabel *welcomeSubtitle = new QLabel(QStringLiteral("Your academic command center."), this);
    welcomeSubtitle->setObjectName(QStringLiteral("welcomeSubtitle"));

    QVBoxLayout *welcomeLayout = new QVBoxLayout;
    welcomeLayout->addWidget(welcomeTitle);
    welcomeLayout->addWidget(welcomeSubtitle);
    welcomeLayout->setSpacing(4);
    mainLayout->addLayout(welcomeLayout);

    // Overview cards
    QFrame *cardsFrame = new QFrame(this);
    cardsFrame->setObjectName(QStringLiteral("cardsFrame"));
    QGridLayout *cardsLayout = new QGridLayout(cardsFrame);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(16);

    // Helper function to create a card
    auto createCard = [](const QString &title, const QString &value, QWidget *parent) -> QFrame* {
        QFrame *card = new QFrame(parent);
        card->setObjectName(QStringLiteral("card"));
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(4);

        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setObjectName(QStringLiteral("cardTitle"));
        QLabel *valueLabel = new QLabel(value, card);
        valueLabel->setObjectName(QStringLiteral("cardValue"));
        valueLabel->setStyleSheet(QStringLiteral("font-size: 24pt; font-weight: bold;"));

        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(valueLabel);
        return card;
    };

    // Add cards
    cardsLayout->addWidget(createCard(QStringLiteral("Today's Classes"), QStringLiteral("3"), this), 0, 0);
    cardsLayout->addWidget(createCard(QStringLiteral("Pending Assignments"), QStringLiteral("5"), this), 0, 1);
    cardsLayout->addWidget(createCard(QStringLiteral("Attendance"), QStringLiteral("95%"), this), 0, 2);
    cardsLayout->addWidget(createCard(QStringLiteral("Upcoming Exams"), QStringLiteral("2"), this), 0, 3);

    mainLayout->addWidget(cardsFrame);

    // Schedule section
    QFrame *scheduleFrame = new QFrame(this);
    scheduleFrame->setObjectName(QStringLiteral("scheduleFrame"));
    QVBoxLayout *scheduleLayout = new QVBoxLayout(scheduleFrame);
    scheduleLayout->setContentsMargins(24, 24, 24, 24);
    scheduleLayout->setSpacing(8);

    QLabel *scheduleTitle = new QLabel(QStringLiteral("Today's Schedule"), scheduleFrame);
    scheduleTitle->setObjectName(QStringLiteral("scheduleTitle"));
    QLabel *scheduleContent = new QLabel(QStringLiteral("No classes scheduled yet."), scheduleFrame);
    scheduleContent->setObjectName(QStringLiteral("scheduleContent"));
    scheduleContent->setWordWrap(true);

    scheduleLayout->addWidget(scheduleTitle);
    scheduleLayout->addWidget(scheduleContent);

    mainLayout->addWidget(scheduleFrame);
    mainLayout->addStretch();
}
