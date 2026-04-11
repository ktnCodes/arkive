cmake_minimum_required(VERSION 3.21)

project(Arkive VERSION 0.1.0 LANGUAGES CXX)

# Default to Debug if no build type specified (fixes Qt Creator empty-config warning)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Sql Network Test)

qt_standard_project_setup(REQUIRES 6.5)

# --- Source files ---
set(CORE_SOURCES
    src/core/VaultManager.cpp
    src/core/VaultManager.h
    src/core/MarkdownParser.cpp
    src/core/MarkdownParser.h
    src/core/IndexManager.cpp
    src/core/IndexManager.h
    src/core/ExifReader.cpp
    src/core/ExifReader.h
    src/core/GeocodeCache.cpp
    src/core/GeocodeCache.h
    src/core/PhotoIngestor.cpp
    src/core/PhotoIngestor.h
    src/core/MessageIngestor.cpp
    src/core/MessageIngestor.h
    src/core/SnapchatIngestor.cpp
    src/core/SnapchatIngestor.h
    src/core/LlmClient.h
    src/core/CloudLlmClient.cpp
    src/core/CloudLlmClient.h
    src/core/WikiCompiler.cpp
    src/core/WikiCompiler.h
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
    NO_CACHEGEN
    QML_FILES
        qml/Main.qml
        qml/FileBrowser.qml
        qml/ArticleView.qml
        qml/SettingsPanel.qml
        qml/ImportPanel.qml
        qml/SkillPanel.qml
        qml/Theme.qml
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

# --- Tests ---
option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    # Build a library from core + model sources so tests can link against it
    add_library(arkive_lib STATIC
        ${CORE_SOURCES}
        ${MODEL_SOURCES}
    )
    target_link_libraries(arkive_lib PUBLIC
        Qt6::Quick
        Qt6::Sql
        Qt6::Network
    )
    target_include_directories(arkive_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_compile_definitions(arkive_lib PUBLIC ARKIVE_TESTING)

    add_executable(arkive_tests
        tests/test_main.cpp
        tests/test_exif_reader.cpp
        tests/test_markdown_parser.cpp
        tests/test_vault_manager.cpp
        tests/test_geocode_cache.cpp
        tests/test_photo_ingestor.cpp
        tests/test_message_ingestor.cpp
        tests/test_snapchat_ingestor.cpp
        tests/test_wiki_compiler.cpp
        tests/test_cloud_llm_client.cpp
        tests/test_index_manager.cpp
        tests/test_file_tree_model.cpp
        tests/test_article_model.cpp
    )
    target_link_libraries(arkive_tests PRIVATE
        arkive_lib
        Qt6::Test
        GTest::gtest
    )
    target_include_directories(arkive_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    include(GoogleTest)
    gtest_discover_tests(arkive_tests)
endif()
