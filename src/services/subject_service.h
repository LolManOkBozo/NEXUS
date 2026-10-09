#pragma once

#include <QList>
#include <QString>

#include "../models/subject.h"

class SubjectDAO;

class SubjectService
{
public:
    explicit SubjectService(SubjectDAO *subjectDAO);

    bool addSubject(const Subject &subject, QString *errorMessage = nullptr) const;
    QList<Subject> getSubjects(int userId, QString *errorMessage = nullptr) const;
    Subject getSubject(int userId, int subjectId, QString *errorMessage = nullptr) const;
    bool updateSubject(const Subject &subject, QString *errorMessage = nullptr) const;
    bool deleteSubject(int userId, int subjectId, QString *errorMessage = nullptr) const;
    QList<Subject> searchSubjects(int userId, const QString &searchTerm,
                                  QString *errorMessage = nullptr) const;

private:
    static bool validateAndNormalize(Subject &subject, QString *errorMessage);

    SubjectDAO *m_subjectDAO;
};
