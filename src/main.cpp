#include "Tga.h"

#include "cli11/cli11.hpp"
#include "fmt/format.h"

#include "DataCompression/LosslessCompression.h"
#include "Platform/MemoryMappedFile.h"
#include "Platform/PathUtils.h"
#include "Platform/Process.h"
#include "core/Log.h"

#include <chrono>
#include <cstdio>

using namespace std;
using namespace std::chrono_literals;
namespace fs = std::filesystem;
using namespace CR;
using namespace CR::Core;

vector<byte> BuildCRSM(const unique_ptr<Platform::IMemoryMappedFile>& vertSpirv,
                       const unique_ptr<Platform::IMemoryMappedFile>& fragSpirv) {
	vector<byte> uncompressed;

	struct Header {
		uint32_t VertSize{0};
		uint32_t FragSize{0};
	};

	Header header;
	header.VertSize = (uint32_t)vertSpirv->size();
	header.FragSize = (uint32_t)fragSpirv->size();

	uncompressed.resize(sizeof(header) + header.VertSize + header.FragSize);

	copy((byte*)&header, (byte*)&header + sizeof(header), begin(uncompressed));
	copy(vertSpirv->data(), vertSpirv->data() + vertSpirv->size(), begin(uncompressed) + sizeof(header));
	copy(fragSpirv->data(), fragSpirv->data() + fragSpirv->size(),
	     begin(uncompressed) + sizeof(header) + vertSpirv->size());

	return DataCompression::Compress(data(uncompressed), (uint32_t)size(uncompressed), 18);
}

int main(int argc, char** argv) {
	CLI::App app{"TextureProcessor"};
	string inputFileName  = "";
	string outputFileName = "";
	app.add_option("-i,--input", inputFileName,
	               "Input TGA texture. filename only, leave off extension, actual file must have .tga extension")
	    ->required();
	app.add_option("-o,--output", outputFileName, "Output file. filename only, leave off extension")->required();

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

	inputPath  = Platform::GetCurrentProcessPath() / inputPath;
	outputPath = Platform::GetCurrentProcessPath() / outputPath;

	Image inputImage = ReadImage(inputPath);

	/*fs::path compiledVertPath = CompileShader(vertPath);
	auto vertSpirv = Platform::OpenMMapFile(compiledVertPath);
	vector<byte> crsm = BuildCRSM(vertSpirv, fragSpirv);

	vertSpirv.reset();
	fs::remove(compiledVertPath);

	FILE *outputFile = nullptr;
	fopen_s(&outputFile, outputPath.string().c_str(), "wb");
	fwrite(crsm.data(), sizeof(byte), crsm.size(), outputFile);
	fclose(outputFile);*/

	return 0;
}
