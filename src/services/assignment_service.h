#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

#include "../models/assignment.h"

class AssignmentDAO;
class SubjectDAO;

class AssignmentService
{
public:
    enum class DeadlineClassification
    {
        Upcoming,
        Overdue,
        Completed,
        Invalid
    };

    AssignmentService(AssignmentDAO *assignmentDAO, SubjectDAO *subjectDAO);

    bool addAssignment(const Assignment &assignment,
                       QString *errorMessage = nullptr) const;
    QList<Assignment> getAssignments(
        int userId, int subjectId = 0, const QString &status = QString(),
        QString *errorMessage = nullptr) const;
    Assignment getAssignment(int userId, int assignmentId,
                             QString *errorMessage = nullptr) const;
    bool updateAssignment(const Assignment &assignment,
                          QString *errorMessage = nullptr) const;
    bool deleteAssignment(int userId, int assignmentId,
                          QString *errorMessage = nullptr) const;

    static QDateTime parseDeadline(const Assignment &assignment);
    static DeadlineClassification classifyDeadline(
        const Assignment &assignment, const QDateTime &referenceDateTime);

private:
    static bool validateAndNormalize(Assignment &assignment,
                                     QString *errorMessage);
    bool subjectBelongsToUser(int userId, int subjectId,
                              QString *errorMessage) const;

    AssignmentDAO *m_assignmentDAO;
    SubjectDAO *m_subjectDAO;
};
