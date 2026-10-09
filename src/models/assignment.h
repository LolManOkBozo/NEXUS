#pragma once

#include <QString>

#include <utility>

class Assignment
{
public:
    Assignment() = default;
    Assignment(int id, int userId, int subjectId, QString title,
               QString description, QString dueDate, QString dueTime,
               QString priority, QString status, QString createdAt);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    int subjectId() const { return m_subjectId; }
    void setSubjectId(int subjectId) { m_subjectId = subjectId; }

    QString title() const { return m_title; }
    void setTitle(QString title) { m_title = std::move(title); }

    QString description() const { return m_description; }
    void setDescription(QString description) { m_description = std::move(description); }

    QString dueDate() const { return m_dueDate; }
    void setDueDate(QString dueDate) { m_dueDate = std::move(dueDate); }

    QString dueTime() const { return m_dueTime; }
    void setDueTime(QString dueTime) { m_dueTime = std::move(dueTime); }

    QString priority() const { return m_priority; }
    void setPriority(QString priority) { m_priority = std::move(priority); }

    QString status() const { return m_status; }
    void setStatus(QString status) { m_status = std::move(status); }

    QString createdAt() const { return m_createdAt; }
    void setCreatedAt(QString createdAt) { m_createdAt = std::move(createdAt); }

private:
    int m_id{0};
    int m_userId{0};
    int m_subjectId{0};
    QString m_title;
    QString m_description;
    QString m_dueDate;
    QString m_dueTime;
    QString m_priority;
    QString m_status;
    QString m_createdAt;
};
