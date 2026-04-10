#include "PhotoIngestor.h"
#include "VaultManager.h"
#include "GeocodeCache.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>
#include <cmath>

QStringList PhotoIngestor::supportedExtensions()
{
    return { "jpg", "jpeg", "png", "tiff", "tif", "heic", "heif" };
}

PhotoIngestor::PhotoIngestor(VaultManager *vault, GeocodeCache *geocode, QObject *parent)
    : QObject(parent), m_vault(vault), m_geocode(geocode)
{
}

QString PhotoIngestor::locationKey(double lat, double lon)
{
    // Round to ~500m grid (0.005 degrees ≈ 500m)
    double gridLat = std::round(lat / 0.005) * 0.005;
    double gridLon = std::round(lon / 0.005) * 0.005;
    return QString("%1,%2").arg(gridLat, 0, 'f', 3).arg(gridLon, 0, 'f', 3);
}

void PhotoIngestor::importFromFolder(const QString &folderPath)
{
    if (m_running) return;

    m_running = true;
    m_cancelled = false;
    m_progress = 0;
    m_entriesCreated = 0;
    m_geocodePending = 0;
    m_groups.clear();
    m_pendingFiles.clear();
    emit runningChanged();

    // Scan for image files
    QDir dir(folderPath);
    if (!dir.exists()) {
        m_statusText = "Folder not found: " + folderPath;
        emit statusTextChanged();
        m_running = false;
        emit runningChanged();
        emit importError(m_statusText);
        return;
    }

    QStringList filters;
    for (const auto &ext : supportedExtensions()) {
        filters << ("*." + ext);
    }

    QDirIterator it(folderPath, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        m_pendingFiles.append(it.next());
    }

    m_totalFiles = m_pendingFiles.size();
    emit totalFilesChanged();

    if (m_totalFiles == 0) {
        m_statusText = "No image files found in folder.";
        emit statusTextChanged();
        m_running = false;
        emit runningChanged();
        emit importFinished(0, 0);
        return;
    }

    m_statusText = QString("Reading EXIF from %1 photos...").arg(m_totalFiles);
    emit statusTextChanged();

    // Process in batches to keep UI responsive
    QTimer::singleShot(0, this, &PhotoIngestor::processNextBatch);
}

void PhotoIngestor::cancel()
{
    m_cancelled = true;
}

void PhotoIngestor::processNextBatch()
{
    if (m_cancelled) {
        m_statusText = "Import cancelled.";
        emit statusTextChanged();
        m_running = false;
        emit runningChanged();
        return;
    }

    // Process up to 20 files per batch
    int batchSize = qMin(20, m_pendingFiles.size());

    for (int i = 0; i < batchSize; ++i) {
        QString filePath = m_pendingFiles.takeFirst();
        m_progress++;
        emit progressChanged();

        ExifData exif = ExifReader::read(filePath);

        // Use file modification date as fallback if no EXIF date
        if (!exif.dateTaken.isValid()) {
            exif.dateTaken = QFileInfo(filePath).lastModified();
        }

        // Build group key: date + location grid
        QString dateKey = exif.dateTaken.date().toString("yyyy-MM-dd");
        QString locKey = exif.hasGps ? locationKey(exif.latitude, exif.longitude) : "unknown";
        QString groupKey = dateKey + "|" + locKey;

        // Add to group
        PhotoGroup &group = m_groups[groupKey];
        if (group.dateKey.isEmpty()) {
            group.dateKey = dateKey;
            group.locationKey = locKey;
            group.earliestDate = exif.dateTaken;
            group.latestDate = exif.dateTaken;
        }
        group.filePaths.append(filePath);
        group.exifList.append(exif);
        if (exif.dateTaken < group.earliestDate) group.earliestDate = exif.dateTaken;
        if (exif.dateTaken > group.latestDate) group.latestDate = exif.dateTaken;
    }

    // More files to process?
    if (!m_pendingFiles.isEmpty()) {
        m_statusText = QString("Reading EXIF... %1/%2").arg(m_progress).arg(m_totalFiles);
        emit statusTextChanged();
        QTimer::singleShot(0, this, &PhotoIngestor::processNextBatch);
        return;
    }

    // All files read — now reverse geocode groups with GPS data
    m_statusText = "Resolving locations...";
    emit statusTextChanged();

    QList<QString> groupKeys = m_groups.keys();
    for (const QString &key : groupKeys) {
        PhotoGroup &group = m_groups[key];
        if (group.locationKey == "unknown") continue;

        // Find first photo with GPS for geocoding
        for (const ExifData &e : group.exifList) {
            if (e.hasGps) {
                m_geocodePending++;
                m_geocode->reverseGeocode(e.latitude, e.longitude,
                    [this, key](const QString &place) {
                        if (m_groups.contains(key)) {
                            m_groups[key].placeName = place;
                        }
                        m_geocodePending--;
                        if (m_geocodePending <= 0) {
                            // All geocoding done — write entries
                            for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
                                writeGroupEntry(it.value());
                            }
                            m_statusText = QString("Done! Created %1 entries from %2 photos.")
                                .arg(m_entriesCreated).arg(m_totalFiles);
                            emit statusTextChanged();
                            m_running = false;
                            emit runningChanged();
                            emit importFinished(m_entriesCreated, m_totalFiles);
                        }
                    });
                break; // Only geocode once per group
            }
        }
    }

    // If no geocoding needed, write immediately
    if (m_geocodePending == 0) {
        for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
            writeGroupEntry(it.value());
        }
        m_statusText = QString("Done! Created %1 entries from %2 photos.")
            .arg(m_entriesCreated).arg(m_totalFiles);
        emit statusTextChanged();
        m_running = false;
        emit runningChanged();
        emit importFinished(m_entriesCreated, m_totalFiles);
    }
}

QString PhotoIngestor::generateSlug(const PhotoGroup &group) const
{
    QString slug = group.dateKey;
    if (!group.placeName.isEmpty()) {
        // Take first part of place name, slugify
        QString place = group.placeName.split(",").first().trimmed().toLower();
        place.replace(QRegularExpression("[^a-z0-9]+"), "-");
        place.remove(QRegularExpression("^-|-$"));
        if (!place.isEmpty()) {
            slug += "-" + place;
        }
    }
    return slug;
}

QString PhotoIngestor::generateMarkdown(const PhotoGroup &group) const
{
    QString md;

    // Title
    QString title = "Photos: " + group.dateKey;
    if (!group.placeName.isEmpty()) {
        title += " — " + group.placeName.split(",").first().trimmed();
    }
    md += "# " + title + "\n\n";

    // Summary
    md += "## Summary\n";
    md += QString("%1 photo(s) taken on %2")
        .arg(group.filePaths.size())
        .arg(group.dateKey);
    if (!group.placeName.isEmpty()) {
        md += " at " + group.placeName;
    }
    md += ".\n\n";

    // Key Facts
    md += "## Key Facts\n";
    md += "- Date: " + group.dateKey + "\n";
    md += "- Time range: " + group.earliestDate.time().toString("HH:mm")
        + " — " + group.latestDate.time().toString("HH:mm") + "\n";
    md += "- Photo count: " + QString::number(group.filePaths.size()) + "\n";

    if (!group.placeName.isEmpty()) {
        md += "- Location: " + group.placeName + "\n";
    }

    // Extract unique camera models
    QStringList cameras;
    for (const ExifData &e : group.exifList) {
        QString cam = e.cameraModel.isEmpty() ? e.cameraMake : e.cameraModel;
        if (!cam.isEmpty() && !cameras.contains(cam)) {
            cameras.append(cam);
        }
    }
    if (!cameras.isEmpty()) {
        md += "- Camera: " + cameras.join(", ") + "\n";
    }

    // GPS coordinates
    for (const ExifData &e : group.exifList) {
        if (e.hasGps) {
            md += QString("- GPS: %1, %2\n")
                .arg(e.latitude, 0, 'f', 6)
                .arg(e.longitude, 0, 'f', 6);
            break;
        }
    }
    md += "\n";

    // Connections
    md += "## Connections\n";
    md += "- [[" + group.dateKey + "-conversations]] (same day)\n";
    md += "\n";

    // Sources
    md += "## Sources\n";
    for (const QString &path : group.filePaths) {
        md += "- " + QFileInfo(path).fileName() + "\n";
    }

    return md;
}

void PhotoIngestor::writeGroupEntry(const PhotoGroup &group)
{
    QString slug = generateSlug(group);
    QString relativePath = "media/" + slug + ".md";

    // Dedup: skip if file already exists
    QString fullPath = m_vault->vaultPath() + "/" + relativePath;
    if (QFile::exists(fullPath)) {
        qDebug() << "PhotoIngestor: skipping existing entry" << relativePath;
        return;
    }

    QString content = generateMarkdown(group);
    if (m_vault->writeFile(relativePath, content)) {
        m_entriesCreated++;
    } else {
        qWarning() << "PhotoIngestor: failed to write" << relativePath;
    }
}
