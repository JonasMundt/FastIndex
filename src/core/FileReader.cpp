
#include "fastindex/core/FileReader.hpp"

#include <array>
#include <fstream>
#include <stdexcept>

namespace fastindex::core {

std::string FileReader::read(
    const std::filesystem::path& file,
    std::size_t maxBytes
) const {
    std::ifstream input(file, std::ios::binary);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Cannot open file: " + file.string()
        );
    }

    constexpr std::size_t bufferSize = 64 * 1024;
    std::array<char, bufferSize> buffer{};

    std::string content;

    while (true) {
        input.read(buffer.data(), buffer.size());

        const auto bytesRead = input.gcount();

        if (bytesRead > 0) {
            const auto count =
                static_cast<std::size_t>(bytesRead);

            if (count > maxBytes - content.size()) {
                throw std::length_error(
                    "File exceeds maximum allowed size: "
                    + file.string()
                );
            }

            content.append(buffer.data(), count);
        }

        if (input.eof()) {
            break;
        }

        if (!input) {
            throw std::runtime_error(
                "Error while reading file: " + file.string()
            );
        }
    }

    return content;
}

} // namespace fastindex::core
