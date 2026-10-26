#include <QtTest/QtTest>

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>

#include "../src/core/application.h"
#include "../src/ui/dialogs/note_dialog.h"
#include "../src/ui/pages/notes_page.h"

class NotesUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    NotesPage *m_page{nullptr};
    int m_subjectId{0};
    int m_otherUserId{0};
    int m_otherSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    int insertNote(int userId, int subjectId, const QString &title);
    void scheduleNoteDialog(const QString &title, int subjectId);

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void showsEmptyStateAndScopesNotes();
    void addsEditsAndDeletesNotes();
};

void NotesUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("notes_ui_tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    m_otherUserId = insertUser(QStringLiteral("Other Notes User"),
                               QStringLiteral("other-notes@example.test"));
    QVERIFY(m_otherUserId > 0);
}

void NotesUiTests::init()
{
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM notes")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    m_subjectId = insertSubject(m_application->currentUserId(),
                                QStringLiteral("Current Subject"));
    m_otherSubjectId = insertSubject(m_otherUserId,
                                     QStringLiteral("Private Subject"));
    QVERIFY(m_subjectId > 0);
    QVERIFY(m_otherSubjectId > 0);

    m_page = new NotesPage(m_application);
    m_page->show();
    QApplication::processEvents();
}

void NotesUiTests::cleanup()
{
    delete m_page;
    m_page = nullptr;
}

int NotesUiTests::insertUser(const QString &name, const QString &email)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO users (name, email, created_at) "
            "VALUES (:name, :email, :created_at)"))) {
        return 0;
    }
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":email"), email);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

int NotesUiTests::insertSubject(int userId, const QString &name)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO subjects (user_id, name, code, teacher, credits, semester, created_at) "
            "VALUES (:user_id, :name, :code, :teacher, :credits, :semester, :created_at)"))) {
        return 0;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":code"), QStringLiteral("SUBJ"));
    query.bindValue(QStringLiteral(":teacher"), QStringLiteral("Staff"));
    query.bindValue(QStringLiteral(":credits"), 3);
    query.bindValue(QStringLiteral(":semester"), 1);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

int NotesUiTests::insertNote(int userId, int subjectId, const QString &title)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO notes (user_id, subject_id, title, content, created_at, updated_at) "
            "VALUES (:user_id, :subject_id, :title, :content, :created_at, :updated_at)"))) {
        return 0;
    }
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":title"), title);
    query.bindValue(QStringLiteral(":content"), QStringLiteral("Note content."));
    query.bindValue(QStringLiteral(":created_at"), now);
    query.bindValue(QStringLiteral(":updated_at"), now);
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

void NotesUiTests::showsEmptyStateAndScopesNotes()
{
    QVERIFY(insertNote(m_otherUserId, m_otherSubjectId,
                       QStringLiteral("Private note")) > 0);
    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("notesTable"));
    QVERIFY(table);
    QVERIFY(!table->isVisible());
    QCOMPARE(m_page->findChild<QLabel *>(
                 QStringLiteral("notesEmptyMessage"))->text(),
             QStringLiteral("No notes added yet."));

    QVERIFY(insertNote(m_application->currentUserId(), m_subjectId,
                       QStringLiteral("Current note")) > 0);
    m_page->hide();
    m_page->show();
    QApplication::processEvents();
    QVERIFY(table->isVisible());
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Current note"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("Current Subject"));
}

void NotesUiTests::addsEditsAndDeletesNotes()
{
    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("notesTable"));
    auto *addButton = m_page->findChild<QPushButton *>(
        QStringLiteral("addNoteButton"));
    QVERIFY(table);
    QVERIFY(addButton);

    scheduleNoteDialog(QStringLiteral("Lecture Notes"), m_subjectId);
    addButton->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Lecture Notes"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("Current Subject"));

    auto *editButton = table->cellWidget(0, 4)->findChild<QPushButton *>(
        QStringLiteral("editNoteButton"));
    QVERIFY(editButton);
    scheduleNoteDialog(QStringLiteral("Updated Lecture Notes"), m_subjectId);
    editButton->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Updated Lecture Notes"));

    auto *deleteButton = table->cellWidget(0, 4)->findChild<QPushButton *>(
        QStringLiteral("deleteNoteButton"));
    QVERIFY(deleteButton);
    QTimer::singleShot(0, this, [] {
        auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (message) {
            message->button(QMessageBox::Yes)->click();
        }
    });
    deleteButton->click();
    QCOMPARE(table->rowCount(), 0);
    QVERIFY(!table->isVisible());
    QCOMPARE(m_page->findChild<QLabel *>(
                 QStringLiteral("notesEmptyMessage"))->text(),
             QStringLiteral("No notes added yet."));
}

QTEST_MAIN(NotesUiTests)
#include "notes_ui_tests.moc"

void NotesUiTests::scheduleNoteDialog(const QString &title, int subjectId)
{
    QTimer::singleShot(0, this, [this, title, subjectId] {
        auto *dialog = qobject_cast<NoteDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto *titleEdit = dialog->findChild<QLineEdit *>(
            QStringLiteral("noteTitleEdit"));
        auto *subjectCombo = dialog->findChild<QComboBox *>(
            QStringLiteral("noteSubjectCombo"));
        auto *saveButton = dialog->findChild<QPushButton *>(
            QStringLiteral("saveNoteButton"));
        if (!titleEdit || !subjectCombo || !saveButton) {
            return;
        }
        titleEdit->setText(title);
        const int index = subjectCombo->findData(subjectId);
        if (index >= 0) {
            subjectCombo->setCurrentIndex(index);
        }
        saveButton->click();
    });
}

void NotesUiTests::cleanupTestCase()
{
    delete m_application;
    m_application = nullptr;
}