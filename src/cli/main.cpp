
#include "fastindex/indexing/IndexingService.hpp"
#include "fastindex/indexing/ParallelIndexingService.hpp"
#include "fastindex/storage/SqliteIndex.hpp"

#include <charconv>
#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

using fastindex::indexing::IndexingService;
using fastindex::indexing::ParallelIndexingService;
using fastindex::indexing::IndexingStats;
using fastindex::storage::SqliteIndex;

namespace {

void printUsage() {
    std::cout
        << "FastIndex CLI\n\n"
        << "Usage:\n"
        << "  fastindex_cli index <directory> <database>\n"
        << "  fastindex_cli index <directory> <database>"
           " --parallel <workers>\n"
        << "  fastindex_cli search <database> <query>\n\n"
        << "Worker count: 1-16\n";
}

std::size_t parseWorkerCount(std::string_view text) {
    std::size_t count = 0;

    const auto [ptr, error] = std::from_chars(
        text.data(),
        text.data() + text.size(),
        count
    );

    if (error != std::errc{} ||
        ptr != text.data() + text.size() ||
        count < 1 || count > 16) {
        throw std::invalid_argument(
            "Worker count must be between 1 and 16"
        );
    }

    return count;
}

int runIndex(int argc, char* argv[]) {
    if (argc != 4 && argc != 6) {
        printUsage();
        return 1;
    }

    const bool parallel = argc == 6;
    std::size_t workerCount = 1;

    if (parallel) {
        if (std::string_view(argv[4]) != "--parallel") {
            printUsage();
            return 1;
        }

        workerCount = parseWorkerCount(argv[5]);
    }

    const fs::path directory = argv[2];
    const fs::path databasePath = argv[3];

    SqliteIndex database(databasePath);

    std::cout << "FastIndex - Indexing started\n"
              << "Mode: "
              << (parallel ? "Parallel" : "Sequential")
              << '\n';

    if (parallel) {
        std::cout << "Workers: " << workerCount << '\n';
    }

    const auto start = std::chrono::steady_clock::now();

    IndexingStats stats;

    if (parallel) {
        ParallelIndexingService service(
            database, workerCount
        );

        stats = service.indexDirectory(directory);
    } else {
        IndexingService service(database);

        stats = service.indexDirectory(directory);
    }

    const auto end = std::chrono::steady_clock::now();

    const auto elapsed =
        std::chrono::duration<double, std::milli>(
            end - start
        );

    std::cout
        << "\nIndexing completed!\n"
        << "Discovered: " << stats.discovered << '\n'
        << "Indexed:    " << stats.indexed << '\n'
        << "Skipped:    " << stats.skipped << '\n'
        << "Cancelled:  "
        << (stats.cancelled ? "yes" : "no") << '\n'
        << "Duration:   " << std::fixed
        << std::setprecision(2)
        << elapsed.count() << " ms\n";

    for (const auto& error : stats.errors) {
        std::cerr
            << "Skipped: " << error.path.string()
            << "\nReason: " << error.message << '\n';
    }

    if (stats.cancelled) {
        return 2;
    }

    return stats.skipped == 0 ? 0 : 2;
}

int runSearch(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage();
        return 1;
    }

    const fs::path databasePath = argv[2];
    const std::string_view query = argv[3];

    if (!fs::exists(databasePath)) {
        std::cerr << "Database does not exist.\n";
        return 1;
    }

    SqliteIndex database(databasePath);

    const auto results = database.search(query);

    std::cout
        << "Results found: " << results.size()
        << "\n\n";

    for (const auto& result : results) {
        std::cout
            << "File: " << result.path << '\n'
            << "Match: " << result.snippet
            << "\n\n";
    }

    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    try {
        const std::string_view command = argv[1];

        if (command == "index") {
            return runIndex(argc, argv);
        }

        if (command == "search") {
            return runSearch(argc, argv);
        }

        printUsage();
        return 1;
    }
    catch (const std::exception& error) {
        std::cerr
            << "FastIndex error: "
            << error.what() << '\n';

        return 1;
    }
}
