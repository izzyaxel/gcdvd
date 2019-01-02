#pragma once

#include <memory>
#include <cstdint>
#include <array>
#include <vector>

static constexpr uint32_t fullDiscSizeBytes = 1459978240;
static constexpr uint32_t numSectors = 712880;
static constexpr uint32_t sectorSizeBytes = 2048; //Files must be aligned to the sectors or bad things will happen
static constexpr uint32_t fstEntrySize = 0x000c;

struct DVDHeader
{
	uint8_t consoleID = 0, countryCode = 0;
	std::array<uint8_t, 2> gameCode{0};
	short makerCode = 0;
	uint8_t discID = 0, version = 0, audioStreaming = 0, streamBufferSize = 0;
	std::array<uint8_t, 0x0012> unused1{0};
	int32_t dvdMagicWord = 0;
	std::array<uint8_t, 0x03e0> gameName{0};
	int32_t dhBinOffset = 0, addrDebugMonitor = 0;
	std::array<uint8_t, 0x0018> unused2{0};
	int32_t bootfileOffset = 0, fstBinOffset = 0, fstSize = 0, maxFSTSize = 0, userPosition = 0, userLength = 0, unknown = 0, unused3 = 0;
};

struct DVDHeaderInformation
{
	int32_t debugMonitorSize = 0, simulatedMemorySize = 0, argumentOffset = 0, debugFlag = 0, trackLocation = 0, trackSize = 0, countryCode = 0, unknown = 0;
};

struct Apploader
{
	std::array<int32_t, 4> versionPadded{0};
	int32_t entrypoint = 0, size = 0, trailerSize = 0;
};

/// Can be either a directory or a file
struct FSTEntry
{
	uint8_t flags = 0; //0: file 1: directory
	std::array<uint8_t, 3> filenameAndOffset{0};
	int32_t fileOffsetOrParentOffset = 0, fileLengthOrNumEntriesOrNextOffset = 0; //file length for files, num entries for root, next offset for directories
};

/// File system tree, used to locate files in the binary blob
struct FST
{
	FSTEntry rootDirectoryEntry{};
	std::vector<FSTEntry> entries{};
	std::vector<std::string> stringTable{};
};

/// Call create with the path to the ISO file, then check DVDStream::initialized
struct DVDStream
{
	/// Opens a stream for the given ISO and parses its header
	/// The binary blob is not read into RAM, use readFile to stream data
	explicit DVDStream(std::string const &isoPath);
	
	/// Closes the stream
	~DVDStream();
	
	/// Create a new DVDStream from the given ISO file
	static std::unique_ptr<DVDStream> create(std::string const &isoPath);
	
	/// Read the given file from the binary blob
	inline std::vector<uint8_t>&& readFile(FSTEntry const &file);
	
	/// Read a file by name from the binary blob
	inline std::vector<uint8_t>&& readFile(std::string const &fileName);
	
	DVDHeader dvdHeader{};
	DVDHeaderInformation dvdHeaderInformation{};
	Apploader apploader{};
	std::vector<uint8_t> apploaderCode{};
	FST fst{};
	bool initialized = false;

private:
	FILE *isoStream = nullptr;
};
