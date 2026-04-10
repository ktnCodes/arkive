#ifndef MARKDOWNPARSER_H
#define MARKDOWNPARSER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>

struct MarkdownSection {
    QString heading;
    int level;          // 1 for #, 2 for ##, etc.
    QString content;
};

struct ParsedArticle {
    QString title;                  // First # heading
    QString rawContent;             // Full file content
    QList<MarkdownSection> sections;
    QStringList backlinks;          // [[slug]] references found
    QVariantMap frontmatter;        // YAML frontmatter if present
};

class MarkdownParser : public QObject
{
    Q_OBJECT

public:
    explicit MarkdownParser(QObject *parent = nullptr);

    // Parse a markdown file and extract structure
    ParsedArticle parse(const QString &content) const;

    // Extract just the backlinks from content
    Q_INVOKABLE QStringList extractBacklinks(const QString &content) const;

    // Extract just the title (first # heading)
    Q_INVOKABLE QString extractTitle(const QString &content) const;

    // Convert markdown to simple HTML for display
    Q_INVOKABLE QString toHtml(const QString &content) const;

private:
    QVariantMap parseFrontmatter(const QString &content, int &bodyStart) const;
    QList<MarkdownSection> parseSections(const QString &content) const;
};

#endif // MARKDOWNPARSER_H
