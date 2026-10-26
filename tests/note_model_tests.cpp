#include <QtTest/QtTest>

#include "../src/models/note.h"

class NoteModelTests : public QObject
{
    Q_OBJECT

private slots:
    void defaultNoteHasNoIdentity();
    void storesNoteFields();
};

void NoteModelTests::defaultNoteHasNoIdentity()
{
    const Note note;
    QCOMPARE(note.id(), 0);
    QCOMPARE(note.userId(), 0);
    QCOMPARE(note.subjectId(), 0);
    QVERIFY(note.title().isEmpty());
    QVERIFY(note.content().isEmpty());
    QVERIFY(note.createdAt().isEmpty());
    QVERIFY(note.updatedAt().isEmpty());
}

void NoteModelTests::storesNoteFields()
{
    Note note(7, 11, 13, QStringLiteral("Lecture Summary"),
              QStringLiteral("Key concepts covered in class."),
              QStringLiteral("2026-10-20T08:00:00Z"),
              QStringLiteral("2026-10-20T08:00:00Z"));
    QCOMPARE(note.id(), 7);
    QCOMPARE(note.userId(), 11);
    QCOMPARE(note.subjectId(), 13);
    QCOMPARE(note.title(), QStringLiteral("Lecture Summary"));
    QCOMPARE(note.content(),
             QStringLiteral("Key concepts covered in class."));
    QCOMPARE(note.createdAt(), QStringLiteral("2026-10-20T08:00:00Z"));
    QCOMPARE(note.updatedAt(), QStringLiteral("2026-10-20T08:00:00Z"));

    note.setTitle(QStringLiteral("Updated Summary"));
    note.setContent(QStringLiteral("Revised notes."));
    note.setSubjectId(0);
    QCOMPARE(note.title(), QStringLiteral("Updated Summary"));
    QCOMPARE(note.content(), QStringLiteral("Revised notes."));
    QCOMPARE(note.subjectId(), 0);
}

QTEST_GUILESS_MAIN(NoteModelTests)
#include "note_model_tests.moc"