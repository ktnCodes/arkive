#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

struct LlmMessage {
    QString role;    // "system", "user", "assistant"
    QString content;
};

struct LlmResponse {
    QString content;
    bool success = false;
    QString error;
    int promptTokens = 0;
    int completionTokens = 0;
};

// Abstract interface for LLM completion backends.
// Subclasses implement the actual HTTP call or local inference.
class LlmClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString provider READ provider CONSTANT)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)

public:
    using Callback = std::function<void(const LlmResponse &)>;

    explicit LlmClient(QObject *parent = nullptr) : QObject(parent) {}

    virtual QString provider() const = 0;
    virtual bool ready() const = 0;

    // Send a chat completion request. Callback fires on the caller's thread.
    virtual void complete(const QList<LlmMessage> &messages, Callback callback) = 0;

    // Convenience: single user prompt with optional system prompt.
    void completeSimple(const QString &systemPrompt, const QString &userPrompt, Callback callback)
    {
        QList<LlmMessage> messages;
        if (!systemPrompt.isEmpty()) {
            messages.append({QStringLiteral("system"), systemPrompt});
        }
        messages.append({QStringLiteral("user"), userPrompt});
        complete(messages, std::move(callback));
    }

signals:
    void readyChanged();
};

#endif // LLMCLIENT_H
