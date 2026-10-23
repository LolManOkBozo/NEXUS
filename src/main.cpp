// src/main.cpp
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "core/application.h"
#include "ui/dialogs/attendance_duplicate_recovery_dialog.h"
#include "ui/main_window.h"

namespace
{
QString defaultDatabasePath()
{
    const QString appDataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appDataPath.isEmpty()
        ? QString()
        : QDir(appDataPath).filePath(QStringLiteral("nexus.db"));
}

int runAttendanceRecovery()
{
    AttendanceDuplicateRecoveryDialog dialog(defaultDatabasePath());
    if (dialog.exec() != QDialog::Accepted) {
        return 1;
    }
    QMessageBox::information(
        nullptr, QStringLiteral("Attendance Recovery Complete"),
        QStringLiteral("The duplicate records were resolved and the uniqueness migration "
                       "was verified.\n\nBackup: %1\n\nRestart NEXUS to continue.")
            .arg(dialog.backupPath()));
    return 0;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(QStringLiteral("NEXUS"));
    QCoreApplication::setApplicationName(QStringLiteral("NEXUS"));
    QApplication app(argc, argv);

    if (app.arguments().contains(QStringLiteral("--recover-attendance-duplicates"))) {
        return runAttendanceRecovery();
    }

    Application application;
    if (!application.initialize()) {
        if (application.lastError().contains(
                QStringLiteral("Attendance uniqueness migration was aborted"))) {
            const auto answer = QMessageBox::question(
                nullptr, QStringLiteral("Attendance Data Needs Review"),
                QStringLiteral(
                    "NEXUS found duplicate attendance records and did not modify them.\n\n"
                    "Would you like to review the records and open the safe recovery workflow?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer == QMessageBox::Yes) {
                return runAttendanceRecovery();
            }
            QMessageBox::critical(
                nullptr, QStringLiteral("Database Error"),
                QStringLiteral("Failed to initialize NEXUS: %1\n\n"
                               "You can reopen the recovery workflow with "
                               "--recover-attendance-duplicates or use the documented "
                               "manual recovery procedure.")
                    .arg(application.lastError()));
            return -1;
        }
        QMessageBox::critical(nullptr, QStringLiteral("Database Error"),
                              QStringLiteral("Failed to initialize NEXUS: %1")
                                  .arg(application.lastError()));
        return -1;
    }

    MainWindow window(&application);
    window.show();
    return app.exec();
}
