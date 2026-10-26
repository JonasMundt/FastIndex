
#pragma once

#include <filesystem>
#include <vector>

namespace fastindex::core {

class FileScanner {
public:
    [[nodiscard]]
    std::vector<std::filesystem::path> scan(
        const std::filesystem::path& directory
    ) const;
};

} // namespace fastindex::core
