#include "VaultManager.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>

namespace {
QString defaultVaultPath()
{
    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return QDir(home).filePath(".arkive/vault");
}
}

VaultManager::VaultManager(QObject *parent)
    : VaultManager(QString(), parent)
{
}

VaultManager::VaultManager(const QString &vaultPathOverride, QObject *parent)
    : QObject(parent)
{
    // Store vault in user's home directory by default, or use an override for tests.
    m_vaultPath = QDir::cleanPath(vaultPathOverride.isEmpty() ? defaultVaultPath() : vaultPathOverride);

    m_sections = {
        "people",
        "conversations",
        "memories",
        "places",
        "media"
    };

    connect(&m_watcher, &QFileSystemWatcher::fileChanged,
            this, &VaultManager::fileChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &VaultManager::directoryChanged);
}

QString VaultManager::vaultPath() const
{
    return m_vaultPath;
}

QStringList VaultManager::sections() const
{
    return m_sections;
}

void VaultManager::ensureVaultExists()
{
    QDir vaultDir(m_vaultPath);

    if (!vaultDir.exists()) {
        qInfo() << "Creating vault at:" << m_vaultPath;
        vaultDir.mkpath(".");
    }

    // Create section folders
    for (const QString &section : m_sections) {
        QDir sectionDir(vaultDir.filePath(section));
        if (!sectionDir.exists()) {
            vaultDir.mkpath(section);
            qInfo() << "Created section:" << section;
        }
    }

    // Create raw/ folder for ingested data before compilation
    QDir rawDir(vaultDir.filePath("raw"));
    if (!rawDir.exists()) {
        vaultDir.mkpath("raw");
    }

    // Create master _index.md if it doesn't exist
    QString indexPath = vaultDir.filePath("_index.md");
    if (!QFile::exists(indexPath)) {
        QFile indexFile(indexPath);
        if (indexFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&indexFile);
            out << "# Arkive Vault Index\n\n";
            out << "Last updated: (auto-generated)\n\n";
            for (const QString &section : m_sections) {
                out << "## " << section << "\n";
                out << "- Article count: 0\n";
                out << "- Last compiled: never\n\n";
            }
            indexFile.close();
        }
    }

    setupWatcher();
}

QString VaultManager::readFile(const QString &filePath) const
{
    // Security: ensure the path is within the vault
    QDir vaultDir(m_vaultPath);
    QString canonicalVault = vaultDir.canonicalPath();
    QString canonicalFile = QFileInfo(filePath).canonicalFilePath();

    if (canonicalFile.isEmpty() || !canonicalFile.startsWith(canonicalVault)) {
        qWarning() << "Attempted to read file outside vault:" << filePath;
        return QString();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot read file:" << filePath;
        return QString();
    }

    QTextStream in(&file);
    return in.readAll();
}

bool VaultManager::writeFile(const QString &relativePath, const QString &content)
{
    QString fullPath = QDir(m_vaultPath).filePath(relativePath);

    // Security: ensure we stay within the vault
    QDir vaultDir(m_vaultPath);
    QString canonicalVault = vaultDir.canonicalPath();

    // Ensure parent directory exists
    QFileInfo fi(fullPath);
    QDir parentDir = fi.dir();
    if (!parentDir.exists()) {
        parentDir.mkpath(".");
    }

    // Recheck after directory creation
    QString canonicalFile = QFileInfo(fullPath).canonicalFilePath();
    if (!canonicalFile.isEmpty() && !canonicalFile.startsWith(canonicalVault)) {
        qWarning() << "Attempted to write file outside vault:" << fullPath;
        return false;
    }

    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot write file:" << fullPath;
        return false;
    }

    QTextStream out(&file);
    out << content;
    file.close();
    return true;
}

bool VaultManager::vaultExists() const
{
    return QDir(m_vaultPath).exists();
}

void VaultManager::setupWatcher()
{
    // Watch the vault root
    m_watcher.addPath(m_vaultPath);

    // Watch each section directory
    QDir vaultDir(m_vaultPath);
    for (const QString &section : m_sections) {
        QString sectionPath = vaultDir.filePath(section);
        if (QDir(sectionPath).exists()) {
            m_watcher.addPath(sectionPath);
        }
    }

    // Watch raw directory
    QString rawPath = vaultDir.filePath("raw");
    if (QDir(rawPath).exists()) {
        m_watcher.addPath(rawPath);
    }
}
