#pragma once

#include <QList>
#include <QString>

#include "../models/exam.h"

class DatabaseManager;

class ExamDAO
{
public:
    explicit ExamDAO(DatabaseManager *databaseManager);

    bool insertExam(const Exam &exam, QString *errorMessage = nullptr) const;
    QList<Exam> getExamsByUserId(int userId,
                                 QString *errorMessage = nullptr) const;
    Exam getExamById(int userId, int examId,
                     QString *errorMessage = nullptr) const;
    bool updateExam(const Exam &exam, QString *errorMessage = nullptr) const;
    bool deleteExam(int userId, int examId,
                    QString *errorMessage = nullptr) const;

private:
    DatabaseManager *m_databaseManager;
};
