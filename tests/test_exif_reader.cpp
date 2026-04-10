#include <gtest/gtest.h>
#include "core/ExifReader.h"
#include <QFile>
#include <QDir>
#include <QTemporaryDir>

class ExifReaderTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;

    // Create a minimal valid JPEG file (SOI + EOI, no EXIF)
    QString createMinimalJpeg(const QString &name = "test.jpg")
    {
        QString path = m_tempDir.filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        // JPEG SOI marker + EOI marker
        const char data[] = { '\xFF', '\xD8', '\xFF', '\xD9' };
        f.write(data, 4);
        f.close();
        return path;
    }

    // Create a completely non-JPEG file
    QString createNonJpeg(const QString &name = "test.txt")
    {
        QString path = m_tempDir.filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write("This is not an image file");
        f.close();
        return path;
    }
};

TEST_F(ExifReaderTest, ReturnsEmptyForNonexistentFile)
{
    ExifData result = ExifReader::read("/nonexistent/path/photo.jpg");

    EXPECT_TRUE(result.dateTaken.isNull());
    EXPECT_FALSE(result.hasGps);
    EXPECT_EQ(result.latitude, 0.0);
    EXPECT_EQ(result.longitude, 0.0);
    EXPECT_TRUE(result.cameraModel.isEmpty());
    EXPECT_TRUE(result.cameraMake.isEmpty());
    EXPECT_EQ(result.width, 0);
    EXPECT_EQ(result.height, 0);
}

TEST_F(ExifReaderTest, ReturnsEmptyForNonJpegFile)
{
    QString path = createNonJpeg();
    ExifData result = ExifReader::read(path);

    EXPECT_TRUE(result.dateTaken.isNull());
    EXPECT_FALSE(result.hasGps);
}

TEST_F(ExifReaderTest, ReturnsEmptyForMinimalJpegWithoutExif)
{
    QString path = createMinimalJpeg();
    ExifData result = ExifReader::read(path);

    // A valid JPEG without APP1/EXIF should return empty data
    EXPECT_TRUE(result.dateTaken.isNull());
    EXPECT_FALSE(result.hasGps);
    EXPECT_TRUE(result.cameraModel.isEmpty());
}

TEST_F(ExifReaderTest, ReturnsEmptyForEmptyFile)
{
    QString path = m_tempDir.filePath("empty.jpg");
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.close();

    ExifData result = ExifReader::read(path);

    EXPECT_TRUE(result.dateTaken.isNull());
    EXPECT_FALSE(result.hasGps);
}

TEST_F(ExifReaderTest, DefaultOrientationIsOne)
{
    ExifData result;
    EXPECT_EQ(result.orientation, 1);
}

TEST_F(ExifReaderTest, DefaultGpsCoordinatesAreZero)
{
    ExifData result;
    EXPECT_DOUBLE_EQ(result.latitude, 0.0);
    EXPECT_DOUBLE_EQ(result.longitude, 0.0);
}

TEST_F(ExifReaderTest, DefaultDimensionsAreZero)
{
    ExifData result;
    EXPECT_EQ(result.width, 0);
    EXPECT_EQ(result.height, 0);
}
