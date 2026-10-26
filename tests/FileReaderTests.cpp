
#include "fastindex/core/FileReader.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace fs = std::filesystem;
using fastindex::core::FileReader;

class FileReaderTest : public ::testing::Test {
protected:
    fs::path root_ = fs::temp_directory_path() /
        ("fastindex_reader_test_" + std::to_string(
            std::chrono::steady_clock::now()
                .time_since_epoch().count()));

    void SetUp() override {
        fs::create_directories(root_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    void writeFile(
        const std::string& name,
        const std::string& content
    ) {
        std::ofstream output(
            root_ / name,
            std::ios::binary
        );

        ASSERT_TRUE(output.is_open());

        output.write(content.data(), content.size());
        ASSERT_TRUE(output.good());
    }
};

TEST_F(FileReaderTest, ReadsTextContent) {
    writeFile("test.txt", "Hello FastIndex!\nSecond line");

    const auto content =
        FileReader{}.read(root_ / "test.txt");

    EXPECT_EQ(
        content,
        "Hello FastIndex!\nSecond line"
    );
}

TEST_F(FileReaderTest, HandlesEmptyFile) {
    writeFile("empty.txt", "");

    const auto content =
        FileReader{}.read(root_ / "empty.txt");

    EXPECT_TRUE(content.empty());
}

TEST_F(FileReaderTest, RejectsMissingFile) {
    EXPECT_THROW(
        static_cast<void>(
            FileReader{}.read(root_ / "missing.txt")
        ),
        std::runtime_error
    );
}

TEST_F(FileReaderTest, RejectsOversizedFile) {
    writeFile("large.txt", "123456789");

    EXPECT_THROW(
        static_cast<void>(
            FileReader{}.read(root_ / "large.txt", 5)
        ),
        std::length_error
    );
}
