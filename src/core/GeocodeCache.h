#ifndef GEOCODECACHE_H
#define GEOCODECACHE_H

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>

#ifndef ARKIVE_TESTING
#define GEOCACHE_PRIVATE private
#else
#define GEOCACHE_PRIVATE public
#endif

// Reverse geocode GPS coordinates to place names using OpenStreetMap Nominatim.
// Results cached in SQLite to avoid hitting the API for repeated coordinates.
class GeocodeCache : public QObject
{
    Q_OBJECT

public:
    explicit GeocodeCache(const QString &dbPath, QObject *parent = nullptr);

    // Look up a cached place name. Returns empty string if not cached.
    QString lookupCached(double lat, double lon) const;

    // Request reverse geocode. Calls callback with place name when done.
    // Uses cache first; falls back to Nominatim API (rate-limited to 1 req/sec).
    void reverseGeocode(double lat, double lon, std::function<void(const QString &)> callback);

GEOCACHE_PRIVATE:
    // Round to ~111m grid for cache key deduplication
    static QString cacheKey(double lat, double lon);
    void initDb();

    QSqlDatabase m_db;
    QNetworkAccessManager m_network;
    qint64 m_lastRequestMs = 0;
};

#endif // GEOCODECACHE_H
