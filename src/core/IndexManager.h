#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QDateTime>

struct SectionInfo {
    QString name;
    int articleCount = 0;
    QDateTime lastCompiled;
    QStringList articles;   // List of article slugs
};

class IndexManager : public QObject
{
    Q_OBJECT

public:
    explicit IndexManager(QObject *parent = nullptr);

    // Set the vault root path
    void setVaultPath(const QString &vaultPath);

    // Rebuild all indexes from scratch
    Q_INVOKABLE void rebuildIndexes();

    // Update a single section's index
    Q_INVOKABLE void updateSectionIndex(const QString &section);

    // Update the master index
    Q_INVOKABLE void updateMasterIndex();

    // Get info about a section
    SectionInfo sectionInfo(const QString &section) const;

    // Get total article count across all sections
    Q_INVOKABLE int totalArticleCount() const;

signals:
    void indexesUpdated();

private:
    void scanSection(const QString &section);
    void writeSectionIndex(const QString &section, const SectionInfo &info);
    void writeMasterIndex();

    QString m_vaultPath;
    QHash<QString, SectionInfo> m_sections;
};

#endif // INDEXMANAGER_H
