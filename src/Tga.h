#pragma once

#include "core/storage_buffer.h"

#include <cstdint>
#include <filesystem>
#include <vector>

struct Image {
	uint32_t Width;
	uint32_t Height;
	bool HasAlpha;    // Image is always either RGB or ARGB
	CR::Core::storage_buffer<std::byte> Data;
};

Image ReadImage(const std::filesystem::path& a_path, bool a_premultiplyAlpha);
