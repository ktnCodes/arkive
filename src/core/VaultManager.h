#ifndef VAULTMANAGER_H
#define VAULTMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileSystemWatcher>

class VaultManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString vaultPath READ vaultPath CONSTANT)
    Q_PROPERTY(QStringList sections READ sections CONSTANT)

public:
    explicit VaultManager(QObject *parent = nullptr);

    QString vaultPath() const;
    QStringList sections() const;

    // Create vault directory structure on first run
    Q_INVOKABLE void ensureVaultExists();

    // Read a file's content
    Q_INVOKABLE QString readFile(const QString &filePath) const;

    // Write content to a file in the vault
    Q_INVOKABLE bool writeFile(const QString &relativePath, const QString &content);

    // Check if vault exists
    Q_INVOKABLE bool vaultExists() const;

signals:
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);

private:
    void setupWatcher();

    QString m_vaultPath;
    QStringList m_sections;
    QFileSystemWatcher m_watcher;
};

#endif // VAULTMANAGER_H
