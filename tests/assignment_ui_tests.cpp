#include <QtTest/QtTest>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimeEdit>
#include <QTimer>

#include "../src/core/application.h"
#include "../src/ui/dialogs/assignment_dialog.h"
#include "../src/ui/pages/assignments_page.h"

class AssignmentUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    AssignmentsPage *m_page{nullptr};
    int m_subjectId{0};
    int m_secondSubjectId{0};
    int m_otherUserId{0};
    int m_otherSubjectId{0};
    bool m_subjectIsolationObserved{false};
    bool m_assignmentDialogOpened{false};
    bool m_noSubjectMessageShown{false};
    bool m_saveFailureRetainedInput{false};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    int insertAssignment(int userId, int subjectId, const QString &title,
                         const QString &deadline, const QString &priority,
                         const QString &status);
    void scheduleAssignmentDialog(const QString &title, int subjectId,
                                  const QString &status = QStringLiteral("Not Started"));
    void scheduleConfirmation(QMessageBox::StandardButton answer);

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void showsEmptyStateAndScopesSubjects();
    void addButtonClickOpensAssignmentDialog();
    void addButtonWithoutSubjectsExplainsRequirement();
    void addsEditsFiltersAndDeletesAssignments();
    void displaysDeadlinesWithoutMarkingCompletedWorkOverdue();
    void failedSaveKeepsDialogValuesAndCanRetry();
    void failedEditKeepsDialogValuesAndCanRetry();
};

void AssignmentUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("assignment_ui_tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    m_otherUserId = insertUser(QStringLiteral("Other Assignment User"),
                               QStringLiteral("other-assignment@example.test"));
    QVERIFY(m_otherUserId > 0);
}

void AssignmentUiTests::init()
{
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM assignments")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM timetable")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));

    m_subjectId = insertSubject(m_application->currentUserId(),
                                QStringLiteral("Biology"));
    m_secondSubjectId = insertSubject(m_application->currentUserId(),
                                      QStringLiteral("Chemistry"));
    m_otherSubjectId = insertSubject(m_otherUserId, QStringLiteral("Private Subject"));
    QVERIFY(m_subjectId > 0);
    QVERIFY(m_secondSubjectId > 0);
    QVERIFY(m_otherSubjectId > 0);

    m_subjectIsolationObserved = false;
    m_assignmentDialogOpened = false;
    m_noSubjectMessageShown = false;
    m_saveFailureRetainedInput = false;
    m_page = new AssignmentsPage(m_application);
    m_page->show();
    QApplication::processEvents();
}

void AssignmentUiTests::cleanup()
{
    delete m_page;
    m_page = nullptr;
}

void AssignmentUiTests::cleanupTestCase()
{
    delete m_application;
    m_application = nullptr;
}

int AssignmentUiTests::insertUser(const QString &name, const QString &email)
{
    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral(
        "INSERT INTO users (name, email, created_at) "
        "VALUES (:name, :email, :created_at)"));
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":email"), email);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

int AssignmentUiTests::insertSubject(int userId, const QString &name)
{
    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral(
        "INSERT INTO subjects (user_id, name, created_at) "
        "VALUES (:user_id, :name, :created_at)"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

int AssignmentUiTests::insertAssignment(int userId, int subjectId,
                                        const QString &title,
                                        const QString &deadline,
                                        const QString &priority,
                                        const QString &status)
{
    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral(
        "INSERT INTO assignments "
        "(user_id, subject_id, title, description, deadline, priority, status, created_at) "
        "VALUES (:user_id, :subject_id, :title, '', :deadline, :priority, :status, :created_at)"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":subject_id"), subjectId);
    query.bindValue(QStringLiteral(":title"), title);
    query.bindValue(QStringLiteral(":deadline"), deadline);
    query.bindValue(QStringLiteral(":priority"), priority);
    query.bindValue(QStringLiteral(":status"), status);
    query.bindValue(QStringLiteral(":created_at"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

void AssignmentUiTests::scheduleAssignmentDialog(const QString &title, int subjectId,
                                                const QString &status)
{
    QTimer::singleShot(0, this, [this, title, subjectId, status] {
        auto *dialog = qobject_cast<AssignmentDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto *titleEdit = dialog->findChild<QLineEdit *>(
            QStringLiteral("assignmentTitleEdit"));
        auto *subjects = dialog->findChild<QComboBox *>(
            QStringLiteral("assignmentSubjectCombo"));
        auto *date = dialog->findChild<QDateEdit *>(
            QStringLiteral("assignmentDueDateEdit"));
        auto *priority = dialog->findChild<QComboBox *>(
            QStringLiteral("assignmentPriorityCombo"));
        auto *statusCombo = dialog->findChild<QComboBox *>(
            QStringLiteral("assignmentStatusCombo"));
        auto *save = dialog->findChild<QPushButton *>(
            QStringLiteral("saveAssignmentButton"));
        if (!titleEdit || !subjects || !date || !priority || !statusCombo || !save) {
            return;
        }
        titleEdit->setText(title);
        subjects->setCurrentIndex(subjects->findData(subjectId));
        date->setDate(QDate(2099, 6, 15));
        priority->setCurrentText(QStringLiteral("High"));
        statusCombo->setCurrentText(status);
        save->click();
    });
}

void AssignmentUiTests::scheduleConfirmation(QMessageBox::StandardButton answer)
{
    QTimer::singleShot(0, this, [answer] {
        auto *messageBox =
            qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (messageBox) {
            messageBox->button(answer)->click();
        }
    });
}

void AssignmentUiTests::showsEmptyStateAndScopesSubjects()
{
    QVERIFY(insertAssignment(m_otherUserId, m_otherSubjectId,
                             QStringLiteral("Private assignment"),
                             QStringLiteral("2099-01-01"),
                             QStringLiteral("Low"),
                             QStringLiteral("Not Started")) > 0);
    delete m_page;
    m_page = new AssignmentsPage(m_application);
    m_page->show();
    QApplication::processEvents();

    auto *table = m_page->findChild<QTableWidget *>(
        QStringLiteral("assignmentsTable"));
    auto *emptyMessage = m_page->findChild<QLabel *>(
        QStringLiteral("assignmentsEmptyMessage"));
    auto *subjectFilter = m_page->findChild<QComboBox *>(
        QStringLiteral("assignmentSubjectFilter"));
    QVERIFY(table);
    QVERIFY(emptyMessage);
    QVERIFY(subjectFilter);
    QVERIFY(table->isHidden());
    QCOMPARE(table->rowCount(), 0);
    QVERIFY(emptyMessage->text().contains(QStringLiteral("No assignments")));
    QCOMPARE(subjectFilter->count(), 3);
    QVERIFY(subjectFilter->findData(m_otherSubjectId) < 0);

    QTimer::singleShot(0, this, [this] {
        auto *dialog = qobject_cast<AssignmentDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        m_assignmentDialogOpened = true;
        const auto fieldLabels = dialog->findChildren<QLabel *>(
            QStringLiteral("assignmentFieldLabel"));
        QStringList fieldLabelTexts;
        for (const QLabel *label : fieldLabels) {
            fieldLabelTexts.append(label->text());
            QVERIFY(label->geometry().isValid());
        }
        QCOMPARE(fieldLabels.size(), 6);
        QVERIFY(fieldLabelTexts.contains(QStringLiteral("Title")));
        QVERIFY(fieldLabelTexts.contains(QStringLiteral("Subject")));
        QVERIFY(fieldLabelTexts.contains(QStringLiteral("Description")));
        auto *subjects = dialog->findChild<QComboBox *>(
            QStringLiteral("assignmentSubjectCombo"));
        m_subjectIsolationObserved =
            subjects && subjects->count() == 2
            && subjects->findData(m_subjectId) >= 0
            && subjects->findData(m_secondSubjectId) >= 0
            && subjects->findData(m_otherSubjectId) < 0;
        dialog->reject();
    });
    auto *addButton = m_page->findChild<QPushButton *>(
        QStringLiteral("addAssignmentButton"));
    QVERIFY(addButton);
    QVERIFY(addButton->isEnabled());
    QVERIFY(addButton->isVisibleTo(m_page));
    QVERIFY(addButton->geometry().isValid());
    QTest::mouseClick(addButton, Qt::LeftButton);
    QVERIFY(m_assignmentDialogOpened);
    QVERIFY(m_subjectIsolationObserved);
    QCOMPARE(table->rowCount(), 0);
}

void AssignmentUiTests::addButtonClickOpensAssignmentDialog()
{
    auto *addButton = m_page->findChild<QPushButton *>(
        QStringLiteral("addAssignmentButton"));
    QVERIFY(addButton);
    QVERIFY(addButton->isEnabled());
    QVERIFY(addButton->isVisibleTo(m_page));
    QVERIFY(addButton->geometry().isValid());

    QTimer::singleShot(0, this, [this] {
        auto *dialog = qobject_cast<AssignmentDialog *>(QApplication::activeModalWidget());
        if (dialog) {
            m_assignmentDialogOpened = true;
            QCOMPARE(dialog->windowTitle(), QStringLiteral("Add Assignment"));
            dialog->reject();
        }
    });
    QTest::mouseClick(addButton, Qt::LeftButton);
    QVERIFY(m_assignmentDialogOpened);
}

void AssignmentUiTests::addButtonWithoutSubjectsExplainsRequirement()
{
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM assignments")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));
    delete m_page;
    m_page = new AssignmentsPage(m_application);
    m_page->show();
    QApplication::processEvents();

    auto *addButton = m_page->findChild<QPushButton *>(
        QStringLiteral("addAssignmentButton"));
    auto *emptyAddButton = m_page->findChild<QPushButton *>(
        QStringLiteral("emptyAddAssignmentButton"));
    QVERIFY(addButton);
    QVERIFY(emptyAddButton);
    QVERIFY(addButton->isEnabled());
    QVERIFY(emptyAddButton->isEnabled());
    QVERIFY(addButton->isVisibleTo(m_page));

    QTimer::singleShot(0, this, [this] {
        auto *messageBox =
            qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (!messageBox) {
            return;
        }
        m_noSubjectMessageShown =
            messageBox->text().contains(QStringLiteral("Add a subject"));
        messageBox->accept();
    });
    QTest::mouseClick(addButton, Qt::LeftButton);
    QVERIFY(m_noSubjectMessageShown);
}

void AssignmentUiTests::addsEditsFiltersAndDeletesAssignments()
{
    auto *addButton = m_page->findChild<QPushButton *>(
        QStringLiteral("addAssignmentButton"));
    auto *table = m_page->findChild<QTableWidget *>(
        QStringLiteral("assignmentsTable"));
    QVERIFY(addButton);
    QVERIFY(table);

    scheduleAssignmentDialog(QStringLiteral("Read chapter"), m_subjectId);
    addButton->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Read chapter"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("Biology"));
    QVERIFY(table->item(0, 2)->text().contains(QStringLiteral("Upcoming")));
    QCOMPARE(table->item(0, 3)->text(), QStringLiteral("High"));

    auto *actions = table->cellWidget(0, 5);
    QVERIFY(actions);
    QCOMPARE(actions->objectName(), QStringLiteral("assignmentActionWidget"));
    auto *actionLayout = qobject_cast<QHBoxLayout *>(actions->layout());
    QVERIFY(actionLayout);
    QCOMPARE(actionLayout->contentsMargins(), QMargins(8, 5, 8, 5));
    QCOMPARE(actionLayout->spacing(), 8);
    QVERIFY(table->rowHeight(0) >= 50);
    auto *editButton = actions->findChild<QPushButton *>(
        QStringLiteral("editAssignmentButton"));
    auto *deleteButton = actions->findChild<QPushButton *>(
        QStringLiteral("deleteAssignmentButton"));
    QVERIFY(editButton);
    QVERIFY(deleteButton);
    QCOMPARE(editButton->text(), QStringLiteral("Edit"));
    QCOMPARE(deleteButton->text(), QStringLiteral("Delete"));
    QVERIFY(editButton->isEnabled());
    QVERIFY(deleteButton->isEnabled());
    QVERIFY(editButton->isVisibleTo(table));
    QVERIFY(deleteButton->isVisibleTo(table));
    QVERIFY(editButton->width() >= editButton->minimumWidth());
    QVERIFY(deleteButton->width() >= deleteButton->minimumWidth());
    QVERIFY(table->columnWidth(5) >= editButton->minimumWidth()
            + deleteButton->minimumWidth() + 18);

    scheduleAssignmentDialog(QStringLiteral("Read chapter 2"), m_secondSubjectId,
                             QStringLiteral("In Progress"));
    editButton->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Read chapter 2"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("Chemistry"));
    QCOMPARE(table->item(0, 4)->text(), QStringLiteral("In Progress"));

    auto *subjectFilter = m_page->findChild<QComboBox *>(
        QStringLiteral("assignmentSubjectFilter"));
    auto *statusFilter = m_page->findChild<QComboBox *>(
        QStringLiteral("assignmentStatusFilter"));
    QVERIFY(subjectFilter);
    QVERIFY(statusFilter);
    subjectFilter->setCurrentIndex(subjectFilter->findData(m_subjectId));
    QVERIFY(table->isHidden());
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("assignmentsEmptyMessage"))->text().contains(
                QStringLiteral("filters")));
    subjectFilter->setCurrentIndex(0);
    statusFilter->setCurrentIndex(statusFilter->findData(QStringLiteral("In Progress")));
    QCOMPARE(table->rowCount(), 1);
    statusFilter->setCurrentIndex(statusFilter->findData(QStringLiteral("Completed")));
    QVERIFY(table->isHidden());

    subjectFilter->setCurrentIndex(0);
    statusFilter->setCurrentIndex(0);
    actions = table->cellWidget(0, 5);
    QVERIFY(actions);
    deleteButton = actions->findChild<QPushButton *>(
        QStringLiteral("deleteAssignmentButton"));
    QVERIFY(deleteButton);
    scheduleConfirmation(QMessageBox::No);
    deleteButton->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Read chapter 2"));

    scheduleConfirmation(QMessageBox::Yes);
    deleteButton->click();
    QVERIFY(table->isHidden());
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("assignmentsEmptyMessage"))->text().contains(
                QStringLiteral("No assignments")));
}

void AssignmentUiTests::displaysDeadlinesWithoutMarkingCompletedWorkOverdue()
{
    QVERIFY(insertAssignment(m_application->currentUserId(), m_subjectId,
                             QStringLiteral("Past task"), QStringLiteral("2000-01-02"),
                             QStringLiteral("Low"), QStringLiteral("Not Started")) > 0);
    QVERIFY(insertAssignment(m_application->currentUserId(), m_subjectId,
                             QStringLiteral("Completed task"), QStringLiteral("2000-01-01"),
                             QStringLiteral("Medium"), QStringLiteral("Completed")) > 0);

    delete m_page;
    m_page = new AssignmentsPage(m_application);
    m_page->show();
    QApplication::processEvents();
    auto *table = m_page->findChild<QTableWidget *>(
        QStringLiteral("assignmentsTable"));
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 2);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Completed task"));
    QVERIFY(!table->item(0, 2)->text().contains(QStringLiteral("Overdue")));
    QCOMPARE(table->item(0, 4)->text(), QStringLiteral("Completed"));
    QCOMPARE(table->item(1, 0)->text(), QStringLiteral("Past task"));
    QVERIFY(table->item(1, 2)->text().startsWith(QStringLiteral("Overdue:")));
}

void AssignmentUiTests::failedSaveKeepsDialogValuesAndCanRetry()
{
    auto *addButton = m_page->findChild<QPushButton *>(
        QStringLiteral("addAssignmentButton"));
    QVERIFY(addButton);
    QTimer::singleShot(0, this, [this] {
        auto *dialog = qobject_cast<AssignmentDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto *title = dialog->findChild<QLineEdit *>(
            QStringLiteral("assignmentTitleEdit"));
        auto *subjects = dialog->findChild<QComboBox *>(
            QStringLiteral("assignmentSubjectCombo"));
        auto *date = dialog->findChild<QDateEdit *>(
            QStringLiteral("assignmentDueDateEdit"));
        auto *timeEnabled = dialog->findChild<QCheckBox *>(
            QStringLiteral("assignmentDueTimeEnabled"));
        auto *time = dialog->findChild<QTimeEdit *>(
            QStringLiteral("assignmentDueTimeEdit"));
        auto *save = dialog->findChild<QPushButton *>(
            QStringLiteral("saveAssignmentButton"));
        if (!title || !subjects || !date || !timeEnabled || !time || !save) {
            return;
        }
        title->setText(QStringLiteral("Keep these values"));
        subjects->setCurrentIndex(subjects->findData(m_subjectId));
        date->setDate(QDate(2098, 12, 20));
        timeEnabled->setChecked(true);
        time->setTime(QTime(14, 25));
        m_application->databaseManager()->close();
        QTimer::singleShot(0, this, [this] {
            auto *messageBox =
                qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (messageBox) {
                messageBox->accept();
            }
        });
        save->click();

        auto *activeDialog =
            qobject_cast<AssignmentDialog *>(QApplication::activeModalWidget());
        auto *retainedTitle = activeDialog ? activeDialog->findChild<QLineEdit *>(
            QStringLiteral("assignmentTitleEdit")) : nullptr;
        auto *retainedDate = activeDialog ? activeDialog->findChild<QDateEdit *>(
            QStringLiteral("assignmentDueDateEdit")) : nullptr;
        auto *retainedTimeEnabled = activeDialog ? activeDialog->findChild<QCheckBox *>(
            QStringLiteral("assignmentDueTimeEnabled")) : nullptr;
        auto *retainedTime = activeDialog ? activeDialog->findChild<QTimeEdit *>(
            QStringLiteral("assignmentDueTimeEdit")) : nullptr;
        m_saveFailureRetainedInput =
            activeDialog == dialog && retainedTitle && retainedDate
            && retainedTimeEnabled && retainedTime
            && retainedTitle->text() == QStringLiteral("Keep these values")
            && retainedDate->date() == QDate(2098, 12, 20)
            && retainedTimeEnabled->isChecked()
            && retainedTime->time() == QTime(14, 25);

        if (!m_application->initialize()) {
            dialog->reject();
            return;
        }
        save->click();
    });

    addButton->click();
    QVERIFY(m_saveFailureRetainedInput);
    auto *table = m_page->findChild<QTableWidget *>(
        QStringLiteral("assignmentsTable"));
    QCOMPARE(table->rowCount(), 1);
    QVERIFY(table->item(0, 2)->text().contains(
        QStringLiteral("2098-12-20 at 14:25")));
}

void AssignmentUiTests::failedEditKeepsDialogValuesAndCanRetry()
{
    QVERIFY(insertAssignment(m_application->currentUserId(), m_subjectId,
                             QStringLiteral("Existing assignment"),
                             QStringLiteral("2099-01-01"),
                             QStringLiteral("Low"),
                             QStringLiteral("Not Started")) > 0);
    delete m_page;
    m_page = new AssignmentsPage(m_application);
    m_page->show();
    QApplication::processEvents();

    auto *table = m_page->findChild<QTableWidget *>(
        QStringLiteral("assignmentsTable"));
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 1);
    auto *editButton = table->cellWidget(0, 5)->findChild<QPushButton *>(
        QStringLiteral("editAssignmentButton"));
    QVERIFY(editButton);

    QTimer::singleShot(0, this, [this] {
        auto *dialog = qobject_cast<AssignmentDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto *title = dialog->findChild<QLineEdit *>(
            QStringLiteral("assignmentTitleEdit"));
        auto *date = dialog->findChild<QDateEdit *>(
            QStringLiteral("assignmentDueDateEdit"));
        auto *save = dialog->findChild<QPushButton *>(
            QStringLiteral("saveAssignmentButton"));
        if (!title || !date || !save) {
            return;
        }
        title->setText(QStringLiteral("Edited assignment"));
        date->setDate(QDate(2098, 11, 19));
        m_application->databaseManager()->close();
        QTimer::singleShot(0, this, [] {
            if (auto *messageBox =
                    qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                messageBox->accept();
            }
        });
        save->click();

        auto *activeDialog =
            qobject_cast<AssignmentDialog *>(QApplication::activeModalWidget());
        auto *retainedTitle = activeDialog ? activeDialog->findChild<QLineEdit *>(
            QStringLiteral("assignmentTitleEdit")) : nullptr;
        auto *retainedDate = activeDialog ? activeDialog->findChild<QDateEdit *>(
            QStringLiteral("assignmentDueDateEdit")) : nullptr;
        m_saveFailureRetainedInput =
            activeDialog == dialog && retainedTitle && retainedDate
            && retainedTitle->text() == QStringLiteral("Edited assignment")
            && retainedDate->date() == QDate(2098, 11, 19);

        if (!m_application->initialize()) {
            dialog->reject();
            return;
        }
        save->click();
    });
    editButton->click();

    QVERIFY(m_saveFailureRetainedInput);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Edited assignment"));
    QVERIFY(table->item(0, 2)->text().contains(QStringLiteral("2098-11-19")));
}

QTEST_MAIN(AssignmentUiTests)

#include "assignment_ui_tests.moc"
