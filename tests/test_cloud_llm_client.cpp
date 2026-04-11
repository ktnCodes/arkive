#include <gtest/gtest.h>

#include "core/CloudLlmClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

class CloudLlmClientTest : public ::testing::Test
{
protected:
    std::unique_ptr<CloudLlmClient> m_client;

    void SetUp() override
    {
        m_client = std::make_unique<CloudLlmClient>();
    }
};

TEST_F(CloudLlmClientTest, DefaultProviderIsOllama)
{
    EXPECT_EQ(m_client->provider(), "Ollama");
    EXPECT_EQ(m_client->backendProvider(), CloudLlmClient::Provider::Ollama);
    EXPECT_TRUE(m_client->baseUrl().contains("localhost:11434"));
}

TEST_F(CloudLlmClientTest, NotReadyWithoutConnection)
{
    // Default is Ollama, not connected
    EXPECT_FALSE(m_client->ready());
}

TEST_F(CloudLlmClientTest, CloudProviderReadyWithApiKey)
{
    m_client->setBackendProvider(static_cast<int>(CloudLlmClient::Provider::Claude));
    EXPECT_FALSE(m_client->ready());
    m_client->setApiKey("sk-test-key");
    EXPECT_TRUE(m_client->ready());
}

TEST_F(CloudLlmClientTest, SetApiKeyEmitsSignals)
{
    m_client->setBackendProvider(static_cast<int>(CloudLlmClient::Provider::Claude));
    bool readyChanged = false;
    bool apiKeyChanged = false;
    QObject::connect(m_client.get(), &CloudLlmClient::readyChanged, [&]() { readyChanged = true; });
    QObject::connect(m_client.get(), &CloudLlmClient::apiKeyChanged, [&]() { apiKeyChanged = true; });

    m_client->setApiKey("sk-test");
    EXPECT_TRUE(readyChanged);
    EXPECT_TRUE(apiKeyChanged);
}

TEST_F(CloudLlmClientTest, SetModelEmitsSignal)
{
    bool modelChanged = false;
    QObject::connect(m_client.get(), &CloudLlmClient::modelChanged, [&]() { modelChanged = true; });

    m_client->setModel("gpt-4o");
    EXPECT_TRUE(modelChanged);
    EXPECT_EQ(m_client->model(), "gpt-4o");
}

TEST_F(CloudLlmClientTest, SwitchToOpenAiProvider)
{
    m_client->setBackendProvider(static_cast<int>(CloudLlmClient::Provider::OpenAI));
    EXPECT_EQ(m_client->provider(), "OpenAI");
    EXPECT_TRUE(m_client->baseUrl().contains("openai"));
}

TEST_F(CloudLlmClientTest, SwitchToClaude)
{
    m_client->setBackendProvider(static_cast<int>(CloudLlmClient::Provider::Claude));
    EXPECT_EQ(m_client->provider(), "Claude");
    EXPECT_TRUE(m_client->baseUrl().contains("anthropic"));
}

TEST_F(CloudLlmClientTest, SwitchBackToOllama)
{
    m_client->setBackendProvider(static_cast<int>(CloudLlmClient::Provider::Claude));
    m_client->setBackendProvider(static_cast<int>(CloudLlmClient::Provider::Ollama));
    EXPECT_EQ(m_client->provider(), "Ollama");
    EXPECT_TRUE(m_client->baseUrl().contains("localhost:11434"));
}

TEST_F(CloudLlmClientTest, CompleteFailsWhenNotReady)
{
    // Default Ollama, not connected
    bool callbackFired = false;
    LlmResponse result;

    m_client->complete({{QStringLiteral("user"), QStringLiteral("hello")}},
        [&](const LlmResponse &response) {
            callbackFired = true;
            result = response;
        });

    EXPECT_TRUE(callbackFired);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error.contains("Ollama"));
}

TEST_F(CloudLlmClientTest, DuplicateApiKeySetSkips)
{
    m_client->setApiKey("key1");
    int signalCount = 0;
    QObject::connect(m_client.get(), &CloudLlmClient::apiKeyChanged, [&]() { signalCount++; });

    m_client->setApiKey("key1");  // same key
    EXPECT_EQ(signalCount, 0);
}

TEST_F(CloudLlmClientTest, DuplicateModelSetSkips)
{
    m_client->setModel("model-a");
    int signalCount = 0;
    QObject::connect(m_client.get(), &CloudLlmClient::modelChanged, [&]() { signalCount++; });

    m_client->setModel("model-a");  // same model
    EXPECT_EQ(signalCount, 0);
}

TEST_F(CloudLlmClientTest, SetBaseUrlEmitsSignal)
{
    bool changed = false;
    QObject::connect(m_client.get(), &CloudLlmClient::baseUrlChanged, [&]() { changed = true; });

    m_client->setBaseUrl("https://custom-api.example.com");
    EXPECT_TRUE(changed);
    EXPECT_EQ(m_client->baseUrl(), "https://custom-api.example.com");
}

TEST_F(CloudLlmClientTest, CompleteSimpleBuildsMessages)
{
    // Verify completeSimple works (without an API key, it fails fast with error)
    bool callbackFired = false;
    m_client->completeSimple("system prompt", "user prompt",
        [&](const LlmResponse &response) {
            callbackFired = true;
            EXPECT_FALSE(response.success);
        });
    EXPECT_TRUE(callbackFired);
}
