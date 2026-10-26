#include "fastindex/storage/SqliteIndex.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <filesystem>
#include <fstream>
#include <stop_token>

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

TEST_F(SqliteIndexTest, CommitsExplicitTransaction) {
    SqliteIndex index(databasePath());

    {
        SqliteIndex::Transaction transaction(index);

        index.upsert("first.txt", "batchalpha");
        index.upsert("second.txt", "batchbeta");

        transaction.commit();
    }

    EXPECT_EQ(index.search("batchalpha").size(), 1u);
    EXPECT_EQ(index.search("batchbeta").size(), 1u);
}

TEST_F(SqliteIndexTest, RollsBackUncommittedTransaction) {
    SqliteIndex index(databasePath());

    {
        SqliteIndex::Transaction transaction(index);

        index.upsert("temporary.txt", "rollbackneedle");

        // No commit: destructor must roll back.
    }

    EXPECT_TRUE(index.search("rollbackneedle").empty());
}

TEST_F(SqliteIndexTest, RemovesIndexedFileFromSearch) {
    SqliteIndex index(databasePath());

    index.upsert(
        "deleted.txt",
        "uniquedeletedneedle"
    );

    ASSERT_EQ(
        index.search("uniquedeletedneedle").size(),
        1u
    );

    // Remove the indexed file.
    EXPECT_TRUE(index.remove("deleted.txt"));

    // It must disappear from full-text search.
    EXPECT_TRUE(
        index.search("uniquedeletedneedle").empty()
    );

    // Removing it again should return false.
    EXPECT_FALSE(index.remove("deleted.txt"));
}

TEST_F(SqliteIndexTest, RollbackRestoresDeletedFile) {
    SqliteIndex index(databasePath());

    index.upsert(
        "important.txt",
        "rollbackprotectedneedle"
    );

    {
        SqliteIndex::Transaction transaction(index);

        EXPECT_TRUE(index.remove("important.txt"));

        EXPECT_TRUE(
            index.search("rollbackprotectedneedle").empty()
        );

        // No commit: deletion must be rolled back.
    }

    EXPECT_EQ(
        index.search("rollbackprotectedneedle").size(),
        1u
    );
}

TEST_F(SqliteIndexTest, CleanupRemovesOnlyMissingFilesInSelectedFolder) {
    namespace fs = std::filesystem;

    const auto base = databasePath().parent_path();
    const auto selected = base / "cleanup_selected";
    const auto other = base / "cleanup_other";

    fs::create_directories(selected);
    fs::create_directories(other);

    const auto deletedFile = selected / "deleted.txt";
    const auto existingFile = selected / "existing.txt";
    const auto otherFile = other / "other.txt";

    {
        std::ofstream(deletedFile) << "deletedneedle";
        std::ofstream(existingFile) << "existingneedle";
        std::ofstream(otherFile) << "otherneedle";
    }

    SqliteIndex index(databasePath());

    index.upsert(deletedFile, "deletedneedle");
    index.upsert(existingFile, "existingneedle");
    index.upsert(otherFile, "otherneedle");

    ASSERT_TRUE(fs::remove(deletedFile));

    const auto removed =
        index.removeMissingFilesUnder(selected);

    EXPECT_EQ(removed, 1u);

    EXPECT_TRUE(index.search("deletedneedle").empty());

    EXPECT_EQ(
        index.search("existingneedle").size(),
        1u
    );

    EXPECT_EQ(
        index.search("otherneedle").size(),
        1u
    );
}

TEST_F(SqliteIndexTest, CleanupRespectsCancellation) {
    namespace fs = std::filesystem;

    const auto root =
        databasePath().parent_path() / "cleanup_cancel";

    fs::create_directories(root);

    const auto missingFile = root / "missing.txt";

    SqliteIndex index(databasePath());
    index.upsert(missingFile, "cancelneedle");

    std::stop_source stopSource;
    stopSource.request_stop();

    const auto removed = index.removeMissingFilesUnder(
        root,
        stopSource.get_token()
    );

    EXPECT_EQ(removed, 0u);

    EXPECT_EQ(
        index.search("cancelneedle").size(),
        1u
    );
}

