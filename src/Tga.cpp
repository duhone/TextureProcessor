#include "Tga.h"

#include "Platform/MemoryMappedFile.h"
#include "core/Log.h"

#include <3rdParty/glm.h>

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

Image ReadImage(const std::filesystem::path& a_path, bool a_premultiplyAlpha) {
	Platform::MemoryMappedFile file(a_path);

	Log::Require(file.size() >= sizeof(TgaHeader), "image {} was corrupt", a_path.string());
	auto fileData     = file.data();
	TgaHeader* header = (TgaHeader*)fileData;
	fileData += sizeof(TgaHeader);

	Log::Require(header->ColorMapType == 0, "image {} has a pallette, only truecolor images are supported",
	             a_path.string());
	Log::Require(header->ImageType == 2 || header->ImageType == 10,
	             "image {} has an usupported image type, only truecolor images are supported", a_path.string());
	Log::Require(header->PixelDepth == 24 || header->PixelDepth == 32,
	             "image {} has an usupported pixel format, only 24 bit rgb and 32 bit argb are supported",
	             a_path.string());
	Log::Require(header->YOffset == 0 || header->YOffset == header->Height,
	             "image {} has an usupported yoffset, origin must be top left or bottom left", a_path.string());

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
		size_t remainingData = file.size() - sizeof(header) - header->IDLength;
		size_t imageData     = result.Width * result.Height * (header->PixelDepth == 32 ? 4 : 3);
		Log::Assert(imageData <= remainingData, "Image {} corrupt, missing data");
		result.Data.prepare(imageData);
		int32_t insert = 0;
		for(int32_t y = yStart; y != yEnd; y += yStep) {
			memcpy(result.Data.data() + insert, fileData + y * stride, stride);
			insert += stride;
		}
		result.Data.commit(imageData);
	}

	// rle encoded image
	if(header->ImageType == 10) {
		size_t remainingData   = file.size() - sizeof(header) - header->IDLength;
		byte* fileEnd          = fileData + remainingData;
		uint32_t bytesPerPixel = header->PixelDepth / 8;
		size_t imageData       = result.Width * result.Height * bytesPerPixel;

		auto readByte = [&]() {
			Log::Assert(fileData < fileEnd, "Image {} corrupt, missing data");
			byte result = *(byte*)fileData;
			++fileData;
			return result;
		};

		size_t outputsize = imageData;
		result.Data.prepare(outputsize);
		byte alpha = (byte)0;
		byte red   = (byte)0;
		byte green = (byte)0;
		byte blue  = (byte)0;
		uint8_t runLength;
		int32_t insertLoc = 0;
		while(imageData > 0) {
			runLength = (uint8_t)readByte();
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
					if(header->PixelDepth == 32) { result.Data[insertLoc++] = alpha; }
					result.Data[insertLoc++] = red;
					result.Data[insertLoc++] = green;
					result.Data[insertLoc++] = blue;
				}
			} else {
				// raw data
				++runLength;
				imageData -= runLength * bytesPerPixel;
				for(uint32_t i = 0; i < runLength; ++i) {
					if(header->PixelDepth == 32) { result.Data[insertLoc++] = readByte(); }
					result.Data[insertLoc++] = readByte();
					result.Data[insertLoc++] = readByte();
					result.Data[insertLoc++] = readByte();
				}
			}
		}
		result.Data.commit(outputsize);

		// flip, slow doing it this way instead of inline above
		if(header->YOffset == 0) {
			uint32_t stride = result.Width * (header->PixelDepth == 32 ? 4 : 3);
			auto src        = move(result.Data);
			result.Data.prepare(src.size());
			int32_t insert = 0;
			for(uint32_t y = 0; y < header->Height; ++y) {
				memcpy(result.Data.data() + insert, begin(src) + (header->Height - y - 1) * stride, stride);
				insert += stride;
			}
			result.Data.commit(src.size());
		}
	}

	// Fix BGRA instead of RGBA issue, slow
	for(uint32_t i = 0; i < result.Data.size(); i += (result.HasAlpha ? 4 : 3)) {
		byte temp          = result.Data[i + 0];
		result.Data[i + 0] = result.Data[i + 2];
		result.Data[i + 2] = temp;
	}

	// Using premultiplied alpha, slow
	if(result.HasAlpha && a_premultiplyAlpha) {
		for(uint32_t i = 0; i < result.Data.size(); i += 4) {
			float alpha = (float)result.Data[i + 3] / 255.0f;
			glm::vec3 color;
			color.r = (float)result.Data[i + 0] / 255.0f;
			color.g = (float)result.Data[i + 1] / 255.0f;
			color.b = (float)result.Data[i + 2] / 255.0f;

			color = glm::convertSRGBToLinear(color);
			color *= alpha;
			color = glm::convertLinearToSRGB(color);

			result.Data[i + 0] = (byte)((uint8_t)round(color.r * 255.0f));
			result.Data[i + 1] = (byte)((uint8_t)round(color.g * 255.0f));
			result.Data[i + 2] = (byte)((uint8_t)round(color.b * 255.0f));
		}
	}

	return result;
}
