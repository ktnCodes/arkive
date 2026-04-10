#include <gtest/gtest.h>
#include "core/VaultManager.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

class VaultManagerTest : public ::testing::Test
{
protected:
    VaultManager vault;
};

TEST_F(VaultManagerTest, VaultPathIsNotEmpty)
{
    EXPECT_FALSE(vault.vaultPath().isEmpty());
}

TEST_F(VaultManagerTest, VaultPathEndsWithArkiveVault)
{
    EXPECT_TRUE(vault.vaultPath().endsWith(".arkive/vault") ||
                vault.vaultPath().endsWith(".arkive\\vault"));
}

TEST_F(VaultManagerTest, SectionsContainsExpectedDefaults)
{
    QStringList sections = vault.sections();

    EXPECT_TRUE(sections.contains("people"));
    EXPECT_TRUE(sections.contains("conversations"));
    EXPECT_TRUE(sections.contains("memories"));
    EXPECT_TRUE(sections.contains("places"));
    EXPECT_TRUE(sections.contains("media"));
}

TEST_F(VaultManagerTest, SectionsHasExactlyFive)
{
    EXPECT_EQ(vault.sections().size(), 5);
}

TEST_F(VaultManagerTest, EnsureVaultExistsCreatesDirectories)
{
    vault.ensureVaultExists();

    QDir vaultDir(vault.vaultPath());
    EXPECT_TRUE(vaultDir.exists());

    for (const QString &section : vault.sections()) {
        EXPECT_TRUE(QDir(vaultDir.filePath(section)).exists())
            << "Section missing: " << section.toStdString();
    }

    EXPECT_TRUE(QDir(vaultDir.filePath("raw")).exists());
}

TEST_F(VaultManagerTest, EnsureVaultExistsCreatesMasterIndex)
{
    vault.ensureVaultExists();

    QString indexPath = QDir(vault.vaultPath()).filePath("_index.md");
    EXPECT_TRUE(QFile::exists(indexPath));
}

TEST_F(VaultManagerTest, VaultExistsReturnsTrueAfterCreation)
{
    vault.ensureVaultExists();
    EXPECT_TRUE(vault.vaultExists());
}

TEST_F(VaultManagerTest, WriteAndReadFileRoundtrip)
{
    vault.ensureVaultExists();

    QString content = "# Test Article\n\nHello world.";
    bool written = vault.writeFile("media/test-roundtrip.md", content);
    EXPECT_TRUE(written);

    QString fullPath = QDir(vault.vaultPath()).filePath("media/test-roundtrip.md");
    QString readBack = vault.readFile(fullPath);
    EXPECT_EQ(readBack, content);

    // Cleanup
    QFile::remove(fullPath);
}

TEST_F(VaultManagerTest, WriteFileCreatesParentDirectories)
{
    vault.ensureVaultExists();

    bool written = vault.writeFile("media/sub/nested/file.md", "content");
    EXPECT_TRUE(written);

    QString fullPath = QDir(vault.vaultPath()).filePath("media/sub/nested/file.md");
    EXPECT_TRUE(QFile::exists(fullPath));

    // Cleanup
    QFile::remove(fullPath);
    QDir(vault.vaultPath()).rmpath("media/sub/nested");
}

TEST_F(VaultManagerTest, ReadFileRejectsPathOutsideVault)
{
    vault.ensureVaultExists();

    // Try to read a file outside the vault
    QString outsidePath = QDir::tempPath() + "/outside.txt";
    QFile f(outsidePath);
    f.open(QIODevice::WriteOnly);
    f.write("secret");
    f.close();

    QString result = vault.readFile(outsidePath);
    EXPECT_TRUE(result.isEmpty());

    QFile::remove(outsidePath);
}

TEST_F(VaultManagerTest, ReadNonexistentFileReturnsEmpty)
{
    vault.ensureVaultExists();

    QString result = vault.readFile(QDir(vault.vaultPath()).filePath("nonexistent.md"));
    EXPECT_TRUE(result.isEmpty());
}
