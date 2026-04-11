#include "WikiCompiler.h"

#include "LlmClient.h"
#include "VaultManager.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

namespace {

QString slugFromFilename(const QString &filename)
{
    return QFileInfo(filename).completeBaseName();
}

// Extract all [[backlink]] slugs from markdown content
QStringList extractBacklinks(const QString &content)
{
    QStringList links;
    static const QRegularExpression re(QStringLiteral("\\[\\[([^\\]]+)\\]\\]"));
    auto it = re.globalMatch(content);
    while (it.hasNext()) {
        links.append(it.next().captured(1));
    }
    links.removeDuplicates();
    return links;
}

// Extract the first ## Summary section content
QString extractSummary(const QString &content)
{
    static const QRegularExpression re(
        QStringLiteral("## Summary\\s*\\n(.*?)(?=\\n## |$)"),
        QRegularExpression::DotMatchesEverythingOption);
    auto match = re.match(content);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

// Extract the first # Title line
QString extractTitle(const QString &content)
{
    static const QRegularExpression re(QStringLiteral("^# (.+)$"), QRegularExpression::MultilineOption);
    auto match = re.match(content);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

} // namespace

WikiCompiler::WikiCompiler(VaultManager *vault, LlmClient *llm, QObject *parent)
    : QObject(parent)
    , m_vault(vault)
    , m_llm(llm)
{
}

void WikiCompiler::setStatusText(const QString &text)
{
    if (m_statusText == text) return;
    m_statusText = text;
    emit statusTextChanged();
}

void WikiCompiler::setProgress(int value)
{
    if (m_progress == value) return;
    m_progress = value;
    emit progressChanged();
}

void WikiCompiler::setTotalCandidates(int value)
{
    if (m_totalCandidates == value) return;
    m_totalCandidates = value;
    emit totalCandidatesChanged();
}

void WikiCompiler::scanCandidates()
{
    if (m_running) return;

    setStatusText(QStringLiteral("Scanning vault for compilation candidates..."));
    qInfo() << "WikiCompiler: scanning vault for candidates";

    m_candidates.clear();
    m_candidates.append(scanPersonSummaries());
    m_candidates.append(scanEventSummaries());
    m_candidates.append(scanTopicClusters());

    setTotalCandidates(m_candidates.size());
    setStatusText(QString("Found %1 compilation candidate(s).").arg(m_candidates.size()));

    qInfo() << "WikiCompiler: found" << m_candidates.size() << "candidates"
            << "(person summaries, event summaries, topic clusters)";

    emit scanFinished(m_candidates.size());
}

void WikiCompiler::compileAll()
{
    if (m_running) return;
    if (!m_llm || !m_llm->ready()) {
        setStatusText(QStringLiteral("LLM not configured. Set your API key in Settings."));
        emit compileError(QStringLiteral("LLM not ready"));
        return;
    }

    if (m_candidates.isEmpty()) {
        scanCandidates();
    }

    if (m_candidates.isEmpty()) {
        setStatusText(QStringLiteral("No candidates found to compile."));
        emit compileFinished(0);
        return;
    }

    m_running = true;
    m_cancelled = false;
    m_progress = 0;
    m_articlesWritten = 0;
    emit runningChanged();

    qInfo() << "WikiCompiler: starting compilation of" << m_candidates.size() << "candidates";
    processNextCandidate();
}

void WikiCompiler::compileBatch(int count)
{
    if (m_running) return;
    if (!m_llm || !m_llm->ready()) {
        setStatusText(QStringLiteral("LLM not configured."));
        emit compileError(QStringLiteral("LLM not ready"));
        return;
    }

    if (m_candidates.isEmpty()) {
        scanCandidates();
    }

    if (m_candidates.isEmpty()) {
        setStatusText(QStringLiteral("No candidates found to compile."));
        emit compileFinished(0);
        return;
    }

    // Truncate to requested batch size
    if (count > 0 && count < m_candidates.size()) {
        m_candidates = m_candidates.mid(0, count);
        setTotalCandidates(m_candidates.size());
    }

    m_running = true;
    m_cancelled = false;
    m_progress = 0;
    m_articlesWritten = 0;
    emit runningChanged();

    qInfo() << "WikiCompiler: starting batch compilation of" << m_candidates.size() << "candidates";
    processNextCandidate();
}

void WikiCompiler::compileOne(const QString &slug)
{
    if (m_running) return;
    if (!m_llm || !m_llm->ready()) {
        setStatusText(QStringLiteral("LLM not configured. Set your API key in Settings."));
        emit compileError(QStringLiteral("LLM not ready"));
        return;
    }

    if (m_candidates.isEmpty()) {
        scanCandidates();
    }

    // Find the candidate
    QList<CompileCandidateGroup> singleList;
    for (const auto &c : std::as_const(m_candidates)) {
        if (c.slug == slug) {
            singleList.append(c);
            break;
        }
    }
    if (singleList.isEmpty()) {
        setStatusText(QString("Candidate '%1' not found.").arg(slug));
        emit compileError(QString("Candidate not found: %1").arg(slug));
        return;
    }

    m_candidates = singleList;
    m_running = true;
    m_cancelled = false;
    m_progress = 0;
    m_articlesWritten = 0;
    setTotalCandidates(1);
    emit runningChanged();

    processNextCandidate();
}

void WikiCompiler::cancel()
{
    m_cancelled = true;
    setStatusText(QStringLiteral("Cancelling compilation..."));
}

void WikiCompiler::processNextCandidate()
{
    if (m_cancelled) {
        qInfo() << "WikiCompiler: compilation cancelled at" << m_progress << "/" << m_totalCandidates;
        setStatusText(QStringLiteral("Compilation cancelled."));
        m_running = false;
        emit runningChanged();
        return;
    }

    if (m_progress >= m_candidates.size()) {
        qInfo() << "WikiCompiler: compilation finished." << m_articlesWritten << "articles written";
        setStatusText(QString("Done! Compiled %1 article(s).").arg(m_articlesWritten));
        m_running = false;
        emit runningChanged();
        emit compileFinished(m_articlesWritten);
        return;
    }

    CompileCandidateGroup &candidate = m_candidates[m_progress];
    setStatusText(QString("Compiling %1... %2/%3")
        .arg(candidate.title)
        .arg(m_progress + 1)
        .arg(m_totalCandidates));

    qInfo() << "WikiCompiler: compiling" << candidate.slug
            << "from" << candidate.sourceFiles.size() << "source(s)";

    // Build prompt based on category
    QString prompt;
    if (candidate.category == QStringLiteral("person-summary")) {
        prompt = buildPersonSummaryPrompt(candidate);
    } else if (candidate.category == QStringLiteral("event-summary")) {
        prompt = buildEventSummaryPrompt(candidate);
    } else {
        prompt = buildTopicClusterPrompt(candidate);
    }
    candidate.prompt = prompt;

    const QString systemPrompt = QStringLiteral(
        "You are a personal wiki compiler for Arkive, a privacy-first life wiki app. "
        "You receive raw imported entries (conversations, people profiles, memories, media logs) "
        "and compile them into structured wiki articles.\n\n"
        "Output format requirements:\n"
        "- Start with # Title\n"
        "- Include ## Summary (2-3 sentence overview)\n"
        "- Include ## Key Facts (bullet list of important details)\n"
        "- Include ## Connections (list of [[backlink-slug]] references to related articles)\n"
        "- Include ## Sources (list source filenames)\n"
        "- Use [[slug]] syntax for backlinks to other articles\n"
        "- Be factual. Only state what the source data contains. Never fabricate.\n"
        "- If data is sparse, say so honestly rather than padding.\n"
        "- Write in third person for people, first person for personal events."
    );

    m_llm->completeSimple(systemPrompt, prompt, [this](const LlmResponse &response) {
        if (m_cancelled) {
            processNextCandidate();
            return;
        }

        CompileResult result;
        result.slug = m_candidates[m_progress].slug;
        result.title = m_candidates[m_progress].title;
        result.section = m_candidates[m_progress].section;

        if (!response.success) {
            qWarning() << "WikiCompiler: LLM error for" << result.slug << ":" << response.error;
            result.error = response.error;
        } else {
            result.markdown = response.content;
            result.success = true;

            if (writeCompiledArticle(result)) {
                m_articlesWritten++;
                qInfo() << "WikiCompiler: wrote" << result.slug;
                emit articleCompiled(result.slug, result.title);
            }
        }

        setProgress(m_progress + 1);
        processNextCandidate();
    });
}

// ─── Scanning ───────────────────────────────────────────────────

QList<CompileCandidateGroup> WikiCompiler::scanPersonSummaries()
{
    QList<CompileCandidateGroup> results;
    const QString peoplePath = m_vault->vaultPath() + QStringLiteral("/people");
    const QString wikiPath = m_vault->vaultPath() + QStringLiteral("/wiki");

    QDir peopleDir(peoplePath);
    if (!peopleDir.exists()) return results;

    // Group people entries by display name root (e.g. all "snapchat-john-doe*" -> one summary)
    QMap<QString, QStringList> personGroups;
    const QStringList entries = peopleDir.entryList({"*.md"}, QDir::Files);
    for (const QString &entry : entries) {
        const QString slug = slugFromFilename(entry);
        // Group key: remove source prefix (snapchat-, imessage-) for cross-source merging
        QString groupKey = slug;
        for (const QString &prefix : {QStringLiteral("snapchat-"), QStringLiteral("imessage-")}) {
            if (groupKey.startsWith(prefix)) {
                groupKey = groupKey.mid(prefix.size());
                break;
            }
        }
        personGroups[groupKey].append(QStringLiteral("people/") + entry);
    }

    for (auto it = personGroups.begin(); it != personGroups.end(); ++it) {
        const QString compiledSlug = QStringLiteral("person-summary-") + it.key();
        const QString compiledPath = wikiPath + "/" + compiledSlug + ".md";

        // Skip if already compiled and sources haven't changed
        if (QFile::exists(compiledPath)) continue;

        // Also gather any conversations mentioning this person
        QStringList allSources = it.value();
        const QString convPath = m_vault->vaultPath() + QStringLiteral("/conversations");
        QDir convDir(convPath);
        if (convDir.exists()) {
            const QStringList convEntries = convDir.entryList({"*.md"}, QDir::Files);
            for (const QString &convEntry : convEntries) {
                if (convEntry.contains(it.key(), Qt::CaseInsensitive)) {
                    allSources.append(QStringLiteral("conversations/") + convEntry);
                }
            }
        }

        CompileCandidateGroup group;
        group.slug = compiledSlug;
        group.title = QStringLiteral("Person Summary: ") + it.key();
        group.section = QStringLiteral("wiki");
        group.category = QStringLiteral("person-summary");
        group.sourceFiles = allSources;
        results.append(group);
    }

    qInfo() << "WikiCompiler: found" << results.size() << "person summary candidates";
    return results;
}

QList<CompileCandidateGroup> WikiCompiler::scanEventSummaries()
{
    QList<CompileCandidateGroup> results;
    const QString convPath = m_vault->vaultPath() + QStringLiteral("/conversations");
    const QString memoriesPath = m_vault->vaultPath() + QStringLiteral("/memories");
    const QString wikiPath = m_vault->vaultPath() + QStringLiteral("/wiki");

    // Group conversations and memories by month for event summaries
    QMap<QString, QStringList> monthGroups;  // "2025-10" -> [files]

    auto collectByMonth = [&](const QString &dirPath, const QString &section) {
        QDir dir(dirPath);
        if (!dir.exists()) return;

        static const QRegularExpression dateRe(QStringLiteral("(\\d{4}-\\d{2})"));
        const QStringList entries = dir.entryList({"*.md"}, QDir::Files);
        for (const QString &entry : entries) {
            // Try to extract a date from the file content
            const QString content = m_vault->readFile(dirPath + "/" + entry);
            const auto match = dateRe.match(content);
            if (match.hasMatch()) {
                monthGroups[match.captured(1)].append(section + "/" + entry);
            }
        }
    };

    collectByMonth(convPath, QStringLiteral("conversations"));
    collectByMonth(memoriesPath, QStringLiteral("memories"));

    for (auto it = monthGroups.begin(); it != monthGroups.end(); ++it) {
        if (it.value().size() < 3) continue;  // Need at least 3 entries for a meaningful summary

        const QString compiledSlug = QStringLiteral("month-summary-") + it.key();
        const QString compiledPath = wikiPath + "/" + compiledSlug + ".md";
        if (QFile::exists(compiledPath)) continue;

        CompileCandidateGroup group;
        group.slug = compiledSlug;
        group.title = QStringLiteral("Monthly Summary: ") + it.key();
        group.section = QStringLiteral("wiki");
        group.category = QStringLiteral("event-summary");
        group.sourceFiles = it.value();
        results.append(group);
    }

    qInfo() << "WikiCompiler: found" << results.size() << "event summary candidates";
    return results;
}

QList<CompileCandidateGroup> WikiCompiler::scanTopicClusters()
{
    QList<CompileCandidateGroup> results;
    const QString wikiPath = m_vault->vaultPath() + QStringLiteral("/wiki");

    // Find articles that share many backlinks — they form a topic cluster
    QMap<QString, QStringList> backlinkIndex;  // target slug -> [source files that link to it]

    QDirIterator it(m_vault->vaultPath(), {"*.md"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString absPath = it.next();
        const QString relPath = QDir(m_vault->vaultPath()).relativeFilePath(absPath);

        // Skip wiki/ articles themselves
        if (relPath.startsWith(QStringLiteral("wiki/"))) continue;
        if (relPath.startsWith(QStringLiteral("_"))) continue;

        const QString content = m_vault->readFile(absPath);
        const QStringList links = extractBacklinks(content);
        for (const QString &link : links) {
            backlinkIndex[link].append(relPath);
        }
    }

    // Topics with 5+ inbound links are cluster candidates
    for (auto bit = backlinkIndex.begin(); bit != backlinkIndex.end(); ++bit) {
        if (bit.value().size() < 5) continue;

        const QString compiledSlug = QStringLiteral("topic-") + bit.key();
        const QString compiledPath = wikiPath + "/" + compiledSlug + ".md";
        if (QFile::exists(compiledPath)) continue;

        CompileCandidateGroup group;
        group.slug = compiledSlug;
        group.title = QStringLiteral("Topic: ") + bit.key();
        group.section = QStringLiteral("wiki");
        group.category = QStringLiteral("topic-cluster");
        group.sourceFiles = bit.value();
        results.append(group);
    }

    qInfo() << "WikiCompiler: found" << results.size() << "topic cluster candidates";
    return results;
}

// ─── Prompt Building ────────────────────────────────────────────

QString WikiCompiler::readSourceContext(const QStringList &relativePaths, int maxChars)
{
    QString context;
    int charsUsed = 0;

    for (const QString &relPath : relativePaths) {
        const QString absPath = m_vault->vaultPath() + "/" + relPath;
        const QString content = m_vault->readFile(absPath);
        if (content.isEmpty()) continue;

        const QString block = QStringLiteral("--- %1 ---\n%2\n\n").arg(relPath, content);
        if (charsUsed + block.size() > maxChars) {
            // Add truncated version
            const int remaining = maxChars - charsUsed;
            if (remaining > 200) {
                context += block.left(remaining) + QStringLiteral("\n[...truncated]\n\n");
            }
            break;
        }
        context += block;
        charsUsed += block.size();
    }

    return context;
}

QString WikiCompiler::buildPersonSummaryPrompt(const CompileCandidateGroup &candidate)
{
    const QString sourceContext = readSourceContext(candidate.sourceFiles);

    return QStringLiteral(
        "Compile a person summary wiki article from the following raw entries.\n"
        "The person's identifier is: %1\n\n"
        "Source entries:\n%2\n"
        "Requirements:\n"
        "- Title should be the person's display name (not their slug)\n"
        "- Summary: who they are and how they appear in the user's life data\n"
        "- Key Facts: username, first/last contact date, message counts, any notable details\n"
        "- Connections: link to conversation articles using [[slug]] syntax\n"
        "- Sources: list the source filenames\n"
    ).arg(candidate.slug, sourceContext);
}

QString WikiCompiler::buildEventSummaryPrompt(const CompileCandidateGroup &candidate)
{
    const QString sourceContext = readSourceContext(candidate.sourceFiles);

    return QStringLiteral(
        "Compile a monthly event summary article from the following raw entries.\n"
        "Time period: %1\n\n"
        "Source entries:\n%2\n"
        "Requirements:\n"
        "- Title: the month and year in human-readable form\n"
        "- Summary: overview of what happened that month\n"
        "- Key Facts: notable conversations, memories, events\n"
        "- Connections: link to people and conversation articles using [[slug]]\n"
        "- Sources: list source filenames\n"
    ).arg(candidate.title, sourceContext);
}

QString WikiCompiler::buildTopicClusterPrompt(const CompileCandidateGroup &candidate)
{
    const QString sourceContext = readSourceContext(candidate.sourceFiles);

    return QStringLiteral(
        "Compile a topic overview article from entries that share a common theme.\n"
        "Topic identifier: %1\n\n"
        "Source entries:\n%2\n"
        "Requirements:\n"
        "- Title: a descriptive name for this topic or recurring theme\n"
        "- Summary: what this topic is about and why it appears repeatedly\n"
        "- Key Facts: frequency, time span, participants involved\n"
        "- Connections: link to related people, conversations, memories using [[slug]]\n"
        "- Sources: list source filenames\n"
    ).arg(candidate.slug, sourceContext);
}

// ─── Output ─────────────────────────────────────────────────────

bool WikiCompiler::writeCompiledArticle(const CompileResult &result)
{
    // Ensure wiki/ section exists
    QDir wikiDir(m_vault->vaultPath() + QStringLiteral("/wiki"));
    if (!wikiDir.exists()) {
        wikiDir.mkpath(QStringLiteral("."));
    }

    const QString relativePath = result.section + "/" + result.slug + ".md";
    return m_vault->writeFile(relativePath, result.markdown);
}
