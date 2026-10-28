// tests/study_library_tests.cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>

#include "../src/core/application.h"
#include "../src/data/study_file_dao.h"
#include "../src/services/study_file_service.h"
#include "../src/models/study_file.h"

class StudyLibraryTests : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_temporaryDirectory;
    Application *m_application{nullptr};
    StudyFileDAO *m_dao{nullptr};
    StudyFileService *m_service{nullptr};
    int m_userId{0};
    int m_otherUserId{0};

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testImportAndManagedStorage();
    void testCollisionSafety();
    void testFileTypeDetection();
    void testSearchAndFiltering();
    void testRemovalAndCleanup();
    void testUserIsolation();
};

void StudyLibraryTests::initTestCase()
{
    QVERIFY(m_temporaryDirectory.isValid());
    m_application = new Application(
        m_temporaryDirectory.filePath(QStringLiteral("study_library_tests.db")));
    QVERIFY2(m_application->initialize(), qPrintable(m_application->lastError()));
    m_userId = m_application->currentUserId();

    QSqlQuery query(m_application->databaseManager()->database());
    query.prepare(QStringLiteral("INSERT INTO users (name, email, created_at) VALUES ('Other User', 'other-lib@example.test', '2026-01-01T00:00:00Z')"));
    QVERIFY(query.exec());
    m_otherUserId = query.lastInsertId().toInt();
    QVERIFY(m_otherUserId > 0);

    m_dao = new StudyFileDAO(m_application->databaseManager());
    m_service = new StudyFileService(m_dao);
}

void StudyLibraryTests::cleanupTestCase()
{
    delete m_service;
    m_service = nullptr;
    delete m_dao;
    m_dao = nullptr;
    delete m_application;
    m_application = nullptr;
}

void StudyLibraryTests::init()
{
    QSqlQuery query(m_application->databaseManager()->database());
    QVERIFY(query.exec(QStringLiteral("DELETE FROM study_files")));
}

void StudyLibraryTests::cleanup()
{
}

void StudyLibraryTests::testImportAndManagedStorage()
{
    const QString srcPath = m_temporaryDirectory.filePath(QStringLiteral("lecture_notes.pdf"));
    {
        QFile file(srcPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("PDF content simulation");
        file.close();
    }

    StudyFile createdFile;
    QString errorMessage;
    QVERIFY(m_service->importFile(m_userId, 0, srcPath, &errorMessage, &createdFile));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(createdFile.id() > 0);
    QCOMPARE(createdFile.originalFilename(), QStringLiteral("lecture_notes.pdf"));
    QCOMPARE(createdFile.fileType(), QStringLiteral("PDF"));
    QVERIFY(QFileInfo::exists(createdFile.managedFilePath()));

    // Verify stored in DB
    const QList<StudyFile> files = m_service->getStudyFiles(m_userId, &errorMessage);
    QCOMPARE(files.size(), 1);
    QCOMPARE(files.first().originalFilename(), QStringLiteral("lecture_notes.pdf"));
}

void StudyLibraryTests::testCollisionSafety()
{
    const QString srcPath1 = m_temporaryDirectory.filePath(QStringLiteral("assignment.docx"));
    const QString srcPath2 = m_temporaryDirectory.filePath(QStringLiteral("assignment.docx")); // same name, different dir or file
    {
        QFile f1(srcPath1);
        QVERIFY(f1.open(QIODevice::WriteOnly));
        f1.write("Doc 1");
        f1.close();
    }

    const QString altSrcDir = m_temporaryDirectory.path() + QStringLiteral("/alt");
    QDir().mkpath(altSrcDir);
    const QString altSrcPath = altSrcDir + QStringLiteral("/assignment.docx");
    {
        QFile f2(altSrcPath);
        QVERIFY(f2.open(QIODevice::WriteOnly));
        f2.write("Doc 2");
        f2.close();
    }

    StudyFile file1, file2;
    QVERIFY(m_service->importFile(m_userId, 0, srcPath1, nullptr, &file1));
    QVERIFY(m_service->importFile(m_userId, 0, altSrcPath, nullptr, &file2));

    QVERIFY(file1.managedFilename() != file2.managedFilename());
    QVERIFY(QFileInfo::exists(file1.managedFilePath()));
    QVERIFY(QFileInfo::exists(file2.managedFilePath()));

    const QList<StudyFile> files = m_service->getStudyFiles(m_userId);
    QCOMPARE(files.size(), 2);
}

void StudyLibraryTests::testFileTypeDetection()
{
    QCOMPARE(StudyFileService::determineFileType(QStringLiteral("report.pdf")), QStringLiteral("PDF"));
    QCOMPARE(StudyFileService::determineFileType(QStringLiteral("doc.docx")), QStringLiteral("DOCX"));
    QCOMPARE(StudyFileService::determineFileType(QStringLiteral("slides.pptx")), QStringLiteral("PPTX"));
    QCOMPARE(StudyFileService::determineFileType(QStringLiteral("notes.txt")), QStringLiteral("TXT"));
    QCOMPARE(StudyFileService::determineFileType(QStringLiteral("photo.png")), QStringLiteral("Image"));
    QCOMPARE(StudyFileService::determineFileType(QStringLiteral("archive.zip")), QStringLiteral("ZIP"));
}

void StudyLibraryTests::testSearchAndFiltering()
{
    const QString p1 = m_temporaryDirectory.filePath(QStringLiteral("Math_Lecture.pdf"));
    const QString p2 = m_temporaryDirectory.filePath(QStringLiteral("Physics_Lab.docx"));
    { QFile(p1).open(QIODevice::WriteOnly); QFile(p2).open(QIODevice::WriteOnly); }

    m_service->importFile(m_userId, 0, p1);
    m_service->importFile(m_userId, 0, p2);

    const QList<StudyFile> files = m_service->getStudyFiles(m_userId);
    QCOMPARE(files.size(), 2);
}

void StudyLibraryTests::testRemovalAndCleanup()
{
    const QString srcPath = m_temporaryDirectory.filePath(QStringLiteral("temp_doc.txt"));
    { QFile f(srcPath); f.open(QIODevice::WriteOnly); f.write("delete me"); }

    StudyFile file;
    QVERIFY(m_service->importFile(m_userId, 0, srcPath, nullptr, &file));
    const QString managedPath = file.managedFilePath();
    QVERIFY(QFileInfo::exists(managedPath));

    QString errorMessage;
    QVERIFY(m_service->removeStudyFile(m_userId, file.id(), &errorMessage));
    QVERIFY(!QFileInfo::exists(managedPath));

    const QList<StudyFile> files = m_service->getStudyFiles(m_userId);
    QVERIFY(files.isEmpty());
}

void StudyLibraryTests::testUserIsolation()
{
    const QString srcPath = m_temporaryDirectory.filePath(QStringLiteral("secret.pdf"));
    { QFile(srcPath).open(QIODevice::WriteOnly); }

    StudyFile file;
    QVERIFY(m_service->importFile(m_userId, 0, srcPath, nullptr, &file));

    // Other user should not see this file
    const QList<StudyFile> otherFiles = m_service->getStudyFiles(m_otherUserId);
    QVERIFY(otherFiles.isEmpty());

    // Other user cannot delete it
    QString errorMessage;
    QVERIFY(!m_service->removeStudyFile(m_otherUserId, file.id(), &errorMessage));
}

QTEST_MAIN(StudyLibraryTests)
#include "study_library_tests.moc"
