# Arkive

**Your life wiki. Local-only. Never leaves your device.**

A privacy-first desktop application that ingests personal data (photos, iMessages, Snapchat exports), compiles it into a queryable wiki, and provides a chat interface backed by hybrid LLM.

## Tech Stack

- **Language:** C++17
- **GUI:** Qt 6.5+ / QML
- **Build:** CMake 3.21+
- **Local LLM:** llama.cpp (Phase 3)
- **Cloud LLM:** Claude / OpenAI API (opt-in)

## Building

```bash
mkdir build && cd build
cmake .. -G Ninja
cmake --build .
```

Requires Qt 6.5+ with QtQuick, QtSql, and QtNetwork modules.

## Project Structure

```
arkive/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── core/           # C++ business logic
│   │   ├── VaultManager     # File system operations
│   │   ├── MarkdownParser   # Parse markdown + backlinks
│   │   └── IndexManager     # Maintain _index.md files
│   └── models/         # Qt models for QML
│       ├── FileTreeModel    # Vault directory tree
│       └── ArticleModel     # Single article data
├── qml/                # QML UI components
│   ├── Main.qml
│   ├── FileBrowser.qml
│   ├── ArticleView.qml
│   └── SettingsPanel.qml
└── resources/
```

## License

TBD
