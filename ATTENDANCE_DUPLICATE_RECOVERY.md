# Attendance duplicate recovery

NEXUS may stop during database initialization if an older database contains
more than one attendance row for the same `(user_id, subject_id, date)` key.
Startup does not change those rows. The recovery workflow is available by
answering **Yes** to the startup prompt or by launching:

```text
NEXUS.exe --recover-attendance-duplicates
```

The recovery screen lists each conflicting user's and subject's names and IDs,
the date, each record ID, and its status. The resolution policy is explicit:
the user must select exactly one existing record to retain in every group.
That chosen row is retained verbatim; every other row in that same key group is
deleted from the active database. NEXUS does not choose a row, merge fields,
or change the retained row. A final confirmation summarizes this deletion policy.

Before changing attendance, NEXUS creates a uniquely named backup beside the
database using SQLite `VACUUM INTO`, verifies the backup with
`PRAGMA integrity_check`, and compares every user-table row count to the source.
If backup creation or verification fails, no record is deleted. The recovery
operation then acquires an immediate write transaction, checks that no other
connection changed the inspected snapshot, verifies the chosen IDs and group
membership again, checks foreign keys and subject ownership, and validates the
records against the Attendance model. It deletes only the unselected members,
rechecks for duplicates, and retries the unique-index migration before commit.
Any failure rolls the transaction back. A failed attempt may leave its complete,
verified backup in place; backups are never automatically removed.

On success, restart NEXUS. The backup path is shown in the completion message.
Do not delete the backup until you have verified the recovered database and
made any separate backup required by your own retention policy.

## Manual recovery

If the recovery screen refuses a database because its records or foreign keys
are invalid, keep the original database unchanged and work on a separate copy.
Use SQLite's online backup command rather than copying an open database file:

```text
sqlite3 nexus.db ".backup 'nexus-before-attendance-recovery.db'"
```

Inspect every conflict and its complete record details before deciding which
record to retain:

```sql
SELECT a.id, a.user_id, u.name AS user_name,
       a.subject_id, s.name AS subject_name, a.date, a.status
FROM attendance AS a
LEFT JOIN users AS u ON u.id = a.user_id
LEFT JOIN subjects AS s ON s.id = a.subject_id
WHERE EXISTS (
    SELECT 1
    FROM attendance AS d
    WHERE d.user_id = a.user_id
      AND d.subject_id = a.subject_id
      AND d.date = a.date
    GROUP BY d.user_id, d.subject_id, d.date
    HAVING COUNT(*) > 1
)
ORDER BY a.user_id, a.subject_id, a.date, a.id;
```

For each duplicate key, decide explicitly which single record to keep. On the
working copy only, perform the selected deletions and index creation in one
transaction, replacing the example values with the reviewed IDs and exact key:

```sql
PRAGMA foreign_keys = ON;
BEGIN IMMEDIATE;
DELETE FROM attendance
WHERE user_id = 1 AND subject_id = 10 AND date = '2026-10-19'
  AND id IN (12, 13);
SELECT user_id, subject_id, date, COUNT(*)
FROM attendance
GROUP BY user_id, subject_id, date
HAVING COUNT(*) > 1;
CREATE UNIQUE INDEX idx_attendance_user_subject_date
    ON attendance(user_id, subject_id, date);
PRAGMA foreign_key_check;
COMMIT;
```

The `DELETE` list must contain every reviewed duplicate ID except the explicitly
selected record. If any query or validation fails, issue `ROLLBACK` instead of
`COMMIT`. Inspect the result rows from both validation queries before committing:
the duplicate query and `PRAGMA foreign_key_check` must return no rows. Do not
use this example on the original database without a verified backup and do not
proceed if the selected row, ownership, date, status, or foreign-key
relationships are uncertain. In that case, retain the original and seek a
project maintainer's help.
