#include "SnapchatIngestor.h"

#include "VaultManager.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QThread>
#include <QTimeZone>
#include <QTimer>

#include <algorithm>
#include <cmath>

namespace {
QString normalizeKey(const QString &text)
{
    QString normalized = text.toLower();
    normalized.remove(QRegularExpression("[^a-z0-9]"));
    return normalized;
}

bool containsAnyNormalized(const QString &text, const QStringList &terms)
{
    const QString normalized = normalizeKey(text);
    for (const QString &term : terms) {
        if (normalized.contains(term)) {
            return true;
        }
    }
    return false;
}

bool objectContainsAnyKey(const QJsonObject &object, const QStringList &terms)
{
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (containsAnyNormalized(it.key(), terms)) {
            return true;
        }
    }
    return false;
}

QString stableHash(const QString &value)
{
    return QString::fromLatin1(QCryptographicHash::hash(value.toUtf8(), QCryptographicHash::Sha1).toHex().left(8));
}

QString slugify(const QString &text)
{
    QString slug = text.toLower().trimmed();
    slug.replace(QRegularExpression("[^a-z0-9]+"), "-");
    slug.remove(QRegularExpression("^-|-$"));
    if (slug.isEmpty()) {
        slug = stableHash(text);
    }
    return slug;
}

QString stringFromValue(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString().trimmed();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'f', 0);
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    return {};
}

QJsonValue findValue(const QJsonObject &object, const QStringList &candidateKeys)
{
    for (const QString &candidate : candidateKeys) {
        const QString normalizedCandidate = normalizeKey(candidate);
        for (auto it = object.begin(); it != object.end(); ++it) {
            if (normalizeKey(it.key()) == normalizedCandidate) {
                return it.value();
            }
        }
    }
    return {};
}

QString firstString(const QJsonObject &object, const QStringList &candidateKeys)
{
    return stringFromValue(findValue(object, candidateKeys));
}

QStringList stringListFromValue(const QJsonValue &value)
{
    QStringList result;

    if (value.isString()) {
        QString stringValue = value.toString().trimmed();
        if (stringValue.contains(',')) {
            for (const QString &part : stringValue.split(',', Qt::SkipEmptyParts)) {
                result.append(part.trimmed());
            }
        } else if (stringValue.contains(';')) {
            for (const QString &part : stringValue.split(';', Qt::SkipEmptyParts)) {
                result.append(part.trimmed());
            }
        } else if (!stringValue.isEmpty()) {
            result.append(stringValue);
        }
        return result;
    }

    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &entry : array) {
            result.append(stringListFromValue(entry));
        }
        return result;
    }

    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        const QString displayName = firstString(object, {"displayname", "name", "title", "friend", "username"});
        if (!displayName.isEmpty()) {
            result.append(displayName);
        }
    }

    result.removeAll({});
    result.removeDuplicates();
    return result;
}

QStringList firstStringList(const QJsonObject &object, const QStringList &candidateKeys)
{
    const QJsonValue value = findValue(object, candidateKeys);
    QStringList values = stringListFromValue(value);
    values.removeAll({});
    values.removeDuplicates();
    return values;
}

QString baseNameForPath(const QString &path)
{
    return QFileInfo(path).completeBaseName();
}

QStringList normalizedTerms(const QStringList &terms)
{
    QStringList normalized;
    for (const QString &term : terms) {
        normalized.append(normalizeKey(term));
    }
    return normalized;
}

QString normalizedFileName(const QString &path)
{
    return normalizeKey(QFileInfo(path).fileName());
}

bool isSupportedExportJson(const QString &path)
{
    const QString fileName = normalizedFileName(path);
    return fileName == QStringLiteral("friendsjson")
        || fileName == QStringLiteral("chathistoryjson")
        || fileName == QStringLiteral("snaphistoryjson")
        || fileName == QStringLiteral("memorieshistoryjson");
}

int supportedExportJsonPriority(const QString &path)
{
    const QString fileName = normalizedFileName(path);
    if (fileName == QStringLiteral("friendsjson")) {
        return 0;
    }
    if (fileName == QStringLiteral("chathistoryjson")) {
        return 1;
    }
    if (fileName == QStringLiteral("snaphistoryjson")) {
        return 2;
    }
    if (fileName == QStringLiteral("memorieshistoryjson")) {
        return 3;
    }
    return 99;
}

bool firstBool(const QJsonObject &object, const QStringList &candidateKeys, bool defaultValue = false)
{
    const QJsonValue value = findValue(object, candidateKeys);
    if (value.isBool()) {
        return value.toBool();
    }
    if (value.isDouble()) {
        return value.toDouble() != 0.0;
    }
    if (value.isString()) {
        const QString normalized = value.toString().trimmed().toLower();
        if (normalized == QStringLiteral("true") || normalized == QStringLiteral("1") || normalized == QStringLiteral("yes")) {
            return true;
        }
        if (normalized == QStringLiteral("false") || normalized == QStringLiteral("0") || normalized == QStringLiteral("no")) {
            return false;
        }
    }
    return defaultValue;
}

bool isRemoteReference(const QString &value)
{
    return value.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
        || value.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

QString titleCase(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    QString normalized = trimmed.toLower();
    normalized[0] = normalized[0].toUpper();
    return normalized;
}

QString normalizeLocation(const QString &location)
{
    const QString trimmed = location.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    static const QRegularExpression coordinatesPattern(
        QStringLiteral("^Latitude,\\s*Longitude:\\s*([-+]?\\d+(?:\\.\\d+)?),\\s*([-+]?\\d+(?:\\.\\d+)?)$"));
    const QRegularExpressionMatch match = coordinatesPattern.match(trimmed);
    if (match.hasMatch()) {
        bool latitudeOk = false;
        bool longitudeOk = false;
        const double latitude = match.captured(1).toDouble(&latitudeOk);
        const double longitude = match.captured(2).toDouble(&longitudeOk);
        if (latitudeOk && longitudeOk && std::abs(latitude) < 0.000001 && std::abs(longitude) < 0.000001) {
            return {};
        }
    }

    return trimmed;
}
}

SnapchatIngestor::SnapchatIngestor(VaultManager *vault, QObject *parent)
    : QObject(parent), m_vault(vault)
{
}

SnapchatIngestor::~SnapchatIngestor()
{
    if (m_workerThread != nullptr) {
        cancel();
        cleanupWorker();
    }
}

void SnapchatIngestor::setProgressValue(int progress)
{
    if (m_progress == progress) {
        return;
    }

    m_progress = progress;
    emit progressValueChanged(m_progress);
    emit progressChanged();
}

void SnapchatIngestor::setTotalFilesValue(int totalFiles)
{
    if (m_totalFiles == totalFiles) {
        return;
    }

    m_totalFiles = totalFiles;
    emit totalFilesValueChanged(m_totalFiles);
    emit totalFilesChanged();
}

void SnapchatIngestor::setStatusTextValue(const QString &statusText)
{
    if (m_statusText == statusText) {
        return;
    }

    m_statusText = statusText;
    emit statusTextValueChanged(m_statusText);
    emit statusTextChanged();
}

void SnapchatIngestor::importExportFolder(const QString &folderPath)
{
    if (m_workerMode) {
        runImportOnCurrentThread(folderPath);
        return;
    }

    startWorkerImport(folderPath);
}

void SnapchatIngestor::startWorkerImport(const QString &folderPath)
{
    if (m_running || m_workerThread != nullptr) {
        return;
    }

    resetImportState();
    m_running = true;
    emit runningChanged();

    qInfo() << "SnapchatIngestor: scheduling background import from" << folderPath;

    auto *workerVault = new VaultManager(m_vault ? m_vault->vaultPath() : QString());
    auto *worker = new SnapchatIngestor(workerVault);
    workerVault->setParent(worker);
    worker->m_workerMode = true;

    m_workerThread = new QThread(this);
    m_worker = worker;
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

    connect(m_worker, &SnapchatIngestor::progressValueChanged, this, [this](int progress) {
        setProgressValue(progress);
    });
    connect(m_worker, &SnapchatIngestor::totalFilesValueChanged, this, [this](int totalFiles) {
        setTotalFilesValue(totalFiles);
    });
    connect(m_worker, &SnapchatIngestor::statusTextValueChanged, this, [this](const QString &statusText) {
        setStatusTextValue(statusText);
    });
    connect(m_worker, &SnapchatIngestor::importFinished, this,
        [this](int peopleWritten, int conversationsWritten, int memoriesWritten) {
            cleanupWorker();
            m_running = false;
            emit runningChanged();
            emit importFinished(peopleWritten, conversationsWritten, memoriesWritten);
        });
    connect(m_worker, &SnapchatIngestor::importError, this, [this](const QString &message) {
        cleanupWorker();
        if (m_running) {
            m_running = false;
            emit runningChanged();
        }
        emit importError(message);
    });
    connect(m_workerThread, &QThread::started, m_worker, [worker = m_worker, folderPath]() {
        worker->importExportFolder(folderPath);
    });

    m_workerThread->start();
}

void SnapchatIngestor::cleanupWorker()
{
    if (m_workerThread == nullptr) {
        return;
    }

    QThread *thread = m_workerThread;
    m_workerThread = nullptr;
    m_worker = nullptr;
    thread->quit();
    thread->wait();
}

void SnapchatIngestor::runImportOnCurrentThread(const QString &folderPath)
{
    if (m_running) {
        return;
    }

    QDir directory(folderPath);
    if (!directory.exists()) {
        failImport("Snapchat export folder not found: " + folderPath);
        return;
    }

    resetImportState();
    m_running = true;
    emit runningChanged();

    qInfo() << "SnapchatIngestor: starting import from" << folderPath;

    QDirIterator it(folderPath, {"*.json"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString candidate = it.next();
        if (isSupportedExportJson(candidate)) {
            m_pendingFiles.append(candidate);
        }
    }

    std::sort(m_pendingFiles.begin(), m_pendingFiles.end(), [](const QString &lhs, const QString &rhs) {
        const int lhsPriority = supportedExportJsonPriority(lhs);
        const int rhsPriority = supportedExportJsonPriority(rhs);
        if (lhsPriority != rhsPriority) {
            return lhsPriority < rhsPriority;
        }
        return lhs < rhs;
    });

    setTotalFilesValue(m_pendingFiles.size());

    qInfo() << "SnapchatIngestor: found" << m_totalFiles << "supported JSON file(s)";
    for (const QString &filePath : std::as_const(m_pendingFiles)) {
        qInfo() << "SnapchatIngestor: queued" << filePath;
    }

    if (m_totalFiles == 0) {
        setStatusTextValue("No supported Snapchat JSON files were found in the selected folder.");
        m_running = false;
        emit runningChanged();
        emit importFinished(0, 0, 0);
        return;
    }

    setStatusTextValue(QString("Scanning %1 Snapchat JSON file(s)...").arg(m_totalFiles));
    QTimer::singleShot(0, this, &SnapchatIngestor::processNextBatch);
}

void SnapchatIngestor::cancel()
{
    if (!m_workerMode && m_worker != nullptr) {
        setStatusTextValue(QStringLiteral("Cancelling Snapchat import..."));
        QMetaObject::invokeMethod(m_worker, &SnapchatIngestor::cancel, Qt::QueuedConnection);
        return;
    }

    m_cancelled = true;
}

void SnapchatIngestor::processNextBatch()
{
    if (m_cancelled) {
        qInfo() << "SnapchatIngestor: import cancelled";
        setStatusTextValue("Snapchat import cancelled.");
        m_running = false;
        emit runningChanged();
        return;
    }

    const int batchSize = qMin(8, m_pendingFiles.size());
    QElapsedTimer batchTimer;
    batchTimer.start();

    for (int index = 0; index < batchSize; ++index) {
        const QString filePath = m_pendingFiles.takeFirst();
        setStatusTextValue(QString("Parsing %1... %2/%3")
            .arg(QFileInfo(filePath).fileName())
            .arg(m_progress + 1)
            .arg(m_totalFiles));
        parseJsonFile(filePath);
        setProgressValue(m_progress + 1);
    }

    qInfo() << "SnapchatIngestor: processed batch in" << batchTimer.elapsed() << "ms"
            << "progress" << m_progress << "/" << m_totalFiles;

    if (!m_pendingFiles.isEmpty()) {
        setStatusTextValue(QString("Parsing Snapchat export... %1/%2").arg(m_progress).arg(m_totalFiles));
        QTimer::singleShot(0, this, &SnapchatIngestor::processNextBatch);
        return;
    }

    writePeopleEntries();
    writeConversationEntries();
    writeMemoryEntries();
    finishImport();
}

void SnapchatIngestor::finishImport()
{
    qInfo() << "SnapchatIngestor: import finished with"
            << m_peopleWritten << "people,"
            << m_conversationsWritten << "conversations,"
            << m_memoriesWritten << "memories";
    setStatusTextValue(QString("Done! Imported %1 people, %2 conversations, and %3 memories.")
        .arg(m_peopleWritten)
        .arg(m_conversationsWritten)
        .arg(m_memoriesWritten));

    m_running = false;
    emit runningChanged();
    emit importFinished(m_peopleWritten, m_conversationsWritten, m_memoriesWritten);
}

void SnapchatIngestor::failImport(const QString &message)
{
    qWarning() << "SnapchatIngestor:" << message;
    setStatusTextValue(message);

    if (m_running) {
        m_running = false;
        emit runningChanged();
    }

    emit importError(message);
}

void SnapchatIngestor::resetImportState()
{
    m_cancelled = false;
    setProgressValue(0);
    setTotalFilesValue(0);
    setStatusTextValue({});
    m_pendingFiles.clear();
    m_people.clear();
    m_personSlugsByUsername.clear();
    m_conversations.clear();
    m_memories.clear();
    m_seenChatMessages.clear();
    m_seenMemories.clear();
    m_reservedMemorySlugs.clear();
    m_peopleWritten = 0;
    m_conversationsWritten = 0;
    m_memoriesWritten = 0;
}

void SnapchatIngestor::parseJsonFile(const QString &filePath)
{
    QElapsedTimer timer;
    timer.start();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "SnapchatIngestor: failed to open" << filePath;
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    qInfo() << "SnapchatIngestor: parsing" << filePath << "size(bytes)=" << data.size();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "SnapchatIngestor: JSON parse failed for" << filePath << parseError.errorString();
        return;
    }

    const QJsonValue root = document.isObject() ? QJsonValue(document.object()) : QJsonValue(document.array());
    const QString fileName = normalizedFileName(filePath);

    if (fileName == QStringLiteral("friendsjson")) {
        parseFriendsValue(root, filePath, true);
        qInfo() << "SnapchatIngestor: finished" << filePath << "in" << timer.elapsed() << "ms";
        return;
    }
    if (fileName == QStringLiteral("chathistoryjson") || fileName == QStringLiteral("snaphistoryjson")) {
        parseChatsValue(root, filePath, true);
        qInfo() << "SnapchatIngestor: finished" << filePath << "in" << timer.elapsed() << "ms";
        return;
    }
    if (fileName == QStringLiteral("memorieshistoryjson")) {
        parseMemoriesValue(root, filePath, true);
        qInfo() << "SnapchatIngestor: finished" << filePath << "in" << timer.elapsed() << "ms";
    }
}

void SnapchatIngestor::parseFriendsValue(const QJsonValue &value, const QString &sourceFile, bool inFriendContext)
{
    if (value.isArray()) {
        for (const QJsonValue &entry : value.toArray()) {
            parseFriendsValue(entry, sourceFile, inFriendContext);
        }
        return;
    }

    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        const bool childContext = inFriendContext || objectContainsAnyKey(object, normalizedTerms({"friend", "friends", "contact"}));

        const QString displayName = firstString(object, {"displayname", "name", "friend", "fullname"});
        const QString username = firstString(object, {"username", "user", "account", "handle", "id"});
        if (childContext && (!displayName.isEmpty() || !username.isEmpty())) {
            ensurePerson(displayName, username, sourceFile);
        }

        for (auto it = object.begin(); it != object.end(); ++it) {
            parseFriendsValue(it.value(), sourceFile, childContext);
        }
        return;
    }

    if (inFriendContext && value.isString()) {
        const QString name = value.toString().trimmed();
        if (!name.isEmpty() && name.size() < 80) {
            ensurePerson(name, {}, sourceFile);
        }
    }
}

void SnapchatIngestor::parseChatsValue(const QJsonValue &value, const QString &sourceFile, bool inChatContext)
{
    if (value.isArray()) {
        for (const QJsonValue &entry : value.toArray()) {
            parseChatsValue(entry, sourceFile, inChatContext);
        }
        return;
    }

    if (!value.isObject()) {
        return;
    }

    const QJsonObject object = value.toObject();
    const bool childContext = inChatContext || objectContainsAnyKey(object, normalizedTerms({"chat", "conversation", "message", "received", "sent"}));

    QStringList participants = firstStringList(object, {
        "participants", "participant", "conversationwith", "friend", "recipient", "recipients", "to",
        "from", "username", "name"
    });
    const QString sender = firstString(object, {"from", "sender", "username", "name"});
    const QString recipient = firstString(object, {"to", "recipient", "recipients"});
    if (!sender.isEmpty()) {
        participants.append(sender);
    }
    if (!recipient.isEmpty()) {
        participants.append(recipient);
    }
    participants.removeAll({});
    participants.removeDuplicates();

    const QString title = firstString(object, {
        "conversationtitle", "conversation", "conversationwith", "chatname", "groupname", "displayname", "friend"
    });
    const QString text = firstString(object, {"message", "text", "content", "body"});
    const QString mediaType = firstString(object, {"mediatype", "type"});
    const bool isOutgoing = firstBool(object, {"issender", "outgoing", "issent", "issentbyuser"});
    const bool isSaved = firstBool(object, {"issaved", "saved"});
    const QDateTime sentAt = parseTimestamp(findValue(object, {
        "createdmicroseconds", "created", "creationdate", "date", "timestamp", "time", "sentat"
    }));

    if (childContext && (!text.isEmpty() || !mediaType.isEmpty() || sentAt.isValid())
        && (!participants.isEmpty() || !title.isEmpty())) {
        addConversationMessage(title, participants, sender, text, mediaType, sentAt, isOutgoing, isSaved, sourceFile);
    }

    for (auto it = object.begin(); it != object.end(); ++it) {
        parseChatsValue(it.value(), sourceFile, childContext);
    }
}

void SnapchatIngestor::parseMemoriesValue(const QJsonValue &value, const QString &sourceFile, bool inMemoryContext)
{
    if (value.isArray()) {
        for (const QJsonValue &entry : value.toArray()) {
            parseMemoriesValue(entry, sourceFile, inMemoryContext);
        }
        return;
    }

    if (!value.isObject()) {
        return;
    }

    const QJsonObject object = value.toObject();
    const bool childContext = inMemoryContext || objectContainsAnyKey(object, normalizedTerms({"memory", "memories", "saved", "story", "media"}));

    const QString title = firstString(object, {"title", "caption", "description", "name"});
    const QString caption = firstString(object, {"caption", "description", "text", "note"});
    const QString mediaPath = firstString(object, {
        "filename", "file", "filepath", "path", "media", "mediapath", "mediadownloadurl", "downloadlink",
        "memoriesdownloadlink", "savedstorydownloadlink"
    });
    const QString mediaType = firstString(object, {"mediatype", "type"});
    const QString location = normalizeLocation(firstString(object, {"location", "place", "city"}));
    const QDateTime createdAt = parseTimestamp(findValue(object, {
        "createdmicroseconds", "created", "creationdate", "date", "timestamp", "time"
    }));
    QStringList participants = firstStringList(object, {"friend", "participant", "participants", "from", "to", "username"});
    participants.removeDuplicates();

    if (childContext && (!mediaPath.isEmpty() || !title.isEmpty() || !caption.isEmpty() || !mediaType.isEmpty())) {
        addMemoryEntry(title, caption, mediaPath, mediaType, location, createdAt, participants, sourceFile);
    }

    for (auto it = object.begin(); it != object.end(); ++it) {
        parseMemoriesValue(it.value(), sourceFile, childContext);
    }
}

SnapchatPerson &SnapchatIngestor::ensurePerson(const QString &displayName, const QString &username, const QString &sourceFile)
{
    QString resolvedDisplayName = displayName.trimmed();
    QString resolvedUsername = username.trimmed();
    if (resolvedDisplayName.isEmpty()) {
        resolvedDisplayName = resolvedUsername;
    }
    if (resolvedDisplayName.isEmpty()) {
        resolvedDisplayName = "Unknown Snapchat Contact";
    }

    const QString usernameKey = normalizeKey(resolvedUsername);
    QString slug;
    if (!usernameKey.isEmpty() && m_personSlugsByUsername.contains(usernameKey)) {
        slug = m_personSlugsByUsername.value(usernameKey);
    } else {
        slug = "snapchat-" + slugify(resolvedDisplayName);
    }

    SnapchatPerson &person = m_people[slug];
    if (person.slug.isEmpty()) {
        person.slug = slug;
        person.displayName = resolvedDisplayName;
        person.username = resolvedUsername;
    }

    if (!usernameKey.isEmpty()) {
        m_personSlugsByUsername.insert(usernameKey, slug);
    }
    if ((person.displayName.startsWith("Unknown")
            || (person.displayName == person.username && !resolvedDisplayName.isEmpty() && resolvedDisplayName != person.username))
        && !resolvedDisplayName.isEmpty()) {
        person.displayName = resolvedDisplayName;
    }
    if (!resolvedUsername.isEmpty()
        && (person.username.isEmpty()
            || (person.username == person.displayName && resolvedUsername != person.displayName))) {
        person.username = resolvedUsername;
    }
    if (!person.sourceFiles.contains(sourceFile)) {
        person.sourceFiles.append(sourceFile);
    }
    return person;
}

void SnapchatIngestor::addConversationMessage(const QString &title, const QStringList &participants, const QString &sender,
    const QString &text, const QString &mediaType, const QDateTime &sentAt, bool isOutgoing,
    bool isSaved, const QString &sourceFile)
{
    QStringList participantNames = participants;
    participantNames.removeAll({});
    participantNames.removeDuplicates();
    std::sort(participantNames.begin(), participantNames.end());

    const QString trimmedSender = sender.trimmed();
    const QString trimmedText = text.trimmed();
    const QString normalizedMediaType = titleCase(mediaType);

    QString resolvedTitle = title.trimmed();
    if (resolvedTitle.isEmpty()) {
        resolvedTitle = participantNames.join(", ");
    }
    if (resolvedTitle.isEmpty()) {
        resolvedTitle = baseNameForPath(sourceFile);
    }

    const QString signature = sourceFile + "|" + resolvedTitle + "|" + trimmedSender + "|"
        + sentAt.toString(Qt::ISODateWithMs) + "|" + trimmedText + "|" + normalizedMediaType + "|"
        + (isSaved ? QStringLiteral("saved") : QStringLiteral("transient"));
    if (m_seenChatMessages.contains(signature)) {
        return;
    }
    m_seenChatMessages.insert(signature);

    const QString slug = "snapchat-chat-" + slugify(resolvedTitle);
    SnapchatConversation &conversation = m_conversations[slug];
    if (conversation.slug.isEmpty()) {
        conversation.slug = slug;
        conversation.title = resolvedTitle;
    }

    QString senderSlug;
    QString senderLabel = trimmedSender;
    if (!trimmedSender.isEmpty()) {
        SnapchatPerson &senderPerson = ensurePerson(trimmedSender, trimmedSender, sourceFile);
        senderSlug = senderPerson.slug;
    } else if (isOutgoing) {
        senderLabel = QStringLiteral("You");
    }

    for (const QString &participant : participantNames) {
        SnapchatPerson &person = ensurePerson(participant, participant, sourceFile);
        if (!conversation.participantSlugs.contains(person.slug)) {
            conversation.participantSlugs.append(person.slug);
        }
        if (!conversation.participantNames.contains(person.displayName)) {
            conversation.participantNames.append(person.displayName);
        }
    }

    conversation.messageCount++;
    if (!normalizedMediaType.isEmpty() && normalizedMediaType.compare(QStringLiteral("Text"), Qt::CaseInsensitive) != 0) {
        conversation.mediaMessageCount++;
    } else if (!trimmedText.isEmpty()) {
        conversation.textMessageCount++;
    }
    if (isSaved) {
        conversation.savedMessageCount++;
    }
    if (sentAt.isValid()) {
        if (!conversation.firstMessageAt.isValid() || sentAt < conversation.firstMessageAt) {
            conversation.firstMessageAt = sentAt;
        }
        if (!conversation.lastMessageAt.isValid() || sentAt > conversation.lastMessageAt) {
            conversation.lastMessageAt = sentAt;
        }
    }
    if (!trimmedText.isEmpty() || !normalizedMediaType.isEmpty()) {
        conversation.recentMessages.append({sentAt, senderSlug, senderLabel, trimmedText, normalizedMediaType, isOutgoing, isSaved});
        std::sort(conversation.recentMessages.begin(), conversation.recentMessages.end(),
            [](const SnapchatConversationMessage &lhs, const SnapchatConversationMessage &rhs) {
                return lhs.sentAt < rhs.sentAt;
            });
        while (conversation.recentMessages.size() > 8) {
            conversation.recentMessages.removeFirst();
        }
    }
    if (!conversation.sourceFiles.contains(sourceFile)) {
        conversation.sourceFiles.append(sourceFile);
    }
}

void SnapchatIngestor::addMemoryEntry(const QString &title, const QString &caption, const QString &mediaPath,
    const QString &mediaType, const QString &location, const QDateTime &createdAt,
    const QStringList &participants, const QString &sourceFile)
{
    const QString trimmedCaption = caption.trimmed();
    const QString trimmedMediaPath = mediaPath.trimmed();
    const QString normalizedMediaType = titleCase(mediaType);

    QString resolvedTitle = title.trimmed();
    if (resolvedTitle.isEmpty() && !trimmedCaption.isEmpty()) {
        resolvedTitle = trimmedCaption;
    }
    if (resolvedTitle.isEmpty() && !trimmedMediaPath.isEmpty() && !isRemoteReference(trimmedMediaPath)) {
        resolvedTitle = baseNameForPath(trimmedMediaPath);
    }
    if (resolvedTitle.isEmpty()) {
        resolvedTitle = normalizedMediaType.isEmpty()
            ? QStringLiteral("Snapchat Memory")
            : QStringLiteral("Snapchat %1").arg(normalizedMediaType);
    }

    const QString signature = sourceFile + "|" + resolvedTitle + "|" + trimmedMediaPath + "|"
        + createdAt.toString(Qt::ISODateWithMs) + "|" + normalizedMediaType;
    if (m_seenMemories.contains(signature)) {
        return;
    }
    m_seenMemories.insert(signature);

    const QString datePrefix = createdAt.isValid() ? createdAt.date().toString("yyyy-MM-dd") + "-" : QString();
    const QString baseSlug = "snapchat-memory-" + datePrefix + slugify(resolvedTitle.isEmpty() ? sourceFile : resolvedTitle);
    QString resolvedSlug = baseSlug;
    if (m_reservedMemorySlugs.contains(resolvedSlug)) {
        resolvedSlug += "-" + stableHash(signature);
    }
    m_reservedMemorySlugs.insert(resolvedSlug);

    SnapchatMemory memory;
    memory.slug = resolvedSlug;
    memory.title = resolvedTitle.isEmpty() ? QStringLiteral("Snapchat Memory") : resolvedTitle;
    memory.caption = trimmedCaption;
    memory.mediaPath = trimmedMediaPath;
    memory.mediaType = normalizedMediaType;
    memory.location = location.trimmed();
    memory.createdAt = createdAt;
    memory.sourceFiles = { sourceFile };

    QStringList participantNames = participants;
    participantNames.removeAll({});
    participantNames.removeDuplicates();
    for (const QString &participant : participantNames) {
        SnapchatPerson &person = ensurePerson(participant, {}, sourceFile);
        memory.participantSlugs.append(person.slug);
        memory.participantNames.append(person.displayName);
    }
    memory.participantSlugs.removeDuplicates();
    memory.participantNames.removeDuplicates();
    m_memories.append(memory);
}

void SnapchatIngestor::writePeopleEntries()
{
    for (auto it = m_people.begin(); it != m_people.end(); ++it) {
        const QString relativePath = "people/" + it.key() + ".md";
        if (m_vault->writeFile(relativePath, generatePersonMarkdown(it.value()))) {
            m_peopleWritten++;
        }
    }
}

void SnapchatIngestor::writeConversationEntries()
{
    for (auto it = m_conversations.begin(); it != m_conversations.end(); ++it) {
        const QString relativePath = "conversations/" + it.key() + ".md";
        if (m_vault->writeFile(relativePath, generateConversationMarkdown(it.value()))) {
            m_conversationsWritten++;
        }
    }
}

void SnapchatIngestor::writeMemoryEntries()
{
    for (const SnapchatMemory &memory : m_memories) {
        const QString relativePath = "memories/" + memory.slug + ".md";
        if (m_vault->writeFile(relativePath, generateMemoryMarkdown(memory))) {
            m_memoriesWritten++;
        }
    }
}

QString SnapchatIngestor::generatePersonMarkdown(const SnapchatPerson &person) const
{
    QString markdown;
    markdown += "# Person: " + person.displayName + "\n\n";
    markdown += "## Summary\n";
    markdown += person.displayName + " appears in the imported Snapchat export.\n\n";
    markdown += "## Key Facts\n";
    markdown += "- Source: Snapchat export\n";
    if (!person.username.isEmpty()) {
        markdown += "- Snapchat username: " + person.username + "\n";
    }
    markdown += "\n## Sources\n";
    for (const QString &source : person.sourceFiles) {
        markdown += "- " + QFileInfo(source).fileName() + "\n";
    }
    return markdown;
}

QString SnapchatIngestor::generateConversationMarkdown(const SnapchatConversation &conversation) const
{
    QStringList participantDisplayNames;
    for (const QString &slug : conversation.participantSlugs) {
        participantDisplayNames.append(displayNameForSlug(slug));
    }
    participantDisplayNames.removeAll({});
    participantDisplayNames.removeDuplicates();

    QString markdown;
    markdown += "# Snapchat Chat: " + conversation.title + "\n\n";
    markdown += "## Summary\n";
    markdown += QString("%1 message(s) imported from Snapchat")
        .arg(conversation.messageCount);
    if (!participantDisplayNames.isEmpty()) {
        markdown += " involving " + participantDisplayNames.join(", ");
    }
    markdown += ".\n\n";

    markdown += "## Key Facts\n";
    markdown += QString("- Message count: %1\n").arg(conversation.messageCount);
    if (conversation.textMessageCount > 0) {
        markdown += QString("- Text messages: %1\n").arg(conversation.textMessageCount);
    }
    if (conversation.mediaMessageCount > 0) {
        markdown += QString("- Snap media events: %1\n").arg(conversation.mediaMessageCount);
    }
    if (conversation.savedMessageCount > 0) {
        markdown += QString("- Saved messages: %1\n").arg(conversation.savedMessageCount);
    }
    if (conversation.firstMessageAt.isValid()) {
        markdown += "- First message: " + conversation.firstMessageAt.toString("yyyy-MM-dd HH:mm") + "\n";
    }
    if (conversation.lastMessageAt.isValid()) {
        markdown += "- Last message: " + conversation.lastMessageAt.toString("yyyy-MM-dd HH:mm") + "\n";
    }
    if (!conversation.participantSlugs.isEmpty()) {
        QStringList links;
        for (const QString &slug : conversation.participantSlugs) {
            links.append("[[" + slug + "]]");
        }
        markdown += "- Participants: " + links.join(", ") + "\n";
    }
    markdown += "\n";

    markdown += "## Recent Messages\n";
    if (conversation.recentMessages.isEmpty()) {
        markdown += "- No message text was available in this Snapchat export.\n";
    } else {
        for (const SnapchatConversationMessage &message : conversation.recentMessages) {
            const QString timestamp = message.sentAt.isValid()
                ? message.sentAt.toString("yyyy-MM-dd HH:mm")
                : QStringLiteral("Unknown time");

            QString sender = message.isOutgoing
                ? QStringLiteral("You")
                : displayNameForSlug(message.senderSlug, message.senderLabel);
            if (sender.isEmpty()) {
                sender = QStringLiteral("Unknown sender");
            }

            QString description = message.text;
            if (description.isEmpty()) {
                if (!message.mediaType.isEmpty() && message.mediaType.compare(QStringLiteral("Text"), Qt::CaseInsensitive) != 0) {
                    description = message.mediaType + QStringLiteral(" snap");
                } else if (!message.mediaType.isEmpty()) {
                    description = QStringLiteral("Text message");
                } else {
                    description = QStringLiteral("Snapchat message");
                }
            }

            QStringList badges;
            if (message.isOutgoing) {
                badges.append(QStringLiteral("sent"));
            }
            if (message.isSaved) {
                badges.append(QStringLiteral("saved"));
            }

            markdown += "- " + timestamp + " — " + sender + ": " + description;
            if (!badges.isEmpty()) {
                markdown += " [" + badges.join("] [") + "]";
            }
            markdown += "\n";
        }
    }
    markdown += "\n## Sources\n";
    for (const QString &source : conversation.sourceFiles) {
        markdown += "- " + QFileInfo(source).fileName() + "\n";
    }
    return markdown;
}

QString SnapchatIngestor::generateMemoryMarkdown(const SnapchatMemory &memory) const
{
    QString markdown;
    markdown += "# Snapchat Memory: " + memory.title + "\n\n";
    markdown += "## Summary\n";
    markdown += "Imported Snapchat";
    if (!memory.mediaType.isEmpty()) {
        markdown += " " + memory.mediaType.toLower();
    } else {
        markdown += " memory";
    }
    if (memory.createdAt.isValid()) {
        markdown += " from " + memory.createdAt.toString("yyyy-MM-dd");
    }
    if (!memory.location.isEmpty()) {
        markdown += " at " + memory.location;
    }
    markdown += ".\n\n";

    markdown += "## Key Facts\n";
    if (!memory.mediaType.isEmpty()) {
        markdown += "- Media type: " + memory.mediaType + "\n";
    }
    if (memory.createdAt.isValid()) {
        markdown += "- Created: " + memory.createdAt.toString("yyyy-MM-dd HH:mm") + "\n";
    }
    if (!memory.location.isEmpty()) {
        markdown += "- Location: " + memory.location + "\n";
    }
    if (!memory.mediaPath.isEmpty()) {
        markdown += "- Media reference: " + memory.mediaPath + "\n";
    }
    if (!memory.caption.isEmpty() && memory.caption != memory.title) {
        markdown += "- Caption: " + memory.caption + "\n";
    }
    if (!memory.participantSlugs.isEmpty()) {
        QStringList links;
        for (const QString &slug : memory.participantSlugs) {
            links.append("[[" + slug + "]]");
        }
        markdown += "- People: " + links.join(", ") + "\n";
    }
    markdown += "\n## Sources\n";
    for (const QString &source : memory.sourceFiles) {
        markdown += "- " + QFileInfo(source).fileName() + "\n";
    }
    return markdown;
}

QString SnapchatIngestor::displayNameForSlug(const QString &slug, const QString &fallback) const
{
    if (!slug.isEmpty()) {
        const auto it = m_people.constFind(slug);
        if (it != m_people.constEnd() && !it.value().displayName.isEmpty()) {
            return it.value().displayName;
        }
    }
    return fallback;
}

QDateTime SnapchatIngestor::parseTimestamp(const QJsonValue &value)
{
    if (value.isUndefined() || value.isNull()) {
        return {};
    }

    if (value.isDouble()) {
        const qint64 raw = static_cast<qint64>(value.toDouble());
        if (raw <= 0) {
            return {};
        }
        if (raw > 100000000000000LL) {
            return QDateTime::fromMSecsSinceEpoch(raw / 1000, QTimeZone::UTC).toLocalTime();
        }
        if (raw > 1000000000000LL) {
            return QDateTime::fromMSecsSinceEpoch(raw, QTimeZone::UTC).toLocalTime();
        }
        return QDateTime::fromSecsSinceEpoch(raw, QTimeZone::UTC).toLocalTime();
    }

    const QString text = stringFromValue(value);
    if (text.isEmpty()) {
        return {};
    }

    const QList<Qt::DateFormat> builtinFormats = { Qt::ISODateWithMs, Qt::ISODate };
    for (Qt::DateFormat format : builtinFormats) {
        const QDateTime parsed = QDateTime::fromString(text, format);
        if (parsed.isValid()) {
            return parsed.timeSpec() == Qt::LocalTime ? parsed : parsed.toLocalTime();
        }
    }

    if (text.endsWith(QStringLiteral(" UTC"), Qt::CaseInsensitive)) {
        const QString utcText = text.left(text.size() - 4).trimmed();
        const QStringList utcFormats = {
            QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"),
            QStringLiteral("yyyy-MM-dd HH:mm:ss")
        };

        for (const QString &format : utcFormats) {
            QDateTime parsed = QDateTime::fromString(utcText, format);
            if (parsed.isValid()) {
                parsed.setTimeZone(QTimeZone::UTC);
                return parsed.toLocalTime();
            }
        }
    }

    const QStringList customFormats = {
        QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"),
        QStringLiteral("yyyy-MM-dd HH:mm:ss"),
        QStringLiteral("M/d/yyyy, h:mm:ss AP"),
        QStringLiteral("M/d/yyyy h:mm:ss AP"),
        QStringLiteral("ddd MMM d HH:mm:ss yyyy")
    };

    for (const QString &format : customFormats) {
        const QDateTime parsed = QDateTime::fromString(text, format);
        if (parsed.isValid()) {
            return parsed.timeSpec() == Qt::LocalTime ? parsed : parsed.toLocalTime();
        }
    }

    bool ok = false;
    const qint64 numeric = text.toLongLong(&ok);
    if (ok) {
        return parseTimestamp(QJsonValue(static_cast<double>(numeric)));
    }

    return {};
}