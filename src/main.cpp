// src/main.cpp
#include <QApplication>
#include <QMessageBox>
#include "core/database_manager.h"
#include "ui/main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Initialize the database manager
    DatabaseManager dbManager;
    if (!dbManager.initialize()) {
        QMessageBox::critical(nullptr, QStringLiteral("Database Error"),
                              QStringLiteral("Failed to initialize the database: %1")
                                      .arg(dbManager.lastError()));
        return -1;
    }

    MainWindow window;
    window.show();
    return app.exec();
}
