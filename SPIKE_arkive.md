# PRD: Arkive — Personal Life Wiki

**Date:** 2026-04-10
**Author:** Kevin
**Status:** Draft

---

## Problem Statement

People accumulate vast amounts of personal data across their devices — iMessages, photos, Snapchat conversations, notes — but this data sits in siloed apps with no way to search across it, find patterns, or surface connections. When you want to recall "what restaurant did I talk about with Mom in February?" or "where was I when I took that photo?", you're left manually scrolling through apps hoping to stumble on the answer.

Existing solutions (Limitless/Rewind, Notion AI, Apple Intelligence) are cloud-dependent, subscription-based, and require trusting a third party with your most personal data. There is no local-first, privacy-respecting tool that ingests personal data from multiple sources, organizes it into a queryable knowledge base, and lets you chat with your own life history.

## Solution Description

**Arkive** is a privacy-first desktop application (Qt 6 + QML + C++) that acts as a personal librarian for your life data. It:

1. **Ingests** personal data from exported phone backups — photos (EXIF metadata), iMessages (chat.db from iTunes backup), and Snapchat data exports (JSON) — into a local markdown vault
2. **Compiles** raw entries into structured wiki articles (person profiles, event summaries, place histories, topic clusters) using a hybrid LLM (local llama.cpp by default, cloud Claude/OpenAI opt-in)
3. **Indexes** all articles with bidirectional backlinks, auto-maintained indexes, and health checks (lint)
4. **Queries** via a chat interface where the LLM has context from the user's wiki — it can answer personal questions, surface patterns, and make recommendations based on the user's own data

All data stays on the user's machine. No accounts, no telemetry, no cloud dependency. Files are Obsidian-compatible markdown — the user owns their data as plain files they can open in any editor.

**Tagline:** "Your life wiki. Local-only. Never leaves your device."

## Modules Affected

| Module | Change Type | Notes |
|--------|------------|-------|
| `core/VaultManager` | New | File system operations — create vault, watch for changes, manage sections (people/, conversations/, memories/, places/, media/) |
| `core/MarkdownParser` | New | Regex-based parser for markdown sections, YAML frontmatter, and `[[backlink]]` extraction |
| `core/IndexManager` | New | Maintain `_index.md` per section and master index. Track article counts, last-compiled timestamps |
| `core/IngestManager` | New | Orchestrate data import from multiple sources. Deduplication, progress reporting, ingestion log |
| `core/PhotoIngestor` | New | Read EXIF metadata (date, GPS, camera), reverse geocode via Nominatim, generate grouped wiki entries |
| `core/MessageIngestor` | New | Parse iMessage `chat.db` (SQLite), extract conversations, participants, timestamps, generate wiki entries |
| `core/SnapchatIngestor` | New | Parse Snapchat JSON exports (chat_history, memories, friends), generate wiki entries |
| `core/LlmClient` | New | Abstract interface with `LocalLlmClient` (llama.cpp) and `CloudLlmClient` (HTTP to Claude/OpenAI) backends |
| `core/WikiCompiler` | New | Scan raw entries, extract concept candidates (frequency + temporal + entity heuristics), prompt LLM to generate structured articles |
| `core/WikiLinter` | New | 6 health checks: broken backlinks, source orphans, isolated articles, coverage gaps, stale indexes, raw backlog |
| `core/QueryEngine` | New | TF-IDF relevance scoring, context window packing, system prompt construction for wiki-aware chat |
| `models/FileTreeModel` | New | QAbstractItemModel exposing vault directory tree to QML |
| `models/ArticleModel` | New | Single article data model for QML rendering |
| `qml/Main.qml` | New | App shell — sidebar navigation + content area |
| `qml/FileBrowser.qml` | New | Tree view of vault with real-time updates |
| `qml/ArticleView.qml` | New | Markdown article renderer with backlink navigation |
| `qml/ChatView.qml` | New | Chat interface — message bubbles, input, streaming responses |
| `qml/ImportPanel.qml` | New | Data import wizard — source selection, folder picker, progress bars |
| `qml/SkillPanel.qml` | New | Action buttons for compile, lint, rebuild index with progress indicators |
| `qml/SettingsPanel.qml` | New | LLM provider toggle, API key entry, model selection, auto-hook toggles |

## User Stories

- As a user, I want to import my iPhone photo library so that my photos are indexed by date, location, and metadata in my personal wiki.
- As a user, I want to import my iMessage conversations from an iTunes backup so that I can search across all my text conversations in one place.
- As a user, I want to import my Snapchat data export so that my Snapchat messages and memories are included in my wiki alongside other data sources.
- As a user, I want the app to automatically compile my raw data into higher-level wiki articles (person profiles, event summaries) so that I don't have to manually organize everything.
- As a user, I want to ask natural language questions like "What did I talk about with Mom last month?" and get accurate answers based on my actual data.
- As a user, I want all my data to stay on my local machine so that I don't have to trust a third party with my personal information.
- As a user, I want to choose between a local LLM and a cloud LLM so that I can balance privacy against response quality.
- As a user, I want to run health checks on my wiki to find broken links, orphaned articles, and gaps in coverage.
- As a user, I want the wiki to auto-update indexes and backlinks so that navigation stays consistent as new data is imported.
- As a user, I want to browse my vault as a file tree and click into articles so that I can explore my wiki manually when I don't have a specific question.

## Acceptance Criteria

- [ ] App launches on Windows, creates vault directory on first run, shows empty state with import prompt
- [ ] Photo import: 50 photos imported → grouped wiki entries with EXIF date, GPS location (reverse geocoded), camera model
- [ ] iMessage import: chat.db from iTunes backup → conversation entries with correct participants, timestamps, message counts
- [ ] Snapchat import: JSON export → conversation and memory entries created, friends mapped to people/ entries
- [ ] Deduplication: re-importing the same data source produces zero duplicate entries
- [ ] Wiki compilation: 20 raw entries → 5+ higher-order articles with Summary, Key Facts, Connections, Sources sections
- [ ] Backlink resolution: all `[[slug]]` references resolve to existing files; broken links flagged
- [ ] Lint check: catches broken backlinks, source orphans, isolated articles, coverage gaps, stale indexes, raw backlog
- [ ] Chat query: "What did I talk about with [person] last week?" returns answer citing correct conversation data
- [ ] Chat query: "Where was I on [date]?" returns answer using photo EXIF location
- [ ] Chat query: unknown topic → LLM explicitly states it doesn't have that information
- [ ] Local LLM mode: zero external API calls (verified via network monitoring)
- [ ] Cloud LLM mode: explicit user opt-in required before any API call is made
- [ ] File browser: adding/deleting a .md file in the vault updates the tree view within 2 seconds
- [ ] All vault data stored as plain markdown files openable in Obsidian or any text editor

## Constraints & Assumptions

- **iPhone data access**: iMessage data requires an iTunes/Apple Devices backup on Windows. User must create this backup manually; the app provides a wizard to locate the backup directory and `chat.db` file.
- **Snapchat data**: User must request their data export from Snapchat (Settings → My Data → Submit Request). Export arrives as a ZIP with JSON files.
- **Photo volume**: Photos grouped by day + location to prevent vault bloat (1000 photos ≠ 1000 entries). Grouping heuristic: same date + GPS within 500m = one entry.
- **Qt 6.5+ required**: QML features and CMake integration depend on Qt 6.5 or later.
- **Local LLM hardware**: llama.cpp with 7-8B parameter model (Q4_K_M quantization) requires ~8GB RAM. Users with less RAM can use cloud LLM only.
- **No real-time sync**: This is import-based, not live monitoring. User re-imports when they want to update.
- **Windows primary, Mac secondary**: Development and testing targets Windows first; Mac support via Qt cross-platform (no platform-specific code except backup directory paths).
- **Obsidian compatibility**: Article format uses `# Title`, `## Section` headers, and `[[slug]]` backlinks — standard Obsidian syntax.

## Open Questions

None — design decisions resolved during planning session (2026-04-10).

## Out of Scope

- Mobile app (future phase — Qt Mobile or separate project)
- Apple Notes ingestion (complex proprietary format)
- Browser history ingestion
- Calendar / reminders ingestion
- Real-time iMessage monitoring (requires Mac companion app with system permissions)
- Voice input / speech-to-text
- Multi-user support or cloud sync
- App Store / distribution packaging
- Vector database / embeddings (TF-IDF sufficient at wiki scale of ~100-1000 articles)
- Custom markdown editor (users edit in Obsidian or any editor; Arkive is read + compile + query)
