#pragma once

#include <QList>
#include <QString>

#include "../models/assignment.h"

class DatabaseManager;

class AssignmentDAO
{
public:
    explicit AssignmentDAO(DatabaseManager *databaseManager);

    bool insertAssignment(const Assignment &assignment,
                          QString *errorMessage = nullptr) const;
    QList<Assignment> getAssignmentsByUserId(
        int userId, int subjectId = 0, const QString &status = QString(),
        QString *errorMessage = nullptr) const;
    Assignment getAssignmentById(int userId, int assignmentId,
                                 QString *errorMessage = nullptr) const;
    bool updateAssignment(const Assignment &assignment,
                          QString *errorMessage = nullptr) const;
    bool deleteAssignment(int userId, int assignmentId,
                          QString *errorMessage = nullptr) const;

private:
    DatabaseManager *m_databaseManager;
};
