# Arkive — Project Context

## What This Project Is

A privacy-first desktop application that ingests personal data (photos, iMessages, Snapchat exports), compiles it into a queryable wiki, and provides a chat interface backed by hybrid LLM. Your life wiki — local-only, never leaves your device.

**Status:** In Progress

---

## Tech Stack

- **Language:** C++17
- **GUI:** Qt 6.8.3 / QML (Basic style, Catppuccin Mocha/Latte themes)
- **Build:** CMake 3.21+ / Ninja / MinGW 13.1
- **Tests:** Google Test 1.14
- **Local LLM:** llama.cpp (Phase 3)
- **Cloud LLM:** Claude / OpenAI API (opt-in)

---

## Folder Structure

```
arkive/
├── CMakeLists.txt       # Root build config
├── CMakePresets.json     # Build presets
├── README.md             # Setup and usage guide
├── SPIKE_arkive.md       # Product requirements document
├── src/                  # C++ source code
├── qml/                  # QML UI files
├── tests/                # Google Test suite
├── docs/                 # Architecture and design docs
└── build/                # Build output (gitignored)
```

---

## Routing Table

| Task                          | Load These                              | Skip These         |
|-------------------------------|-----------------------------------------|--------------------|
| Build or run the app          | README.md (Quick Start section)         | docs/, tests/      |
| Understand requirements       | SPIKE_arkive.md                         | src/, build/       |
| Implement a feature           | Relevant src/ and qml/ files            | build/             |
| Write or run tests            | tests/, src/ (unit under test)          | qml/, docs/        |
| Review architecture           | docs/                                   | build/             |

---

## Build Commands

```powershell
# Set PATH (each new terminal)
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\6.8.3\mingw_64\bin;$env:PATH"

# Configure (first time)
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"

# Build
cmake --build build

# Run tests
cmake --build build --target test
```
