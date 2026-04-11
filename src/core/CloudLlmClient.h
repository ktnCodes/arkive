#ifndef CLOUDLLMCLIENT_H
#define CLOUDLLMCLIENT_H

#include "LlmClient.h"

#include <QNetworkAccessManager>
#include <QString>

#ifndef ARKIVE_TESTING
#define CLOUD_LLM_PRIVATE private
#else
#define CLOUD_LLM_PRIVATE public
#endif

// LLM backend supporting Ollama (local, free), Claude (Anthropic), and OpenAI-compatible APIs.
class CloudLlmClient : public LlmClient
{
    Q_OBJECT
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(bool ollamaConnected READ ollamaConnected NOTIFY ollamaConnectedChanged)

public:
    enum class Provider { Ollama, Claude, OpenAI };
    Q_ENUM(Provider)

    explicit CloudLlmClient(QObject *parent = nullptr);

    QString provider() const override;
    bool ready() const override;

    void complete(const QList<LlmMessage> &messages, Callback callback) override;

    QString apiKey() const { return m_apiKey; }
    void setApiKey(const QString &key);

    QString model() const { return m_model; }
    void setModel(const QString &model);

    QString baseUrl() const { return m_baseUrl; }
    void setBaseUrl(const QString &url);

    Provider backendProvider() const { return m_provider; }
    Q_INVOKABLE void setBackendProvider(int provider);

    bool ollamaConnected() const { return m_ollamaConnected; }

    // Ping Ollama to check if it's running. Updates ollamaConnected property.
    Q_INVOKABLE void checkOllamaConnection();

signals:
    void apiKeyChanged();
    void modelChanged();
    void baseUrlChanged();
    void ollamaConnectedChanged();

CLOUD_LLM_PRIVATE:
    void sendClaudeRequest(const QList<LlmMessage> &messages, Callback callback);
    void sendOpenAiRequest(const QList<LlmMessage> &messages, Callback callback);

    QNetworkAccessManager m_network;
    QString m_apiKey;
    QString m_model;
    QString m_baseUrl;
    Provider m_provider = Provider::Ollama;
    bool m_ollamaConnected = false;
};

#endif // CLOUDLLMCLIENT_H
