#include <gtest/gtest.h>
#include "core/IndexManager.h"
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>

class IndexManagerTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    IndexManager indexManager;

    void SetUp() override
    {
        indexManager.setVaultPath(m_tempDir.path());

        // Create section directories
        QStringList sections = {"people", "conversations", "memories", "places", "media"};
        for (const QString &s : sections) {
            QDir(m_tempDir.path()).mkpath(s);
        }
    }

    void createArticle(const QString &section, const QString &name, const QString &content = "# Test")
    {
        QString path = QDir(m_tempDir.path()).filePath(section + "/" + name);
        QFile f(path);
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&f);
        out << content;
        f.close();
    }
};

TEST_F(IndexManagerTest, TotalArticleCountStartsAtZero)
{
    EXPECT_EQ(indexManager.totalArticleCount(), 0);
}

TEST_F(IndexManagerTest, RebuildIndexesCountsArticles)
{
    createArticle("media", "photo-1.md");
    createArticle("media", "photo-2.md");
    createArticle("people", "alice.md");

    indexManager.rebuildIndexes();

    EXPECT_EQ(indexManager.totalArticleCount(), 3);
}

TEST_F(IndexManagerTest, SectionInfoReturnsCorrectCount)
{
    createArticle("media", "photo-1.md");
    createArticle("media", "photo-2.md");

    indexManager.rebuildIndexes();

    SectionInfo info = indexManager.sectionInfo("media");
    EXPECT_EQ(info.articleCount, 2);
    EXPECT_EQ(info.name, "media");
}

TEST_F(IndexManagerTest, SectionInfoListsArticleSlugs)
{
    createArticle("media", "2025-10-17-austin.md");

    indexManager.rebuildIndexes();

    SectionInfo info = indexManager.sectionInfo("media");
    EXPECT_TRUE(info.articles.contains("2025-10-17-austin"));
}

TEST_F(IndexManagerTest, SkipsIndexFilesStartingWithUnderscore)
{
    createArticle("media", "_index.md");
    createArticle("media", "real-article.md");

    indexManager.rebuildIndexes();

    SectionInfo info = indexManager.sectionInfo("media");
    EXPECT_EQ(info.articleCount, 1);
}

TEST_F(IndexManagerTest, RebuildCreatesSectionIndexFiles)
{
    createArticle("media", "photo-1.md");

    indexManager.rebuildIndexes();

    QString indexPath = QDir(m_tempDir.path()).filePath("media/_index.md");
    EXPECT_TRUE(QFile::exists(indexPath));
}

TEST_F(IndexManagerTest, RebuildCreatesMasterIndex)
{
    indexManager.rebuildIndexes();

    QString indexPath = QDir(m_tempDir.path()).filePath("_index.md");
    EXPECT_TRUE(QFile::exists(indexPath));
}

TEST_F(IndexManagerTest, UpdateSectionIndexUpdatesOnlyThatSection)
{
    createArticle("media", "photo-1.md");
    createArticle("people", "alice.md");

    indexManager.rebuildIndexes();
    EXPECT_EQ(indexManager.sectionInfo("media").articleCount, 1);

    // Add another article and update only media
    createArticle("media", "photo-2.md");
    indexManager.updateSectionIndex("media");

    EXPECT_EQ(indexManager.sectionInfo("media").articleCount, 2);
}

TEST_F(IndexManagerTest, EmptySectionHasZeroArticles)
{
    indexManager.rebuildIndexes();

    SectionInfo info = indexManager.sectionInfo("conversations");
    EXPECT_EQ(info.articleCount, 0);
    EXPECT_TRUE(info.articles.isEmpty());
}
