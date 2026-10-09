#include "subject_service.h"

#include <QDateTime>

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
}

SubjectService::SubjectService(SubjectDAO *subjectDAO)
    : m_subjectDAO(subjectDAO)
{
}

bool SubjectService::validateAndNormalize(Subject &subject, QString *errorMessage)
{
    subject.setName(subject.name().trimmed());
    subject.setCode(subject.code().trimmed());
    subject.setTeacher(subject.teacher().trimmed());

    if (subject.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required for this subject."));
    }
    if (subject.name().isEmpty()) {
        return fail(errorMessage, QStringLiteral("Enter a subject name."));
    }
    if (subject.code().isEmpty()) {
        return fail(errorMessage, QStringLiteral("Enter a subject code."));
    }
    if (subject.credits() < 1 || subject.credits() > 30) {
        return fail(errorMessage, QStringLiteral("Credits must be between 1 and 30."));
    }
    if (subject.semester() < 1 || subject.semester() > 12) {
        return fail(errorMessage, QStringLiteral("Semester must be between 1 and 12."));
    }
    return true;
}

bool SubjectService::addSubject(const Subject &subject, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Subject normalizedSubject = subject;
    if (!validateAndNormalize(normalizedSubject, errorMessage)) {
        return false;
    }
    if (!m_subjectDAO) {
        return fail(errorMessage, QStringLiteral("The subject data service is unavailable."));
    }
    if (normalizedSubject.createdAt().isEmpty()) {
        normalizedSubject.setCreatedAt(
            QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }
    return m_subjectDAO->insertSubject(normalizedSubject, errorMessage);
}

QList<Subject> SubjectService::getSubjects(int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user is required to load subjects.");
        }
        return {};
    }
    if (!m_subjectDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The subject data service is unavailable.");
        }
        return {};
    }
    return m_subjectDAO->getSubjectsByUserId(userId, errorMessage);
}

Subject SubjectService::getSubject(int userId, int subjectId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || subjectId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user and subject are required.");
        }
        return {};
    }
    if (!m_subjectDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The subject data service is unavailable.");
        }
        return {};
    }
    return m_subjectDAO->getSubjectById(userId, subjectId, errorMessage);
}

bool SubjectService::updateSubject(const Subject &subject, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Subject normalizedSubject = subject;
    if (normalizedSubject.id() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid subject is required to update it."));
    }
    if (!validateAndNormalize(normalizedSubject, errorMessage)) {
        return false;
    }
    if (!m_subjectDAO) {
        return fail(errorMessage, QStringLiteral("The subject data service is unavailable."));
    }
    return m_subjectDAO->updateSubject(normalizedSubject, errorMessage);
}

bool SubjectService::deleteSubject(int userId, int subjectId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || subjectId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and subject are required."));
    }
    if (!m_subjectDAO) {
        return fail(errorMessage, QStringLiteral("The subject data service is unavailable."));
    }
    return m_subjectDAO->deleteSubject(userId, subjectId, errorMessage);
}

QList<Subject> SubjectService::searchSubjects(int userId, const QString &searchTerm,
                                               QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user is required to search subjects.");
        }
        return {};
    }
    if (!m_subjectDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The subject data service is unavailable.");
        }
        return {};
    }
    const QString normalizedTerm = searchTerm.trimmed();
    if (normalizedTerm.isEmpty()) {
        return m_subjectDAO->getSubjectsByUserId(userId, errorMessage);
    }
    return m_subjectDAO->searchSubjects(userId, normalizedTerm, errorMessage);
}
