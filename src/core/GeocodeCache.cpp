#include "GeocodeCache.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QDebug>

GeocodeCache::GeocodeCache(const QString &dbPath, QObject *parent)
    : QObject(parent)
{
    // Ensure parent directory exists
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    m_db = QSqlDatabase::addDatabase("QSQLITE", "geocode_cache");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        qWarning() << "GeocodeCache: cannot open DB" << dbPath << m_db.lastError().text();
        return;
    }
    initDb();
}

void GeocodeCache::initDb()
{
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS geocode_cache ("
           "  key TEXT PRIMARY KEY,"
           "  lat REAL,"
           "  lon REAL,"
           "  place TEXT,"
           "  fetched_at TEXT"
           ")");
}

QString GeocodeCache::cacheKey(double lat, double lon)
{
    // Round to 3 decimal places (~111m precision) for cache grouping
    return QString("%1,%2")
        .arg(qRound(lat * 1000) / 1000.0, 0, 'f', 3)
        .arg(qRound(lon * 1000) / 1000.0, 0, 'f', 3);
}

QString GeocodeCache::lookupCached(double lat, double lon) const
{
    if (!m_db.isOpen()) return {};

    QSqlQuery q(m_db);
    q.prepare("SELECT place FROM geocode_cache WHERE key = ?");
    q.addBindValue(cacheKey(lat, lon));
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return {};
}

void GeocodeCache::reverseGeocode(double lat, double lon, std::function<void(const QString &)> callback)
{
    // Check cache first
    QString cached = lookupCached(lat, lon);
    if (!cached.isEmpty()) {
        callback(cached);
        return;
    }

    // Rate limit: Nominatim requires max 1 request per second
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 delay = qMax(static_cast<qint64>(0), 1100 - (now - m_lastRequestMs));

    QTimer::singleShot(static_cast<int>(delay), this, [this, lat, lon, callback]() {
        m_lastRequestMs = QDateTime::currentMSecsSinceEpoch();

        QUrl url("https://nominatim.openstreetmap.org/reverse");
        QUrlQuery params;
        params.addQueryItem("format", "json");
        params.addQueryItem("lat", QString::number(lat, 'f', 6));
        params.addQueryItem("lon", QString::number(lon, 'f', 6));
        params.addQueryItem("zoom", "16"); // Neighborhood level
        url.setQuery(params);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, "Arkive/0.1 (personal wiki app)");

        QNetworkReply *reply = m_network.get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, lat, lon, callback]() {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                qWarning() << "GeocodeCache: network error" << reply->errorString();
                callback({});
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();
            QString displayName = obj.value("display_name").toString();

            // Shorten: take first 2-3 components (e.g. "Coffee Shop, Main St, Chicago")
            QStringList parts = displayName.split(", ");
            QString place;
            if (parts.size() >= 3) {
                place = parts[0] + ", " + parts[1] + ", " + parts[2];
            } else {
                place = displayName;
            }

            // Cache it
            if (m_db.isOpen() && !place.isEmpty()) {
                QSqlQuery q(m_db);
                q.prepare("INSERT OR REPLACE INTO geocode_cache (key, lat, lon, place, fetched_at) "
                          "VALUES (?, ?, ?, ?, ?)");
                q.addBindValue(cacheKey(lat, lon));
                q.addBindValue(lat);
                q.addBindValue(lon);
                q.addBindValue(place);
                q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                q.exec();
            }

            callback(place);
        });
    });
}
