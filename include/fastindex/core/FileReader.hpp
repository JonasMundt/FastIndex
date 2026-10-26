
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace fastindex::core {

class FileReader {
public:
    [[nodiscard]]
    std::string read(
        const std::filesystem::path& file,
        std::size_t maxBytes = 10 * 1024 * 1024
    ) const;
};

} // namespace fastindex::core
