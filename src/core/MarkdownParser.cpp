#include "MarkdownParser.h"

#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

MarkdownParser::MarkdownParser(QObject *parent)
    : QObject(parent)
{
}

ParsedArticle MarkdownParser::parse(const QString &content) const
{
    ParsedArticle article;
    article.rawContent = content;

    // Parse YAML frontmatter if present
    int bodyStart = 0;
    article.frontmatter = parseFrontmatter(content, bodyStart);

    QString body = content.mid(bodyStart);

    // Parse sections
    article.sections = parseSections(body);

    // Extract title (first h1)
    article.title = extractTitle(body);

    // Extract backlinks
    article.backlinks = extractBacklinks(body);

    return article;
}

QVariantMap MarkdownParser::parseFrontmatter(const QString &content, int &bodyStart) const
{
    QVariantMap fm;
    bodyStart = 0;

    if (!content.startsWith("---"))
        return fm;

    int endIndex = content.indexOf("---", 3);
    if (endIndex < 0)
        return fm;

    // Simple key: value parsing for YAML frontmatter
    QString fmBlock = content.mid(3, endIndex - 3).trimmed();
    QStringList lines = fmBlock.split('\n');

    for (const QString &line : lines) {
        int colonPos = line.indexOf(':');
        if (colonPos > 0) {
            QString key = line.left(colonPos).trimmed();
            QString value = line.mid(colonPos + 1).trimmed();
            fm[key] = value;
        }
    }

    bodyStart = endIndex + 3;
    // Skip newline after closing ---
    if (bodyStart < content.length() && content[bodyStart] == '\n')
        bodyStart++;

    return fm;
}

QList<MarkdownSection> MarkdownParser::parseSections(const QString &content) const
{
    QList<MarkdownSection> sections;

    static QRegularExpression headingRe(R"(^(#{1,6})\s+(.+)$)",
                                         QRegularExpression::MultilineOption);

    auto matches = headingRe.globalMatch(content);
    QVector<QPair<int, MarkdownSection>> headings;

    while (matches.hasNext()) {
        auto match = matches.next();
        MarkdownSection sec;
        sec.level = match.captured(1).length();
        sec.heading = match.captured(2).trimmed();
        headings.append({match.capturedStart(), sec});
    }

    // Fill in content between headings
    for (int i = 0; i < headings.size(); i++) {
        int contentStart = headings[i].first +
            headings[i].second.level + 1 + headings[i].second.heading.length();
        // Skip the newline after heading
        if (contentStart < content.length() && content[contentStart] == '\n')
            contentStart++;

        int contentEnd = (i + 1 < headings.size())
            ? headings[i + 1].first
            : content.length();

        headings[i].second.content = content.mid(contentStart, contentEnd - contentStart).trimmed();
        sections.append(headings[i].second);
    }

    return sections;
}

QStringList MarkdownParser::extractBacklinks(const QString &content) const
{
    QStringList links;

    static QRegularExpression backlinkRe(R"(\[\[([^\]]+)\]\])");
    auto matches = backlinkRe.globalMatch(content);

    while (matches.hasNext()) {
        auto match = matches.next();
        QString link = match.captured(1).trimmed();
        if (!links.contains(link)) {
            links.append(link);
        }
    }

    return links;
}

QString MarkdownParser::extractTitle(const QString &content) const
{
    static QRegularExpression titleRe(R"(^#\s+(.+)$)",
                                       QRegularExpression::MultilineOption);
    auto match = titleRe.match(content);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    return QString();
}

QString MarkdownParser::toHtml(const QString &content) const
{
    QString html = content;

    // Headings: # → <h1>, ## → <h2>, etc.
    static QRegularExpression h1Re(R"(^# (.+)$)", QRegularExpression::MultilineOption);
    static QRegularExpression h2Re(R"(^## (.+)$)", QRegularExpression::MultilineOption);
    static QRegularExpression h3Re(R"(^### (.+)$)", QRegularExpression::MultilineOption);

    html.replace(h3Re, "<h3>\\1</h3>");
    html.replace(h2Re, "<h2>\\1</h2>");
    html.replace(h1Re, "<h1>\\1</h1>");

    // Bold: **text** → <strong>text</strong>
    static QRegularExpression boldRe(R"(\*\*(.+?)\*\*)");
    html.replace(boldRe, "<strong>\\1</strong>");

    // Italic: *text* → <em>text</em>
    static QRegularExpression italicRe(R"(\*(.+?)\*)");
    html.replace(italicRe, "<em>\\1</em>");

    // Backlinks: [[slug]] → clickable link
    static QRegularExpression backlinkRe(R"(\[\[([^\]]+)\]\])");
    html.replace(backlinkRe, R"(<a href="arkive://article/\1" class="backlink">\1</a>)");

    // List items: - text → <li>text</li>
    static QRegularExpression listRe(R"(^- (.+)$)", QRegularExpression::MultilineOption);
    html.replace(listRe, "<li>\\1</li>");

    // Wrap consecutive <li> in <ul>
    static QRegularExpression ulRe(R"((<li>.+</li>\n?)+)");
    html.replace(ulRe, "<ul>\\0</ul>");

    // Paragraphs: double newline → <p>
    html.replace("\n\n", "</p><p>");
    html = "<p>" + html + "</p>";

    // Clean up empty paragraphs
    html.replace("<p></p>", "");

    return html;
}
