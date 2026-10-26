
#include "fastindex/storage/SqliteIndex.hpp"

#include <sqlite3.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace fastindex::storage {

namespace {

using Statement = std::unique_ptr<
    sqlite3_stmt,
    decltype(&sqlite3_finalize)
>;

std::string toUtf8(
    const std::filesystem::path& path
) {
    const auto utf8 =
        path.lexically_normal().generic_u8string();

    return {utf8.begin(), utf8.end()};
}

std::runtime_error makeError(
    sqlite3* database,
    const std::string& operation
) {
    return std::runtime_error(
        operation + ": " + sqlite3_errmsg(database)
    );
}

Statement prepare(
    sqlite3* database,
    const char* sql
) {
    sqlite3_stmt* raw = nullptr;

    if (sqlite3_prepare_v2(
            database, sql, -1, &raw, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(raw);
        throw makeError(database, "Prepare failed");
    }

    return Statement(raw, sqlite3_finalize);
}

void bindText(
    sqlite3* database,
    sqlite3_stmt* statement,
    int index,
    std::string_view text
) {
    const int result = sqlite3_bind_text64(
        statement,
        index,
        text.data(),
        static_cast<sqlite3_uint64>(text.size()),
        SQLITE_TRANSIENT,
        SQLITE_UTF8
    );

    if (result != SQLITE_OK) {
        throw makeError(database, "Bind failed");
    }
}

} // anonymous namespace

SqliteIndex::SqliteIndex(
    const std::filesystem::path& databasePath
) {
    const auto path = toUtf8(databasePath);

    const int result = sqlite3_open_v2(
        path.c_str(),
        &database_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );

    if (result != SQLITE_OK) {
        const std::string message = database_
            ? sqlite3_errmsg(database_)
            : "Unknown SQLite error";

        sqlite3_close(database_);
        database_ = nullptr;

        throw std::runtime_error(
            "Cannot open database: " + message
        );
    }

    try {
        execute(R"SQL(
            CREATE TABLE IF NOT EXISTS files (
                id INTEGER PRIMARY KEY,
                path TEXT NOT NULL UNIQUE,
                content TEXT NOT NULL
            );

            CREATE VIRTUAL TABLE IF NOT EXISTS files_fts
            USING fts5(
                content,
                content='files',
                content_rowid='id'
            );

            CREATE TRIGGER IF NOT EXISTS files_ai
            AFTER INSERT ON files BEGIN
                INSERT INTO files_fts(rowid, content)
                VALUES (new.id, new.content);
            END;

            CREATE TRIGGER IF NOT EXISTS files_ad
            AFTER DELETE ON files BEGIN
                INSERT INTO files_fts(
                    files_fts, rowid, content
                )
                VALUES (
                    'delete', old.id, old.content
                );
            END;

            CREATE TRIGGER IF NOT EXISTS files_au
            AFTER UPDATE ON files BEGIN
                INSERT INTO files_fts(
                    files_fts, rowid, content
                )
                VALUES (
                    'delete', old.id, old.content
                );

                INSERT INTO files_fts(rowid, content)
                VALUES (new.id, new.content);
            END;
        )SQL");
    } catch (...) {
        sqlite3_close(database_);
        database_ = nullptr;
        throw;
    }
}

SqliteIndex::~SqliteIndex() {
    sqlite3_close(database_);
}

void SqliteIndex::execute(const char* sql) {
    if (sqlite3_exec(
            database_, sql, nullptr, nullptr, nullptr
        ) != SQLITE_OK) {
        throw makeError(database_, "SQL execution failed");
    }
}

void SqliteIndex::upsert(
    const std::filesystem::path& filePath,
    std::string_view content
) {
    constexpr auto sql = R"SQL(
        INSERT INTO files(path, content)
        VALUES (?1, ?2)
        ON CONFLICT(path)
        DO UPDATE SET content = excluded.content;
    )SQL";

    auto statement = prepare(database_, sql);

    const auto path = toUtf8(filePath);

    bindText(database_, statement.get(), 1, path);
    bindText(database_, statement.get(), 2, content);

    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        throw makeError(database_, "Upsert failed");
    }
}

std::vector<SearchResult> SqliteIndex::search(
    std::string_view query
) const {
    if (query.empty()) {
        return {};
    }

    constexpr auto sql = R"SQL(
        SELECT
            files.path,
            snippet(
                files_fts, 0, '[', ']', '...', 12
            )
        FROM files_fts
        JOIN files
            ON files.id = files_fts.rowid
        WHERE files_fts MATCH ?1
        ORDER BY bm25(files_fts);
    )SQL";

    auto statement = prepare(database_, sql);

    bindText(database_, statement.get(), 1, query);

    std::vector<SearchResult> results;

    int status = SQLITE_OK;

    while ((status = sqlite3_step(statement.get()))
           == SQLITE_ROW) {

        const auto* path =
            sqlite3_column_text(statement.get(), 0);

        const auto* snippet =
            sqlite3_column_text(statement.get(), 1);

        results.push_back({
            path
                ? reinterpret_cast<const char*>(path)
                : "",
            snippet
                ? reinterpret_cast<const char*>(snippet)
                : ""
        });
    }

    if (status != SQLITE_DONE) {
        throw makeError(database_, "Search failed");
    }

    return results;
}

} // namespace fastindex::storage
