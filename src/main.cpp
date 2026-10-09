// src/main.cpp
#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>

#include "core/application.h"
#include "ui/main_window.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(QStringLiteral("NEXUS"));
    QCoreApplication::setApplicationName(QStringLiteral("NEXUS"));
    QApplication app(argc, argv);

    Application application;
    if (!application.initialize()) {
        QMessageBox::critical(nullptr, QStringLiteral("Database Error"),
                              QStringLiteral("Failed to initialize NEXUS: %1")
                                  .arg(application.lastError()));
        return -1;
    }

    MainWindow window(&application);
    window.show();
    return app.exec();
}
