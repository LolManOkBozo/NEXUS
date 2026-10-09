#pragma once

#include <QList>
#include <QString>

#include "../models/subject.h"

class DatabaseManager;

class SubjectDAO
{
public:
    explicit SubjectDAO(DatabaseManager *databaseManager);

    bool insertSubject(const Subject &subject, QString *errorMessage = nullptr) const;
    QList<Subject> getSubjectsByUserId(int userId, QString *errorMessage = nullptr) const;
    Subject getSubjectById(int userId, int subjectId, QString *errorMessage = nullptr) const;
    bool updateSubject(const Subject &subject, QString *errorMessage = nullptr) const;
    bool deleteSubject(int userId, int subjectId, QString *errorMessage = nullptr) const;
    QList<Subject> searchSubjects(int userId, const QString &searchTerm,
                                  QString *errorMessage = nullptr) const;

private:
    DatabaseManager *m_databaseManager;
};
