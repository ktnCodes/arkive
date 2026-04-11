#include "CloudLlmClient.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

CloudLlmClient::CloudLlmClient(QObject *parent)
    : LlmClient(parent)
    , m_model(QStringLiteral("llama3.2"))
    , m_baseUrl(QStringLiteral("http://localhost:11434"))
    , m_provider(Provider::Ollama)
{
}

QString CloudLlmClient::provider() const
{
    switch (m_provider) {
    case Provider::Ollama: return QStringLiteral("Ollama");
    case Provider::Claude: return QStringLiteral("Claude");
    case Provider::OpenAI: return QStringLiteral("OpenAI");
    }
    return QStringLiteral("Unknown");
}

bool CloudLlmClient::ready() const
{
    if (m_provider == Provider::Ollama) {
        return m_ollamaConnected;
    }
    return !m_apiKey.isEmpty();
}

void CloudLlmClient::setApiKey(const QString &key)
{
    if (m_apiKey == key) return;
    m_apiKey = key;
    emit apiKeyChanged();
    emit readyChanged();
}

void CloudLlmClient::setModel(const QString &model)
{
    if (m_model == model) return;
    m_model = model;
    emit modelChanged();
}

void CloudLlmClient::setBaseUrl(const QString &url)
{
    if (m_baseUrl == url) return;
    m_baseUrl = url;
    emit baseUrlChanged();
}

void CloudLlmClient::setBackendProvider(int providerInt)
{
    auto provider = static_cast<Provider>(providerInt);
    if (m_provider == provider) return;
    m_provider = provider;
    switch (m_provider) {
    case Provider::Ollama:
        m_baseUrl = QStringLiteral("http://localhost:11434");
        if (m_model.contains(QStringLiteral("claude")) || m_model.contains(QStringLiteral("gpt"))) {
            m_model = QStringLiteral("llama3.2");
        }
        break;
    case Provider::Claude:
        m_baseUrl = QStringLiteral("https://api.anthropic.com");
        if (!m_model.contains(QStringLiteral("claude"))) {
            m_model = QStringLiteral("claude-sonnet-4-20250514");
        }
        break;
    case Provider::OpenAI:
        m_baseUrl = QStringLiteral("https://api.openai.com");
        if (!m_model.contains(QStringLiteral("gpt"))) {
            m_model = QStringLiteral("gpt-4o");
        }
        break;
    }
    emit baseUrlChanged();
    emit modelChanged();
    emit readyChanged();
}

void CloudLlmClient::checkOllamaConnection()
{
    QUrl url(m_baseUrl + QStringLiteral("/api/tags"));
    QNetworkRequest request(url);
    request.setTransferTimeout(3000);

    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const bool connected = (reply->error() == QNetworkReply::NoError);
        if (connected != m_ollamaConnected) {
            m_ollamaConnected = connected;
            emit ollamaConnectedChanged();
            emit readyChanged();
        }
        if (connected) {
            // Parse available models from response
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            const QJsonArray models = doc.object().value(QStringLiteral("models")).toArray();
            QStringList names;
            for (const QJsonValue &m : models) {
                names.append(m.toObject().value(QStringLiteral("name")).toString());
            }
            qInfo() << "CloudLlmClient: Ollama connected. Available models:" << names;
        } else {
            qInfo() << "CloudLlmClient: Ollama not reachable at" << m_baseUrl;
        }
    });
}

void CloudLlmClient::complete(const QList<LlmMessage> &messages, Callback callback)
{
    if (!ready()) {
        if (m_provider == Provider::Ollama) {
            callback({QString(), false, QStringLiteral("Ollama not connected. Install Ollama and run 'ollama pull llama3.2'"), 0, 0});
        } else {
            callback({QString(), false, QStringLiteral("API key not set"), 0, 0});
        }
        return;
    }

    qInfo() << "CloudLlmClient: sending request to" << provider() << "model:" << m_model;

    if (m_provider == Provider::Claude) {
        sendClaudeRequest(messages, std::move(callback));
    } else {
        // Both Ollama and OpenAI use the same chat completions API format
        sendOpenAiRequest(messages, std::move(callback));
    }
}

void CloudLlmClient::sendClaudeRequest(const QList<LlmMessage> &messages, Callback callback)
{
    QUrl url(m_baseUrl + QStringLiteral("/v1/messages"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");

    QString systemPrompt;
    QJsonArray messagesArray;
    for (const LlmMessage &msg : messages) {
        if (msg.role == QStringLiteral("system")) {
            systemPrompt = msg.content;
            continue;
        }
        QJsonObject msgObj;
        msgObj[QStringLiteral("role")] = msg.role;
        msgObj[QStringLiteral("content")] = msg.content;
        messagesArray.append(msgObj);
    }

    QJsonObject body;
    body[QStringLiteral("model")] = m_model;
    body[QStringLiteral("max_tokens")] = 4096;
    body[QStringLiteral("messages")] = messagesArray;
    if (!systemPrompt.isEmpty()) {
        body[QStringLiteral("system")] = systemPrompt;
    }

    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();

        LlmResponse response;
        const QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            response.error = QStringLiteral("HTTP %1: %2")
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
                .arg(reply->errorString());
            qWarning() << "CloudLlmClient: Claude request failed:" << response.error;
            if (!data.isEmpty()) {
                qWarning() << "CloudLlmClient: response body:" << data.left(500);
            }
            callback(response);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const QJsonObject obj = doc.object();

        // Extract content from Claude's response format
        const QJsonArray contentArray = obj.value(QStringLiteral("content")).toArray();
        for (const QJsonValue &block : contentArray) {
            if (block.toObject().value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
                response.content = block.toObject().value(QStringLiteral("text")).toString();
                break;
            }
        }

        const QJsonObject usage = obj.value(QStringLiteral("usage")).toObject();
        response.promptTokens = usage.value(QStringLiteral("input_tokens")).toInt();
        response.completionTokens = usage.value(QStringLiteral("output_tokens")).toInt();
        response.success = !response.content.isEmpty();

        qInfo() << "CloudLlmClient: Claude response received. tokens:"
                << response.promptTokens << "in /" << response.completionTokens << "out";

        callback(response);
    });
}

void CloudLlmClient::sendOpenAiRequest(const QList<LlmMessage> &messages, Callback callback)
{
    QUrl url(m_baseUrl + QStringLiteral("/v1/chat/completions"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    // Ollama doesn't need auth; OpenAI/compatible APIs need Bearer token
    if (m_provider != Provider::Ollama && !m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    }

    QJsonArray messagesArray;
    for (const LlmMessage &msg : messages) {
        QJsonObject msgObj;
        msgObj[QStringLiteral("role")] = msg.role;
        msgObj[QStringLiteral("content")] = msg.content;
        messagesArray.append(msgObj);
    }

    QJsonObject body;
    body[QStringLiteral("model")] = m_model;
    body[QStringLiteral("max_tokens")] = 4096;
    body[QStringLiteral("messages")] = messagesArray;

    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();

        LlmResponse response;
        const QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            response.error = QStringLiteral("HTTP %1: %2")
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
                .arg(reply->errorString());
            qWarning() << "CloudLlmClient: OpenAI request failed:" << response.error;
            if (!data.isEmpty()) {
                qWarning() << "CloudLlmClient: response body:" << data.left(500);
            }
            callback(response);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const QJsonObject obj = doc.object();

        const QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
        if (!choices.isEmpty()) {
            response.content = choices[0].toObject()
                .value(QStringLiteral("message")).toObject()
                .value(QStringLiteral("content")).toString();
        }

        const QJsonObject usage = obj.value(QStringLiteral("usage")).toObject();
        response.promptTokens = usage.value(QStringLiteral("prompt_tokens")).toInt();
        response.completionTokens = usage.value(QStringLiteral("completion_tokens")).toInt();
        response.success = !response.content.isEmpty();

        qInfo() << "CloudLlmClient: OpenAI response received. tokens:"
                << response.promptTokens << "in /" << response.completionTokens << "out";

        callback(response);
    });
}
