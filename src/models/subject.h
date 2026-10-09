
#pragma once

#include <QString>

class Subject
{
public:
    Subject() = default;
    Subject(int id, int userId, QString name, QString code,
            QString teacher, int credits, int semester, QString createdAt);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    QString name() const { return m_name; }
    void setName(QString name) { m_name = name; }

    QString code() const { return m_code; }
    void setCode(QString code) { m_code = code; }

    QString teacher() const { return m_teacher; }
    void setTeacher(QString teacher) { m_teacher = teacher; }

    int credits() const { return m_credits; }
    void setCredits(int credits) { m_credits = credits; }

    int semester() const { return m_semester; }
    void setSemester(int semester) { m_semester = semester; }

    QString createdAt() const { return m_createdAt; }
    void setCreatedAt(QString createdAt) { m_createdAt = createdAt; }

private:
    int m_id{0};
    int m_userId{0};
    QString m_name;
    QString m_code;
    QString m_teacher;
    int m_credits{0};
    int m_semester{0};
    QString m_createdAt;
};
