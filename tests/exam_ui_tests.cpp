#include <QtTest/QtTest>

#include <QApplication>
#include <QDate>
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
#include "../src/ui/dialogs/exam_dialog.h"
#include "../src/ui/pages/exams_page.h"

class ExamUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    ExamsPage *m_page{nullptr};
    int m_subjectId{0};
    int m_otherUserId{0};
    int m_otherSubjectId{0};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    int insertExam(int userId, int subjectId, const QString &title);
    void fillExamDialog(const QString &title);

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void showsEmptyStateAndScopesExams();
    void addsEditsAndDeletesExams();
};

void ExamUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("exam_ui_tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    m_otherUserId = insertUser(QStringLiteral("Other Exam User"),
                               QStringLiteral("other-exam@example.test"));
    QVERIFY(m_otherUserId > 0);
}

void ExamUiTests::init()
{
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM exams")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    m_subjectId = insertSubject(m_application->currentUserId(),
                                QStringLiteral("Current Subject"));
    m_otherSubjectId = insertSubject(m_otherUserId,
                                     QStringLiteral("Private Subject"));
    QVERIFY(m_subjectId > 0);
    QVERIFY(m_otherSubjectId > 0);

    m_page = new ExamsPage(m_application);
    m_page->show();
    QApplication::processEvents();
}

void ExamUiTests::cleanup()
{
    delete m_page;
    m_page = nullptr;
}

void ExamUiTests::cleanupTestCase()
{
    delete m_application;
    m_application = nullptr;
}

int ExamUiTests::insertUser(const QString &name, const QString &email)
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

int ExamUiTests::insertSubject(int userId, const QString &name)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO subjects (user_id, name, created_at) "
            "VALUES (:user_id, :name, :created_at)"))) {
        return 0;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

int ExamUiTests::insertExam(int userId, int subjectId, const QString &title)
{
    QSqlQuery query(m_application->databaseManager()->database());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO exams (user_id, subject_id, title, exam_date) "
            "VALUES (:user_id, :subject_id, :title, :date)"))) {
        return 0;
    }
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":title"), title);
    query.bindValue(QStringLiteral(":date"),
                    QDate::currentDate().addDays(7).toString(Qt::ISODate));
    return query.exec() ? query.lastInsertId().toInt() : 0;
}

void ExamUiTests::fillExamDialog(const QString &title)
{
    QTimer::singleShot(0, this, [this, title] {
        auto *dialog = qobject_cast<ExamDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto *titleEdit = dialog->findChild<QLineEdit *>(
            QStringLiteral("examTitleEdit"));
        auto *saveButton = dialog->findChild<QPushButton *>(
            QStringLiteral("saveExamButton"));
        if (!titleEdit || !saveButton) {
            return;
        }
        titleEdit->setText(title);
        saveButton->click();
    });
}

void ExamUiTests::showsEmptyStateAndScopesExams()
{
    QVERIFY(insertExam(m_otherUserId, m_otherSubjectId,
                       QStringLiteral("Private examination")) > 0);
    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("examsTable"));
    QVERIFY(table);
    QVERIFY(!table->isVisible());
    QCOMPARE(m_page->findChild<QLabel *>(
                 QStringLiteral("examsEmptyMessage"))->text(),
             QStringLiteral("No exams added yet."));

    QVERIFY(insertExam(m_application->currentUserId(), m_subjectId,
                       QStringLiteral("Current examination")) > 0);
    m_page->hide();
    m_page->show();
    QApplication::processEvents();
    QVERIFY(table->isVisible());
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Current examination"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("Current Subject"));
}

void ExamUiTests::addsEditsAndDeletesExams()
{
    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("examsTable"));
    auto *addButton = m_page->findChild<QPushButton *>(
        QStringLiteral("addExamButton"));
    QVERIFY(table);
    QVERIFY(addButton);

    fillExamDialog(QStringLiteral("Midterm Exam"));
    addButton->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Midterm Exam"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("Current Subject"));

    auto *editButton = table->cellWidget(0, 5)->findChild<QPushButton *>(
        QStringLiteral("editExamButton"));
    QVERIFY(editButton);
    fillExamDialog(QStringLiteral("Updated Midterm"));
    editButton->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Updated Midterm"));

    auto *deleteButton = table->cellWidget(0, 5)->findChild<QPushButton *>(
        QStringLiteral("deleteExamButton"));
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
                 QStringLiteral("examsEmptyMessage"))->text(),
             QStringLiteral("No exams added yet."));
}

QTEST_MAIN(ExamUiTests)
#include "exam_ui_tests.moc"
