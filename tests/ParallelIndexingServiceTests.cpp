
#include "fastindex/indexing/ParallelIndexingService.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stop_token>
#include <stdexcept>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

using fastindex::indexing::ParallelIndexingService;
using fastindex::storage::SqliteIndex;

class ParallelIndexingServiceTest
    : public ::testing::Test {
protected:
    fs::path root_;
    fs::path documents_;

    void SetUp() override {
        const auto id =
            std::chrono::steady_clock::now()
                .time_since_epoch().count();

        root_ = fs::temp_directory_path() /
            ("fastindex_parallel_" + std::to_string(id));

        documents_ = root_ / "documents";

        fs::create_directories(documents_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    fs::path databasePath() const {
        return root_ / "index.db";
    }

    void writeFile(
        const fs::path& relativePath,
        const std::string& content
    ) {
        const auto path = documents_ / relativePath;

        fs::create_directories(path.parent_path());

        std::ofstream output(
            path,
            std::ios::binary
        );

        ASSERT_TRUE(output.is_open());

        output.write(
            content.data(),
            static_cast<std::streamsize>(content.size())
        );

        ASSERT_TRUE(output.good());
    }
};

TEST_F(
    ParallelIndexingServiceTest,
    IndexesFilesWithMultipleWorkers
) {
    for (int i = 0; i < 24; ++i) {
        writeFile(
            "file" + std::to_string(i) + ".txt",
            "needle number " + std::to_string(i)
        );
    }

    SqliteIndex index(databasePath());
    ParallelIndexingService service(index, 4);

    const auto stats =
        service.indexDirectory(documents_);

    EXPECT_EQ(stats.discovered, 24u);
    EXPECT_EQ(stats.indexed, 24u);
    EXPECT_EQ(stats.skipped, 0u);
    EXPECT_FALSE(stats.cancelled);

    EXPECT_EQ(index.search("needle").size(), 24u);
}

TEST_F(
    ParallelIndexingServiceTest,
    ReindexingUpdatesContents
) {
    writeFile("notes.txt", "oldneedle");

    SqliteIndex index(databasePath());
    ParallelIndexingService service(index, 3);

    const auto first =
        service.indexDirectory(documents_);

    ASSERT_EQ(first.indexed, 1u);

    writeFile("notes.txt", "newneedle");

    const auto second =
        service.indexDirectory(documents_);

    EXPECT_EQ(second.indexed, 1u);
    EXPECT_TRUE(index.search("oldneedle").empty());
    EXPECT_EQ(index.search("newneedle").size(), 1u);
}

TEST_F(
    ParallelIndexingServiceTest,
    SkipsOversizedFiles
) {
    writeFile("small.txt", "tiny");
    writeFile("large.txt", "123456789");

    SqliteIndex index(databasePath());

    ParallelIndexingService service(index, 4, 4);

    const auto stats =
        service.indexDirectory(documents_);

    EXPECT_EQ(stats.discovered, 2u);
    EXPECT_EQ(stats.indexed, 1u);
    EXPECT_EQ(stats.skipped, 1u);

    ASSERT_EQ(stats.errors.size(), 1u);

    EXPECT_EQ(
        stats.errors[0].path,
        documents_ / "large.txt"
    );

    EXPECT_EQ(index.search("tiny").size(), 1u);
}

TEST_F(
    ParallelIndexingServiceTest,
    RejectsZeroWorkers
) {
    SqliteIndex index(databasePath());

    EXPECT_THROW(
        ParallelIndexingService(index, 0),
        std::invalid_argument
    );
}

TEST_F(
    ParallelIndexingServiceTest,
    SupportsPreRequestedCancellation
) {
    writeFile("notes.txt", "searchable content");

    SqliteIndex index(databasePath());
    ParallelIndexingService service(index, 4);

    std::stop_source stopSource;
    stopSource.request_stop();

    const auto stats = service.indexDirectory(
        documents_,
        stopSource.get_token()
    );

    EXPECT_TRUE(stats.cancelled);
    EXPECT_EQ(stats.indexed, 0u);
    EXPECT_TRUE(index.search("searchable").empty());
}

TEST_F(
    ParallelIndexingServiceTest,
    CancelsAfterFirstCommittedBatch
) {
    constexpr int fileCount = 250;
    constexpr std::size_t batchSize = 100;

    // Create 250 test files.
    for (int i = 0; i < fileCount; ++i) {
        writeFile(
            "file" + std::to_string(i) + ".txt",
            "cancelneedle document " + std::to_string(i)
        );
    }

    std::stop_source stopSource;

    std::size_t progressCalls = 0;

    {
        SqliteIndex index(databasePath());
        ParallelIndexingService service(index, 4);

        const auto stats = service.indexDirectory(
            documents_,
            stopSource.get_token(),

            [&](const auto& progress) {
                ++progressCalls;

                // Cancel after the first committed batch.
                if (progress.indexed >= batchSize) {
                    stopSource.request_stop();
                }
            }
        );

        EXPECT_TRUE(stats.cancelled);

        EXPECT_EQ(stats.discovered, 250u);
        EXPECT_EQ(stats.indexed, 100u);
        EXPECT_EQ(stats.skipped, 0u);

        EXPECT_EQ(progressCalls, 1u);
    }

    // Reopen the database and verify persistence.
    {
        SqliteIndex index(databasePath());

        const auto results =
            index.search("cancelneedle");

        EXPECT_EQ(results.size(), batchSize);
    }
}

