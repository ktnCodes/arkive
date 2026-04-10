#include <gtest/gtest.h>
#include "models/ArticleModel.h"
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QSignalSpy>

class ArticleModelTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    ArticleModel model;

    QString createMarkdownFile(const QString &name, const QString &content)
    {
        QString path = QDir(m_tempDir.path()).filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&f);
        out << content;
        f.close();
        return path;
    }
};

TEST_F(ArticleModelTest, NotLoadedByDefault)
{
    EXPECT_FALSE(model.loaded());
    EXPECT_TRUE(model.title().isEmpty());
    EXPECT_TRUE(model.rawContent().isEmpty());
}

TEST_F(ArticleModelTest, LoadFromFileSetsTitleFromH1)
{
    QString path = createMarkdownFile("test.md", "# My Article\n\nContent here");

    model.loadFromFile(path);

    EXPECT_TRUE(model.loaded());
    EXPECT_EQ(model.title(), "My Article");
}

TEST_F(ArticleModelTest, LoadFromFileUsesFilenameAsFallbackTitle)
{
    // No H1 heading in this file
    QString path = createMarkdownFile("fallback-title.md", "Just some text without headings");

    model.loadFromFile(path);

    EXPECT_TRUE(model.loaded());
    EXPECT_EQ(model.title(), "fallback-title");
}

TEST_F(ArticleModelTest, LoadFromFilePreservesRawContent)
{
    QString content = "# Title\n\n## Section\n\nBody text.";
    QString path = createMarkdownFile("raw.md", content);

    model.loadFromFile(path);

    EXPECT_EQ(model.rawContent(), content);
}

TEST_F(ArticleModelTest, LoadFromFileExtractsBacklinks)
{
    QString path = createMarkdownFile("links.md",
        "# Article\n\nSee [[person-a]] and [[event-2025]]");

    model.loadFromFile(path);

    EXPECT_EQ(model.backlinks().size(), 2);
    EXPECT_TRUE(model.backlinks().contains("person-a"));
    EXPECT_TRUE(model.backlinks().contains("event-2025"));
}

TEST_F(ArticleModelTest, LoadFromFileExtractsSectionHeadings)
{
    QString path = createMarkdownFile("sections.md",
        "# Title\n\n## Summary\n\nText\n\n## Details\n\nMore text");

    model.loadFromFile(path);

    EXPECT_EQ(model.sectionHeadings().size(), 3);
    EXPECT_EQ(model.sectionHeadings()[0], "Title");
    EXPECT_EQ(model.sectionHeadings()[1], "Summary");
    EXPECT_EQ(model.sectionHeadings()[2], "Details");
}

TEST_F(ArticleModelTest, LoadFromFileGeneratesHtml)
{
    QString path = createMarkdownFile("html.md",
        "# Title\n\n## Section\n\n- Item 1\n- Item 2");

    model.loadFromFile(path);

    EXPECT_FALSE(model.htmlContent().isEmpty());
    EXPECT_TRUE(model.htmlContent().contains("<h1>"));
    EXPECT_TRUE(model.htmlContent().contains("<li>"));
}

TEST_F(ArticleModelTest, LoadFromFileSetsFilePath)
{
    QString path = createMarkdownFile("path.md", "# Test");

    model.loadFromFile(path);

    EXPECT_EQ(model.filePath(), path);
}

TEST_F(ArticleModelTest, LoadFromFileEmitsArticleChanged)
{
    QString path = createMarkdownFile("signal.md", "# Test");

    QSignalSpy spy(&model, &ArticleModel::articleChanged);
    model.loadFromFile(path);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ArticleModelTest, ClearResetsAllFields)
{
    QString path = createMarkdownFile("clear.md", "# Title\n\n[[link]]");
    model.loadFromFile(path);
    ASSERT_TRUE(model.loaded());

    model.clear();

    EXPECT_FALSE(model.loaded());
    EXPECT_TRUE(model.title().isEmpty());
    EXPECT_TRUE(model.rawContent().isEmpty());
    EXPECT_TRUE(model.htmlContent().isEmpty());
    EXPECT_TRUE(model.backlinks().isEmpty());
    EXPECT_TRUE(model.sectionHeadings().isEmpty());
}

TEST_F(ArticleModelTest, LoadNonexistentFileClearsModel)
{
    // First load a valid file
    QString path = createMarkdownFile("valid.md", "# Valid");
    model.loadFromFile(path);
    ASSERT_TRUE(model.loaded());

    // Now load nonexistent
    model.loadFromFile("/nonexistent/file.md");

    EXPECT_FALSE(model.loaded());
}
