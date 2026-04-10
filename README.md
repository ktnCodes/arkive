# Arkive

**Your life wiki. Local-only. Never leaves your device.**

A privacy-first desktop application that ingests personal data (photos, iMessages, Snapchat exports), compiles it into a queryable wiki, and provides a chat interface backed by hybrid LLM.

## Tech Stack

- **Language:** C++17
- **GUI:** Qt 6.8.3 / QML (Basic style, Catppuccin Mocha/Latte themes)
- **Build:** CMake 3.21+ / Ninja / MinGW 13.1
- **Tests:** Google Test 1.14
- **Local LLM:** llama.cpp (Phase 3)
- **Cloud LLM:** Claude / OpenAI API (opt-in)

## Prerequisites

| Tool | Version | Install |
|------|---------|---------|
| Qt | 6.8.3 | `pip install aqtinstall && aqt install-qt windows desktop 6.8.3 win64_mingw --outputdir C:\Qt --modules qtimageformats` |
| MinGW | 13.1.0 | Included with Qt (`C:\Qt\Tools\mingw1310_64`) |
| CMake | 3.21+ | Included with Qt (`C:\Qt\Tools\CMake_64`) |
| Ninja | 1.12+ | Included with Qt (`C:\Qt\Tools\Ninja`) |

## Quick Start

### 1. Set up PATH

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\6.8.3\mingw_64\bin;$env:PATH"
```

> Run this in every new terminal session, or add it to your PowerShell profile.

### 2. Configure (first time only)

```powershell
cd coding-projects\arkive
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
```

### 3. Build

```powershell
cmake --build build
```

### 4. Run

```powershell
.\build\arkive.exe
```

### Logs

Arkive now writes runtime logs to:

```text
%LOCALAPPDATA%\Arkive\Arkive\logs\arkive.log
```

The same messages are also echoed to stderr when launched from a terminal.

### 5. Test

```powershell
.\build\arkive_tests.exe
```

## Commands Reference

| Command | What it does |
|---------|-------------|
| `cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"` | Configure the project (first time / after CMakeLists changes) |
| `cmake --build build` | Build the app and tests |
| `cmake --build build --target arkive` | Build only the app (skip tests) |
| `cmake --build build --target arkive_tests` | Build only the tests |
| `.\build\arkive.exe` | Launch the app |
| `.\build\arkive_tests.exe` | Run all 92 tests |
| `.\build\arkive_tests.exe --gtest_filter="SuiteName.*"` | Run tests matching a pattern |
| `.\build\arkive_tests.exe --gtest_list_tests` | List all available tests |
| `cmake --build build --clean-first` | Clean rebuild |

### Test Filter Examples

```powershell
# Run all MarkdownParser tests
.\build\arkive_tests.exe --gtest_filter="MarkdownParserTest.*"

# Run a single test
.\build\arkive_tests.exe --gtest_filter="ExifReaderTest.ReturnsEmptyForNonexistentFile"

# Run all tests except slow ones
.\build\arkive_tests.exe --gtest_filter="-VaultManagerTest.*"
```

## Project Structure

```
arkive/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   ├── core/                  # C++ business logic
│   │   ├── VaultManager       # Vault filesystem ops (create, read, write)
│   │   ├── MarkdownParser     # Parse markdown → sections, backlinks, HTML
│   │   ├── IndexManager       # Auto-generate _index.md per section
│   │   ├── ExifReader         # JPEG EXIF parser (date, GPS, camera)
│   │   ├── GeocodeCache       # Reverse geocode GPS → place names (SQLite cache)
│   │   └── PhotoIngestor      # Import photos → grouped wiki entries
│   └── models/                # Qt models for QML binding
│       ├── FileTreeModel      # Vault directory tree (QAbstractItemModel)
│       └── ArticleModel       # Single article view data
├── qml/                       # QML UI components
│   ├── Main.qml               # App shell, toolbar, SplitView
│   ├── FileBrowser.qml        # Sidebar TreeView
│   ├── ArticleView.qml        # Article reader with TOC
│   ├── ImportPanel.qml         # Photo import UI with progress
│   ├── SettingsPanel.qml       # Settings drawer (theme, vault, LLM)
│   └── Theme.qml              # Design tokens (Catppuccin Mocha/Latte)
└── tests/                     # Google Test suite (92 tests)
    ├── test_main.cpp           # Custom main with QCoreApplication
    ├── test_exif_reader.cpp
    ├── test_markdown_parser.cpp
    ├── test_vault_manager.cpp
    ├── test_geocode_cache.cpp
    ├── test_photo_ingestor.cpp
    ├── test_index_manager.cpp
    ├── test_file_tree_model.cpp
    └── test_article_model.cpp
```

## Vault Location

The vault is stored at `~/.arkive/vault/` with this structure:

```
~/.arkive/vault/
├── _index.md          # Master index (auto-generated)
├── people/            # Person articles
├── conversations/     # Chat/message articles
├── memories/          # Memory articles
├── places/            # Location articles
├── media/             # Photo import entries
└── raw/               # Raw ingested data
```

## License

TBD
