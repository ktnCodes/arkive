#include <gtest/gtest.h>

#include "core/SnapchatIngestor.h"
#include "core/VaultManager.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

class SnapchatIngestorTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    std::unique_ptr<VaultManager> m_vault;
    std::unique_ptr<SnapchatIngestor> m_ingestor;

    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());
        m_vault = std::make_unique<VaultManager>(m_tempDir.filePath("vault"));
        m_vault->ensureVaultExists();
        m_ingestor = std::make_unique<SnapchatIngestor>(m_vault.get());
    }

    QString createTextFile(const QString &relativePath, const QString &content)
    {
        const QString path = m_tempDir.filePath(relativePath);
        QDir().mkpath(QFileInfo(path).dir().absolutePath());
        QFile file(path);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        out << content;
        file.close();
        return path;
    }
};

TEST_F(SnapchatIngestorTest, ParseTimestampSupportsIsoStrings)
{
    const QDateTime parsed = SnapchatIngestor::parseTimestamp(QJsonValue(QStringLiteral("2026-04-03T18:15:00Z")));
    ASSERT_TRUE(parsed.isValid());
    EXPECT_EQ(parsed.toUTC().date(), QDate(2026, 4, 3));
    EXPECT_EQ(parsed.toUTC().time(), QTime(18, 15));

    const QDateTime snapchatUtc = SnapchatIngestor::parseTimestamp(QJsonValue(QStringLiteral("2026-03-07 18:19:57 UTC")));
    ASSERT_TRUE(snapchatUtc.isValid());
    EXPECT_EQ(snapchatUtc.toUTC().date(), QDate(2026, 3, 7));
    EXPECT_EQ(snapchatUtc.toUTC().time().hour(), 18);
    EXPECT_EQ(snapchatUtc.toUTC().time().minute(), 19);
    EXPECT_EQ(snapchatUtc.toUTC().time().second(), 57);

    const QDateTime epochMilliseconds = SnapchatIngestor::parseTimestamp(QJsonValue(1772907597844.0));
    ASSERT_TRUE(epochMilliseconds.isValid());
    EXPECT_EQ(epochMilliseconds.toUTC().date(), QDate(2026, 3, 7));
    EXPECT_EQ(epochMilliseconds.toUTC().time().hour(), 18);
    EXPECT_EQ(epochMilliseconds.toUTC().time().minute(), 19);
    EXPECT_EQ(epochMilliseconds.toUTC().time().second(), 57);
}

TEST_F(SnapchatIngestorTest, ImportExportFolderCreatesPeopleConversationAndMemoryEntries)
{
    createTextFile("bundle/mydata~main/json/friends.json",
        R"JSON({"Friends":[{"Display Name":"Alice","Username":"alice123"},{"Display Name":"Kevin","Username":"meuser"}]})JSON");

    createTextFile("bundle/mydata~main/json/chat_history.json",
        R"JSON({
            "11742185-728e-4797-9f64-ef189de33ad0":[
                {
                    "From":"alice123",
                    "Media Type":"TEXT",
                    "Created":"2026-03-07 18:19:57 UTC",
                    "Content":"hey there",
                    "Conversation Title":"Squad Chat",
                    "IsSender":false,
                    "Created(microseconds)":1772907597844,
                    "IsSaved":true
                },
                {
                    "From":"meuser",
                    "Media Type":"TEXT",
                    "Created":"2026-03-07 18:20:00 UTC",
                    "Content":"reply",
                    "Conversation Title":"Squad Chat",
                    "IsSender":true,
                    "Created(microseconds)":1772907600000,
                    "IsSaved":false
                }
            ]
        })JSON");

    createTextFile("bundle/mydata~main/json/snap_history.json",
        R"JSON({
            "11742185-728e-4797-9f64-ef189de33ad0":[
                {
                    "From":"alice123",
                    "Media Type":"VIDEO",
                    "Created":"2026-03-07 18:21:00 UTC",
                    "Conversation Title":"Squad Chat",
                    "IsSender":false,
                    "Created(microseconds)":1772907660000
                }
            ]
        })JSON");

    createTextFile("bundle/mydata~main/json/memories_history.json",
        R"JSON({"Saved Media":[
                {
                    "Date":"2026-01-01 12:21:11 UTC",
                    "Media Type":"Video",
                    "Location":"Latitude, Longitude: 0.0, 0.0",
                    "Download Link":"https://app.snapchat.com/dmd/memories?sid=first",
                    "Media Download Url":"https://media.example/video-1.mp4"
                },
                {
                    "Date":"2026-01-01 12:22:11 UTC",
                    "Media Type":"Video",
                    "Location":"Austin, TX",
                    "Download Link":"https://app.snapchat.com/dmd/memories?sid=second",
                    "Media Download Url":"https://media.example/video-2.mp4"
                }
            ]})JSON");

    createTextFile("bundle/mydata~main/json/story_history.json",
        R"JSON({"Your Story Views":[{"Story Date":"2026-03-07 01:00:00 UTC","Story Views":5}]})JSON");
    createTextFile("bundle/mydata~main/index.html", R"(<html><body>snapchat export</body></html>)");
    createTextFile("bundle/mydata~main-2/chat_media/placeholder.txt", R"(chat media companion folder)");
    createTextFile("bundle/mydata~main-3/memories/placeholder.txt", R"(memories companion folder)");
    createTextFile("bundle/mydata~main-3/shared_story/placeholder.txt", R"(shared story companion folder)");

    int peopleWritten = -1;
    int conversationsWritten = -1;
    int memoriesWritten = -1;
    QEventLoop loop;

    QObject::connect(
        m_ingestor.get(),
        &SnapchatIngestor::importFinished,
        &loop,
        [&loop, &peopleWritten, &conversationsWritten, &memoriesWritten](int people, int conversations, int memories) {
            peopleWritten = people;
            conversationsWritten = conversations;
            memoriesWritten = memories;
            loop.quit();
        });

    m_ingestor->importExportFolder(m_tempDir.filePath("bundle"));
    if (m_ingestor->running()) {
        loop.exec();
    }

    EXPECT_GE(peopleWritten, 2);
    EXPECT_EQ(conversationsWritten, 1);
    EXPECT_EQ(memoriesWritten, 2);

    const QString alicePath = QDir(m_vault->vaultPath()).filePath("people/snapchat-alice.md");
    const QString conversationPath = QDir(m_vault->vaultPath()).filePath("conversations/snapchat-chat-squad-chat.md");
    const QString memoriesDirPath = QDir(m_vault->vaultPath()).filePath("memories");

    ASSERT_TRUE(QFile::exists(alicePath));
    ASSERT_TRUE(QFile::exists(conversationPath));

    const QString personMarkdown = m_vault->readFile(alicePath);
    const QString conversationMarkdown = m_vault->readFile(conversationPath);

    EXPECT_TRUE(personMarkdown.contains("# Person: Alice"));
    EXPECT_TRUE(personMarkdown.contains("alice123"));
    EXPECT_TRUE(conversationMarkdown.contains("# Snapchat Chat: Squad Chat"));
    EXPECT_TRUE(conversationMarkdown.contains("hey there"));
    EXPECT_TRUE(conversationMarkdown.contains("reply"));
    EXPECT_TRUE(conversationMarkdown.contains("Video snap"));
    EXPECT_TRUE(conversationMarkdown.contains("[saved]"));
    EXPECT_TRUE(conversationMarkdown.contains("- Snap media events: 1"));
    EXPECT_TRUE(conversationMarkdown.contains("[[snapchat-alice]]"));

    QDir memoriesDir(memoriesDirPath);
    const QStringList memoryFiles = memoriesDir.entryList({"*.md"}, QDir::Files, QDir::Name);
    ASSERT_EQ(memoryFiles.size(), 2);

    bool foundSanitizedLocation = false;
    bool foundAustinLocation = false;
    for (const QString &fileName : memoryFiles) {
        const QString memoryMarkdown = m_vault->readFile(memoriesDir.filePath(fileName));
        EXPECT_TRUE(memoryMarkdown.contains("- Media type: Video"));
        EXPECT_TRUE(memoryMarkdown.contains("https://media.example/video-"));
        EXPECT_FALSE(memoryMarkdown.contains("Latitude, Longitude: 0.0, 0.0"));
        if (memoryMarkdown.contains("Austin, TX")) {
            foundAustinLocation = true;
        }
        if (!memoryMarkdown.contains("Austin, TX")) {
            foundSanitizedLocation = true;
        }
    }

    EXPECT_TRUE(foundSanitizedLocation);
    EXPECT_TRUE(foundAustinLocation);
}

TEST_F(SnapchatIngestorTest, ImportFolderWithoutSupportedJsonEmitsEmptyImport)
{
    createTextFile("snapchat/story_history.json",
        R"JSON({"Your Story Views":[{"Story Date":"2026-03-07 01:00:00 UTC","Story Views":5}]})JSON");

    int peopleWritten = -1;
    int conversationsWritten = -1;
    int memoriesWritten = -1;
    QEventLoop loop;

    QObject::connect(
        m_ingestor.get(),
        &SnapchatIngestor::importFinished,
        &loop,
        [&loop, &peopleWritten, &conversationsWritten, &memoriesWritten](int people, int conversations, int memories) {
            peopleWritten = people;
            conversationsWritten = conversations;
            memoriesWritten = memories;
            loop.quit();
        });

    m_ingestor->importExportFolder(m_tempDir.filePath("snapchat"));
    if (m_ingestor->running()) {
        loop.exec();
    }

    EXPECT_EQ(peopleWritten, 0);
    EXPECT_EQ(conversationsWritten, 0);
    EXPECT_EQ(memoriesWritten, 0);
    EXPECT_TRUE(m_ingestor->statusText().contains("No supported Snapchat JSON"));
}

TEST_F(SnapchatIngestorTest, ImportErrorForMissingFolder)
{
    bool errorEmitted = false;
    QEventLoop loop;
    QObject::connect(m_ingestor.get(), &SnapchatIngestor::importError, &loop, [&errorEmitted, &loop](const QString &) {
        errorEmitted = true;
        loop.quit();
    });

    m_ingestor->importExportFolder(m_tempDir.filePath("missing-snapchat"));
    if (m_ingestor->running()) {
        loop.exec();
    }

    EXPECT_TRUE(errorEmitted);
    EXPECT_FALSE(m_ingestor->running());
}