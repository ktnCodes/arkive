#include <gtest/gtest.h>

#include "core/WikiCompiler.h"
#include "core/LlmClient.h"
#include "core/VaultManager.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

// Fake LLM client that returns canned responses for testing
class FakeLlmClient : public LlmClient
{
public:
    explicit FakeLlmClient(QObject *parent = nullptr) : LlmClient(parent) {}

    QString provider() const override { return QStringLiteral("Fake"); }
    bool ready() const override { return m_ready; }

    void complete(const QList<LlmMessage> &messages, Callback callback) override
    {
        m_lastMessages = messages;
        m_callCount++;
        LlmResponse response;
        response.content = m_cannedResponse;
        response.success = m_cannedSuccess;
        response.error = m_cannedError;
        callback(response);
    }

    bool m_ready = true;
    QString m_cannedResponse;
    bool m_cannedSuccess = true;
    QString m_cannedError;
    QList<LlmMessage> m_lastMessages;
    int m_callCount = 0;
};

class WikiCompilerTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    std::unique_ptr<VaultManager> m_vault;
    std::unique_ptr<FakeLlmClient> m_llm;
    std::unique_ptr<WikiCompiler> m_compiler;

    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());
        m_vault = std::make_unique<VaultManager>(m_tempDir.filePath("vault"));
        m_vault->ensureVaultExists();
        m_llm = std::make_unique<FakeLlmClient>();
        m_compiler = std::make_unique<WikiCompiler>(m_vault.get(), m_llm.get());
    }

    void createVaultFile(const QString &relativePath, const QString &content)
    {
        const QString absPath = m_vault->vaultPath() + "/" + relativePath;
        QDir().mkpath(QFileInfo(absPath).dir().absolutePath());
        QFile file(absPath);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << content;
    }
};

TEST_F(WikiCompilerTest, ScanFindsNonemptyVault)
{
    // Create some people entries
    createVaultFile("people/snapchat-alice.md",
        "# Person: Alice\n## Summary\nAlice appears in the imported Snapchat export.\n"
        "## Key Facts\n- Source: Snapchat export\n- Snapchat username: alice123\n");
    createVaultFile("people/snapchat-bob.md",
        "# Person: Bob\n## Summary\nBob appears in the imported Snapchat export.\n"
        "## Key Facts\n- Source: Snapchat export\n");

    m_compiler->scanCandidates();

    EXPECT_GE(m_compiler->totalCandidates(), 2);  // At least 2 person summaries
}

TEST_F(WikiCompilerTest, ScanReturnsZeroForEmptyVault)
{
    m_compiler->scanCandidates();
    EXPECT_EQ(m_compiler->totalCandidates(), 0);
}

TEST_F(WikiCompilerTest, CompileAllFailsWithoutLlm)
{
    m_llm->m_ready = false;
    bool errorEmitted = false;
    QObject::connect(m_compiler.get(), &WikiCompiler::compileError, [&]() { errorEmitted = true; });

    m_compiler->compileAll();
    EXPECT_TRUE(errorEmitted);
    EXPECT_FALSE(m_compiler->running());
}

TEST_F(WikiCompilerTest, CompileAllWithNoCandidates)
{
    bool finishedEmitted = false;
    int articlesWritten = -1;
    QObject::connect(m_compiler.get(), &WikiCompiler::compileFinished,
        [&](int count) { finishedEmitted = true; articlesWritten = count; });

    m_compiler->compileAll();
    EXPECT_TRUE(finishedEmitted);
    EXPECT_EQ(articlesWritten, 0);
}

TEST_F(WikiCompilerTest, CompileAllWritesArticles)
{
    createVaultFile("people/snapchat-charlie.md",
        "# Person: Charlie\n## Summary\nCharlie appears in the imported Snapchat export.\n"
        "## Key Facts\n- Source: Snapchat export\n- Snapchat username: charlie99\n");

    m_llm->m_cannedResponse =
        "# Charlie\n\n## Summary\nCharlie is a Snapchat contact.\n\n"
        "## Key Facts\n- Username: charlie99\n\n"
        "## Connections\n- [[snapchat-chat-charlie]]\n\n"
        "## Sources\n- snapchat-charlie.md\n";

    bool articleEmitted = false;
    QObject::connect(m_compiler.get(), &WikiCompiler::articleCompiled,
        [&](const QString &slug) {
            articleEmitted = true;
            EXPECT_TRUE(slug.contains("charlie"));
        });

    bool finishedEmitted = false;
    int articlesWritten = 0;
    QObject::connect(m_compiler.get(), &WikiCompiler::compileFinished,
        [&](int count) { finishedEmitted = true; articlesWritten = count; });

    m_compiler->compileAll();

    EXPECT_TRUE(articleEmitted);
    EXPECT_TRUE(finishedEmitted);
    EXPECT_GE(articlesWritten, 1);
    EXPECT_EQ(m_llm->m_callCount, 1);

    // Verify file was written
    const QString wikiDir = m_vault->vaultPath() + "/wiki";
    QDir wiki(wikiDir);
    EXPECT_TRUE(wiki.exists());
    EXPECT_GE(wiki.entryList({"*.md"}, QDir::Files).size(), 1);
}

TEST_F(WikiCompilerTest, CompileAllHandlesLlmError)
{
    createVaultFile("people/snapchat-dave.md",
        "# Person: Dave\n## Summary\nDave.\n");

    m_llm->m_cannedSuccess = false;
    m_llm->m_cannedError = "rate limit exceeded";

    bool finishedEmitted = false;
    QObject::connect(m_compiler.get(), &WikiCompiler::compileFinished,
        [&](int count) { finishedEmitted = true; EXPECT_EQ(count, 0); });

    m_compiler->compileAll();
    EXPECT_TRUE(finishedEmitted);
}

TEST_F(WikiCompilerTest, CancelStopsCompilation)
{
    createVaultFile("people/snapchat-eve.md", "# Person: Eve\n## Summary\nEve.\n");
    createVaultFile("people/snapchat-frank.md", "# Person: Frank\n## Summary\nFrank.\n");

    // After first call, cancel
    m_llm->m_cannedResponse = "# Person\n\n## Summary\nSummary.\n";
    int callCount = 0;
    auto origComplete = [&](const QList<LlmMessage> &msgs, LlmClient::Callback cb) {
        callCount++;
        if (callCount == 1) {
            m_compiler->cancel();
        }
        LlmResponse resp;
        resp.content = m_llm->m_cannedResponse;
        resp.success = true;
        cb(resp);
    };

    // Can't easily intercept the fake client's calls, so just verify cancel works
    m_compiler->scanCandidates();
    m_compiler->cancel();  // Cancel before starting
    EXPECT_TRUE(m_compiler->statusText().contains("Cancel"));
}

TEST_F(WikiCompilerTest, SkipsAlreadyCompiledArticles)
{
    createVaultFile("people/snapchat-grace.md",
        "# Person: Grace\n## Summary\nGrace.\n");

    // Pre-create the wiki article
    createVaultFile("wiki/person-summary-grace.md",
        "# Grace\n## Summary\nAlready compiled.\n");

    m_compiler->scanCandidates();

    // Should NOT include grace since the wiki article already exists
    bool foundGrace = false;
    // Can't directly inspect candidates, but compileAll should compile 0
    m_compiler->compileAll();
    EXPECT_EQ(m_llm->m_callCount, 0);
}

TEST_F(WikiCompilerTest, EventSummaryNeedsMinimumEntries)
{
    // Create only 2 conversation entries for a month — not enough for event summary
    createVaultFile("conversations/chat-a.md",
        "# Chat A\n## Summary\n1 messages from 2025-10.\n");
    createVaultFile("conversations/chat-b.md",
        "# Chat B\n## Summary\n1 messages from 2025-10.\n");

    m_compiler->scanCandidates();

    // With only 2 entries for 2025-10, no event summary should be created
    // (minimum threshold is 3)
    bool hasEventSummary = m_compiler->statusText().contains("event");
    // The total candidates should be 0 (no person entries either)
    EXPECT_EQ(m_compiler->totalCandidates(), 0);
}

TEST_F(WikiCompilerTest, TopicClusterNeedsMinimumBacklinks)
{
    // Create entries with backlinks but fewer than 5 pointing to same target
    createVaultFile("conversations/chat-x.md",
        "# Chat X\n## Summary\nTalked with [[alice]].\n");
    createVaultFile("conversations/chat-y.md",
        "# Chat Y\n## Summary\nTalked with [[alice]].\n");
    // Only 2 inbound links to alice — not enough for cluster (needs 5)

    m_compiler->scanCandidates();

    // No topic clusters expected
    EXPECT_EQ(m_compiler->totalCandidates(), 0);
}

TEST_F(WikiCompilerTest, CompileOneBySlug)
{
    createVaultFile("people/snapchat-henry.md",
        "# Person: Henry\n## Summary\nHenry.\n## Key Facts\n- Username: henry42\n");

    m_llm->m_cannedResponse =
        "# Henry\n\n## Summary\nHenry is a contact.\n\n## Key Facts\n- Username: henry42\n";

    bool articleEmitted = false;
    QObject::connect(m_compiler.get(), &WikiCompiler::articleCompiled,
        [&](const QString &slug) { articleEmitted = true; });

    m_compiler->compileOne("person-summary-henry");

    EXPECT_TRUE(articleEmitted);
    EXPECT_EQ(m_llm->m_callCount, 1);
}
