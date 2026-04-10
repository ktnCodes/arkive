cmake_minimum_required(VERSION 3.21)

project(Arkive VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Sql Network)

qt_standard_project_setup(REQUIRES 6.5)

# --- Source files ---
set(CORE_SOURCES
    src/core/VaultManager.cpp
    src/core/VaultManager.h
    src/core/MarkdownParser.cpp
    src/core/MarkdownParser.h
    src/core/IndexManager.cpp
    src/core/IndexManager.h
)

set(MODEL_SOURCES
    src/models/FileTreeModel.cpp
    src/models/FileTreeModel.h
    src/models/ArticleModel.cpp
    src/models/ArticleModel.h
)

# --- Executable ---
qt_add_executable(arkive
    src/main.cpp
    ${CORE_SOURCES}
    ${MODEL_SOURCES}
)

# --- QML module ---
qt_add_qml_module(arkive
    URI Arkive
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/FileBrowser.qml
        qml/ArticleView.qml
        qml/SettingsPanel.qml
)

# --- Link libraries ---
target_link_libraries(arkive PRIVATE
    Qt6::Quick
    Qt6::Sql
    Qt6::Network
)

target_include_directories(arkive PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --- Install ---
install(TARGETS arkive
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
