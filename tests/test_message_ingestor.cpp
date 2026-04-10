#include <gtest/gtest.h>

#include "core/MessageIngestor.h"
#include "core/VaultManager.h"

#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QUuid>

namespace {
qint64 appleNanoseconds(const QDateTime &dateTime)
{
    static const QDateTime appleEpoch(QDate(2001, 1, 1), QTime(0, 0), QTimeZone::UTC);
    return appleEpoch.msecsTo(dateTime.toUTC()) * 1000000LL;
}
}

class MessageIngestorTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    std::unique_ptr<VaultManager> m_vault;
    std::unique_ptr<MessageIngestor> m_ingestor;

    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());
        m_vault = std::make_unique<VaultManager>(m_tempDir.filePath("vault"));
        m_vault->ensureVaultExists();
        m_ingestor = std::make_unique<MessageIngestor>(m_vault.get());
    }

    QString createFixtureDatabase(bool includeDisplayName = true)
    {
        const QString path = m_tempDir.filePath(includeDisplayName ? "chat.db" : "chat_no_name.db");
        const QString connectionName = QString("message_ingestor_fixture_%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(path);
            const bool opened = db.open();
            EXPECT_TRUE(opened) << db.lastError().text().toStdString();
            if (!opened) {
                db = QSqlDatabase();
                QSqlDatabase::removeDatabase(connectionName);
                return path;
            }

            QSqlQuery query(db);
            const auto execOrFail = [&query](const QString &sql) {
                const bool ok = query.exec(sql);
                EXPECT_TRUE(ok) << query.lastError().text().toStdString() << " SQL: " << sql.toStdString();
                return ok;
            };

            if (!execOrFail("CREATE TABLE chat (ROWID INTEGER PRIMARY KEY, display_name TEXT)")) return path;
            if (!execOrFail("CREATE TABLE handle (ROWID INTEGER PRIMARY KEY, id TEXT, uncanonicalized_id TEXT)")) return path;
            if (!execOrFail("CREATE TABLE chat_handle_join (chat_id INTEGER, handle_id INTEGER)")) return path;
            if (!execOrFail("CREATE TABLE message (ROWID INTEGER PRIMARY KEY, handle_id INTEGER, text TEXT, date INTEGER)")) return path;
            if (!execOrFail("CREATE TABLE chat_message_join (chat_id INTEGER, message_id INTEGER)")) return path;

            if (!execOrFail(QString(
                "INSERT INTO chat (ROWID, display_name) VALUES (42, %1)")
                .arg(includeDisplayName ? "'Family Chat'" : "NULL"))) return path;
            if (!execOrFail("INSERT INTO handle (ROWID, id, uncanonicalized_id) VALUES (1, '+15125550123', 'Mom')")) return path;
            if (!execOrFail("INSERT INTO handle (ROWID, id, uncanonicalized_id) VALUES (2, '+15125550124', 'Dad')")) return path;
            if (!execOrFail("INSERT INTO chat_handle_join (chat_id, handle_id) VALUES (42, 1)")) return path;
            if (!execOrFail("INSERT INTO chat_handle_join (chat_id, handle_id) VALUES (42, 2)")) return path;

            const qint64 first = appleNanoseconds(QDateTime(QDate(2026, 4, 3), QTime(18, 15), QTimeZone::UTC));
            const qint64 second = appleNanoseconds(QDateTime(QDate(2026, 4, 3), QTime(18, 17), QTimeZone::UTC));
            const qint64 third = appleNanoseconds(QDateTime(QDate(2026, 4, 3), QTime(18, 20), QTimeZone::UTC));

            if (!execOrFail(QString(
                "INSERT INTO message (ROWID, handle_id, text, date) VALUES (100, 1, 'Dinner at 7?', %1)")
                .arg(first))) return path;
            if (!execOrFail(QString(
                "INSERT INTO message (ROWID, handle_id, text, date) VALUES (101, 2, 'Works for me', %1)")
                .arg(second))) return path;
            if (!execOrFail(QString(
                "INSERT INTO message (ROWID, handle_id, text, date) VALUES (102, 1, 'See you there', %1)")
                .arg(third))) return path;

            if (!execOrFail("INSERT INTO chat_message_join (chat_id, message_id) VALUES (42, 100)")) return path;
            if (!execOrFail("INSERT INTO chat_message_join (chat_id, message_id) VALUES (42, 101)")) return path;
            if (!execOrFail("INSERT INTO chat_message_join (chat_id, message_id) VALUES (42, 102)")) return path;

            db.close();
        }

        QSqlDatabase::removeDatabase(connectionName);
        return path;
    }
};

TEST_F(MessageIngestorTest, ParseAppleTimestampHandlesNanoseconds)
{
    const QDateTime input(QDate(2026, 4, 10), QTime(18, 30), QTimeZone::UTC);
    const QDateTime parsed = MessageIngestor::parseAppleTimestamp(appleNanoseconds(input));

    ASSERT_TRUE(parsed.isValid());
    EXPECT_EQ(parsed.toUTC().date(), input.date());
    EXPECT_EQ(parsed.toUTC().time(), input.time());
}

TEST_F(MessageIngestorTest, GenerateMarkdownContainsConversationFacts)
{
    MessageConversation conversation;
    conversation.chatId = 42;
    conversation.displayName = "Family Chat";
    conversation.participants = {"Mom", "Dad"};
    conversation.messageCount = 3;
    conversation.firstMessageAt = QDateTime(QDate(2026, 4, 3), QTime(13, 15));
    conversation.lastMessageAt = QDateTime(QDate(2026, 4, 3), QTime(13, 20));
    conversation.recentMessages = {
        { QDateTime(QDate(2026, 4, 3), QTime(13, 15)), "Dinner at 7?" },
        { QDateTime(QDate(2026, 4, 3), QTime(13, 17)), "Works for me" }
    };
    conversation.sourceDatabaseName = "chat.db";

    const QString markdown = m_ingestor->generateMarkdown(conversation);

    EXPECT_TRUE(markdown.contains("# Conversation: Family Chat"));
    EXPECT_TRUE(markdown.contains("## Key Facts"));
    EXPECT_TRUE(markdown.contains("Message count: 3"));
    EXPECT_TRUE(markdown.contains("- Mom"));
    EXPECT_TRUE(markdown.contains("Dinner at 7?"));
}

TEST_F(MessageIngestorTest, ImportDatabaseCreatesConversationEntry)
{
    const QString databasePath = createFixtureDatabase();

    int conversationsWritten = -1;
    int messagesProcessed = -1;
    QEventLoop loop;

    QObject::connect(
        m_ingestor.get(),
        &MessageIngestor::importFinished,
        &loop,
        [&loop, &conversationsWritten, &messagesProcessed](int conversations, int messages) {
            conversationsWritten = conversations;
            messagesProcessed = messages;
            loop.quit();
        });

    m_ingestor->importChatDatabase(databasePath);
    if (m_ingestor->running()) {
        loop.exec();
    }

    EXPECT_EQ(conversationsWritten, 1);
    EXPECT_EQ(messagesProcessed, 3);

    const QString outputPath = QDir(m_vault->vaultPath()).filePath("conversations/chat-42.md");
    ASSERT_TRUE(QFile::exists(outputPath));

    const QString content = m_vault->readFile(outputPath);
    EXPECT_TRUE(content.contains("Family Chat"));
    EXPECT_TRUE(content.contains("Mom"));
    EXPECT_TRUE(content.contains("Dad"));
    EXPECT_TRUE(content.contains("Dinner at 7?"));
    EXPECT_TRUE(content.contains("Message count: 3"));
}

TEST_F(MessageIngestorTest, ConversationTitleFallsBackToParticipants)
{
    const QString databasePath = createFixtureDatabase(false);

    QEventLoop loop;
    QObject::connect(m_ingestor.get(), &MessageIngestor::importFinished, &loop, &QEventLoop::quit);

    m_ingestor->importChatDatabase(databasePath);
    if (m_ingestor->running()) {
        loop.exec();
    }

    const QString outputPath = QDir(m_vault->vaultPath()).filePath("conversations/chat-42.md");
    const QString content = m_vault->readFile(outputPath);
    EXPECT_TRUE(content.contains("# Conversation: Mom, Dad"));
}

TEST_F(MessageIngestorTest, ImportErrorForMissingDatabase)
{
    bool errorEmitted = false;
    QObject::connect(
        m_ingestor.get(),
        &MessageIngestor::importError,
        [&errorEmitted](const QString &) {
            errorEmitted = true;
        });

    m_ingestor->importChatDatabase(m_tempDir.filePath("missing-chat.db"));

    EXPECT_TRUE(errorEmitted);
    EXPECT_FALSE(m_ingestor->running());
}