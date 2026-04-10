#include <gtest/gtest.h>
#include "models/FileTreeModel.h"
#include <QTemporaryDir>
#include <QDir>
#include <QFile>

class FileTreeModelTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    FileTreeModel model;

    void createFile(const QString &relativePath, const QString &content = "test")
    {
        QString path = QDir(m_tempDir.path()).filePath(relativePath);
        QDir().mkpath(QFileInfo(path).dir().absolutePath());
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(content.toUtf8());
        f.close();
    }
};

TEST_F(FileTreeModelTest, RootPathIsEmptyByDefault)
{
    EXPECT_TRUE(model.rootPath().isEmpty());
}

TEST_F(FileTreeModelTest, FileCountIsZeroByDefault)
{
    EXPECT_EQ(model.fileCount(), 0);
}

TEST_F(FileTreeModelTest, SetRootPathTriggersRefresh)
{
    createFile("media/test.md", "# Test");

    model.setRootPath(m_tempDir.path());

    EXPECT_EQ(model.rootPath(), m_tempDir.path());
    EXPECT_GE(model.fileCount(), 1);
}

TEST_F(FileTreeModelTest, CountsOnlyMarkdownFiles)
{
    createFile("photo.jpg", "data");
    createFile("article.md", "# Article");
    createFile("notes.md", "# Notes");

    model.setRootPath(m_tempDir.path());

    EXPECT_EQ(model.fileCount(), 2);
}

TEST_F(FileTreeModelTest, RowCountReflectsTopLevelEntries)
{
    createFile("folder1/a.md", "# A");
    createFile("folder2/b.md", "# B");
    createFile("root.md", "# Root");

    model.setRootPath(m_tempDir.path());

    int topLevelCount = model.rowCount(QModelIndex());
    EXPECT_EQ(topLevelCount, 3); // folder1, folder2, root.md
}

TEST_F(FileTreeModelTest, DirectoryEntriesReturnIsDir)
{
    createFile("media/photo.md", "# Photo");

    model.setRootPath(m_tempDir.path());

    QModelIndex idx = model.index(0, 0, QModelIndex());
    EXPECT_TRUE(idx.isValid());
    EXPECT_TRUE(model.data(idx, FileTreeModel::IsDirRole).toBool());
}

TEST_F(FileTreeModelTest, FileEntriesReturnNotIsDir)
{
    createFile("article.md", "# Article");

    model.setRootPath(m_tempDir.path());

    // Find the file entry (not a directory)
    for (int i = 0; i < model.rowCount(QModelIndex()); ++i) {
        QModelIndex idx = model.index(i, 0, QModelIndex());
        QString name = model.data(idx, FileTreeModel::NameRole).toString();
        if (name == "article.md") {
            EXPECT_FALSE(model.data(idx, FileTreeModel::IsDirRole).toBool());
            EXPECT_TRUE(model.data(idx, FileTreeModel::IsMarkdownRole).toBool());
            return;
        }
    }
    FAIL() << "article.md not found in model";
}

TEST_F(FileTreeModelTest, ChildEntriesAccessibleViaParentIndex)
{
    createFile("media/photo.md", "# Photo");

    model.setRootPath(m_tempDir.path());

    // Find the media directory
    QModelIndex mediaIdx;
    for (int i = 0; i < model.rowCount(QModelIndex()); ++i) {
        QModelIndex idx = model.index(i, 0, QModelIndex());
        if (model.data(idx, FileTreeModel::NameRole).toString() == "media") {
            mediaIdx = idx;
            break;
        }
    }
    ASSERT_TRUE(mediaIdx.isValid());

    // Check children
    EXPECT_EQ(model.rowCount(mediaIdx), 1);
    QModelIndex childIdx = model.index(0, 0, mediaIdx);
    EXPECT_EQ(model.data(childIdx, FileTreeModel::NameRole).toString(), "photo.md");
}

TEST_F(FileTreeModelTest, FullPathRoleReturnsAbsolutePath)
{
    createFile("test.md", "# Test");

    model.setRootPath(m_tempDir.path());

    QModelIndex idx = model.index(0, 0, QModelIndex());
    QString fullPath = model.data(idx, FileTreeModel::FullPathRole).toString();
    EXPECT_FALSE(fullPath.isEmpty());
    EXPECT_TRUE(QFile::exists(fullPath));
}

TEST_F(FileTreeModelTest, HiddenFilesAreExcluded)
{
    createFile(".hidden.md", "# Hidden");
    createFile("visible.md", "# Visible");

    model.setRootPath(m_tempDir.path());

    // Should only see visible.md
    EXPECT_EQ(model.fileCount(), 1);
}

TEST_F(FileTreeModelTest, ColumnCountIsAlwaysOne)
{
    EXPECT_EQ(model.columnCount(QModelIndex()), 1);
}

TEST_F(FileTreeModelTest, RoleNamesContainsExpectedRoles)
{
    auto roles = model.roleNames();
    EXPECT_TRUE(roles.values().contains("name"));
    EXPECT_TRUE(roles.values().contains("fullPath"));
    EXPECT_TRUE(roles.values().contains("isDir"));
    EXPECT_TRUE(roles.values().contains("isMarkdown"));
}
