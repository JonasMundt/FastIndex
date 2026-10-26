
#include "fastindex/core/FileScanner.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace fs = std::filesystem;
using fastindex::core::FileScanner;

class FileScannerTest : public ::testing::Test {
protected:
    fs::path root_ = fs::temp_directory_path() /
        ("fastindex_test_" + std::to_string(
            std::chrono::steady_clock::now()
                .time_since_epoch().count()));

    void SetUp() override {
        fs::create_directories(root_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    void createFile(const fs::path& relativePath) {
        const auto file = root_ / relativePath;
        fs::create_directories(file.parent_path());

        std::ofstream stream(file);
        ASSERT_TRUE(stream.is_open());
        stream << "Test content";
    }
};

TEST_F(FileScannerTest, FindsSupportedFilesRecursively) {
    createFile("notes.txt");
    createFile("nested/README.MD");
    createFile("nested/data.csv");

    const auto files = FileScanner{}.scan(root_);

    ASSERT_EQ(files.size(), 3u);

    EXPECT_NE(
        std::find(files.begin(), files.end(),
                  root_ / "notes.txt"),
        files.end()
    );

    EXPECT_NE(
        std::find(files.begin(), files.end(),
                  root_ / "nested/README.MD"),
        files.end()
    );

    EXPECT_TRUE(std::is_sorted(files.begin(), files.end()));
}

TEST_F(FileScannerTest, IgnoresUnsupportedFiles) {
    createFile("image.png");
    createFile("archive.zip");
    createFile("report.log");

    const auto files = FileScanner{}.scan(root_);

    ASSERT_EQ(files.size(), 1u);
    EXPECT_EQ(files.front().filename(), "report.log");
}

TEST_F(FileScannerTest, RejectsInvalidDirectory) {
    EXPECT_THROW(
        static_cast<void>(
            FileScanner{}.scan(root_ / "missing")
        ),
        std::invalid_argument
    );
}

