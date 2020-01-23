#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

struct Image {
	uint32_t Width;
	uint32_t Height;
	bool HasAlpha;    // Image is always either RGB or ARGB
	std::vector<uint8_t> Data;
};

Image ReadImage(const std::filesystem::path& a_path);
