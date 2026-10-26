
#include "fastindex/indexing/IndexingService.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

using fastindex::indexing::IndexingService;
using fastindex::storage::SqliteIndex;

class IndexingServiceTest : public ::testing::Test {
protected:
    fs::path root_;
    fs::path documents_;

    void SetUp() override {
        const auto id =
            std::chrono::steady_clock::now()
                .time_since_epoch().count();

        root_ = fs::temp_directory_path() /
            ("fastindex_integration_" + std::to_string(id));

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

        std::ofstream output(path, std::ios::binary);

        ASSERT_TRUE(output.is_open());

        output.write(
            content.data(),
            static_cast<std::streamsize>(content.size())
        );

        ASSERT_TRUE(output.good());
    }
};

TEST_F(IndexingServiceTest, IndexesFilesRecursively) {
    writeFile("notes.txt", "The moon is beautiful");
    writeFile("nested/report.md", "Information about Mars");
    writeFile("image.png", "The moon");

    SqliteIndex index(databasePath());
    IndexingService service(index);

    const auto stats = service.indexDirectory(documents_);

    EXPECT_EQ(stats.discovered, 2u);
    EXPECT_EQ(stats.indexed, 2u);
    EXPECT_EQ(stats.skipped, 0u);
    EXPECT_TRUE(stats.errors.empty());

    const auto results = index.search("moon");

    ASSERT_EQ(results.size(), 1u);

    EXPECT_EQ(
        fs::path(results[0].path),
        documents_ / "notes.txt"
    );
}

TEST_F(IndexingServiceTest, ReindexingUpdatesContent) {
    writeFile("notes.txt", "oldneedle");

    SqliteIndex index(databasePath());
    IndexingService service(index);

    const auto first = service.indexDirectory(documents_);

    ASSERT_EQ(first.indexed, 1u);
    ASSERT_EQ(index.search("oldneedle").size(), 1u);

    writeFile("notes.txt", "newneedle");

    const auto second = service.indexDirectory(documents_);

    EXPECT_EQ(second.indexed, 1u);

    EXPECT_TRUE(index.search("oldneedle").empty());

    EXPECT_EQ(
        index.search("newneedle").size(),
        1u
    );
}

TEST_F(IndexingServiceTest, SkipsOversizedFiles) {
    writeFile("large.txt", "123456789");
    writeFile("small.txt", "tiny");

    SqliteIndex index(databasePath());

    // Maximum of five bytes for this test.
    IndexingService service(index, 5);

    const auto stats = service.indexDirectory(documents_);

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

TEST_F(IndexingServiceTest, RejectsInvalidDirectory) {
    SqliteIndex index(databasePath());
    IndexingService service(index);

    EXPECT_THROW(
        static_cast<void>(
            service.indexDirectory(root_ / "missing")
        ),
        std::invalid_argument
    );
}
