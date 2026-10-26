
cmake_minimum_required(VERSION 3.21)

project(FastIndex VERSION 0.1 LANGUAGES C CXX)

# --------------------------------------------------
# C++ Configuration
# --------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------------------------------
# Qt Configuration
# --------------------------------------------------

find_package(Qt6 6.10 REQUIRED COMPONENTS
    Quick
    QuickControls2
)


qt_standard_project_setup(REQUIRES 6.8)

# --------------------------------------------------
# SQLite with FTS5
# --------------------------------------------------

add_library(sqlite3 STATIC
    third_party/sqlite/sqlite3.c
)

target_include_directories(sqlite3
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)

target_compile_definitions(sqlite3
    PRIVATE
        SQLITE_ENABLE_FTS5=1
        SQLITE_OMIT_LOAD_EXTENSION=1
)

# --------------------------------------------------
# Core Library
# --------------------------------------------------

add_library(fastindex_core STATIC
    src/core/FileScanner.cpp
    src/core/FileReader.cpp
)

target_include_directories(fastindex_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(fastindex_core
    PUBLIC cxx_std_20
)

# --------------------------------------------------
# Storage Library
# --------------------------------------------------

add_library(fastindex_storage STATIC
    src/storage/SqliteIndex.cpp
)

target_include_directories(fastindex_storage
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fastindex_storage
    PRIVATE sqlite3
)

# --------------------------------------------------
# Indexing Library
# --------------------------------------------------

add_library(fastindex_indexing STATIC
    src/indexing/IndexingService.cpp
    src/indexing/ParallelIndexingService.cpp
)

target_include_directories(fastindex_indexing
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fastindex_indexing
    PUBLIC
        fastindex_core
        fastindex_storage
)

# --------------------------------------------------
# Command Line Application
# --------------------------------------------------

add_executable(fastindex_cli
    src/cli/main.cpp
)

target_link_libraries(fastindex_cli
    PRIVATE
        fastindex_indexing
)

# --------------------------------------------------
# Desktop Application
# --------------------------------------------------

qt_add_executable(FastIndex
    src/main.cpp
    src/app/IndexingController.hpp
    src/app/IndexingController.cpp
)

target_include_directories(FastIndex
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

qt_add_qml_module(FastIndex
    URI FastIndex
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(FastIndex
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        fastindex_indexing
)

# --------------------------------------------------
# Automated Tests
# --------------------------------------------------

include(CTest)

if(BUILD_TESTING)
    include(FetchContent)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.18.0
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(googletest)

add_executable(fastindex_tests
    tests/FileScannerTests.cpp
    tests/FileReaderTests.cpp
    tests/SqliteIndexTests.cpp
    tests/IndexingServiceTests.cpp
    tests/BlockingQueueTests.cpp
    tests/ParallelIndexingServiceTests.cpp
)



    target_link_libraries(fastindex_tests
        PRIVATE
            fastindex_indexing
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(fastindex_tests)
endif()
