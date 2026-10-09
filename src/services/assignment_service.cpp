#include "assignment_service.h"

#include <algorithm>
#include <QDate>
#include <QTime>
#include <QTimeZone>

#include "../data/assignment_dao.h"
#include "../data/subject_dao.h"

namespace
{
bool fail(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

bool isValidPriority(const QString &priority)
{
    return priority == QStringLiteral("Low")
           || priority == QStringLiteral("Medium")
           || priority == QStringLiteral("High");
}

bool isValidStatus(const QString &status)
{
    return status == QStringLiteral("Not Started")
           || status == QStringLiteral("In Progress")
           || status == QStringLiteral("Completed");
}
}

AssignmentService::AssignmentService(AssignmentDAO *assignmentDAO,
                                     SubjectDAO *subjectDAO)
    : m_assignmentDAO(assignmentDAO)
    , m_subjectDAO(subjectDAO)
{
}

bool AssignmentService::validateAndNormalize(Assignment &assignment,
                                             QString *errorMessage)
{
    assignment.setTitle(assignment.title().trimmed());
    assignment.setDueDate(assignment.dueDate().trimmed());
    assignment.setDueTime(assignment.dueTime().trimmed());

    if (assignment.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required for this assignment."));
    }
    if (assignment.subjectId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid subject is required for this assignment."));
    }
    if (assignment.title().isEmpty()) {
        return fail(errorMessage, QStringLiteral("Enter an assignment title."));
    }

    const QDate dueDate = QDate::fromString(assignment.dueDate(), Qt::ISODate);
    if (!dueDate.isValid()
        || dueDate.toString(Qt::ISODate) != assignment.dueDate()) {
        return fail(errorMessage,
                    QStringLiteral("The assignment due date must use YYYY-MM-DD format."));
    }
    if (!assignment.dueTime().isEmpty()) {
        const QTime dueTime = QTime::fromString(assignment.dueTime(),
                                                QStringLiteral("HH:mm"));
        if (!dueTime.isValid()
            || dueTime.toString(QStringLiteral("HH:mm")) != assignment.dueTime()) {
            return fail(errorMessage,
                        QStringLiteral("The optional due time must use HH:mm format."));
        }
    }
    if (!isValidPriority(assignment.priority())) {
        return fail(errorMessage,
                    QStringLiteral("Priority must be Low, Medium, or High."));
    }
    if (!isValidStatus(assignment.status())) {
        return fail(errorMessage,
                    QStringLiteral("Status must be Not Started, In Progress, or Completed."));
    }
    return true;
}

bool AssignmentService::subjectBelongsToUser(int userId, int subjectId,
                                             QString *errorMessage) const
{
    if (!m_subjectDAO) {
        return fail(errorMessage, QStringLiteral("The subject data service is unavailable."));
    }
    QString subjectError;
    const Subject subject = m_subjectDAO->getSubjectById(userId, subjectId, &subjectError);
    if (subject.id() <= 0) {
        if (subjectError.isEmpty()) {
            subjectError = QStringLiteral("The subject does not belong to the specified user.");
        }
        return fail(errorMessage, subjectError);
    }
    return true;
}

bool AssignmentService::addAssignment(const Assignment &assignment,
                                      QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Assignment normalizedAssignment = assignment;
    if (!validateAndNormalize(normalizedAssignment, errorMessage)) {
        return false;
    }
    if (!m_assignmentDAO) {
        return fail(errorMessage, QStringLiteral("The assignment data service is unavailable."));
    }
    if (!subjectBelongsToUser(normalizedAssignment.userId(),
                              normalizedAssignment.subjectId(), errorMessage)) {
        return false;
    }
    if (normalizedAssignment.createdAt().isEmpty()) {
        normalizedAssignment.setCreatedAt(
            QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }
    return m_assignmentDAO->insertAssignment(normalizedAssignment, errorMessage);
}

QList<Assignment> AssignmentService::getAssignments(
    int userId, int subjectId, const QString &status, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        fail(errorMessage, QStringLiteral("A valid user is required to load assignments."));
        return {};
    }
    if (subjectId < 0) {
        fail(errorMessage, QStringLiteral("A valid subject filter is required."));
        return {};
    }
    if (!status.isEmpty() && !isValidStatus(status)) {
        fail(errorMessage,
             QStringLiteral("Status filter must be Not Started, In Progress, or Completed."));
        return {};
    }
    if (!m_assignmentDAO) {
        fail(errorMessage, QStringLiteral("The assignment data service is unavailable."));
        return {};
    }
    if (subjectId > 0
        && !subjectBelongsToUser(userId, subjectId, errorMessage)) {
        return {};
    }

    QList<Assignment> assignments =
        m_assignmentDAO->getAssignmentsByUserId(userId, subjectId, status, errorMessage);
    if (errorMessage && !errorMessage->isEmpty()) {
        return {};
    }
    std::stable_sort(assignments.begin(), assignments.end(),
                     [](const Assignment &left, const Assignment &right) {
                         const QDateTime leftDeadline = parseDeadline(left);
                         const QDateTime rightDeadline = parseDeadline(right);
                         if (leftDeadline.isValid() != rightDeadline.isValid()) {
                             return leftDeadline.isValid();
                         }
                         if (leftDeadline.isValid() && leftDeadline != rightDeadline) {
                             return leftDeadline < rightDeadline;
                         }
                         return left.id() < right.id();
                     });
    return assignments;
}

Assignment AssignmentService::getAssignment(int userId, int assignmentId,
                                            QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || assignmentId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user and assignment are required.");
        }
        return {};
    }
    if (!m_assignmentDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The assignment data service is unavailable.");
        }
        return {};
    }
    return m_assignmentDAO->getAssignmentById(userId, assignmentId, errorMessage);
}

bool AssignmentService::updateAssignment(const Assignment &assignment,
                                         QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Assignment normalizedAssignment = assignment;
    if (normalizedAssignment.id() <= 0) {
        return fail(errorMessage,
                    QStringLiteral("A valid assignment is required to update it."));
    }
    if (!validateAndNormalize(normalizedAssignment, errorMessage)) {
        return false;
    }
    if (!m_assignmentDAO) {
        return fail(errorMessage, QStringLiteral("The assignment data service is unavailable."));
    }
    if (!subjectBelongsToUser(normalizedAssignment.userId(),
                              normalizedAssignment.subjectId(), errorMessage)) {
        return false;
    }
    return m_assignmentDAO->updateAssignment(normalizedAssignment, errorMessage);
}

bool AssignmentService::deleteAssignment(int userId, int assignmentId,
                                         QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || assignmentId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and assignment are required."));
    }
    if (!m_assignmentDAO) {
        return fail(errorMessage, QStringLiteral("The assignment data service is unavailable."));
    }
    return m_assignmentDAO->deleteAssignment(userId, assignmentId, errorMessage);
}

QDateTime AssignmentService::parseDeadline(const Assignment &assignment)
{
    const QString dateText = assignment.dueDate();
    const QDate date = QDate::fromString(dateText, Qt::ISODate);
    if (!date.isValid() || date.toString(Qt::ISODate) != dateText) {
        return {};
    }

    if (assignment.dueTime().isEmpty()) {
        return QDateTime(date, QTime(23, 59, 59, 999), QTimeZone::systemTimeZone());
    }

    const QString timeText = assignment.dueTime();
    const QTime time = QTime::fromString(timeText, QStringLiteral("HH:mm"));
    if (!time.isValid() || time.toString(QStringLiteral("HH:mm")) != timeText) {
        return {};
    }
    return QDateTime(date, time, QTimeZone::systemTimeZone());
}

AssignmentService::DeadlineClassification AssignmentService::classifyDeadline(
    const Assignment &assignment, const QDateTime &referenceDateTime)
{
    if (assignment.status() == QStringLiteral("Completed")) {
        return DeadlineClassification::Completed;
    }
    const QDateTime deadline = parseDeadline(assignment);
    if (!deadline.isValid() || !referenceDateTime.isValid()) {
        return DeadlineClassification::Invalid;
    }
    return deadline < referenceDateTime ? DeadlineClassification::Overdue
                                        : DeadlineClassification::Upcoming;
}
