#include "FileTreeModel.h"
#include <QDebug>
#include <algorithm>

FileTreeModel::FileTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &FileTreeModel::refresh);
}

void FileTreeModel::setRootPath(const QString &path)
{
    if (m_rootPath == path)
        return;

    m_rootPath = path;

    // Clear old watchers
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());

    refresh();
    emit rootPathChanged();
}

QString FileTreeModel::rootPath() const
{
    return m_rootPath;
}

int FileTreeModel::fileCount() const
{
    return m_fileCount;
}

void FileTreeModel::refresh()
{
    beginResetModel();

    m_root = std::make_unique<TreeNode>();
    m_root->name = QFileInfo(m_rootPath).fileName();
    m_root->fullPath = m_rootPath;
    m_root->isDir = true;
    m_fileCount = 0;

    QDir rootDir(m_rootPath);
    if (rootDir.exists()) {
        buildTree(m_root.get(), rootDir);
        watchRecursive(m_rootPath);
    }

    endResetModel();
    emit fileCountChanged();
}

void FileTreeModel::buildTree(TreeNode *parentNode, const QDir &dir)
{
    // Get directories first (sorted), then files (sorted)
    QFileInfoList entries = dir.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    for (const QFileInfo &info : entries) {
        // Skip hidden files and _index files at this stage
        if (info.fileName().startsWith("."))
            continue;

        auto node = std::make_unique<TreeNode>();
        node->name = info.fileName();
        node->fullPath = info.absoluteFilePath();
        node->isDir = info.isDir();
        node->parent = parentNode;
        node->row = parentNode->children.size();

        if (info.isDir()) {
            buildTree(node.get(), QDir(info.absoluteFilePath()));
        } else {
            // Only count markdown files
            if (info.suffix().toLower() == "md") {
                m_fileCount++;
            }
        }

        parentNode->children.push_back(std::move(node));
    }
}

void FileTreeModel::watchRecursive(const QString &path)
{
    m_watcher.addPath(path);

    QDir dir(path);
    QFileInfoList entries = dir.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo &info : entries) {
        if (!info.fileName().startsWith(".")) {
            watchRecursive(info.absoluteFilePath());
        }
    }
}

TreeNode *FileTreeModel::nodeFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return m_root.get();

    return static_cast<TreeNode *>(index.internalPointer());
}

QModelIndex FileTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!m_root || column != 0)
        return QModelIndex();

    TreeNode *parentNode = nodeFromIndex(parent);
    if (!parentNode || row < 0 || row >= parentNode->children.size())
        return QModelIndex();

    return createIndex(row, column, parentNode->children[row].get());
}

QModelIndex FileTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    TreeNode *node = nodeFromIndex(child);
    if (!node || !node->parent || node->parent == m_root.get())
        return QModelIndex();

    return createIndex(node->parent->row, 0, node->parent);
}

int FileTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!m_root)
        return 0;

    TreeNode *node = nodeFromIndex(parent);
    return node ? node->children.size() : 0;
}

int FileTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant FileTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TreeNode *node = nodeFromIndex(index);
    if (!node)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return node->name;
    case FullPathRole:
        return node->fullPath;
    case IsDirRole:
        return node->isDir;
    case IsMarkdownRole:
        return !node->isDir && node->name.endsWith(".md", Qt::CaseInsensitive);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> FileTreeModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {FullPathRole, "fullPath"},
        {IsDirRole, "isDir"},
        {IsMarkdownRole, "isMarkdown"}
    };
}

QString FileTreeModel::filePath(const QModelIndex &index) const
{
    TreeNode *node = nodeFromIndex(index);
    return node ? node->fullPath : QString();
}
