#include "exam_service.h"

#include <QDate>
#include <QTime>

#include "../data/exam_dao.h"
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

ExamService::ExamService(ExamDAO *examDAO, SubjectDAO *subjectDAO)
    : m_examDAO(examDAO)
    , m_subjectDAO(subjectDAO)
{
}

bool ExamService::validateAndNormalize(Exam &exam, QString *errorMessage)
{
    exam.setTitle(exam.title().trimmed());
    exam.setExamDate(exam.examDate().trimmed());
    exam.setStartTime(exam.startTime().trimmed());
    exam.setRoom(exam.room().trimmed());

    if (exam.userId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user is required for this exam."));
    }
    if (exam.subjectId() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid subject is required for this exam."));
    }
    if (exam.title().isEmpty()) {
        return fail(errorMessage, QStringLiteral("Enter an exam title."));
    }
    const QDate date = QDate::fromString(exam.examDate(), Qt::ISODate);
    if (!date.isValid() || date.toString(Qt::ISODate) != exam.examDate()) {
        return fail(errorMessage,
                    QStringLiteral("The exam date must use YYYY-MM-DD format."));
    }
    if (!exam.startTime().isEmpty()) {
        const QTime time = QTime::fromString(exam.startTime(),
                                             QStringLiteral("HH:mm"));
        if (!time.isValid()
            || time.toString(QStringLiteral("HH:mm")) != exam.startTime()) {
            return fail(errorMessage,
                        QStringLiteral("The optional exam time must use HH:mm format."));
        }
    }
    return true;
}

bool ExamService::subjectBelongsToUser(int userId, int subjectId,
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

bool ExamService::addExam(const Exam &exam, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Exam normalized = exam;
    if (!validateAndNormalize(normalized, errorMessage)) {
        return false;
    }
    if (!m_examDAO) {
        return fail(errorMessage, QStringLiteral("The exam data service is unavailable."));
    }
    if (!subjectBelongsToUser(normalized.userId(), normalized.subjectId(),
                              errorMessage)) {
        return false;
    }
    return m_examDAO->insertExam(normalized, errorMessage);
}

QList<Exam> ExamService::getExams(int userId, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0) {
        fail(errorMessage, QStringLiteral("A valid user is required to load exams."));
        return {};
    }
    if (!m_examDAO) {
        fail(errorMessage, QStringLiteral("The exam data service is unavailable."));
        return {};
    }
    return m_examDAO->getExamsByUserId(userId, errorMessage);
}

Exam ExamService::getExam(int userId, int examId,
                          QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || examId <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("A valid user and exam are required.");
        }
        return {};
    }
    if (!m_examDAO) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The exam data service is unavailable.");
        }
        return {};
    }
    return m_examDAO->getExamById(userId, examId, errorMessage);
}

bool ExamService::updateExam(const Exam &exam, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    Exam normalized = exam;
    if (normalized.id() <= 0) {
        return fail(errorMessage, QStringLiteral("A valid exam is required to update it."));
    }
    if (!validateAndNormalize(normalized, errorMessage)) {
        return false;
    }
    if (!m_examDAO) {
        return fail(errorMessage, QStringLiteral("The exam data service is unavailable."));
    }
    if (!subjectBelongsToUser(normalized.userId(), normalized.subjectId(),
                              errorMessage)) {
        return false;
    }
    return m_examDAO->updateExam(normalized, errorMessage);
}

bool ExamService::deleteExam(int userId, int examId,
                             QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (userId <= 0 || examId <= 0) {
        return fail(errorMessage, QStringLiteral("A valid user and exam are required."));
    }
    if (!m_examDAO) {
        return fail(errorMessage, QStringLiteral("The exam data service is unavailable."));
    }
    return m_examDAO->deleteExam(userId, examId, errorMessage);
}
