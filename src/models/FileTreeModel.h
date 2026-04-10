#ifndef FILETREEMODEL_H
#define FILETREEMODEL_H

#include <QAbstractItemModel>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFileInfo>
#include <memory>
#include <vector>

struct TreeNode {
    QString name;
    QString fullPath;
    bool isDir;
    TreeNode *parent = nullptr;
    std::vector<std::unique_ptr<TreeNode>> children;
    int row = 0;
};

class FileTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(int fileCount READ fileCount NOTIFY fileCountChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        FullPathRole,
        IsDirRole,
        IsMarkdownRole
    };

    explicit FileTreeModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString rootPath() const;
    void setRootPath(const QString &path);
    int fileCount() const;

    Q_INVOKABLE QString filePath(const QModelIndex &index) const;

signals:
    void rootPathChanged();
    void fileCountChanged();

public slots:
    void refresh();

private:
    void buildTree(TreeNode *parentNode, const QDir &dir);
    TreeNode *nodeFromIndex(const QModelIndex &index) const;
    void watchRecursive(const QString &path);

    std::unique_ptr<TreeNode> m_root;
    QString m_rootPath;
    QFileSystemWatcher m_watcher;
    int m_fileCount = 0;
};

#endif // FILETREEMODEL_H
