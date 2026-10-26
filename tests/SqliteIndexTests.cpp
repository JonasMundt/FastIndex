#include "fastindex/storage/SqliteIndex.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

using fastindex::storage::SqliteIndex;

class SqliteIndexTest : public ::testing::Test {
protected:
    fs::path root_ = fs::temp_directory_path() /
        ("fastindex_sqlite_test_" + std::to_string(
            std::chrono::steady_clock::now()
                .time_since_epoch().count()));

    void SetUp() override {
        fs::create_directories(root_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    fs::path databasePath() const {
        return root_ / "test.db";
    }
};

TEST_F(SqliteIndexTest, FindsIndexedContent) {
    SqliteIndex index(databasePath());

    index.upsert(
        "notes.txt",
        "FastIndex supports full text search"
    );

    const auto results = index.search("FastIndex");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].path, "notes.txt");

    EXPECT_NE(
        results[0].snippet.find("[FastIndex]"),
        std::string::npos
    );
}

TEST_F(SqliteIndexTest, UpdatesExistingFiles) {
    SqliteIndex index(databasePath());

    index.upsert("notes.txt", "apple banana");
    index.upsert("notes.txt", "orange lemon");

    EXPECT_TRUE(index.search("apple").empty());

    const auto results = index.search("orange");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].path, "notes.txt");
}

TEST_F(SqliteIndexTest, PersistsDataAfterReopening) {
    {
        SqliteIndex index(databasePath());

        index.upsert(
            "saved.txt",
            "Persistent database content"
        );
    }

    {
        SqliteIndex index(databasePath());

        const auto results =
            index.search("Persistent");

        ASSERT_EQ(results.size(), 1u);
        EXPECT_EQ(results[0].path, "saved.txt");
    }
}
