#include "attendance_duplicate_recovery_dialog.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

AttendanceDuplicateRecoveryDialog::AttendanceDuplicateRecoveryDialog(
    const QString &databasePath, QWidget *parent)
    : QDialog(parent)
    , m_recovery(databasePath)
{
    setWindowTitle(QStringLiteral("Attendance Duplicate Recovery"));
    setMinimumSize(900, 460);

    auto *layout = new QVBoxLayout(this);
    auto *instructions = new QLabel(
        QStringLiteral(
            "Each group contains records with the same user, subject, and date. "
            "Choose exactly one record to retain in every group. All other records "
            "in that group will be deleted only after a verified database backup and "
            "your final confirmation."),
        this);
    instructions->setWordWrap(true);
    instructions->setObjectName(QStringLiteral("recoveryInstructions"));
    layout->addWidget(instructions);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("duplicateAttendanceTable"));
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("User"), QStringLiteral("User ID"),
        QStringLiteral("Subject"), QStringLiteral("Subject ID"),
        QStringLiteral("Date"), QStringLiteral("Record ID"),
        QStringLiteral("Status"), QStringLiteral("Retain")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(m_table);

    m_feedback = new QLabel(this);
    m_feedback->setObjectName(QStringLiteral("recoveryFeedback"));
    m_feedback->setWordWrap(true);
    layout->addWidget(m_feedback);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto *recoverButton = buttons->addButton(
        QStringLiteral("Back Up and Resolve"), QDialogButtonBox::AcceptRole);
    recoverButton->setObjectName(QStringLiteral("resolveAttendanceDuplicatesButton"));
    connect(recoverButton, &QPushButton::clicked,
            this, &AttendanceDuplicateRecoveryDialog::resolveSelectedRecords);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    populateConflicts();
}

QString AttendanceDuplicateRecoveryDialog::backupPath() const
{
    return m_backupPath;
}

void AttendanceDuplicateRecoveryDialog::populateConflicts()
{
    QString errorMessage;
    m_groups = m_recovery.findDuplicateGroups(&errorMessage);
    if (!errorMessage.isEmpty()) {
        m_feedback->setText(errorMessage);
        m_feedback->setStyleSheet(QStringLiteral("color: #e57373;"));
        m_table->setRowCount(0);
        if (auto *button = findChild<QPushButton *>(
                QStringLiteral("resolveAttendanceDuplicatesButton"))) {
            button->setEnabled(false);
        }
        return;
    }
    if (m_groups.isEmpty()) {
        m_feedback->setText(QStringLiteral(
            "No duplicate attendance records were found. Restart NEXUS to retry database initialization."));
        m_table->setRowCount(0);
        if (auto *button = findChild<QPushButton *>(
                QStringLiteral("resolveAttendanceDuplicatesButton"))) {
            button->setEnabled(false);
        }
        return;
    }

    int row = 0;
    m_table->setRowCount([&]() {
        int count = 0;
        for (const AttendanceDuplicateGroup &group : m_groups) {
            count += group.records.size();
        }
        return count;
    }());
    for (const AttendanceDuplicateGroup &group : m_groups) {
        const int firstRow = row;
        for (const AttendanceDuplicateRecord &record : group.records) {
            m_table->setItem(row, 0, new QTableWidgetItem(record.userName));
            m_table->setItem(row, 1, new QTableWidgetItem(QString::number(record.userId)));
            m_table->setItem(row, 2, new QTableWidgetItem(record.subjectName));
            m_table->setItem(row, 3, new QTableWidgetItem(QString::number(record.subjectId)));
            m_table->setItem(row, 4, new QTableWidgetItem(record.date));
            m_table->setItem(row, 5, new QTableWidgetItem(QString::number(record.id)));
            m_table->setItem(row, 6, new QTableWidgetItem(record.status));
            ++row;
        }
        auto *keepSelector = new QComboBox(m_table);
        keepSelector->setObjectName(QStringLiteral("keepRecordSelector_%1")
                                        .arg(group.records.first().id));
        keepSelector->addItem(QStringLiteral("Choose record to retain"), 0);
        for (const AttendanceDuplicateRecord &record : group.records) {
            keepSelector->addItem(
                QStringLiteral("Keep #%1 (%2)").arg(record.id).arg(record.status),
                record.id);
        }
        m_table->setCellWidget(firstRow, 7, keepSelector);
        if (group.records.size() > 1) {
            m_table->setSpan(firstRow, 7, group.records.size(), 1);
        }
    }
}

void AttendanceDuplicateRecoveryDialog::resolveSelectedRecords()
{
    QList<AttendanceDuplicateResolution> resolutions;
    for (const AttendanceDuplicateGroup &group : m_groups) {
        const QString selectorName = QStringLiteral("keepRecordSelector_%1")
                                         .arg(group.records.first().id);
        auto *selector = m_table->findChild<QComboBox *>(selectorName);
        if (!selector || selector->currentData().toInt() <= 0) {
            m_feedback->setText(QStringLiteral(
                "Choose one record to retain in every duplicate group before continuing."));
            m_feedback->setStyleSheet(QStringLiteral("color: #e57373;"));
            return;
        }
        AttendanceDuplicateResolution resolution;
        resolution.userId = group.userId;
        resolution.subjectId = group.subjectId;
        resolution.date = group.date;
        resolution.keepRecordId = selector->currentData().toInt();
        for (const AttendanceDuplicateRecord &record : group.records) {
            resolution.inspectedRecords.append(record);
        }
        resolutions.append(resolution);
    }

    const auto answer = QMessageBox::question(
        this, QStringLiteral("Confirm Attendance Recovery"),
        QStringLiteral(
            "A consistent SQLite backup will be created and verified first.\n\n"
            "For each group, only the record you selected will be retained. "
            "All other records in those groups will be permanently deleted from "
            "the active database. The backup will remain untouched.\n\n"
            "Continue with recovery?"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString errorMessage;
    if (!m_recovery.resolveDuplicates(resolutions, true, &m_backupPath,
                                      &errorMessage)) {
        m_feedback->setText(errorMessage);
        m_feedback->setStyleSheet(QStringLiteral("color: #e57373;"));
        return;
    }
    m_feedback->setText(QStringLiteral(
        "Recovery completed. The selected records were retained and the uniqueness "
        "index was verified. Restart NEXUS to continue."));
    m_feedback->setStyleSheet(QStringLiteral("color: #81c784;"));
    accept();
}
