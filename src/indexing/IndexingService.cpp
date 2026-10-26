
#include "fastindex/indexing/IndexingService.hpp"

#include <stdexcept>
#include <string>
#include <utility>

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
        index_.upsert(file, content);

        ++stats.indexed;
    }

    return stats;
}

} // namespace fastindex::indexing
