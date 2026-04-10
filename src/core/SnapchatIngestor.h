#ifndef SNAPCHATINGESTOR_H
#define SNAPCHATINGESTOR_H

#include <QDateTime>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

class QThread;

#ifndef ARKIVE_PRIVATE
#ifndef ARKIVE_TESTING
#define ARKIVE_PRIVATE private
#else
#define ARKIVE_PRIVATE public
#endif
#endif

class VaultManager;

struct SnapchatConversationMessage {
    QDateTime sentAt;
    QString senderSlug;
    QString senderLabel;
    QString text;
    QString mediaType;
    bool isOutgoing = false;
    bool isSaved = false;
};

struct SnapchatPerson {
    QString slug;
    QString displayName;
    QString username;
    QStringList sourceFiles;
};

struct SnapchatConversation {
    QString slug;
    QString title;
    QStringList participantSlugs;
    QStringList participantNames;
    QList<SnapchatConversationMessage> recentMessages;
    int messageCount = 0;
    int textMessageCount = 0;
    int mediaMessageCount = 0;
    int savedMessageCount = 0;
    QDateTime firstMessageAt;
    QDateTime lastMessageAt;
    QStringList sourceFiles;
};

struct SnapchatMemory {
    QString slug;
    QString title;
    QString caption;
    QString mediaPath;
    QString mediaType;
    QString location;
    QDateTime createdAt;
    QStringList participantSlugs;
    QStringList participantNames;
    QStringList sourceFiles;
};

class SnapchatIngestor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int totalFiles READ totalFiles NOTIFY totalFilesChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit SnapchatIngestor(VaultManager *vault, QObject *parent = nullptr);
    ~SnapchatIngestor() override;

    bool running() const { return m_running; }
    int progress() const { return m_progress; }
    int totalFiles() const { return m_totalFiles; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE void importExportFolder(const QString &folderPath);
    Q_INVOKABLE void cancel();

signals:
    void runningChanged();
    void progressChanged();
    void totalFilesChanged();
    void statusTextChanged();
    void progressValueChanged(int progress);
    void totalFilesValueChanged(int totalFiles);
    void statusTextValueChanged(const QString &statusText);
    void importFinished(int peopleWritten, int conversationsWritten, int memoriesWritten);
    void importError(const QString &message);

ARKIVE_PRIVATE:
    void startWorkerImport(const QString &folderPath);
    void runImportOnCurrentThread(const QString &folderPath);
    void cleanupWorker();
    void setProgressValue(int progress);
    void setTotalFilesValue(int totalFiles);
    void setStatusTextValue(const QString &statusText);
    void processNextBatch();
    void finishImport();
    void failImport(const QString &message);
    void resetImportState();
    void parseJsonFile(const QString &filePath);
    void parseFriendsValue(const QJsonValue &value, const QString &sourceFile, bool inFriendContext = false);
    void parseChatsValue(const QJsonValue &value, const QString &sourceFile, bool inChatContext = false);
    void parseMemoriesValue(const QJsonValue &value, const QString &sourceFile, bool inMemoryContext = false);
    SnapchatPerson &ensurePerson(const QString &displayName, const QString &username, const QString &sourceFile);
    void addConversationMessage(const QString &title, const QStringList &participants, const QString &sender,
        const QString &text, const QString &mediaType, const QDateTime &sentAt, bool isOutgoing,
        bool isSaved, const QString &sourceFile);
    void addMemoryEntry(const QString &title, const QString &caption, const QString &mediaPath,
        const QString &mediaType, const QString &location, const QDateTime &createdAt,
        const QStringList &participants, const QString &sourceFile);
    void writePeopleEntries();
    void writeConversationEntries();
    void writeMemoryEntries();
    QString generatePersonMarkdown(const SnapchatPerson &person) const;
    QString generateConversationMarkdown(const SnapchatConversation &conversation) const;
    QString generateMemoryMarkdown(const SnapchatMemory &memory) const;
    QString displayNameForSlug(const QString &slug, const QString &fallback = {}) const;
    static QDateTime parseTimestamp(const QJsonValue &value);

    VaultManager *m_vault;
    QThread *m_workerThread = nullptr;
    SnapchatIngestor *m_worker = nullptr;
    bool m_workerMode = false;
    bool m_running = false;
    bool m_cancelled = false;
    int m_progress = 0;
    int m_totalFiles = 0;
    QString m_statusText;
    QStringList m_pendingFiles;
    QMap<QString, SnapchatPerson> m_people;
    QMap<QString, QString> m_personSlugsByUsername;
    QMap<QString, SnapchatConversation> m_conversations;
    QList<SnapchatMemory> m_memories;
    QSet<QString> m_seenChatMessages;
    QSet<QString> m_seenMemories;
    QSet<QString> m_reservedMemorySlugs;
    int m_peopleWritten = 0;
    int m_conversationsWritten = 0;
    int m_memoriesWritten = 0;
};

#endif // SNAPCHATINGESTOR_H