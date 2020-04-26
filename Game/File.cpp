#include "pch.h"

#include "File.h"

#include <fstream>
#include <filesystem>
#include <fmt/format.h>
#include <stdexcept>

std::vector<u8> loadFile(const std::filesystem::path& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error(fmt::format("File {} not found", filename.generic_string()));
    }

    auto size = std::filesystem::file_size(filename);
    
    std::vector<u8> buf(size, 0);
    file.read(reinterpret_cast<char*>(buf.data()), size);

    return buf;
}
