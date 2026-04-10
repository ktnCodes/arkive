#include <gtest/gtest.h>
#include "core/GeocodeCache.h"
#include <QTemporaryDir>
#include <QDir>

class GeocodeCacheTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;

    std::unique_ptr<GeocodeCache> createCache()
    {
        QString dbPath = m_tempDir.filePath("test_geocode.db");
        return std::make_unique<GeocodeCache>(dbPath);
    }
};

TEST_F(GeocodeCacheTest, CacheKeyRoundsToGrid)
{
    // Two coordinates within the ~111m rounding grid should produce the same key
    QString key1 = GeocodeCache::cacheKey(30.4800, -97.7890);
    QString key2 = GeocodeCache::cacheKey(30.4804, -97.7894);
    EXPECT_EQ(key1, key2);
}

TEST_F(GeocodeCacheTest, CacheKeyDifferentForDistantPoints)
{
    QString key1 = GeocodeCache::cacheKey(30.480, -97.789);
    QString key2 = GeocodeCache::cacheKey(31.000, -98.000);
    EXPECT_NE(key1, key2);
}

TEST_F(GeocodeCacheTest, LookupCachedReturnsEmptyForMissing)
{
    auto cache = createCache();
    QString place = cache->lookupCached(30.48, -97.79);
    EXPECT_TRUE(place.isEmpty());
}

TEST_F(GeocodeCacheTest, DatabaseFileCreatedOnConstruction)
{
    auto cache = createCache();
    QString dbPath = m_tempDir.filePath("test_geocode.db");
    EXPECT_TRUE(QFile::exists(dbPath));
}

TEST_F(GeocodeCacheTest, CacheKeyIsConsistentForSameInput)
{
    QString key1 = GeocodeCache::cacheKey(40.7128, -74.0060);
    QString key2 = GeocodeCache::cacheKey(40.7128, -74.0060);
    EXPECT_EQ(key1, key2);
}

TEST_F(GeocodeCacheTest, CacheKeyHandlesNegativeCoordinates)
{
    QString key = GeocodeCache::cacheKey(-33.8688, 151.2093);
    EXPECT_FALSE(key.isEmpty());
}

TEST_F(GeocodeCacheTest, CacheKeyHandlesZeroCoordinates)
{
    QString key = GeocodeCache::cacheKey(0.0, 0.0);
    EXPECT_FALSE(key.isEmpty());
}
