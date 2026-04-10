#include "ExifReader.h"
#include <QFile>
#include <QDebug>

// EXIF tag IDs we care about
static constexpr uint16_t TAG_IMAGE_WIDTH       = 0x0100;
static constexpr uint16_t TAG_IMAGE_HEIGHT      = 0x0101;
static constexpr uint16_t TAG_MAKE              = 0x010F;
static constexpr uint16_t TAG_MODEL             = 0x0110;
static constexpr uint16_t TAG_ORIENTATION       = 0x0112;
static constexpr uint16_t TAG_DATETIME          = 0x0132;
static constexpr uint16_t TAG_DATETIME_ORIGINAL = 0x9003;
static constexpr uint16_t TAG_EXIF_IFD          = 0x8769;
static constexpr uint16_t TAG_GPS_IFD           = 0x8825;
static constexpr uint16_t TAG_GPS_LAT_REF       = 0x0001;
static constexpr uint16_t TAG_GPS_LAT           = 0x0002;
static constexpr uint16_t TAG_GPS_LON_REF       = 0x0003;
static constexpr uint16_t TAG_GPS_LON           = 0x0004;

// EXIF type sizes
static constexpr int TYPE_BYTE      = 1;
static constexpr int TYPE_ASCII     = 2;
static constexpr int TYPE_SHORT     = 3;
static constexpr int TYPE_LONG      = 4;
static constexpr int TYPE_RATIONAL  = 5;

static int typeSize(int type) {
    switch (type) {
        case TYPE_BYTE:     return 1;
        case TYPE_ASCII:    return 1;
        case TYPE_SHORT:    return 2;
        case TYPE_LONG:     return 4;
        case TYPE_RATIONAL: return 8;
        default:            return 1;
    }
}

uint16_t ExifReader::readU16(const QByteArray &data, int offset, bool littleEndian)
{
    if (offset + 1 >= data.size()) return 0;
    auto d = reinterpret_cast<const uint8_t*>(data.constData());
    if (littleEndian)
        return static_cast<uint16_t>(d[offset]) | (static_cast<uint16_t>(d[offset + 1]) << 8);
    else
        return (static_cast<uint16_t>(d[offset]) << 8) | static_cast<uint16_t>(d[offset + 1]);
}

uint32_t ExifReader::readU32(const QByteArray &data, int offset, bool littleEndian)
{
    if (offset + 3 >= data.size()) return 0;
    auto d = reinterpret_cast<const uint8_t*>(data.constData());
    if (littleEndian)
        return static_cast<uint32_t>(d[offset])
             | (static_cast<uint32_t>(d[offset + 1]) << 8)
             | (static_cast<uint32_t>(d[offset + 2]) << 16)
             | (static_cast<uint32_t>(d[offset + 3]) << 24);
    else
        return (static_cast<uint32_t>(d[offset]) << 24)
             | (static_cast<uint32_t>(d[offset + 1]) << 16)
             | (static_cast<uint32_t>(d[offset + 2]) << 8)
             | static_cast<uint32_t>(d[offset + 3]);
}

double ExifReader::readRational(const QByteArray &data, int offset, bool littleEndian)
{
    uint32_t num = readU32(data, offset, littleEndian);
    uint32_t den = readU32(data, offset + 4, littleEndian);
    if (den == 0) return 0.0;
    return static_cast<double>(num) / static_cast<double>(den);
}

QString ExifReader::readAsciiString(const QByteArray &data, int offset, int count)
{
    if (offset + count > data.size()) return {};
    // EXIF ASCII strings are null-terminated
    return QString::fromLatin1(data.constData() + offset, count).trimmed().remove(QChar('\0'));
}

double ExifReader::parseDegMinSec(const QByteArray &data, int offset, bool littleEndian, int tiffStart)
{
    double deg = readRational(data, tiffStart + offset, littleEndian);
    double min = readRational(data, tiffStart + offset + 8, littleEndian);
    double sec = readRational(data, tiffStart + offset + 16, littleEndian);
    return deg + min / 60.0 + sec / 3600.0;
}

QVariantMap ExifReader::parseIfdEntries(const QByteArray &data, int ifdOffset, bool littleEndian, int tiffStart)
{
    QVariantMap result;
    int abs = tiffStart + ifdOffset;
    if (abs + 2 > data.size()) return result;

    uint16_t entryCount = readU16(data, abs, littleEndian);
    abs += 2;

    for (int i = 0; i < entryCount; ++i) {
        int entryPos = abs + i * 12;
        if (entryPos + 12 > data.size()) break;

        uint16_t tag  = readU16(data, entryPos, littleEndian);
        uint16_t type = readU16(data, entryPos + 2, littleEndian);
        uint32_t count = readU32(data, entryPos + 4, littleEndian);

        int totalBytes = static_cast<int>(count) * typeSize(type);
        int valueOffset;
        if (totalBytes <= 4) {
            valueOffset = entryPos + 8 - tiffStart; // inline value
        } else {
            valueOffset = static_cast<int>(readU32(data, entryPos + 8, littleEndian));
        }

        if (tag == TAG_MAKE || tag == TAG_MODEL || tag == TAG_DATETIME || tag == TAG_DATETIME_ORIGINAL) {
            result[QString::number(tag)] = readAsciiString(data, tiffStart + valueOffset, static_cast<int>(count));
        } else if (tag == TAG_ORIENTATION || tag == TAG_IMAGE_WIDTH || tag == TAG_IMAGE_HEIGHT) {
            if (type == TYPE_SHORT)
                result[QString::number(tag)] = readU16(data, entryPos + 8, littleEndian);
            else
                result[QString::number(tag)] = readU32(data, entryPos + 8, littleEndian);
        } else if (tag == TAG_EXIF_IFD || tag == TAG_GPS_IFD) {
            result[QString::number(tag)] = static_cast<int>(readU32(data, entryPos + 8, littleEndian));
        } else if (tag == TAG_GPS_LAT || tag == TAG_GPS_LON) {
            result[QString::number(tag)] = parseDegMinSec(data, valueOffset, littleEndian, tiffStart);
        } else if (tag == TAG_GPS_LAT_REF || tag == TAG_GPS_LON_REF) {
            result[QString::number(tag)] = readAsciiString(data, entryPos + 8, static_cast<int>(count));
        }
    }
    return result;
}

ExifData ExifReader::parseExifFromTiff(const QByteArray &data, int tiffStart)
{
    ExifData exif;
    if (tiffStart + 8 > data.size()) return exif;

    // Byte order
    bool littleEndian = (data[tiffStart] == 'I' && data[tiffStart + 1] == 'I');

    // First IFD offset
    uint32_t ifd0Offset = readU32(data, tiffStart + 4, littleEndian);

    // Parse IFD0
    QVariantMap ifd0 = parseIfdEntries(data, static_cast<int>(ifd0Offset), littleEndian, tiffStart);

    // Extract basic fields
    if (ifd0.contains(QString::number(TAG_MAKE)))
        exif.cameraMake = ifd0[QString::number(TAG_MAKE)].toString();
    if (ifd0.contains(QString::number(TAG_MODEL)))
        exif.cameraModel = ifd0[QString::number(TAG_MODEL)].toString();
    if (ifd0.contains(QString::number(TAG_ORIENTATION)))
        exif.orientation = ifd0[QString::number(TAG_ORIENTATION)].toInt();
    if (ifd0.contains(QString::number(TAG_IMAGE_WIDTH)))
        exif.width = ifd0[QString::number(TAG_IMAGE_WIDTH)].toInt();
    if (ifd0.contains(QString::number(TAG_IMAGE_HEIGHT)))
        exif.height = ifd0[QString::number(TAG_IMAGE_HEIGHT)].toInt();
    if (ifd0.contains(QString::number(TAG_DATETIME))) {
        exif.dateTaken = QDateTime::fromString(
            ifd0[QString::number(TAG_DATETIME)].toString(),
            "yyyy:MM:dd HH:mm:ss");
    }

    // Parse EXIF sub-IFD
    if (ifd0.contains(QString::number(TAG_EXIF_IFD))) {
        int exifIfdOffset = ifd0[QString::number(TAG_EXIF_IFD)].toInt();
        QVariantMap exifIfd = parseIfdEntries(data, exifIfdOffset, littleEndian, tiffStart);

        if (exifIfd.contains(QString::number(TAG_DATETIME_ORIGINAL))) {
            exif.dateTaken = QDateTime::fromString(
                exifIfd[QString::number(TAG_DATETIME_ORIGINAL)].toString(),
                "yyyy:MM:dd HH:mm:ss");
        }
    }

    // Parse GPS sub-IFD
    if (ifd0.contains(QString::number(TAG_GPS_IFD))) {
        int gpsIfdOffset = ifd0[QString::number(TAG_GPS_IFD)].toInt();
        QVariantMap gpsIfd = parseIfdEntries(data, gpsIfdOffset, littleEndian, tiffStart);

        bool hasLat = gpsIfd.contains(QString::number(TAG_GPS_LAT));
        bool hasLon = gpsIfd.contains(QString::number(TAG_GPS_LON));

        if (hasLat && hasLon) {
            exif.latitude = gpsIfd[QString::number(TAG_GPS_LAT)].toDouble();
            exif.longitude = gpsIfd[QString::number(TAG_GPS_LON)].toDouble();

            // Apply hemisphere
            QString latRef = gpsIfd.value(QString::number(TAG_GPS_LAT_REF), "N").toString();
            QString lonRef = gpsIfd.value(QString::number(TAG_GPS_LON_REF), "E").toString();
            if (latRef == "S") exif.latitude = -exif.latitude;
            if (lonRef == "W") exif.longitude = -exif.longitude;

            exif.hasGps = true;
        }
    }

    return exif;
}

ExifData ExifReader::parseExifFromJpeg(const QByteArray &data)
{
    ExifData exif;
    if (data.size() < 12) return exif;

    // Check JPEG SOI
    if (static_cast<uint8_t>(data[0]) != 0xFF || static_cast<uint8_t>(data[1]) != 0xD8)
        return exif;

    // Find APP1 marker (EXIF)
    int pos = 2;
    while (pos + 4 < data.size()) {
        if (static_cast<uint8_t>(data[pos]) != 0xFF) break;

        uint8_t marker = static_cast<uint8_t>(data[pos + 1]);
        uint16_t segLen = (static_cast<uint8_t>(data[pos + 2]) << 8) | static_cast<uint8_t>(data[pos + 3]);

        if (marker == 0xE1) { // APP1
            // Check "Exif\0\0" header
            if (pos + 10 < data.size() &&
                data[pos + 4] == 'E' && data[pos + 5] == 'x' &&
                data[pos + 6] == 'i' && data[pos + 7] == 'f') {
                int tiffStart = pos + 10; // After "Exif\0\0"
                return parseExifFromTiff(data, tiffStart);
            }
        }

        pos += 2 + segLen;
        if (marker == 0xDA) break; // SOS — stop scanning
    }

    return exif;
}

ExifData ExifReader::read(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ExifReader: cannot open" << filePath;
        return {};
    }

    // Read enough for EXIF (usually in first 64KB)
    QByteArray header = file.read(65536);
    file.close();

    if (header.size() < 12) return {};

    // Check JPEG
    if (static_cast<uint8_t>(header[0]) == 0xFF && static_cast<uint8_t>(header[1]) == 0xD8) {
        return parseExifFromJpeg(header);
    }

    return {};
}
