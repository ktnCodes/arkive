#ifndef MESSAGEINGESTOR_H
#define MESSAGEINGESTOR_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

#ifndef ARKIVE_PRIVATE
#ifndef ARKIVE_TESTING
#define ARKIVE_PRIVATE private
#else
#define ARKIVE_PRIVATE public
#endif
#endif

class VaultManager;

struct ConversationMessage {
    QDateTime sentAt;
    QString text;
};

struct MessageConversation {
    qint64 chatId = 0;
    QString displayName;
    QStringList participants;
    QList<ConversationMessage> recentMessages;
    int messageCount = 0;
    QDateTime firstMessageAt;
    QDateTime lastMessageAt;
    QString sourceDatabaseName;
};

class MessageIngestor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int totalChats READ totalChats NOTIFY totalChatsChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit MessageIngestor(VaultManager *vault, QObject *parent = nullptr);

    bool running() const { return m_running; }
    int progress() const { return m_progress; }
    int totalChats() const { return m_totalChats; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE void importChatDatabase(const QString &databasePath);
    Q_INVOKABLE void cancel();

signals:
    void runningChanged();
    void progressChanged();
    void totalChatsChanged();
    void statusTextChanged();
    void importFinished(int conversationsWritten, int messagesProcessed);
    void importError(const QString &message);

ARKIVE_PRIVATE:
    struct PendingChat {
        qint64 chatId = 0;
        QString displayName;
    };

    void processNextBatch();
    void finishImport();
    void failImport(const QString &message);
    void cleanupDatabaseConnection();
    MessageConversation loadConversation(qint64 chatId, const QString &displayName) const;
    QStringList loadParticipants(qint64 chatId) const;
    QString conversationTitle(const MessageConversation &conversation) const;
    QString generateSlug(const MessageConversation &conversation) const;
    QString generateMarkdown(const MessageConversation &conversation) const;
    static QDateTime parseAppleTimestamp(qint64 rawValue);

    VaultManager *m_vault;
    bool m_running = false;
    bool m_cancelled = false;
    int m_progress = 0;
    int m_totalChats = 0;
    int m_conversationsWritten = 0;
    int m_messagesProcessed = 0;
    QString m_statusText;
    QString m_databasePath;
    QString m_connectionName;
    QList<PendingChat> m_pendingChats;
};

#endif // MESSAGEINGESTOR_H