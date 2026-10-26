
#pragma once

#include "fastindex/indexing/IndexingService.hpp"

#include <cstddef>
#include <filesystem>
#include <stop_token>

namespace fastindex::indexing {

class ParallelIndexingService {
public:
    explicit ParallelIndexingService(
        storage::SqliteIndex& index,
        std::size_t workerCount = 4,
        std::size_t maxFileBytes = 10 * 1024 * 1024
    );

    [[nodiscard]]
    IndexingStats indexDirectory(
        const std::filesystem::path& directory,
        std::stop_token stop = {}
    );

private:
    storage::SqliteIndex& index_;
    std::size_t workerCount_;
    std::size_t maxFileBytes_;

    core::FileScanner scanner_;
};

} // namespace fastindex::indexing
