#ifndef EXIFREADER_H
#define EXIFREADER_H

#include <QString>
#include <QDateTime>
#include <QVariantMap>

struct ExifData {
    QDateTime dateTaken;
    double latitude = 0.0;
    double longitude = 0.0;
    bool hasGps = false;
    QString cameraModel;
    QString cameraMake;
    int width = 0;
    int height = 0;
    int orientation = 1;
};

// Lightweight EXIF parser — reads only the tags we need for wiki entries.
// Supports JPEG and TIFF containers. No external library required.
class ExifReader
{
public:
    static ExifData read(const QString &filePath);

private:
    static ExifData parseExifFromJpeg(const QByteArray &data);
    static ExifData parseExifFromTiff(const QByteArray &data, int offset);
    static QVariantMap parseIfdEntries(const QByteArray &data, int ifdOffset, bool littleEndian, int tiffStart);
    static double parseDegMinSec(const QByteArray &data, int offset, bool littleEndian, int tiffStart);
    static QString readAsciiString(const QByteArray &data, int offset, int count);
    static uint16_t readU16(const QByteArray &data, int offset, bool littleEndian);
    static uint32_t readU32(const QByteArray &data, int offset, bool littleEndian);
    static double readRational(const QByteArray &data, int offset, bool littleEndian);
};

#endif // EXIFREADER_H
