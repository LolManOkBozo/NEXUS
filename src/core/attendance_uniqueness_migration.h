#pragma once

#include <QSqlDatabase>
#include <QString>

bool ensureAttendanceUniquenessMigration(const QSqlDatabase &database,
                                        QString *errorMessage);
