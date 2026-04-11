#ifndef WIKICOMPILER_H
#define WIKICOMPILER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

class LlmClient;
class VaultManager;

#ifndef ARKIVE_TESTING
#define WIKI_COMPILER_PRIVATE private
#else
#define WIKI_COMPILER_PRIVATE public
#endif

struct CompileCandidateGroup {
    QString slug;            // e.g. "person-summary-mom"
    QString title;           // e.g. "Person Summary: Mom"
    QString section;         // target vault section, e.g. "wiki"
    QString category;        // "person-summary", "event-summary", "topic-cluster"
    QStringList sourceFiles; // relative paths of raw entries used as input
    QString prompt;          // assembled LLM prompt
};

struct CompileResult {
    QString slug;
    QString title;
    QString section;
    QString markdown;       // generated article content
    bool success = false;
    QString error;
};

class WikiCompiler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int totalCandidates READ totalCandidates NOTIFY totalCandidatesChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit WikiCompiler(VaultManager *vault, LlmClient *llm, QObject *parent = nullptr);

    bool running() const { return m_running; }
    int progress() const { return m_progress; }
    int totalCandidates() const { return m_totalCandidates; }
    QString statusText() const { return m_statusText; }

    // Scan vault and return candidate groups for compilation.
    Q_INVOKABLE void scanCandidates();

    // Compile all candidates by sending them to the LLM one at a time.
    Q_INVOKABLE void compileAll();

    // Compile up to `count` candidates (for testing with small batches).
    Q_INVOKABLE void compileBatch(int count);

    // Compile a single candidate group by slug.
    Q_INVOKABLE void compileOne(const QString &slug);

    // Cancel in-progress compilation.
    Q_INVOKABLE void cancel();

signals:
    void runningChanged();
    void progressChanged();
    void totalCandidatesChanged();
    void statusTextChanged();
    void scanFinished(int candidateCount);
    void articleCompiled(const QString &slug, const QString &title);
    void compileFinished(int articlesWritten);
    void compileError(const QString &message);

WIKI_COMPILER_PRIVATE:
    void setStatusText(const QString &text);
    void setProgress(int value);
    void setTotalCandidates(int value);

    // Scanning phases
    QList<CompileCandidateGroup> scanPersonSummaries();
    QList<CompileCandidateGroup> scanEventSummaries();
    QList<CompileCandidateGroup> scanTopicClusters();

    // Build the LLM prompt for a candidate
    QString buildPersonSummaryPrompt(const CompileCandidateGroup &candidate);
    QString buildEventSummaryPrompt(const CompileCandidateGroup &candidate);
    QString buildTopicClusterPrompt(const CompileCandidateGroup &candidate);

    // Read source entries and concatenate for prompt context
    QString readSourceContext(const QStringList &relativePaths, int maxChars = 12000);

    // Write compiled article to vault
    bool writeCompiledArticle(const CompileResult &result);

    // Process the next candidate in the queue
    void processNextCandidate();

    VaultManager *m_vault;
    LlmClient *m_llm;
    bool m_running = false;
    bool m_cancelled = false;
    int m_progress = 0;
    int m_totalCandidates = 0;
    int m_articlesWritten = 0;
    QString m_statusText;
    QList<CompileCandidateGroup> m_candidates;
};

#endif // WIKICOMPILER_H
