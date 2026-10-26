
#pragma once

#include "fastindex/core/FileReader.hpp"
#include "fastindex/core/FileScanner.hpp"
#include "fastindex/storage/SqliteIndex.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace fastindex::indexing {

struct IndexingError {
    std::filesystem::path path;
    std::string message;
};

struct IndexingStats {
    std::size_t discovered = 0;
    std::size_t indexed = 0;
    std::size_t skipped = 0;

    std::vector<IndexingError> errors;
};

class IndexingService {
public:
    explicit IndexingService(
        storage::SqliteIndex& index,
        std::size_t maxFileBytes = 10 * 1024 * 1024
    );

    [[nodiscard]]
    IndexingStats indexDirectory(
        const std::filesystem::path& directory
    );

private:
    storage::SqliteIndex& index_;
    std::size_t maxFileBytes_;

    core::FileScanner scanner_;
    core::FileReader reader_;
};

} // namespace fastindex::indexing
