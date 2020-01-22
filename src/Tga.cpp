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
	fileData += header->IDLength;

	Image result;
	result.Width    = header->Width;
	result.Height   = header->Height;
	result.HasAlpha = header->PixelDepth == 32 ? true : false;

	// raw image
	if(header->ImageType == 2) {
		size_t remainingData = file->size() - sizeof(header) - header->IDLength;
		size_t imageData     = result.Width * result.Height * (header->PixelDepth == 32 ? 4 : 3);
		Log::Assert(imageData <= remainingData, "Image {} corrupt, missing data");
		result.Data.insert(begin(result.Data), fileData, fileData + imageData);
	}

	return result;
}
