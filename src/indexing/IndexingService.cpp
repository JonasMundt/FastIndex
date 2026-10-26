#include "fastindex/indexing/IndexingService.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <memory>

namespace fastindex::indexing {

IndexingService::IndexingService(
    storage::SqliteIndex& index,
    std::size_t maxFileBytes
)
    : index_(index),
      maxFileBytes_(maxFileBytes) {
}

IndexingStats IndexingService::indexDirectory(
    const std::filesystem::path& directory
) {
    IndexingStats stats;

    const auto files = scanner_.scan(directory);

    stats.discovered = files.size();
    
    constexpr std::size_t batchSize = 100;

    std::unique_ptr<storage::SqliteIndex::Transaction>
        transaction;

    std::size_t pendingWrites = 0;


    for (const auto& file : files) {
        std::string content;

        try {
            content = reader_.read(
                file,
                maxFileBytes_
            );
        }
        catch (const std::length_error& error) {
            ++stats.skipped;

            stats.errors.push_back({
                file,
                error.what()
            });

            continue;
        }
        catch (const std::runtime_error& error) {
            ++stats.skipped;

            stats.errors.push_back({
                file,
                error.what()
            });

            continue;
        }

        // Database errors must not be silently ignored.
        
        if (!transaction) {
            transaction =
                std::make_unique<storage::SqliteIndex::Transaction>(
                    index_
                );
        }

        index_.upsert(file, content);

        ++stats.indexed;
        ++pendingWrites;

        if (pendingWrites == batchSize) {
            transaction->commit();
            transaction.reset();
            pendingWrites = 0;
        }

    }

    if (transaction) {
    transaction->commit();
    }
    return stats;
}

} // namespace fastindex::indexing
