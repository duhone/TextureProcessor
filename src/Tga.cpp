#include "Tga.h"

#include "Platform/MemoryMappedFile.h"
#include "core/Log.h"

using namespace std;
using namespace CR;
using namespace CR::Core;

#pragma pack(1)
struct TgaHeader {
	uint8_t IDLength;        // size of optional data after the header
	uint8_t ColorMapType;    // if image has pallette, I require 0
	uint8_t ImageType;       // i only support 2 and 10
	uint16_t CMapStart;      // i don't use, must be 0
	uint16_t CMapLength;     // i don't use, must be 0
	uint8_t CMapDepth;       // ignore
	uint16_t XOffset;        // must be 0
	uint16_t YOffset;        // must be 0
	uint16_t Width;
	uint16_t Height;
	uint8_t PixelDepth;         // must be 24 or 32(with alpha)
	uint8_t ImageDescriptor;    // ignore
};
#pragma pack()

Image ReadImage(const std::filesystem::path& a_path) {
	auto file = Platform::OpenMMapFile(a_path);

	if(file->size() < sizeof(TgaHeader)) { Log::Fail("image {} was corrupt", a_path.string()); }
	auto fileData     = file->data();
	TgaHeader* header = (TgaHeader*)fileData;
	fileData += sizeof(TgaHeader);

	if(header->ColorMapType != 0) {
		Log::Fail("image {} has a pallette, only truecolor images are supported", a_path.string());
	}
	if(!(header->ImageType == 2 || header->ImageType == 10)) {
		Log::Fail("image {} has an usupported image type, only truecolor images are supported", a_path.string());
	}
	if(!(header->PixelDepth == 24 || header->PixelDepth == 32)) {
		Log::Fail("image {} has an usupported pixel format, only 24 bit rgb and 32 bit argb are supported",
		          a_path.string());
	}
	if(!(header->YOffset == 0 || header->YOffset == header->Height)) {
		Log::Fail("image {} has an usupported yoffset, origin must be top left or bottom left", a_path.string());
	}
	fileData += header->IDLength;

	Image result;
	result.Width    = header->Width;
	result.Height   = header->Height;
	result.HasAlpha = header->PixelDepth == 32 ? true : false;

	// raw image
	if(header->ImageType == 2) {
		uint32_t stride = result.Width * (header->PixelDepth == 32 ? 4 : 3);
		int32_t yStart, yEnd, yStep;
		if(header->YOffset == 0) {
			yStart = header->Height - 1;
			yEnd   = -1;
			yStep  = -1;
		} else {
			yStart = 0;
			yEnd   = header->Height;
			yStep  = 1;
		}
		size_t remainingData = file->size() - sizeof(header) - header->IDLength;
		size_t imageData     = result.Width * result.Height * (header->PixelDepth == 32 ? 4 : 3);
		Log::Assert(imageData <= remainingData, "Image {} corrupt, missing data");
		for(int32_t y = yStart; y != yEnd; y += yStep) {
			result.Data.insert(end(result.Data), (uint8_t*)fileData + y * stride,
			                   (uint8_t*)fileData + (y + 1) * stride);
		}
	}

	// rle encoded image
	if(header->ImageType == 10) {
		size_t remainingData   = file->size() - sizeof(header) - header->IDLength;
		byte* fileEnd          = fileData + remainingData;
		uint32_t bytesPerPixel = header->PixelDepth / 8;
		size_t imageData       = result.Width * result.Height * bytesPerPixel;

		auto readByte = [&]() {
			Log::Assert(fileData < fileEnd, "Image {} corrupt, missing data");
			uint8_t result = *(uint8_t*)fileData;
			++fileData;
			return result;
		};

		result.Data.reserve(imageData);
		uint8_t alpha, red, green, blue;
		uint8_t runLength;
		while(imageData > 0) {
			runLength = readByte();
			if(runLength & 0x80) {
				// rle data
				runLength &= 0x7f;
				++runLength;
				if(header->PixelDepth == 32) { alpha = readByte(); }
				red   = readByte();
				green = readByte();
				blue  = readByte();
				imageData -= runLength * bytesPerPixel;
				for(uint32_t i = 0; i < runLength; ++i) {
					if(header->PixelDepth == 32) { result.Data.push_back(alpha); }
					result.Data.push_back(red);
					result.Data.push_back(green);
					result.Data.push_back(blue);
				}
			} else {
				// raw data
				++runLength;
				imageData -= runLength * bytesPerPixel;
				for(uint32_t i = 0; i < runLength; ++i) {
					if(header->PixelDepth == 32) { result.Data.push_back(readByte()); }
					result.Data.push_back(readByte());
					result.Data.push_back(readByte());
					result.Data.push_back(readByte());
				}
			}
		}

		// flip
		if(header->YOffset == 0) {
			uint32_t stride = result.Width * (header->PixelDepth == 32 ? 4 : 3);
			auto src        = move(result.Data);
			for(uint32_t y = 0; y < header->Height; ++y) {
				result.Data.insert(end(result.Data), begin(src) + (header->Height - y - 1) * stride,
				                   begin(src) + (header->Height - y) * stride);
			}
		}
	}

	return result;
}
