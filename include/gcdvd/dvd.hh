#pragma once

#include <memory>
#include <cstdint>
#include <array>
#include <vector>

static constexpr uint32_t fullDiscSizeBytes = 1459978240;
static constexpr uint32_t numSectors = 712880;
static constexpr uint32_t sectorSizeBytes = 2048; //Files must be aligned to the sectors or bad things will happen
static constexpr uint32_t headerSize = 1088;
static constexpr uint32_t headerInformationSize = 32;
static constexpr uint32_t apploaderSize = 28;
static constexpr uint32_t fstEntrySize = 12;

struct Navigator
{
	explicit Navigator(std::string const &path);
	void set(std::string const &newPath);
	void go(std::string &folderName);
	void back();
	void backto(std::string const &folderName);
	std::string get();
private: std::string path;
};

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

/// File symbol tree, used to locate files in the binary blob
struct FST
{
	FSTEntry root; //convenience, this is still added to entries too
	std::vector<FSTEntry> entries{};
};

/// Call DVDStream::create() with the path to the ISO file, then check DVDStream::initialized
struct DVDStream
{
	/// Opens a stream for the given ISO and parses its header
	/// The binary blob is not read into RAM, use readFile to stream data
	explicit DVDStream(std::string const &isoPathIn);
	
	/// Closes the stream
	~DVDStream();
	
	/// Create a new DVDStream from the given ISO file
	static std::unique_ptr<DVDStream> create(std::string const &isoPathIn);
	
	/// Read the given file from the binary blob
	std::vector<uint8_t> readFile(FSTEntry const &entry);
	
	/// Read a file by name from the binary blob
	std::vector<uint8_t> readFile(std::string const &fileName);
	
	//TODO think about API for rebuilding/writing ISOs
	//void write(std::string const &isoPathOut);
	
	void dumpFiles(std::string const &outPath);
	
	Header header{};
	HeaderInfo headerInfo{};
	Apploader apploader{};
	std::vector<uint8_t> apploaderCode{};
	std::vector<uint8_t> apploaderPadding{};
	FST fst{};
	bool initialized = false;

private:
	FILE *isoStreamIn = nullptr;
};

void flipEndianness(Header &header);
void flipEndianness(HeaderInfo &headerInfo);
void flipEndianness(Apploader &apploader);
void flipEndianness(FSTEntry &fstEntry);
void flipEndianness(FST &fst);
