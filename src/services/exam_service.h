#pragma once

#include <QList>
#include <QString>

#include "../models/exam.h"

class ExamDAO;
class SubjectDAO;

class ExamService
{
public:
    ExamService(ExamDAO *examDAO, SubjectDAO *subjectDAO);

    bool addExam(const Exam &exam, QString *errorMessage = nullptr) const;
    QList<Exam> getExams(int userId, QString *errorMessage = nullptr) const;
    Exam getExam(int userId, int examId,
                 QString *errorMessage = nullptr) const;
    bool updateExam(const Exam &exam, QString *errorMessage = nullptr) const;
    bool deleteExam(int userId, int examId,
                    QString *errorMessage = nullptr) const;

private:
    static bool validateAndNormalize(Exam &exam, QString *errorMessage);
    bool subjectBelongsToUser(int userId, int subjectId,
                              QString *errorMessage) const;

    ExamDAO *m_examDAO;
    SubjectDAO *m_subjectDAO;
};
