
#include "fastindex/indexing/IndexingService.hpp"
#include "fastindex/storage/SqliteIndex.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace fs = std::filesystem;

using fastindex::indexing::IndexingService;
using fastindex::storage::SqliteIndex;

void printUsage() {
    std::cout
        << "FastIndex CLI\n\n"
        << "Usage:\n"
        << "  fastindex_cli index <directory> <database>\n"
        << "  fastindex_cli search <database> <query>\n";
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage();
        return 1;
    }

    const std::string_view command = argv[1];

    try {
        if (command == "index") {
            const fs::path directory = argv[2];
            const fs::path databasePath = argv[3];

            SqliteIndex database(databasePath);
            IndexingService service(database);

            std::cout << "Indexing started...\n";

            const auto stats =
                service.indexDirectory(directory);

            std::cout
                << "\nIndexing completed!\n"
                << "Discovered: " << stats.discovered << '\n'
                << "Indexed:    " << stats.indexed << '\n'
                << "Skipped:    " << stats.skipped << '\n';

            for (const auto& error : stats.errors) {
                std::cerr
                    << "Skipped: " << error.path.string()
                    << "\nReason: " << error.message << '\n';
            }

            return stats.skipped == 0 ? 0 : 2;
        }

        if (command == "search") {
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

        printUsage();
        return 1;
    }
    catch (const std::exception& error) {
        std::cerr
            << "FastIndex error: " << error.what()
            << '\n';

        return 1;
    }
}
