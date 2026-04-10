#include "IndexManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

IndexManager::IndexManager(QObject *parent)
    : QObject(parent)
{
}

void IndexManager::setVaultPath(const QString &vaultPath)
{
    m_vaultPath = vaultPath;
}

void IndexManager::rebuildIndexes()
{
    if (m_vaultPath.isEmpty())
        return;

    QDir vaultDir(m_vaultPath);
    QStringList sections = {"people", "conversations", "memories", "places", "media"};

    m_sections.clear();

    for (const QString &section : sections) {
        if (vaultDir.exists(section)) {
            scanSection(section);
            writeSectionIndex(section, m_sections[section]);
        }
    }

    writeMasterIndex();
    emit indexesUpdated();
}

void IndexManager::updateSectionIndex(const QString &section)
{
    scanSection(section);
    writeSectionIndex(section, m_sections[section]);
    writeMasterIndex();
    emit indexesUpdated();
}

void IndexManager::updateMasterIndex()
{
    writeMasterIndex();
    emit indexesUpdated();
}

SectionInfo IndexManager::sectionInfo(const QString &section) const
{
    return m_sections.value(section);
}

int IndexManager::totalArticleCount() const
{
    int total = 0;
    for (const auto &info : m_sections) {
        total += info.articleCount;
    }
    return total;
}

void IndexManager::scanSection(const QString &section)
{
    QDir sectionDir(QDir(m_vaultPath).filePath(section));
    if (!sectionDir.exists())
        return;

    SectionInfo info;
    info.name = section;

    QFileInfoList files = sectionDir.entryInfoList(
        QStringList{"*.md"}, QDir::Files, QDir::Name);

    for (const QFileInfo &fi : files) {
        // Skip index files
        if (fi.fileName().startsWith("_"))
            continue;

        info.articles.append(fi.baseName());
        info.articleCount++;
    }

    m_sections[section] = info;
}

void IndexManager::writeSectionIndex(const QString &section, const SectionInfo &info)
{
    QString indexPath = QDir(m_vaultPath).filePath(section + "/_index.md");

    QFile file(indexPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot write section index:" << indexPath;
        return;
    }

    QTextStream out(&file);
    out << "# " << section << "\n\n";
    out << "Article count: " << info.articleCount << "\n";
    out << "Last compiled: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";

    for (const QString &slug : info.articles) {
        out << "- [[" << slug << "]]\n";
    }

    file.close();
}

void IndexManager::writeMasterIndex()
{
    QString indexPath = QDir(m_vaultPath).filePath("_index.md");

    QFile file(indexPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot write master index:" << indexPath;
        return;
    }

    QTextStream out(&file);
    out << "# Arkive Vault Index\n\n";
    out << "Last updated: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";

    int total = 0;
    for (auto it = m_sections.constBegin(); it != m_sections.constEnd(); ++it) {
        const SectionInfo &info = it.value();
        out << "## " << info.name << "\n";
        out << "- Article count: " << info.articleCount << "\n";
        if (info.lastCompiled.isValid()) {
            out << "- Last compiled: " << info.lastCompiled.toString(Qt::ISODate) << "\n";
        } else {
            out << "- Last compiled: never\n";
        }
        out << "\n";
        total += info.articleCount;
    }

    out << "---\n";
    out << "**Total articles:** " << total << "\n";

    file.close();
}
