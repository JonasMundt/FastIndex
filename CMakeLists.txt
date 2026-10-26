
cmake_minimum_required(VERSION 3.21)

project(FastIndex VERSION 0.1 LANGUAGES CXX)

# --------------------------------------------------
# C++ Configuration
# --------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------------------------------
# Qt Configuration
# --------------------------------------------------

find_package(Qt6 6.12 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

# --------------------------------------------------
# Core Library (independent from Qt)
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
# Desktop Application
# --------------------------------------------------

qt_add_executable(FastIndex
    src/main.cpp
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
        fastindex_core
)

# --------------------------------------------------
# Automated Tests (GoogleTest)
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
    )

    target_link_libraries(fastindex_tests
        PRIVATE
            fastindex_core
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(fastindex_tests)
endif()
