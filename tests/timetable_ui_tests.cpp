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
#include <QTime>
#include <QTimeEdit>
#include <QTimer>

#include "../src/core/application.h"
#include "../src/ui/dialogs/timetable_entry_dialog.h"
#include "../src/ui/pages/timetable_page.h"

class TimetableUiTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    TimetablePage *m_page{nullptr};
    int m_firstSubjectId{0};
    int m_secondSubjectId{0};
    int m_otherUsersSubjectId{0};
    int m_otherUserId{0};
    QString m_warningText;
    bool m_confirmationOpened{false};
    bool m_invalidValuesRetained{false};
    bool m_overlapValuesRetained{false};

    int insertUser(const QString &name, const QString &email);
    int insertSubject(int userId, const QString &name);
    void scheduleEntryDialog(int subjectIndex, int dayOfWeek,
                             const QString &startTime, const QString &endTime,
                             const QString &room);
    void scheduleMessageBox(QMessageBox::StandardButton button);

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void initializesWithEmptyStateAndCurrentUserSubjects();
    void addsEntriesAndFiltersWeekdays();
    void editsEntryAndRefreshesSelectedDay();
    void deletesEntry();
    void reportsInvalidTimeRange();
    void rejectsOverlapAndSavesAfterCorrection();
    void reportsDatabaseUnavailable();
};

void TimetableUiTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("timetable_ui_tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    m_otherUserId = insertUser(QStringLiteral("Other User"),
                               QStringLiteral("other@example.test"));
    QVERIFY(m_otherUserId > 0);
}

void TimetableUiTests::init()
{
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM timetable")));
    QVERIFY(query.exec(QStringLiteral("DELETE FROM subjects")));

    const int currentUserId = m_application->currentUserId();
    m_firstSubjectId = insertSubject(currentUserId, QStringLiteral("Data Structures"));
    m_secondSubjectId = insertSubject(currentUserId, QStringLiteral("Operating Systems"));
    m_otherUsersSubjectId = insertSubject(m_otherUserId,
                                          QStringLiteral("Private Subject"));
    QVERIFY(m_firstSubjectId > 0);
    QVERIFY(m_secondSubjectId > 0);
    QVERIFY(m_otherUsersSubjectId > 0);

    m_page = new TimetablePage(m_application);
    m_page->show();
    QApplication::processEvents();
    m_warningText.clear();
    m_confirmationOpened = false;
    m_invalidValuesRetained = false;
    m_overlapValuesRetained = false;
}

void TimetableUiTests::cleanup()
{
    delete m_page;
    m_page = nullptr;
}

void TimetableUiTests::cleanupTestCase()
{
    delete m_application;
    m_application = nullptr;
}

int TimetableUiTests::insertUser(const QString &name, const QString &email)
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
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

int TimetableUiTests::insertSubject(int userId, const QString &name)
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
    if (!query.exec()) {
        return 0;
    }
    return query.lastInsertId().toInt();
}

void TimetableUiTests::scheduleEntryDialog(int subjectIndex, int dayOfWeek,
                                           const QString &startTime,
                                           const QString &endTime,
                                           const QString &room)
{
    QTimer::singleShot(0, this, [this, subjectIndex, dayOfWeek, startTime,
                                 endTime, room] {
        auto *dialog = qobject_cast<TimetableEntryDialog *>(
            QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto *subjects =
            dialog->findChild<QComboBox *>(QStringLiteral("timetableSubjectCombo"));
        auto *weekdays = dialog->findChild<QComboBox *>(
            QStringLiteral("timetableEntryWeekdayCombo"));
        auto *start = dialog->findChild<QTimeEdit *>(
            QStringLiteral("timetableStartTimeEdit"));
        auto *end = dialog->findChild<QTimeEdit *>(
            QStringLiteral("timetableEndTimeEdit"));
        auto *roomEdit =
            dialog->findChild<QLineEdit *>(QStringLiteral("timetableRoomEdit"));
        auto *save =
            dialog->findChild<QPushButton *>(QStringLiteral("saveTimetableButton"));
        if (!subjects || !weekdays || !start || !end || !roomEdit || !save) {
            return;
        }
        if (subjectIndex >= 0) {
            subjects->setCurrentIndex(subjectIndex);
        }
        weekdays->setCurrentIndex(weekdays->findData(dayOfWeek));
        start->setTime(QTime::fromString(startTime, QStringLiteral("HH:mm")));
        end->setTime(QTime::fromString(endTime, QStringLiteral("HH:mm")));
        roomEdit->setText(room);
        if (QTime::fromString(startTime, QStringLiteral("HH:mm"))
            >= QTime::fromString(endTime, QStringLiteral("HH:mm"))) {
            QTimer::singleShot(0, this, [this] {
                auto *messageBox = qobject_cast<QMessageBox *>(
                    QApplication::activeModalWidget());
                if (messageBox) {
                    m_warningText = messageBox->text();
                    messageBox->accept();
                }
            });
        }
        save->click();
        if (QTime::fromString(startTime, QStringLiteral("HH:mm"))
            >= QTime::fromString(endTime, QStringLiteral("HH:mm"))) {
            auto *dialogAfterError = qobject_cast<TimetableEntryDialog *>(
                QApplication::activeModalWidget());
            if (dialogAfterError) {
                auto *retainedStart = dialogAfterError->findChild<QTimeEdit *>(
                    QStringLiteral("timetableStartTimeEdit"));
                auto *retainedEnd = dialogAfterError->findChild<QTimeEdit *>(
                    QStringLiteral("timetableEndTimeEdit"));
                m_invalidValuesRetained =
                    retainedStart && retainedEnd
                    && retainedStart->time()
                        == QTime::fromString(startTime, QStringLiteral("HH:mm"))
                    && retainedEnd->time()
                        == QTime::fromString(endTime, QStringLiteral("HH:mm"));
                dialogAfterError->reject();
            }
        }
    });
}

void TimetableUiTests::scheduleMessageBox(QMessageBox::StandardButton button)
{
    QTimer::singleShot(0, this, [this, button] {
        auto *messageBox =
            qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (!messageBox) {
            return;
        }
        m_confirmationOpened = true;
        messageBox->button(button)->click();
    });
}

void TimetableUiTests::initializesWithEmptyStateAndCurrentUserSubjects()
{
    auto *weekday =
        m_page->findChild<QComboBox *>(QStringLiteral("timetableWeekdayCombo"));
    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("timetableTable"));
    auto *emptyMessage =
        m_page->findChild<QLabel *>(QStringLiteral("timetableEmptyMessage"));
    auto *addButton =
        m_page->findChild<QPushButton *>(QStringLiteral("addTimetableButton"));
    QVERIFY(weekday);
    QVERIFY(table);
    QVERIFY(emptyMessage);
    QVERIFY(addButton);
    QCOMPARE(weekday->count(), 7);
    QCOMPARE(weekday->currentData().toInt(), 1);
    for (int index = 0; index < weekday->count(); ++index) {
        QCOMPARE(weekday->itemData(index).toInt(), index + 1);
    }
    QVERIFY(table->isHidden());
    QVERIFY(emptyMessage->text().contains(QStringLiteral("No timetable entries")));

    int observedSubjectCount = 0;
    bool otherUsersSubjectWasOffered = false;
    QTimer::singleShot(0, this, [&] {
        auto *dialog = qobject_cast<TimetableEntryDialog *>(
            QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto *subjects = dialog->findChild<QComboBox *>(
            QStringLiteral("timetableSubjectCombo"));
        if (subjects) {
            observedSubjectCount = subjects->count();
            otherUsersSubjectWasOffered =
                subjects->findData(m_otherUsersSubjectId) >= 0;
        }
        dialog->reject();
    });
    addButton->click();
    QCOMPARE(observedSubjectCount, 2);
    QVERIFY(!otherUsersSubjectWasOffered);
}

void TimetableUiTests::addsEntriesAndFiltersWeekdays()
{
    auto *addButton =
        m_page->findChild<QPushButton *>(QStringLiteral("addTimetableButton"));
    auto *weekday =
        m_page->findChild<QComboBox *>(QStringLiteral("timetableWeekdayCombo"));
    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("timetableTable"));
    QVERIFY(addButton);
    QVERIFY(weekday);
    QVERIFY(table);

    scheduleEntryDialog(0, 1, QStringLiteral("09:15"), QStringLiteral("10:00"),
                        QStringLiteral("A-101"));
    addButton->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Data Structures"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("09:15 - 10:00"));
    QCOMPARE(table->item(0, 2)->text(), QStringLiteral("A-101"));

    weekday->setCurrentIndex(weekday->findData(2));
    QVERIFY(table->isHidden());
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("timetableEmptyMessage"))->text().contains(
                QStringLiteral("Tuesday")));
    scheduleEntryDialog(1, 2, QStringLiteral("11:00"), QStringLiteral("12:00"),
                        QStringLiteral("B-202"));
    addButton->click();
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Operating Systems"));

    weekday->setCurrentIndex(weekday->findData(1));
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Data Structures"));
}

void TimetableUiTests::editsEntryAndRefreshesSelectedDay()
{
    auto *addButton =
        m_page->findChild<QPushButton *>(QStringLiteral("addTimetableButton"));
    QVERIFY(addButton);
    scheduleEntryDialog(0, 1, QStringLiteral("09:00"), QStringLiteral("10:00"),
                        QStringLiteral("A-101"));
    addButton->click();

    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("timetableTable"));
    QVERIFY(table);
    auto *actions = table->cellWidget(0, 3);
    QVERIFY(actions);
    auto *editButton =
        actions->findChild<QPushButton *>(QStringLiteral("editTimetableButton"));
    QVERIFY(editButton);
    scheduleEntryDialog(1, 3, QStringLiteral("13:00"), QStringLiteral("14:30"),
                        QStringLiteral("C-303"));
    editButton->click();

    auto *weekday =
        m_page->findChild<QComboBox *>(QStringLiteral("timetableWeekdayCombo"));
    QCOMPARE(weekday->currentData().toInt(), 3);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("Operating Systems"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("13:00 - 14:30"));
    QCOMPARE(table->item(0, 2)->text(), QStringLiteral("C-303"));
}

void TimetableUiTests::deletesEntry()
{
    auto *addButton =
        m_page->findChild<QPushButton *>(QStringLiteral("addTimetableButton"));
    QVERIFY(addButton);
    scheduleEntryDialog(0, 1, QStringLiteral("09:00"), QStringLiteral("10:00"),
                        QStringLiteral("A-101"));
    addButton->click();

    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("timetableTable"));
    auto *actions = table->cellWidget(0, 3);
    QVERIFY(actions);
    auto *deleteButton =
        actions->findChild<QPushButton *>(QStringLiteral("deleteTimetableButton"));
    QVERIFY(deleteButton);
    scheduleMessageBox(QMessageBox::Yes);
    deleteButton->click();
    QVERIFY(m_confirmationOpened);
    QVERIFY(table->isHidden());
    QVERIFY(m_page->findChild<QLabel *>(
                QStringLiteral("timetableEmptyMessage"))->text().contains(
                QStringLiteral("No timetable entries")));
}

void TimetableUiTests::reportsInvalidTimeRange()
{
    auto *addButton =
        m_page->findChild<QPushButton *>(QStringLiteral("addTimetableButton"));
    QVERIFY(addButton);
    scheduleEntryDialog(0, 1, QStringLiteral("11:00"), QStringLiteral("10:00"),
                        QStringLiteral("A-101"));
    addButton->click();

    QVERIFY(m_warningText.contains(QStringLiteral("earlier")));
    QVERIFY(m_invalidValuesRetained);
    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("timetableTable"));
    QVERIFY(table->isHidden());
}

void TimetableUiTests::rejectsOverlapAndSavesAfterCorrection()
{
    auto *addButton =
        m_page->findChild<QPushButton *>(QStringLiteral("addTimetableButton"));
    auto *table = m_page->findChild<QTableWidget *>(QStringLiteral("timetableTable"));
    QVERIFY(addButton);
    QVERIFY(table);

    scheduleEntryDialog(0, 1, QStringLiteral("10:00"), QStringLiteral("11:00"),
                        QStringLiteral("A-101"));
    addButton->click();
    QCOMPARE(table->rowCount(), 1);

    QTimer::singleShot(0, this, [this] {
        auto *dialog = qobject_cast<TimetableEntryDialog *>(
            QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto *subjects = dialog->findChild<QComboBox *>(
            QStringLiteral("timetableSubjectCombo"));
        auto *weekdays = dialog->findChild<QComboBox *>(
            QStringLiteral("timetableEntryWeekdayCombo"));
        auto *start = dialog->findChild<QTimeEdit *>(
            QStringLiteral("timetableStartTimeEdit"));
        auto *end = dialog->findChild<QTimeEdit *>(
            QStringLiteral("timetableEndTimeEdit"));
        auto *room = dialog->findChild<QLineEdit *>(
            QStringLiteral("timetableRoomEdit"));
        auto *save = dialog->findChild<QPushButton *>(
            QStringLiteral("saveTimetableButton"));
        if (!subjects || !weekdays || !start || !end || !room || !save) {
            return;
        }

        subjects->setCurrentIndex(1);
        weekdays->setCurrentIndex(weekdays->findData(1));
        start->setTime(QTime(10, 30));
        end->setTime(QTime(11, 30));
        room->setText(QStringLiteral("B-202"));

        QTimer::singleShot(0, this, [this] {
            auto *messageBox = qobject_cast<QMessageBox *>(
                QApplication::activeModalWidget());
            if (messageBox) {
                m_warningText = messageBox->text();
                messageBox->accept();
            }
        });
        save->click();

        auto *dialogAfterError = qobject_cast<TimetableEntryDialog *>(
            QApplication::activeModalWidget());
        if (!dialogAfterError) {
            return;
        }
        auto *retainedSubject = dialogAfterError->findChild<QComboBox *>(
            QStringLiteral("timetableSubjectCombo"));
        auto *retainedWeekday = dialogAfterError->findChild<QComboBox *>(
            QStringLiteral("timetableEntryWeekdayCombo"));
        auto *retainedStart = dialogAfterError->findChild<QTimeEdit *>(
            QStringLiteral("timetableStartTimeEdit"));
        auto *retainedEnd = dialogAfterError->findChild<QTimeEdit *>(
            QStringLiteral("timetableEndTimeEdit"));
        auto *retainedRoom = dialogAfterError->findChild<QLineEdit *>(
            QStringLiteral("timetableRoomEdit"));
        m_overlapValuesRetained =
            retainedSubject && retainedWeekday && retainedStart && retainedEnd
            && retainedRoom
            && retainedSubject->currentData().toInt() == m_secondSubjectId
            && retainedWeekday->currentData().toInt() == 1
            && retainedStart->time() == QTime(10, 30)
            && retainedEnd->time() == QTime(11, 30)
            && retainedRoom->text() == QStringLiteral("B-202");

        retainedStart->setTime(QTime(11, 0));
        retainedEnd->setTime(QTime(12, 0));
        auto *correctedSave = dialogAfterError->findChild<QPushButton *>(
            QStringLiteral("saveTimetableButton"));
        correctedSave->click();
    });
    addButton->click();

    QCOMPARE(m_warningText,
             QStringLiteral("This timetable entry overlaps another entry on "
                            "the selected day."));
    QVERIFY(m_overlapValuesRetained);
    QCOMPARE(table->rowCount(), 2);
    QCOMPARE(table->item(1, 0)->text(), QStringLiteral("Operating Systems"));
    QCOMPARE(table->item(1, 1)->text(), QStringLiteral("11:00 - 12:00"));
    QCOMPARE(table->item(1, 2)->text(), QStringLiteral("B-202"));
}

void TimetableUiTests::reportsDatabaseUnavailable()
{
    delete m_page;
    m_page = nullptr;
    m_application->databaseManager()->close();

    m_page = new TimetablePage(m_application);
    auto *emptyMessage =
        m_page->findChild<QLabel *>(QStringLiteral("timetableEmptyMessage"));
    QVERIFY(emptyMessage);
    QVERIFY(emptyMessage->text().contains(QStringLiteral("Could not load subjects")));
    QVERIFY(emptyMessage->text().contains(QStringLiteral("database"),
                                          Qt::CaseInsensitive));

    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
}

QTEST_MAIN(TimetableUiTests)
#include "timetable_ui_tests.moc"
