#include "ArticleModel.h"
#include "core/MarkdownParser.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

ArticleModel::ArticleModel(QObject *parent)
    : QObject(parent)
{
}

QString ArticleModel::title() const { return m_title; }
QString ArticleModel::filePath() const { return m_filePath; }
QString ArticleModel::rawContent() const { return m_rawContent; }
QString ArticleModel::htmlContent() const { return m_htmlContent; }
QStringList ArticleModel::backlinks() const { return m_backlinks; }
QStringList ArticleModel::sectionHeadings() const { return m_sectionHeadings; }
bool ArticleModel::loaded() const { return m_loaded; }

void ArticleModel::loadFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open article:" << path;
        clear();
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    MarkdownParser parser;
    ParsedArticle article = parser.parse(content);

    m_filePath = path;
    m_rawContent = content;
    m_title = article.title.isEmpty() ? QFileInfo(path).baseName() : article.title;
    m_htmlContent = parser.toHtml(content);
    m_backlinks = article.backlinks;

    m_sectionHeadings.clear();
    for (const auto &section : article.sections) {
        m_sectionHeadings.append(section.heading);
    }

    m_loaded = true;
    emit articleChanged();
}

void ArticleModel::clear()
{
    m_title.clear();
    m_filePath.clear();
    m_rawContent.clear();
    m_htmlContent.clear();
    m_backlinks.clear();
    m_sectionHeadings.clear();
    m_loaded = false;
    emit articleChanged();
}
