#include "Tga.h"

#include "Core/BinaryStream.h"
#include "DataCompression/LosslessCompression.h"
#include "Platform/MemoryMappedFile.h"
#include "Platform/PathUtils.h"
#include "Platform/Process.h"
#include "core/BinaryStream.h"
#include "core/Log.h"

#include <3rdParty/amdcompress.h>
#include <3rdParty/cli11.h>
#include <3rdParty/fmt.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <3rdParty/doctest.h>

#include <chrono>
#include <cstdio>
#include <regex>

using namespace std;
using namespace std::chrono_literals;
namespace fs = std::filesystem;
using namespace CR;
using namespace CR::Core;

storage_buffer<byte> CompressTexture(const Image& a_image, bool a_fast) {
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

	storage_buffer<byte> result;
	dest.dwDataSize = AMD_TC_CalculateBufferSize(&dest);
	result.prepare(dest.dwDataSize);
	result.commit(dest.dwDataSize);
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
	options.dwnumThreads           = 8;
	options.fquality               = a_fast ? 0.05f : 0.8f;

	AMD_TC_ConvertTexture(&source, &dest, &options, progress, 0, 0);

	AMD_TC_ShutdownBCLibrary();

	return result;
}

// a_data not const, so we can free once not needed. keep memory usage under control
void WriteCRTexd(const fs::path a_outputPath, const Image& a_image, vector<storage_buffer<byte>>& a_data, bool a_fast) {
#pragma pack(1)
	struct Header {
		uint32_t FourCC{'CRTX'};
		uint16_t Version{1};
		uint16_t Width{0};
		uint16_t Height{0};
		uint16_t Frames{0};
	};
#pragma pack()
	vector<storage_buffer<byte>> compressedTextures;
	for(auto& frameData : a_data) {
		compressedTextures.push_back(
		    DataCompression::Compress(Span<const byte>(data(frameData), (uint32_t)size(frameData)), a_fast ? 3 : 18));
		frameData.clear();
		frameData.shrink_to_fit();
	}

	fs::path outputFolder = a_outputPath;
	outputFolder.remove_filename();
	fs::create_directories(outputFolder);

	Core::FileHandle file{a_outputPath};

	Header header;
	header.Width  = (uint16_t)a_image.Width;
	header.Height = (uint16_t)a_image.Height;
	header.Frames = (uint16_t)a_data.size();
	Write(file, header);
	for(auto& texture : compressedTextures) {
		Write(file, texture);
		texture.clear();
		texture.shrink_to_fit();
	}
}

int main(int argc, char** argv) {
	CLI::App app{"TextureProcessor"};
	string inputFileName  = "";
	string outputFileName = "";
	bool fast             = false;
	bool premultiplyAlpha = false;
	app.add_option("-i,--input", inputFileName,
	               "Input TGA texture. filename only, leave off extension, actual file must have .tga extension. Leave "
	               "off _xxx for texture arrays")
	    ->required();
	app.add_option("-o,--output", outputFileName, "Output file. filename only, leave off extension")->required();
	app.add_flag("-f,--fast", fast, "use faster, but lower quality compression");
	app.add_flag("-p,--prealp", premultiplyAlpha, "premultiply alpha, should do this by default");

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

	filesystem::current_path(Platform::GetCurrentProcessPath());
	outputPath.replace_extension("crtexd");

	struct InputFile {
		fs::path Path;
		uint32_t Frame{0};
	};
	std::vector<InputFile> inputFiles;
	{
		fs::path searchPath = inputPath;
		searchPath.remove_filename();
		std::string filename = inputPath.filename().string();
		for(auto& pathIter : fs::directory_iterator{searchPath}) {
			auto& path = pathIter.path();

			if(path.filename().string()._Starts_with(filename) && path.has_extension() &&
			   path.extension().string() == ".tga") {
				string frameString = path.filename().string().substr(filename.size() + 1);
				uint32_t frame     = 0;
				if(frameString != "tga") { frame = std::stoul(frameString); }
				inputFiles.push_back({path, frame});
			}
		}

		sort(inputFiles.begin(), inputFiles.end(), [](auto& file1, auto& file2) { return file1.Frame < file2.Frame; });

		for(uint32_t i = 0; i < inputFiles.size(); ++i) {
			if(inputFiles[i].Frame != i) {
				CLI::Error error{"invalidarg", "Input files were missing frames, or did not start on frame 0",
				                 CLI::ExitCodes::FileError};
				app.exit(error);
			}
		}
	}

	bool needsUpdating = false;
	for(const auto& inputFile : inputFiles) {
		if(fs::last_write_time(outputPath) <= fs::last_write_time(inputFile.Path)) { needsUpdating = true; }
	}
	if(!needsUpdating) {
		return 0;    // nothing to do
	}

	if(inputFiles.size() > 256) {
		CLI::Error error{"invalidarg", "Too many frames of animation, 256 is the maximum", CLI::ExitCodes::FileError};
		app.exit(error);
	}

	vector<Image> images;
	for(auto& path : inputFiles) { images.push_back(ReadImage(path.Path, premultiplyAlpha)); }

	for(uint32_t i = 1; i < images.size(); ++i) {
		if(images[i].Height != images[0].Height || images[i].Width != images[0].Width) {
			CLI::Error error{
			    "invalidarg",
			    "Input files were not all the same resolution, every frame in an animation must the same size",
			    CLI::ExitCodes::FileError};
			app.exit(error);
		}
	}

	vector<storage_buffer<byte>> compressedTextures;
	for(const auto& image : images) { compressedTextures.push_back(CompressTexture(image, fast)); }

	WriteCRTexd(outputPath, images[0], compressedTextures, fast);

	return 0;
}
