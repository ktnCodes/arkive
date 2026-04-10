#include <gtest/gtest.h>
#include "core/MarkdownParser.h"

class MarkdownParserTest : public ::testing::Test
{
protected:
    MarkdownParser parser;
};

// --- Title extraction ---

TEST_F(MarkdownParserTest, ExtractsTitleFromH1)
{
    QString title = parser.extractTitle("# My Article\n\nSome content");
    EXPECT_EQ(title, "My Article");
}

TEST_F(MarkdownParserTest, ExtractsTitleIgnoresH2)
{
    QString title = parser.extractTitle("## Not a title\n\nContent");
    EXPECT_TRUE(title.isEmpty());
}

TEST_F(MarkdownParserTest, ExtractsTitleFromFirstH1Only)
{
    QString title = parser.extractTitle("# First\n\n# Second");
    EXPECT_EQ(title, "First");
}

TEST_F(MarkdownParserTest, ExtractsTitleReturnsEmptyForNoHeadings)
{
    QString title = parser.extractTitle("Just plain text");
    EXPECT_TRUE(title.isEmpty());
}

// --- Backlink extraction ---

TEST_F(MarkdownParserTest, ExtractsBacklinks)
{
    QStringList links = parser.extractBacklinks("See [[person-name]] and [[event-2024]]");
    EXPECT_EQ(links.size(), 2);
    EXPECT_TRUE(links.contains("person-name"));
    EXPECT_TRUE(links.contains("event-2024"));
}

TEST_F(MarkdownParserTest, DeduplicatesBacklinks)
{
    QStringList links = parser.extractBacklinks("[[foo]] and [[foo]] again");
    EXPECT_EQ(links.size(), 1);
    EXPECT_EQ(links.first(), "foo");
}

TEST_F(MarkdownParserTest, ReturnsEmptyForNoBacklinks)
{
    QStringList links = parser.extractBacklinks("No links here");
    EXPECT_TRUE(links.isEmpty());
}

TEST_F(MarkdownParserTest, HandlesBacklinkWithSpaces)
{
    QStringList links = parser.extractBacklinks("See [[ spaced link ]]");
    EXPECT_EQ(links.size(), 1);
    EXPECT_EQ(links.first(), "spaced link");
}

// --- Section parsing ---

TEST_F(MarkdownParserTest, ParsesSectionsFromHeadings)
{
    ParsedArticle article = parser.parse("# Title\n\nIntro text\n\n## Section A\n\nContent A\n\n## Section B\n\nContent B");

    EXPECT_EQ(article.sections.size(), 3);
    EXPECT_EQ(article.sections[0].heading, "Title");
    EXPECT_EQ(article.sections[0].level, 1);
    EXPECT_EQ(article.sections[1].heading, "Section A");
    EXPECT_EQ(article.sections[1].level, 2);
    EXPECT_EQ(article.sections[2].heading, "Section B");
    EXPECT_EQ(article.sections[2].level, 2);
}

TEST_F(MarkdownParserTest, SectionContentExcludesNextHeading)
{
    ParsedArticle article = parser.parse("# Title\n\nIntro\n\n## Next\n\nBody");

    EXPECT_TRUE(article.sections[0].content.contains("Intro"));
    EXPECT_FALSE(article.sections[0].content.contains("## Next"));
}

// --- Frontmatter parsing ---

TEST_F(MarkdownParserTest, ParsesYamlFrontmatter)
{
    ParsedArticle article = parser.parse("---\ntitle: My Doc\ntags: test\n---\n# Heading\n\nBody");

    EXPECT_EQ(article.frontmatter["title"].toString(), "My Doc");
    EXPECT_EQ(article.frontmatter["tags"].toString(), "test");
    EXPECT_EQ(article.title, "Heading");
}

TEST_F(MarkdownParserTest, HandlesNoFrontmatter)
{
    ParsedArticle article = parser.parse("# Just a heading\n\nContent");

    EXPECT_TRUE(article.frontmatter.isEmpty());
    EXPECT_EQ(article.title, "Just a heading");
}

// --- HTML conversion ---

TEST_F(MarkdownParserTest, ConvertsH1ToHtml)
{
    QString html = parser.toHtml("# Title");
    EXPECT_TRUE(html.contains("<h1>Title</h1>"));
}

TEST_F(MarkdownParserTest, ConvertsH2ToHtml)
{
    QString html = parser.toHtml("## Subtitle");
    EXPECT_TRUE(html.contains("<h2>Subtitle</h2>"));
}

TEST_F(MarkdownParserTest, ConvertsBoldToHtml)
{
    QString html = parser.toHtml("Some **bold** text");
    EXPECT_TRUE(html.contains("<strong>bold</strong>"));
}

TEST_F(MarkdownParserTest, ConvertsItalicToHtml)
{
    QString html = parser.toHtml("Some *italic* text");
    EXPECT_TRUE(html.contains("<em>italic</em>"));
}

TEST_F(MarkdownParserTest, ConvertsBacklinksToHtml)
{
    QString html = parser.toHtml("See [[my-article]]");
    EXPECT_TRUE(html.contains("arkive://article/my-article"));
    EXPECT_TRUE(html.contains("my-article"));
}

TEST_F(MarkdownParserTest, ConvertsListItemsToHtml)
{
    QString html = parser.toHtml("- Item one\n- Item two");
    EXPECT_TRUE(html.contains("<li>Item one</li>"));
    EXPECT_TRUE(html.contains("<li>Item two</li>"));
    EXPECT_TRUE(html.contains("<ul>"));
}

TEST_F(MarkdownParserTest, ConvertsH3ToHtml)
{
    QString html = parser.toHtml("### Sub-subtitle");
    EXPECT_TRUE(html.contains("<h3>Sub-subtitle</h3>"));
}

// --- Full parse integration ---

TEST_F(MarkdownParserTest, FullParseReturnsAllFields)
{
    QString content = "# Photos: 2025-10-17\n\n## Summary\n\n1 photo taken.\n\n## Connections\n\n- [[2025-10-17-conversations]] (same day)";

    ParsedArticle article = parser.parse(content);

    EXPECT_EQ(article.title, "Photos: 2025-10-17");
    EXPECT_EQ(article.rawContent, content);
    EXPECT_GE(article.sections.size(), 3);
    EXPECT_EQ(article.backlinks.size(), 1);
    EXPECT_EQ(article.backlinks.first(), "2025-10-17-conversations");
}
