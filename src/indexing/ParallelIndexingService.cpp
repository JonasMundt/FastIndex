
#include "fastindex/indexing/ParallelIndexingService.hpp"
#include "fastindex/concurrency/BlockingQueue.hpp"

#include <atomic>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fastindex::indexing {

namespace {

struct ReadResult {
    std::filesystem::path path;
    std::string content;
    std::optional<std::string> error;
};

} // anonymous namespace

// --------------------------------------------------
// Constructor
// --------------------------------------------------

ParallelIndexingService::ParallelIndexingService(
    storage::SqliteIndex& index,
    std::size_t workerCount,
    std::size_t maxFileBytes
)
    : index_(index),
      workerCount_(workerCount),
      maxFileBytes_(maxFileBytes) {

    if (workerCount == 0 || workerCount > 16) {
        throw std::invalid_argument(
            "Worker count must be between 1 and 16"
        );
    }
}

// --------------------------------------------------
// Parallel Indexing
// --------------------------------------------------

IndexingStats ParallelIndexingService::indexDirectory(
    const std::filesystem::path& directory,
    std::stop_token externalStop,
    ProgressCallback onProgress
) {
    IndexingStats stats;

    if (externalStop.stop_requested()) {
        stats.cancelled = true;
        return stats;
    }

    // Discover supported files.
    const auto files = scanner_.scan(directory);
    stats.discovered = files.size();

    // Bound the number of buffered file contents.
    concurrency::BlockingQueue<ReadResult> results(
        workerCount_
    );

    std::atomic<std::size_t> nextFile{0};
    std::atomic<std::size_t> activeWorkers{
        workerCount_
    };

    // --------------------------------------------------
    // Cancellation Management
    // --------------------------------------------------

    std::stop_source stopSource;

    std::stop_callback externalCallback(
        externalStop,
        [&] {
            stopSource.request_stop();
        }
    );

    const auto token = stopSource.get_token();

    // --------------------------------------------------
    // Worker Error Handling
    // --------------------------------------------------

    std::mutex failureMutex;
    std::exception_ptr workerFailure;

    // --------------------------------------------------
    // Worker Function
    // --------------------------------------------------

    auto worker = [&](std::stop_token threadStop) {
        try {
            core::FileReader reader;

            while (!threadStop.stop_requested() &&
                   !token.stop_requested()) {

                const auto position =
                    nextFile.fetch_add(1);

                if (position >= files.size()) {
                    break;
                }

                ReadResult result{
                    files[position],
                    {},
                    std::nullopt
                };

                try {
                    result.content = reader.read(
                        result.path,
                        maxFileBytes_
                    );
                }
                catch (const std::length_error& error) {
                    result.error = error.what();
                }
                catch (const std::runtime_error& error) {
                    result.error = error.what();
                }

                if (!results.push(
                        std::move(result), token
                    )) {
                    break;
                }
            }
        }
        catch (...) {
            {
                std::lock_guard lock(failureMutex);

                if (!workerFailure) {
                    workerFailure =
                        std::current_exception();
                }
            }

            stopSource.request_stop();
        }

        // Last worker closes the queue.
        if (activeWorkers.fetch_sub(1) == 1) {
            results.close();
        }
    };

    // --------------------------------------------------
    // Start Worker Threads
    // --------------------------------------------------

    std::vector<std::jthread> workers;
    workers.reserve(workerCount_);

    try {
        for (std::size_t i = 0;
             i < workerCount_; ++i) {

            workers.emplace_back(worker);
        }
    }
    catch (...) {
        stopSource.request_stop();
        results.close();

        for (auto& thread : workers) {
            thread.join();
        }

        throw;
    }

    // --------------------------------------------------
    // SQLite Writer with Batch Transactions
    // --------------------------------------------------

    constexpr std::size_t batchSize = 100;

    std::unique_ptr<storage::SqliteIndex::Transaction>
        transaction;

    std::size_t pendingWrites = 0;

    // Only the calling thread writes to SQLite.
    try {
        while (auto result = results.pop(token)) {

            // Handle failed file reads.
            if (result->error) {
                ++stats.skipped;

                stats.errors.push_back({
                    result->path,
                    *result->error
                });

                continue;
            }

            // Start transaction when required.
            if (!transaction) {
                transaction = std::make_unique<
                    storage::SqliteIndex::Transaction
                >(index_);
            }

            // Store file content.
            index_.upsert(
                result->path,
                result->content
            );

            ++stats.indexed;
            ++pendingWrites;

            // Commit full batch.
            if (pendingWrites == batchSize) {
                transaction->commit();
                transaction.reset();

                pendingWrites = 0;

                // Report committed progress.
                if (onProgress) {
                    onProgress(stats);
                }
            }
        }

        // Commit remaining files.
        if (transaction) {
            transaction->commit();
            transaction.reset();

            if (onProgress) {
                onProgress(stats);
            }
        }
    }
    catch (...) {
        stopSource.request_stop();
        results.close();

        for (auto& thread : workers) {
            thread.join();
        }

        throw;
    }

    // --------------------------------------------------
    // Wait for Workers
    // --------------------------------------------------

    for (auto& thread : workers) {
        thread.join();
    }

    // --------------------------------------------------
    // Propagate Unexpected Worker Failures
    // --------------------------------------------------

    {
        std::lock_guard lock(failureMutex);

        if (workerFailure) {
            std::rethrow_exception(workerFailure);
        }
    }

    // --------------------------------------------------
    // Final Statistics
    // --------------------------------------------------

    stats.cancelled =
        token.stop_requested() &&
        (stats.indexed + stats.skipped < stats.discovered);

    return stats;
}

} // namespace fastindex::indexing
