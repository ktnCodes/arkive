#ifndef PHOTOINGESTOR_H
#define PHOTOINGESTOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "ExifReader.h"

// For test access to private methods
#ifndef ARKIVE_PRIVATE
#ifndef ARKIVE_TESTING
#define ARKIVE_PRIVATE private
#else
#define ARKIVE_PRIVATE public
#endif
#endif


class VaultManager;
class GeocodeCache;

struct PhotoGroup {
    QString dateKey;         // "2026-03-15"
    QString locationKey;     // "41.878,-87.630" or "unknown"
    QString placeName;       // Reverse-geocoded place name
    QStringList filePaths;   // Original photo paths in this group
    QList<ExifData> exifList;
    QDateTime earliestDate;
    QDateTime latestDate;
};

class PhotoIngestor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int totalFiles READ totalFiles NOTIFY totalFilesChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit PhotoIngestor(VaultManager *vault, GeocodeCache *geocode, QObject *parent = nullptr);

    bool running() const { return m_running; }
    int progress() const { return m_progress; }
    int totalFiles() const { return m_totalFiles; }
    QString statusText() const { return m_statusText; }

    // Start import from a folder of images
    Q_INVOKABLE void importFromFolder(const QString &folderPath);

    // Cancel in-progress import
    Q_INVOKABLE void cancel();

signals:
    void runningChanged();
    void progressChanged();
    void totalFilesChanged();
    void statusTextChanged();
    void importFinished(int entriesCreated, int photosProcessed);
    void importError(const QString &message);

ARKIVE_PRIVATE:
    void processNextBatch();
    void writeGroupEntry(const PhotoGroup &group);
    QString generateSlug(const PhotoGroup &group) const;
    QString generateMarkdown(const PhotoGroup &group) const;
    static QString locationKey(double lat, double lon);
    static QStringList supportedExtensions();

    VaultManager *m_vault;
    GeocodeCache *m_geocode;

    bool m_running = false;
    bool m_cancelled = false;
    int m_progress = 0;
    int m_totalFiles = 0;
    QString m_statusText;

    QStringList m_pendingFiles;
    QMap<QString, PhotoGroup> m_groups; // key = "date|locationKey"
    int m_entriesCreated = 0;
    int m_geocodePending = 0;
};

#endif // PHOTOINGESTOR_H
