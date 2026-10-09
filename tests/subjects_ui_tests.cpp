#include <QtTest/QtTest>

#include <QApplication>
#include <QAbstractButton>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QSpinBox>

#include "../src/core/application.h"
#include "../src/ui/dialogs/subject_dialog.h"
#include "../src/ui/main_window.h"

class SubjectsUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    MainWindow *m_window{nullptr};
    bool m_dialogOpened{false};
    bool m_confirmationOpened{false};

private slots:
    void initTestCase();
    void cleanupTestCase();
    void navigateAndManageSubjects();

private:
    void scheduleSubjectDialog(const QString &name, const QString &code,
                               const QString &teacher, int credits, int semester);
    void scheduleDeleteConfirmation(QMessageBox::StandardButton answer);
};

void SubjectsUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("nexus_ui_tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    m_window = new MainWindow(m_application);
}

void SubjectsUiTests::cleanupTestCase()
{
    delete m_window;
    m_window = nullptr;
    delete m_application;
    m_application = nullptr;
}

void SubjectsUiTests::scheduleSubjectDialog(const QString &name, const QString &code,
                                            const QString &teacher, int credits, int semester)
{
    QTimer::singleShot(0, this, [this, name, code, teacher, credits, semester] {
        auto *dialog = qobject_cast<SubjectDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        m_dialogOpened = true;
        dialog->findChild<QLineEdit *>(QStringLiteral("subjectNameEdit"))->setText(name);
        dialog->findChild<QLineEdit *>(QStringLiteral("subjectCodeEdit"))->setText(code);
        dialog->findChild<QLineEdit *>(QStringLiteral("subjectTeacherEdit"))->setText(teacher);
        dialog->findChild<QSpinBox *>(QStringLiteral("subjectCreditsSpinBox"))->setValue(credits);
        dialog->findChild<QSpinBox *>(QStringLiteral("subjectSemesterSpinBox"))->setValue(semester);
        dialog->findChild<QPushButton *>(QStringLiteral("saveSubjectButton"))->click();
    });
}

void SubjectsUiTests::scheduleDeleteConfirmation(QMessageBox::StandardButton answer)
{
    QTimer::singleShot(0, this, [this, answer] {
        auto *messageBox = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (!messageBox) {
            return;
        }
        m_confirmationOpened = true;
        messageBox->button(answer)->click();
    });
}

void SubjectsUiTests::navigateAndManageSubjects()
{
    m_window->show();
    QApplication::processEvents();
    QVERIFY(!m_window->isHidden());

    auto *sidebar = m_window->findChild<QListWidget *>(QStringLiteral("sidebar"));
    auto *stack = m_window->findChild<QStackedWidget *>();
    QVERIFY(sidebar);
    QVERIFY(stack);
    QCOMPARE(sidebar->count(), 10);
    QCOMPARE(stack->count(), 10);
    for (int row = 0; row < sidebar->count(); ++row) {
        sidebar->setCurrentRow(row);
        QCOMPARE(stack->currentIndex(), row);
    }

    sidebar->setCurrentRow(1);
    auto *table = m_window->findChild<QTableWidget *>(QStringLiteral("subjectsTable"));
    auto *search = m_window->findChild<QLineEdit *>(QStringLiteral("subjectSearchEdit"));
    auto *emptyMessage = m_window->findChild<QLabel *>(QStringLiteral("subjectsEmptyMessage"));
    auto *emptyState = m_window->findChild<QWidget *>(QStringLiteral("subjectsEmptyState"));
    auto *addButton = m_window->findChild<QPushButton *>(QStringLiteral("addSubjectButton"));
    QVERIFY(table);
    QVERIFY(search);
    QVERIFY(emptyMessage);
    QVERIFY(emptyState);
    QVERIFY(addButton);
    QCOMPARE(emptyMessage->text(), QStringLiteral("No subjects added yet."));
    QVERIFY(emptyState->isVisible());

    scheduleSubjectDialog(QStringLiteral("Data Structures"), QStringLiteral("CSIT-214"),
                          QStringLiteral("Alex Sharma"), 3, 2);
    addButton->click();
    QVERIFY(m_dialogOpened);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Data Structures"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("CSIT-214"));
    QCOMPARE(table->item(0, 2)->text(), QStringLiteral("Alex Sharma"));
    QCOMPARE(table->item(0, 3)->text(), QStringLiteral("3"));
    QCOMPARE(table->item(0, 4)->text(), QStringLiteral("2"));

    search->setText(QStringLiteral("CSIT-214"));
    QCOMPARE(table->rowCount(), 1);
    search->setText(QStringLiteral("missing subject"));
    QVERIFY(emptyState->isVisible());
    QCOMPARE(emptyMessage->text(), QStringLiteral("No subjects match your search."));
    search->clear();
    QCOMPARE(table->rowCount(), 1);

    auto *rowActions = table->cellWidget(0, 5);
    QVERIFY(rowActions);
    auto *editButton = rowActions->findChild<QPushButton *>(
        QStringLiteral("editSubjectButton"));
    QVERIFY(editButton);
    m_dialogOpened = false;
    scheduleSubjectDialog(QStringLiteral("Advanced Data Structures"),
                          QStringLiteral("CSIT-315"), QStringLiteral("Priya Shah"), 4, 3);
    editButton->click();
    QVERIFY(m_dialogOpened);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Advanced Data Structures"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("CSIT-315"));

    rowActions = table->cellWidget(0, 5);
    auto *deleteButton = rowActions->findChild<QPushButton *>(
        QStringLiteral("deleteSubjectButton"));
    QVERIFY(deleteButton);
    scheduleDeleteConfirmation(QMessageBox::No);
    deleteButton->click();
    QVERIFY(m_confirmationOpened);
    QCOMPARE(table->rowCount(), 1);

    m_confirmationOpened = false;
    scheduleDeleteConfirmation(QMessageBox::Yes);
    deleteButton->click();
    QVERIFY(m_confirmationOpened);
    QVERIFY(emptyState->isVisible());
    QCOMPARE(emptyMessage->text(), QStringLiteral("No subjects added yet."));

    search->clear();
    scheduleSubjectDialog(QStringLiteral("Computer Networks"), QStringLiteral("CSIT-301"),
                          QStringLiteral("Prof. Rai"), 3, 4);
    addButton->click();
    QCOMPARE(table->rowCount(), 1);

    delete m_window;
    m_window = nullptr;
    m_application->databaseManager()->close();
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    m_window = new MainWindow(m_application);
    m_window->show();
    QApplication::processEvents();
    sidebar = m_window->findChild<QListWidget *>(QStringLiteral("sidebar"));
    sidebar->setCurrentRow(1);
    table = m_window->findChild<QTableWidget *>(QStringLiteral("subjectsTable"));
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Computer Networks"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("CSIT-301"));
}

QTEST_MAIN(SubjectsUiTests)
#include "subjects_ui_tests.moc"
