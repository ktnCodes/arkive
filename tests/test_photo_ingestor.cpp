#include <gtest/gtest.h>
#include "core/PhotoIngestor.h"
#include "core/VaultManager.h"
#include "core/GeocodeCache.h"
#include <QTemporaryDir>
#include <QDir>
#include <QFile>

class PhotoIngestorTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    VaultManager m_vault;

    std::unique_ptr<GeocodeCache> m_geocode;
    std::unique_ptr<PhotoIngestor> m_ingestor;

    void SetUp() override
    {
        m_vault.ensureVaultExists();
        QString dbPath = m_tempDir.filePath("geocode.db");
        m_geocode = std::make_unique<GeocodeCache>(dbPath);
        m_ingestor = std::make_unique<PhotoIngestor>(&m_vault, m_geocode.get());
    }

    // Create dummy JPEG files (minimal SOI+EOI, no EXIF)
    void createDummyJpegs(const QString &dir, int count)
    {
        QDir d(dir);
        d.mkpath(".");
        for (int i = 0; i < count; ++i) {
            QFile f(d.filePath(QString("IMG_%1.jpg").arg(i, 4, 10, QChar('0'))));
            f.open(QIODevice::WriteOnly);
            const char data[] = { '\xFF', '\xD8', '\xFF', '\xD9' };
            f.write(data, 4);
            f.close();
        }
    }
};

// --- supportedExtensions ---

TEST_F(PhotoIngestorTest, SupportedExtensionsIncludesJpeg)
{
    QStringList exts = PhotoIngestor::supportedExtensions();
    EXPECT_TRUE(exts.contains("jpg"));
    EXPECT_TRUE(exts.contains("jpeg"));
}

TEST_F(PhotoIngestorTest, SupportedExtensionsIncludesCommonFormats)
{
    QStringList exts = PhotoIngestor::supportedExtensions();
    EXPECT_TRUE(exts.contains("png"));
    EXPECT_TRUE(exts.contains("heic"));
}

// --- locationKey ---

TEST_F(PhotoIngestorTest, LocationKeyGroupsNearbyCoordinates)
{
    // Two points within ~500m should get the same key
    QString key1 = PhotoIngestor::locationKey(30.480, -97.789);
    QString key2 = PhotoIngestor::locationKey(30.482, -97.790);
    EXPECT_EQ(key1, key2);
}

TEST_F(PhotoIngestorTest, LocationKeyDiffersForDistantPoints)
{
    QString key1 = PhotoIngestor::locationKey(30.480, -97.789);
    QString key2 = PhotoIngestor::locationKey(30.500, -97.789);
    EXPECT_NE(key1, key2);
}

// --- generateSlug ---

TEST_F(PhotoIngestorTest, GenerateSlugUsesDateOnly)
{
    PhotoGroup group;
    group.dateKey = "2025-10-17";
    // No place name

    QString slug = m_ingestor->generateSlug(group);
    EXPECT_EQ(slug, "2025-10-17");
}

TEST_F(PhotoIngestorTest, GenerateSlugAppendsPlaceName)
{
    PhotoGroup group;
    group.dateKey = "2025-10-17";
    group.placeName = "Lyndhurst Street, Austin, Williamson County";

    QString slug = m_ingestor->generateSlug(group);
    EXPECT_EQ(slug, "2025-10-17-lyndhurst-street");
}

TEST_F(PhotoIngestorTest, GenerateSlugHandlesSpecialCharacters)
{
    PhotoGroup group;
    group.dateKey = "2025-01-01";
    group.placeName = "Café & Bar #5, City";

    QString slug = m_ingestor->generateSlug(group);
    // Should only contain alphanumeric and hyphens
    EXPECT_FALSE(slug.contains("&"));
    EXPECT_FALSE(slug.contains("#"));
    EXPECT_TRUE(slug.startsWith("2025-01-01"));
}

// --- generateMarkdown ---

TEST_F(PhotoIngestorTest, GenerateMarkdownContainsTitle)
{
    PhotoGroup group;
    group.dateKey = "2025-10-17";
    group.placeName = "Austin, TX";
    group.filePaths = { "photo1.jpg" };
    group.earliestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 0));
    group.latestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 30));
    group.exifList = { ExifData{} };

    QString md = m_ingestor->generateMarkdown(group);
    EXPECT_TRUE(md.contains("# Photos: 2025-10-17"));
}

TEST_F(PhotoIngestorTest, GenerateMarkdownContainsSections)
{
    PhotoGroup group;
    group.dateKey = "2025-10-17";
    group.filePaths = { "photo1.jpg" };
    group.earliestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 0));
    group.latestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 30));
    group.exifList = { ExifData{} };

    QString md = m_ingestor->generateMarkdown(group);
    EXPECT_TRUE(md.contains("## Summary"));
    EXPECT_TRUE(md.contains("## Key Facts"));
    EXPECT_TRUE(md.contains("## Connections"));
    EXPECT_TRUE(md.contains("## Sources"));
}

TEST_F(PhotoIngestorTest, GenerateMarkdownIncludesConnectionBacklink)
{
    PhotoGroup group;
    group.dateKey = "2025-10-17";
    group.filePaths = { "photo1.jpg" };
    group.earliestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 0));
    group.latestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 0));
    group.exifList = { ExifData{} };

    QString md = m_ingestor->generateMarkdown(group);
    EXPECT_TRUE(md.contains("[[2025-10-17-conversations]]"));
}

TEST_F(PhotoIngestorTest, GenerateMarkdownIncludesCameraModel)
{
    PhotoGroup group;
    group.dateKey = "2025-10-17";
    group.filePaths = { "photo1.jpg" };
    group.earliestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 0));
    group.latestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 0));

    ExifData exif;
    exif.cameraModel = "iPhone 13 Pro";
    group.exifList = { exif };

    QString md = m_ingestor->generateMarkdown(group);
    EXPECT_TRUE(md.contains("iPhone 13 Pro"));
}

TEST_F(PhotoIngestorTest, GenerateMarkdownIncludesGpsCoords)
{
    PhotoGroup group;
    group.dateKey = "2025-10-17";
    group.filePaths = { "photo1.jpg" };
    group.earliestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 0));
    group.latestDate = QDateTime(QDate(2025, 10, 17), QTime(11, 0));

    ExifData exif;
    exif.hasGps = true;
    exif.latitude = 30.48;
    exif.longitude = -97.79;
    group.exifList = { exif };

    QString md = m_ingestor->generateMarkdown(group);
    EXPECT_TRUE(md.contains("GPS:"));
    EXPECT_TRUE(md.contains("30.48"));
}

// --- Initial state ---

TEST_F(PhotoIngestorTest, NotRunningByDefault)
{
    EXPECT_FALSE(m_ingestor->running());
    EXPECT_EQ(m_ingestor->progress(), 0);
    EXPECT_EQ(m_ingestor->totalFiles(), 0);
}

// --- Import from nonexistent folder ---

TEST_F(PhotoIngestorTest, ImportErrorForNonexistentFolder)
{
    bool errorEmitted = false;
    QObject::connect(m_ingestor.get(), &PhotoIngestor::importError,
                     [&errorEmitted](const QString &) { errorEmitted = true; });

    m_ingestor->importFromFolder("/nonexistent/folder");
    EXPECT_TRUE(errorEmitted);
    EXPECT_FALSE(m_ingestor->running());
}

// --- Import from empty folder ---

TEST_F(PhotoIngestorTest, ImportFinishesWithZeroForEmptyFolder)
{
    QString emptyDir = m_tempDir.filePath("empty");
    QDir().mkpath(emptyDir);

    int entries = -1;
    int photos = -1;
    QObject::connect(m_ingestor.get(), &PhotoIngestor::importFinished,
                     [&entries, &photos](int e, int p) { entries = e; photos = p; });

    m_ingestor->importFromFolder(emptyDir);
    EXPECT_EQ(entries, 0);
    EXPECT_EQ(photos, 0);
    EXPECT_FALSE(m_ingestor->running());
}
