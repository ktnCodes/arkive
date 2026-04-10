#include "MessageIngestor.h"

#include "VaultManager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimer>
#include <QTimeZone>
#include <QUuid>

namespace {
QString connectionNameForImport()
{
    return QString("arkive_message_import_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

bool tableHasColumn(const QSqlDatabase &db, const QString &tableName, const QString &columnName)
{
    QSqlQuery query(db);
    if (!query.exec(QString("PRAGMA table_info(%1)").arg(tableName))) {
        return false;
    }

    while (query.next()) {
        if (query.value(1).toString() == columnName) {
            return true;
        }
    }
    return false;
}

QString handleLabelExpression(const QSqlDatabase &db)
{
    QStringList candidates;
    if (tableHasColumn(db, "handle", "uncanonicalized_id")) {
        candidates << "NULLIF(handle.uncanonicalized_id, '')";
    }
    if (tableHasColumn(db, "handle", "id")) {
        candidates << "NULLIF(handle.id, '')";
    }
    candidates << "'Unknown'";
    return QString("COALESCE(%1)").arg(candidates.join(", "));
}

QString sanitizeMessageText(const QString &text)
{
    QString clean = text.simplified();
    if (clean.size() > 140) {
        clean = clean.left(137) + "...";
    }
    return clean;
}
}

MessageIngestor::MessageIngestor(VaultManager *vault, QObject *parent)
    : QObject(parent), m_vault(vault)
{
}

void MessageIngestor::importChatDatabase(const QString &databasePath)
{
    if (m_running) {
        return;
    }

    QFileInfo dbInfo(databasePath);
    if (!dbInfo.exists() || !dbInfo.isFile()) {
        failImport("chat.db not found: " + databasePath);
        return;
    }

    m_running = true;
    m_cancelled = false;
    m_progress = 0;
    m_totalChats = 0;
    m_conversationsWritten = 0;
    m_messagesProcessed = 0;
    m_statusText = "Opening chat database...";
    m_databasePath = dbInfo.absoluteFilePath();
    m_pendingChats.clear();
    emit runningChanged();
    emit progressChanged();
    emit totalChatsChanged();
    emit statusTextChanged();

    m_connectionName = connectionNameForImport();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setConnectOptions("QSQLITE_OPEN_READONLY");
    db.setDatabaseName(m_databasePath);

    if (!db.open()) {
        failImport("Failed to open chat.db: " + db.lastError().text());
        return;
    }

    const QStringList tables = db.tables();
    const QStringList requiredTables = {"chat", "message", "chat_message_join"};
    for (const QString &table : requiredTables) {
        if (!tables.contains(table)) {
            failImport("chat.db is missing required table: " + table);
            return;
        }
    }

    const bool hasDisplayName = tableHasColumn(db, "chat", "display_name");
    QSqlQuery chatQuery(db);
    chatQuery.prepare(
        QString("SELECT ROWID, %1 FROM chat ORDER BY ROWID")
            .arg(hasDisplayName ? "COALESCE(display_name, '')" : "''"));

    if (!chatQuery.exec()) {
        failImport("Failed to query chats: " + chatQuery.lastError().text());
        return;
    }

    while (chatQuery.next()) {
        PendingChat pending;
        pending.chatId = chatQuery.value(0).toLongLong();
        pending.displayName = chatQuery.value(1).toString().trimmed();
        m_pendingChats.append(pending);
    }

    m_totalChats = m_pendingChats.size();
    emit totalChatsChanged();

    if (m_totalChats == 0) {
        finishImport();
        return;
    }

    m_statusText = QString("Importing %1 chat(s)...").arg(m_totalChats);
    emit statusTextChanged();
    QTimer::singleShot(0, this, &MessageIngestor::processNextBatch);
}

void MessageIngestor::cancel()
{
    m_cancelled = true;
}

void MessageIngestor::processNextBatch()
{
    if (m_cancelled) {
        cleanupDatabaseConnection();
        m_statusText = "iMessage import cancelled.";
        emit statusTextChanged();
        m_running = false;
        emit runningChanged();
        return;
    }

    const int batchSize = qMin(10, m_pendingChats.size());
    for (int index = 0; index < batchSize; ++index) {
        const PendingChat pending = m_pendingChats.takeFirst();
        const MessageConversation conversation = loadConversation(pending.chatId, pending.displayName);

        if (conversation.messageCount > 0) {
            const QString relativePath = "conversations/" + generateSlug(conversation) + ".md";
            if (m_vault->writeFile(relativePath, generateMarkdown(conversation))) {
                m_conversationsWritten++;
                m_messagesProcessed += conversation.messageCount;
            }
        }

        m_progress++;
        emit progressChanged();
    }

    if (!m_pendingChats.isEmpty()) {
        m_statusText = QString("Importing chats... %1/%2").arg(m_progress).arg(m_totalChats);
        emit statusTextChanged();
        QTimer::singleShot(0, this, &MessageIngestor::processNextBatch);
        return;
    }

    finishImport();
}

void MessageIngestor::finishImport()
{
    cleanupDatabaseConnection();

    if (m_totalChats == 0) {
        m_statusText = "No chats found in chat.db.";
    } else {
        m_statusText = QString("Done! Imported %1 conversation(s) from %2 message(s).")
            .arg(m_conversationsWritten)
            .arg(m_messagesProcessed);
    }
    emit statusTextChanged();

    m_running = false;
    emit runningChanged();
    emit importFinished(m_conversationsWritten, m_messagesProcessed);
}

void MessageIngestor::failImport(const QString &message)
{
    cleanupDatabaseConnection();
    m_statusText = message;
    emit statusTextChanged();

    if (m_running) {
        m_running = false;
        emit runningChanged();
    }

    emit importError(message);
}

void MessageIngestor::cleanupDatabaseConnection()
{
    if (m_connectionName.isEmpty()) {
        return;
    }

    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid() && db.isOpen()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    m_connectionName.clear();
}

MessageConversation MessageIngestor::loadConversation(qint64 chatId, const QString &displayName) const
{
    MessageConversation conversation;
    conversation.chatId = chatId;
    conversation.displayName = displayName.trimmed();
    conversation.participants = loadParticipants(chatId);
    conversation.sourceDatabaseName = QFileInfo(m_databasePath).fileName();

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    const bool hasTextColumn = tableHasColumn(db, "message", "text");
    const bool hasDateColumn = tableHasColumn(db, "message", "date");

    QSqlQuery query(db);
    query.prepare(
        QString(
            "SELECT %1, %2 "
            "FROM chat_message_join "
            "JOIN message ON message.ROWID = chat_message_join.message_id "
            "WHERE chat_message_join.chat_id = ? "
            "ORDER BY %3, message.ROWID")
            .arg(hasTextColumn ? "COALESCE(message.text, '')" : "''")
            .arg(hasDateColumn ? "COALESCE(message.date, 0)" : "0")
            .arg(hasDateColumn ? "message.date" : "message.ROWID"));
    query.addBindValue(chatId);

    if (!query.exec()) {
        qWarning() << "MessageIngestor: failed to load messages for chat" << chatId << query.lastError();
        return conversation;
    }

    while (query.next()) {
        const QString text = query.value(0).toString();
        const QDateTime sentAt = parseAppleTimestamp(query.value(1).toLongLong());

        conversation.messageCount++;
        if (!conversation.firstMessageAt.isValid() && sentAt.isValid()) {
            conversation.firstMessageAt = sentAt;
        }
        if (sentAt.isValid()) {
            conversation.lastMessageAt = sentAt;
        }

        if (!text.trimmed().isEmpty()) {
            conversation.recentMessages.append({ sentAt, sanitizeMessageText(text) });
            if (conversation.recentMessages.size() > 5) {
                conversation.recentMessages.removeFirst();
            }
        }
    }

    return conversation;
}

QStringList MessageIngestor::loadParticipants(qint64 chatId) const
{
    QStringList participants;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    if (!db.isOpen() || !db.tables().contains("handle")) {
        return participants;
    }

    const QString labelExpr = handleLabelExpression(db);

    if (db.tables().contains("chat_handle_join")) {
        QSqlQuery query(db);
        query.prepare(
            QString(
                "SELECT DISTINCT %1 "
                "FROM chat_handle_join "
                "JOIN handle ON handle.ROWID = chat_handle_join.handle_id "
                "WHERE chat_handle_join.chat_id = ? "
                "ORDER BY handle.ROWID")
                .arg(labelExpr));
        query.addBindValue(chatId);

        if (query.exec()) {
            while (query.next()) {
                const QString label = query.value(0).toString().trimmed();
                if (!label.isEmpty()) {
                    participants.append(label);
                }
            }
        }
    }

    if (!participants.isEmpty()) {
        participants.removeDuplicates();
        return participants;
    }

    if (!tableHasColumn(db, "message", "handle_id")) {
        return participants;
    }

    QSqlQuery fallbackQuery(db);
    fallbackQuery.prepare(
        QString(
            "SELECT DISTINCT %1 "
            "FROM chat_message_join "
            "JOIN message ON message.ROWID = chat_message_join.message_id "
            "JOIN handle ON handle.ROWID = message.handle_id "
            "WHERE chat_message_join.chat_id = ? "
            "ORDER BY handle.ROWID")
            .arg(labelExpr));
    fallbackQuery.addBindValue(chatId);

    if (!fallbackQuery.exec()) {
        return participants;
    }

    while (fallbackQuery.next()) {
        const QString label = fallbackQuery.value(0).toString().trimmed();
        if (!label.isEmpty()) {
            participants.append(label);
        }
    }

    participants.removeDuplicates();
    return participants;
}

QString MessageIngestor::conversationTitle(const MessageConversation &conversation) const
{
    if (!conversation.displayName.isEmpty()) {
        return conversation.displayName;
    }

    if (!conversation.participants.isEmpty()) {
        QStringList preview = conversation.participants.mid(0, 3);
        QString title = preview.join(", ");
        if (conversation.participants.size() > preview.size()) {
            title += QString(" +%1").arg(conversation.participants.size() - preview.size());
        }
        return title;
    }

    return QString("Chat %1").arg(conversation.chatId);
}

QString MessageIngestor::generateSlug(const MessageConversation &conversation) const
{
    Q_UNUSED(conversation);
    return QString("chat-%1").arg(conversation.chatId);
}

QString MessageIngestor::generateMarkdown(const MessageConversation &conversation) const
{
    QString markdown;
    const QString title = conversationTitle(conversation);

    markdown += "# Conversation: " + title + "\n\n";

    markdown += "## Summary\n";
    markdown += QString("%1 message(s)").arg(conversation.messageCount);
    if (!conversation.participants.isEmpty()) {
        markdown += " with " + conversation.participants.join(", ");
    }
    if (conversation.firstMessageAt.isValid() && conversation.lastMessageAt.isValid()) {
        markdown += QString(" spanning %1 to %2")
            .arg(conversation.firstMessageAt.toString("yyyy-MM-dd HH:mm"))
            .arg(conversation.lastMessageAt.toString("yyyy-MM-dd HH:mm"));
    }
    markdown += ".\n\n";

    markdown += "## Key Facts\n";
    markdown += QString("- Chat ID: %1\n").arg(conversation.chatId);
    markdown += QString("- Message count: %1\n").arg(conversation.messageCount);
    markdown += QString("- Participants: %1\n")
        .arg(conversation.participants.isEmpty() ? QString("Unknown") : conversation.participants.join(", "));
    if (conversation.firstMessageAt.isValid()) {
        markdown += QString("- First message: %1\n")
            .arg(conversation.firstMessageAt.toString("yyyy-MM-dd HH:mm"));
    }
    if (conversation.lastMessageAt.isValid()) {
        markdown += QString("- Last message: %1\n")
            .arg(conversation.lastMessageAt.toString("yyyy-MM-dd HH:mm"));
    }
    markdown += "\n";

    markdown += "## Participants\n";
    if (conversation.participants.isEmpty()) {
        markdown += "- Unknown\n";
    } else {
        for (const QString &participant : conversation.participants) {
            markdown += "- " + participant + "\n";
        }
    }
    markdown += "\n";

    markdown += "## Recent Messages\n";
    if (conversation.recentMessages.isEmpty()) {
        markdown += "- No text messages available in this chat export.\n";
    } else {
        for (const ConversationMessage &message : conversation.recentMessages) {
            const QString prefix = message.sentAt.isValid()
                ? message.sentAt.toString("yyyy-MM-dd HH:mm")
                : QString("Unknown time");
            markdown += QString("- %1 — %2\n").arg(prefix, message.text);
        }
    }
    markdown += "\n";

    markdown += "## Sources\n";
    markdown += "- Imported from " + conversation.sourceDatabaseName + "\n";

    return markdown;
}

QDateTime MessageIngestor::parseAppleTimestamp(qint64 rawValue)
{
    if (rawValue == 0) {
        return QDateTime();
    }

    static const QDateTime appleEpoch(QDate(2001, 1, 1), QTime(0, 0), QTimeZone::UTC);

    const qint64 magnitude = qAbs(rawValue);
    qint64 milliseconds = 0;

    if (magnitude > 10000000000000000LL) {
        milliseconds = rawValue / 1000000LL;
    } else if (magnitude > 10000000000000LL) {
        milliseconds = rawValue / 1000LL;
    } else {
        milliseconds = rawValue * 1000LL;
    }

    return appleEpoch.addMSecs(milliseconds).toLocalTime();
}