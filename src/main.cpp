#include "Tga.h"

#include "cli11/cli11.hpp"
#include "fmt/format.h"

#include "DataCompression/LosslessCompression.h"
#include "Platform/MemoryMappedFile.h"
#include "Platform/PathUtils.h"
#include "Platform/Process.h"
#include "core/Log.h"

#include "AMDCompress.h"

#include <chrono>
#include <cstdio>

using namespace std;
using namespace std::chrono_literals;
namespace fs = std::filesystem;
using namespace CR;
using namespace CR::Core;

vector<byte> CompressTexture(const Image& a_image, bool a_fast) {
	AMD_TC_InitializeBCLibrary();

	AMD_TC_Texture source;
	source.dwSize   = sizeof(AMD_TC_Texture);
	source.dwWidth  = a_image.Width;
	source.dwHeight = a_image.Height;
	source.dwPitch  = a_image.Width * (a_image.HasAlpha ? 4 : 3);
	source.format   = a_image.HasAlpha ? AMD_TC_FORMAT::AMD_TC_FORMAT_ARGB_8888 : AMD_TC_FORMAT::AMD_TC_FORMAT_RGB_888;
	source.dwDataSize = (AMD_TC_DWORD)a_image.Data.size();
	source.pData      = (AMD_TC_BYTE*)a_image.Data.data();

	AMD_TC_Texture dest;
	dest.dwSize   = sizeof(AMD_TC_Texture);
	dest.dwWidth  = a_image.Width;
	dest.dwHeight = a_image.Height;
	dest.dwPitch  = 0;    // unused
	dest.format   = AMD_TC_FORMAT::AMD_TC_FORMAT_BC7;

	vector<byte> result;
	dest.dwDataSize = AMD_TC_CalculateBufferSize(&dest);
	result.resize(dest.dwDataSize);
	dest.pData = (AMD_TC_BYTE*)result.data();

	auto progress = [](float a_prog, DWORD_PTR, DWORD_PTR) {
		Log::Info("{}", a_prog);
		return false;
	};

	AMD_TC_CompressOptions options;
	options.bDisableMultiThreading = false;
	options.brestrictColour        = true;
	options.brestrictAlpha         = true;
	options.bUseAdaptiveWeighting  = false;
	options.bUseChannelWeighting   = false;
	options.NumCmds                = 0;
	options.dwmodeMask             = 0xCF;
	options.nCompressionSpeed      = AMD_TC_Speed_Normal;
	options.dwnumThreads           = 9;
	options.fquality               = a_fast ? 0.05f : 0.8f;

	AMD_TC_ConvertTexture(&source, &dest, &options, progress, 0, 0);

	AMD_TC_ShutdownBCLibrary();

	return result;
}

vector<byte> BuildCRTexd(const Image& a_image, const vector<byte>& a_data, bool a_fast) {
	vector<byte> uncompressed;

#pragma pack(4)
	struct Header {
		uint16_t Width{0};
		uint16_t Height{0};
	};
#pragma pack()

	Header header;
	header.Width  = a_image.Width;
	header.Height = a_image.Height;

	uncompressed.resize(sizeof(header) + a_data.size());

	copy((byte*)&header, (byte*)&header + sizeof(header), begin(uncompressed));
	copy(a_data.data(), a_data.data() + a_data.size(), begin(uncompressed) + sizeof(header));

	return DataCompression::Compress(data(uncompressed), (uint32_t)size(uncompressed), a_fast ? 3 : 18);
}

int main(int argc, char** argv) {
	CLI::App app{"TextureProcessor"};
	string inputFileName  = "";
	string outputFileName = "";
	bool fast             = false;
	app.add_option("-i,--input", inputFileName,
	               "Input TGA texture. filename only, leave off extension, actual file must have .tga extension")
	    ->required();
	app.add_option("-o,--output", outputFileName, "Output file. filename only, leave off extension")->required();
	app.add_flag("-f,--fast", fast, "use faster, but lower quality compression");

	CLI11_PARSE(app, argc, argv);

	fs::path inputPath{inputFileName};
	fs::path outputPath{outputFileName};

	if(inputPath.has_extension()) {
		CLI::Error error{"extension", "don't pass input file extension", CLI::ExitCodes::FileError};
		app.exit(error);
	}
	if(outputPath.has_extension()) {
		CLI::Error error{"extension", "do not add an extension to the output file name", CLI::ExitCodes::FileError};
		app.exit(error);
	}

	inputPath = Platform::GetCurrentProcessPath() / inputPath;
	inputPath.replace_extension("tga");
	outputPath = Platform::GetCurrentProcessPath() / outputPath;
	outputPath.replace_extension("crtexd");

	Image inputImage = ReadImage(inputPath);

	auto compTex = CompressTexture(inputImage, fast);
	auto crtexd  = BuildCRTexd(inputImage, compTex, fast);

	FILE* outputFile = nullptr;
	fopen_s(&outputFile, outputPath.string().c_str(), "wb");
	fwrite(crtexd.data(), sizeof(byte), crtexd.size(), outputFile);
	fclose(outputFile);

	return 0;
}
