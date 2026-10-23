#include <QString>

struct Note {
    QString id;
    QString title;
    QString content;
    QString subjectId;
    QString userId;
    QDate created;
    QDate updated;
};