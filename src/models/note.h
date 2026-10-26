#pragma once

#include <QString>

#include <utility>

class Note {
public:
    Note() = default;
    Note(int id, int userId, int subjectId, QString title, QString content,
         QString createdAt, QString updatedAt);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    int subjectId() const { return m_subjectId; }
    void setSubjectId(int subjectId) { m_subjectId = subjectId; }

    QString title() const { return m_title; }
    void setTitle(QString title) { m_title = std::move(title); }

    QString content() const { return m_content; }
    void setContent(QString content) { m_content = std::move(content); }

    QString createdAt() const { return m_createdAt; }
    void setCreatedAt(QString createdAt) { m_createdAt = std::move(createdAt); }

    QString updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(QString updatedAt) { m_updatedAt = std::move(updatedAt); }

private:
    int m_id{0};
    int m_userId{0};
    int m_subjectId{0};
    QString m_title;
    QString m_content;
    QString m_createdAt;
    QString m_updatedAt;
};