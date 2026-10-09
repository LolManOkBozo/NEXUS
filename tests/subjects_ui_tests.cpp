#include <QtTest/QtTest>

#include <QApplication>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QHeaderView>
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
#include <QSqlQuery>

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
    bool m_saveFailureDialogRetained{false};
    bool m_saveFailureLeftDataUnchanged{false};
    QString m_confirmationText;
    QString m_saveFailureWarning;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void navigateAndManageSubjects();
    void failedSubjectAddPreservesInput();
    void failedSubjectEditPreservesInput();

private:
    void scheduleSubjectDialog(const QString &name, const QString &code,
                               const QString &teacher, int credits, int semester);
    void scheduleSubjectSaveFailure(const QString &name, const QString &code,
                                    const QString &teacher, int credits, int semester,
                                    int subjectId);
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
        m_confirmationText = messageBox->text();
        messageBox->button(answer)->click();
    });
}

void SubjectsUiTests::scheduleSubjectSaveFailure(const QString &name,
                                                 const QString &code,
                                                 const QString &teacher,
                                                 int credits, int semester,
                                                 int subjectId)
{
    QTimer::singleShot(0, this, [this, name, code, teacher, credits, semester,
                                 subjectId] {
        auto *dialog = qobject_cast<SubjectDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        m_dialogOpened = true;
        auto *nameEdit = dialog->findChild<QLineEdit *>(QStringLiteral("subjectNameEdit"));
        auto *codeEdit = dialog->findChild<QLineEdit *>(QStringLiteral("subjectCodeEdit"));
        auto *teacherEdit = dialog->findChild<QLineEdit *>(QStringLiteral("subjectTeacherEdit"));
        auto *creditsSpinBox =
            dialog->findChild<QSpinBox *>(QStringLiteral("subjectCreditsSpinBox"));
        auto *semesterSpinBox =
            dialog->findChild<QSpinBox *>(QStringLiteral("subjectSemesterSpinBox"));
        auto *save = dialog->findChild<QPushButton *>(QStringLiteral("saveSubjectButton"));
        if (!nameEdit || !codeEdit || !teacherEdit || !creditsSpinBox
            || !semesterSpinBox || !save) {
            return;
        }
        nameEdit->setText(name);
        codeEdit->setText(code);
        teacherEdit->setText(teacher);
        creditsSpinBox->setValue(credits);
        semesterSpinBox->setValue(semester);

        m_application->databaseManager()->close();
        QTimer::singleShot(0, this, [this] {
            auto *messageBox =
                qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (!messageBox) {
                return;
            }
            m_saveFailureWarning = messageBox->text();
            messageBox->accept();
        });
        save->click();

        auto *activeDialog =
            qobject_cast<SubjectDialog *>(QApplication::activeModalWidget());
        auto *retainedName = activeDialog
            ? activeDialog->findChild<QLineEdit *>(QStringLiteral("subjectNameEdit"))
            : nullptr;
        auto *retainedCode = activeDialog
            ? activeDialog->findChild<QLineEdit *>(QStringLiteral("subjectCodeEdit"))
            : nullptr;
        auto *retainedTeacher = activeDialog
            ? activeDialog->findChild<QLineEdit *>(QStringLiteral("subjectTeacherEdit"))
            : nullptr;
        auto *retainedCredits = activeDialog
            ? activeDialog->findChild<QSpinBox *>(QStringLiteral("subjectCreditsSpinBox"))
            : nullptr;
        auto *retainedSemester = activeDialog
            ? activeDialog->findChild<QSpinBox *>(QStringLiteral("subjectSemesterSpinBox"))
            : nullptr;
        m_saveFailureDialogRetained =
            activeDialog == dialog && retainedName && retainedCode && retainedTeacher
            && retainedCredits && retainedSemester
            && retainedName->text() == name && retainedCode->text() == code
            && retainedTeacher->text() == teacher && retainedCredits->value() == credits
            && retainedSemester->value() == semester;

        if (!m_application->initialize()) {
            dialog->reject();
            return;
        }
        QSqlQuery query(m_application->databaseManager()->database());
        if (subjectId > 0) {
            query.prepare(QStringLiteral("SELECT name, code FROM subjects WHERE id = :id"));
            query.bindValue(QStringLiteral(":id"), subjectId);
            m_saveFailureLeftDataUnchanged =
                query.exec() && query.next()
                && query.value(0).toString() != name
                && query.value(1).toString() != code;
        } else {
            query.prepare(QStringLiteral(
                "SELECT COUNT(*) FROM subjects WHERE user_id = :user_id AND code = :code"));
            query.bindValue(QStringLiteral(":user_id"),
                            m_application->currentUserId());
            query.bindValue(QStringLiteral(":code"), code);
            m_saveFailureLeftDataUnchanged =
                query.exec() && query.next() && query.value(0).toInt() == 0;
        }
        if (activeDialog == dialog) {
            save->click();
        }
    });
}

void SubjectsUiTests::navigateAndManageSubjects()
{
    m_window->show();
    QApplication::processEvents();
    QVERIFY(!m_window->isHidden());
    const QString sharedStyle = qApp->styleSheet();
    QVERIFY(sharedStyle.contains(QStringLiteral("QComboBox")));
    QVERIFY(sharedStyle.contains(QStringLiteral("QAbstractSpinBox")));
    QVERIFY(sharedStyle.contains(QStringLiteral("QTableWidget::item")));
    QVERIFY(sharedStyle.contains(QStringLiteral("padding: 8px 9px")));
    QVERIFY(sharedStyle.contains(QStringLiteral("padding: 7px 8px")));

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
    sidebar->setCurrentRow(2);
    QCOMPARE(stack->currentWidget()->objectName(), QStringLiteral("timetablePage"));

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
    QCOMPARE(rowActions->objectName(), QStringLiteral("subjectActionWidget"));
    auto *subjectActionLayout = qobject_cast<QHBoxLayout *>(rowActions->layout());
    QVERIFY(subjectActionLayout);
    QCOMPARE(subjectActionLayout->contentsMargins(), QMargins(8, 5, 8, 5));
    QCOMPARE(subjectActionLayout->spacing(), 8);
    QVERIFY(table->rowHeight(0) >= 50);
    QVERIFY(table->columnWidth(5) >= 194);
    const int actionsPosition = table->horizontalHeader()->sectionViewportPosition(5);
    QVERIFY(actionsPosition >= 0);
    QVERIFY(actionsPosition + table->columnWidth(5) <= table->viewport()->width());
    auto *editButton = rowActions->findChild<QPushButton *>(
        QStringLiteral("editSubjectButton"));
    QVERIFY(editButton);
    QCOMPARE(editButton->text(), QStringLiteral("Edit"));
    QVERIFY(editButton->isEnabled());
    QVERIFY(editButton->isVisibleTo(table));
    QVERIFY(editButton->width() >= editButton->minimumWidth());
    QVERIFY(editButton->height() < table->rowHeight(0) - 10);
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
    QCOMPARE(deleteButton->text(), QStringLiteral("Delete"));
    QVERIFY(deleteButton->isEnabled());
    QVERIFY(deleteButton->isVisibleTo(table));
    QVERIFY(deleteButton->width() >= deleteButton->minimumWidth());
    scheduleDeleteConfirmation(QMessageBox::No);
    deleteButton->click();
    QVERIFY(m_confirmationOpened);
    QVERIFY(m_confirmationText.contains(QStringLiteral("timetable entries")));
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

void SubjectsUiTests::failedSubjectAddPreservesInput()
{
    auto *sidebar = m_window->findChild<QListWidget *>(QStringLiteral("sidebar"));
    sidebar->setCurrentRow(1);
    auto *addButton = m_window->findChild<QPushButton *>(QStringLiteral("addSubjectButton"));
    QVERIFY(addButton);

    m_dialogOpened = false;
    m_saveFailureDialogRetained = false;
    m_saveFailureLeftDataUnchanged = false;
    m_saveFailureWarning.clear();
    scheduleSubjectSaveFailure(QStringLiteral("Retained Add Subject"),
                               QStringLiteral("TEST-ADD-FAIL"),
                               QStringLiteral("Test Teacher"), 4, 5, 0);
    addButton->click();

    QVERIFY(m_dialogOpened);
    QVERIFY(m_saveFailureDialogRetained);
    QVERIFY(m_saveFailureLeftDataUnchanged);
    QVERIFY(m_saveFailureWarning.contains(QStringLiteral("database is not available")));
    auto *table = m_window->findChild<QTableWidget *>(QStringLiteral("subjectsTable"));
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 2);
    QVERIFY(table->findItems(QStringLiteral("Retained Add Subject"), Qt::MatchExactly).size()
            == 1);
}

void SubjectsUiTests::failedSubjectEditPreservesInput()
{
    auto *table = m_window->findChild<QTableWidget *>(QStringLiteral("subjectsTable"));
    QVERIFY(table);
    int row = -1;
    for (int candidate = 0; candidate < table->rowCount(); ++candidate) {
        if (table->item(candidate, 0)->text() == QStringLiteral("Retained Add Subject")) {
            row = candidate;
            break;
        }
    }
    QVERIFY(row >= 0);
    const int subjectId = table->item(row, 0)->data(Qt::UserRole).toInt();
    auto *actions = table->cellWidget(row, 5);
    QVERIFY(actions);
    auto *editButton = actions->findChild<QPushButton *>(QStringLiteral("editSubjectButton"));
    QVERIFY(editButton);

    m_dialogOpened = false;
    m_saveFailureDialogRetained = false;
    m_saveFailureLeftDataUnchanged = false;
    m_saveFailureWarning.clear();
    scheduleSubjectSaveFailure(QStringLiteral("Retained Edit Subject"),
                               QStringLiteral("TEST-EDIT-FAIL"),
                               QStringLiteral("Updated Teacher"), 5, 6, subjectId);
    editButton->click();

    QVERIFY(m_dialogOpened);
    QVERIFY(m_saveFailureDialogRetained);
    QVERIFY(m_saveFailureLeftDataUnchanged);
    QVERIFY(m_saveFailureWarning.contains(QStringLiteral("database is not available")));
    QCOMPARE(table->item(row, 0)->text(), QStringLiteral("Retained Edit Subject"));
    QCOMPARE(table->item(row, 1)->text(), QStringLiteral("TEST-EDIT-FAIL"));
}

QTEST_MAIN(SubjectsUiTests)
#include "subjects_ui_tests.moc"
