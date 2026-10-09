#include "subject.h"

Subject::Subject(int id, int userId, QString name, QString code, QString teacher,
                 int credits, int semester, QString createdAt)
    : m_id(id)
    , m_userId(userId)
    , m_name(std::move(name))
    , m_code(std::move(code))
    , m_teacher(std::move(teacher))
    , m_credits(credits)
    , m_semester(semester)
    , m_createdAt(std::move(createdAt))
{
}
