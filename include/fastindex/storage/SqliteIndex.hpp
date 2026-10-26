#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

struct sqlite3;

namespace fastindex::storage {

struct SearchResult {
    std::string path;
    std::string snippet;
};

class SqliteIndex {
public:
    explicit SqliteIndex(
        const std::filesystem::path& databasePath
    );

    ~SqliteIndex();

    SqliteIndex(const SqliteIndex&) = delete;
    SqliteIndex& operator=(const SqliteIndex&) = delete;

    SqliteIndex(SqliteIndex&&) = delete;
    SqliteIndex& operator=(SqliteIndex&&) = delete;

    void upsert(
        const std::filesystem::path& filePath,
        std::string_view content
    );

    [[nodiscard]]
    std::vector<SearchResult> search(
        std::string_view query
    ) const;

private:
    void execute(const char* sql);

    sqlite3* database_ = nullptr;
};

} // namespace fastindex::storage
