
#include "fastindex/indexing/ParallelIndexingService.hpp"
#include "fastindex/concurrency/BlockingQueue.hpp"

#include <atomic>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
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

} // namespace

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

IndexingStats ParallelIndexingService::indexDirectory(
    const std::filesystem::path& directory,
    std::stop_token externalStop
) {
    IndexingStats stats;

    if (externalStop.stop_requested()) {
        stats.cancelled = true;
        return stats;
    }

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

    // Shared cancellation state.
    std::stop_source stopSource;

    std::stop_callback externalCallback(
        externalStop,
        [&] {
            stopSource.request_stop();
        }
    );

    const auto token = stopSource.get_token();

    std::mutex failureMutex;
    std::exception_ptr workerFailure;

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
                catch (const std::length_error& e) {
                    result.error = e.what();
                }
                catch (const std::runtime_error& e) {
                    result.error = e.what();
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

        // Last worker closes the result queue.
        if (activeWorkers.fetch_sub(1) == 1) {
            results.close();
        }
    };

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

    // The calling thread is the only SQLite writer.
    try {
        while (auto result = results.pop(token)) {
            if (result->error) {
                ++stats.skipped;

                stats.errors.push_back({
                    result->path,
                    *result->error
                });

                continue;
            }

            index_.upsert(
                result->path,
                result->content
            );

            ++stats.indexed;
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

    for (auto& thread : workers) {
        thread.join();
    }

    // Propagate unexpected worker failures.
    {
        std::lock_guard lock(failureMutex);

        if (workerFailure) {
            std::rethrow_exception(workerFailure);
        }
    }

    stats.cancelled = token.stop_requested();

    return stats;
}

} // namespace fastindex::indexing
