#pragma once

#include <memory>
#include <cstdint>
#include <array>
#include <vector>

// name                                                         size (bytes)

//-----------------------Table of Contents----------------------//
// boot.bin (header)                                            1088
// bi2.bin (header info)                                        32
// appldr.bin (apploader)                                       28 + code size
// fst.bin (file symbol tree)                                   varies
// binary data                                                  varies

//-------------------------Disc Header--------------------------//
// Console ID                                                   1
// Game code                                                    2
// Country code                                                 1
// Maker code                                                   2
// Disc ID                                                      1
// Version                                                      1
// Audio streaming flag                                         1
// Stream buffer size                                           1
// Padding                                                      18
// Magic word                                                   4
// Game name                                                    992
// dh.bin offset                                                4
// Address to load dh.bin                                       4
// Padding                                                      24
// main.dol offset                                              4
// fst.bin offset                                               4
// FST size                                                     4
// Max FST size                                                 4
// User position                                                4
// User length                                                  4
// Unknown                                                      4
// Padding                                                      4

//------------------Disc Header Information---------------------//
// dh.bin size                                                  4
// Simulated memory size                                        4
// Argument offset                                              4
// Debug flag                                                   4
// Track location                                               4
// Track size                                                   4
// Country code                                                 4
// Unknown                                                      4
// Padding                                                      8160

//--------------------------Apploader---------------------------//
// Date of apploader                                            10
// Padding                                                      6
// Apploader entry point                                        4
// Apploader code size                                          4
// Trailer size                                                 4
// Apploader code                                               varies
// Padding                                                      varies

//-----------------------------FST------------------------------//
// Root directory entry                                         12
// List of FST entries                                          12 * num entries
// String table                                                 varies

//-------------------------FST Entry----------------------------//
// Flag 0: file 1: dir                                          1
// Filename offset in str table (<< into lower 3 bytes of u32)  3
// File offset or parent offset                                 4
// File length(file) or num entries(root) or next offset(dir)   4

static constexpr uint32_t fullDiscSizeBytes = 1459978240;
static constexpr uint32_t numSectors = 712880;
static constexpr uint32_t sectorSizeBytes = 2048; //Files must be aligned to the sectors or bad things will happen
static constexpr uint32_t headerSize = 1088;
static constexpr uint32_t headerInformationSize = 32;
static constexpr uint32_t apploaderSize = 28;
static constexpr uint32_t fstEntrySize = 12;

struct Header
{
	uint8_t consoleID = 0;
	std::array<uint8_t, 2> gameCode{0};
	uint8_t countryCode = 0;
	std::array<uint8_t, 2> makerCode = {0};
	uint8_t discID = 0, version = 0, audioStreaming = 0, streamBufferSize = 0;
	std::array<uint8_t, 18> padding1{0};
	uint32_t magicWord = 0;
	std::array<uint8_t, 992> gameName{0};
	uint32_t dhBinOffset = 0, addrDHBin = 0;
	std::array<uint8_t, 24> padding2{0};
	uint32_t mainDOLOffset = 0, fstOffset = 0, fstSize = 0, maxFSTSize = 0, userPos = 0, userLen = 0, unknown = 0, padding3 = 0;
};

struct HeaderInfo
{
	uint32_t dhBinSize = 0, simulatedMemorySize = 0, argOffset = 0, debugFlag = 0, trackLocation = 0, trackSize = 0, countryCode = 0, unknown = 0;
	std::array<uint8_t, 8160> padding = {0};
};

struct Apploader
{
	std::array<uint8_t, 16> versionPadded{0};
	uint32_t entrypoint = 0, size = 0, trailerSize = 0;
};

/// Can be either a directory or a file
struct FSTEntry
{
	uint8_t flag = 0;
	std::array<uint8_t, 3> filenameOffset{0};
	union
	{
		uint32_t fileOffset;
		uint32_t parentOffset;
	};
	
	union
	{
		uint32_t fileLength;
		uint32_t numEntries;
		uint32_t nextOffset;
	};
	std::string name = "";
};

/// File system tree, used to locate files in the binary blob
struct FST
{
	FSTEntry root;
	std::vector<FSTEntry> entries{};
};

/// Call DVDStream::create() with the path to the ISO file, then check DVDStream::initialized
struct DVDStream
{
	/// Opens a stream for the given ISO and parses its header
	/// The binary blob is not read into RAM, use readFile to stream data
	explicit DVDStream(std::string const &isoPathIn, std::string const &isoPathOut);
	
	/// Closes the stream
	~DVDStream();
	
	/// Create a new DVDStream from the given ISO file
	static std::unique_ptr<DVDStream> create(std::string const &isoPathIn, std::string const &isoPathOut);
	
	/// Read the given file from the binary blob
	std::vector<uint8_t> readFile(FSTEntry const &entry);
	
	/// Read a file by name from the binary blob
	std::vector<uint8_t> readFile(std::string const &fileName);
	
	void writeHeader();
	
	void writeFST();
	
	void dumpFiles(std::string const &outPath);
	
	Header header{};
	HeaderInfo headerInfo{};
	Apploader apploader{};
	std::vector<uint8_t> apploaderCode{};
	std::vector<uint8_t> apploaderPadding{};
	FST fst{};
	bool initialized = false, lowMemoryMode = false;

private:
	FILE *isoStreamIn = nullptr;
	FILE *isoStreamOut = nullptr;
};

void flipEndianness(Header &header);
void flipEndianness(HeaderInfo &headerInfo);
void flipEndianness(Apploader &apploader);
void flipEndianness(FSTEntry &fstEntry);
void flipEndianness(FST &fst);
