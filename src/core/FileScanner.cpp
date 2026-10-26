
#include "fastindex/core/FileScanner.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace fastindex::core {

std::vector<std::filesystem::path> FileScanner::scan(
    const std::filesystem::path& directory
) const {
    namespace fs = std::filesystem;

    if (!fs::is_directory(directory)) {
        throw std::invalid_argument(
            "Scan path is not a directory: " + directory.string()
        );
    }

    std::vector<fs::path> files;

    for (const auto& entry : fs::recursive_directory_iterator(
             directory,
             fs::directory_options::skip_permission_denied
         )) {

        if (!entry.is_regular_file()) {
            continue;
        }

        std::string extension =
            entry.path().extension().string();

        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            }
        );

        if (extension == ".txt" ||
            extension == ".md"  ||
            extension == ".csv" ||
            extension == ".log") {

            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    return files;
}

} // namespace fastindex::core
