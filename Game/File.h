#pragma once

#include "Common.h"

#include <filesystem>
#include <vector>

std::vector<u8> loadFile(const std::filesystem::path& filename);
