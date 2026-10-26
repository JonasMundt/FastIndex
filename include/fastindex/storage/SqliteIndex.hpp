
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>
#include <stop_token>

struct sqlite3;

namespace fastindex::storage {

struct SearchResult {
    std::string path;
    std::string snippet;
};

class SqliteIndex {
public:
    // ------------------------------------------
    // Transaction Management
    // ------------------------------------------

    class Transaction {
    public:
        explicit Transaction(SqliteIndex& index);
        ~Transaction() noexcept;

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        void commit();

    private:
        SqliteIndex& index_;
        bool committed_ = false;
    };

    // ------------------------------------------
    // Database Lifecycle
    // ------------------------------------------

    explicit SqliteIndex(
        const std::filesystem::path& databasePath
    );

    ~SqliteIndex();

    SqliteIndex(const SqliteIndex&) = delete;
    SqliteIndex& operator=(const SqliteIndex&) = delete;

    SqliteIndex(SqliteIndex&&) = delete;
    SqliteIndex& operator=(SqliteIndex&&) = delete;

    // ------------------------------------------
    // File Operations
    // ------------------------------------------

    void upsert(
        const std::filesystem::path& filePath,
        std::string_view content
    );

    [[nodiscard]]
    bool remove(
        const std::filesystem::path& filePath
    );

    
    [[nodiscard]]
    std::size_t removeMissingFilesUnder(
        const std::filesystem::path& directory,
        std::stop_token stop = {}
    );


    // ------------------------------------------
    // Search
    // ------------------------------------------

    [[nodiscard]]
    std::vector<SearchResult> search(
        std::string_view query
    ) const;

private:
    void execute(const char* sql);

    sqlite3* database_ = nullptr;
};

} // namespace fastindex::storage
