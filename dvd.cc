#include "include/gcdvd/dvd.hh"
#include "include/gcdvd/navigator.hh"

#include <dir.h>
#include <algorithm>

DVDStream::DVDStream(std::string const &isoPathIn, std::string const &isoPathOut)
{
	this->isoStreamIn = fopen(isoPathIn.data(), "rb");
	if(!this->isoStreamIn) return;
	this->isoStreamOut = fopen(isoPathOut.data(), "wb");
	if(!this->isoStreamOut) return;
	
	fread(&this->header, sizeof(Header), 1, this->isoStreamIn);
	flipEndianness(this->header);
	
	fread(&this->headerInfo, sizeof(HeaderInfo), 1, this->isoStreamIn);
	flipEndianness(this->headerInfo);
	
	fread(&this->apploader, sizeof(Apploader), 1, this->isoStreamIn);
	flipEndianness(this->apploader);
	
	this->apploaderCode.resize(static_cast<size_t>(this->apploader.size));
	fread(this->apploaderCode.data(), static_cast<long>(this->apploader.size), 1, this->isoStreamIn);
	long apploaderEnd = ftell(this->isoStreamIn);
	
	fseek(this->isoStreamIn, static_cast<long>(this->header.fstOffset), SEEK_SET);
	
	this->apploaderPadding.resize(static_cast<size_t>(this->header.fstOffset - apploaderEnd), 0);
	
	{
		long begin = ftell(this->isoStreamIn);
		FSTEntry root{};
		fread(&root.flag, 1, 1, this->isoStreamIn);
		fread(&root.filenameOffset, 3, 1, this->isoStreamIn);
		fread(&root.fileOffset, 4, 1, this->isoStreamIn);
		fread(&root.fileLength, 4, 1, this->isoStreamIn);
		flipEndianness(root);
		long end = ftell(this->isoStreamIn);
		uint32_t offset =  ((root.filenameOffset[0] << 16) | (root.filenameOffset[1] << 8) | (root.filenameOffset[2]));
		fseek(this->isoStreamIn, static_cast<long>(this->header.fstOffset + (root.numEntries * fstEntrySize) + offset), SEEK_SET);
		char c = ' ';
		while(true)
		{
			fread(&c, 1, 1, this->isoStreamIn);
			root.name += c;
			if(c == '\0') break;
		}
		root.name = root.name.substr(1, 4);
		fseek(this->isoStreamIn, end, SEEK_SET);
		this->fst.entries.push_back(root);
		this->fst.root = root;
	}
	
	for(uint32_t i = 0; i < this->fst.entries[0].numEntries; i++)
	{
		long begin = ftell(this->isoStreamIn);
		FSTEntry entry{};
		fread(&entry.flag, 1, 1, this->isoStreamIn);
		fread(&entry.filenameOffset, 3, 1, this->isoStreamIn);
		fread(&entry.fileOffset, 4, 1, this->isoStreamIn);
		fread(&entry.fileLength, 4, 1, this->isoStreamIn);
		flipEndianness(entry);
		
		long end = ftell(this->isoStreamIn);
		uint32_t offset =  ((entry.filenameOffset[0] << 16) |
							(entry.filenameOffset[1] << 8) |
							(entry.filenameOffset[2]));
		fseek(this->isoStreamIn, static_cast<long>(this->header.fstOffset + (this->fst.entries[0].numEntries * fstEntrySize) + offset), SEEK_SET);
		char c = ' ';
		while(true)
		{
			fread(&c, 1, 1, this->isoStreamIn);
			entry.name += c;
			if(c == '\0') break;
		}
		fseek(this->isoStreamIn, end, SEEK_SET);
		this->fst.entries.push_back(entry);
	}
	this->initialized = true;
}

DVDStream::~DVDStream()
{
	fclose(this->isoStreamIn);
	fclose(this->isoStreamOut);
}

std::unique_ptr<DVDStream> DVDStream::create(std::string const &isoPathIn, std::string const &isoPathOut)
{
	return std::make_unique<DVDStream>(isoPathIn, isoPathOut);
}

std::vector<uint8_t> DVDStream::readFile(FSTEntry const &entry)
{
	std::vector<uint8_t> out;
	if(entry.flag == 1) return out;
	out.resize(entry.fileLength);
	fseek(this->isoStreamIn, static_cast<long>(entry.fileOffset), SEEK_SET);
	fread(out.data(), entry.fileLength, 1, this->isoStreamIn);
	return out;
}

std::vector<uint8_t> DVDStream::readFile(std::string const &fileName)
{
	std::vector<uint8_t> out;
	for(auto const &entry : this->fst.entries)
	{
		if(entry.flag == 0 && entry.name == fileName)
		{
			out.resize(entry.fileLength);
			fseek(this->isoStreamIn, static_cast<long>(entry.fileOffset), SEEK_SET);
			fread(out.data(), entry.fileLength, 1, this->isoStreamIn);
			return out;
		}
	}
	return out;
}

void DVDStream::writeHeader()
{
	rewind(this->isoStreamOut);
	fwrite(reinterpret_cast<void *>(&this->header), sizeof(Header), 1, this->isoStreamOut);
	fwrite(reinterpret_cast<void *>(&this->headerInfo), sizeof(HeaderInfo), 1, this->isoStreamOut);
	fwrite(reinterpret_cast<void *>(&this->apploader), sizeof(Apploader), 1, this->isoStreamOut);
	fwrite(this->apploaderCode.data(), this->apploaderCode.size(), 1, this->isoStreamOut);
}

void DVDStream::writeFST()
{
	fseek(this->isoStreamOut, static_cast<long>(this->header.fstOffset), SEEK_SET);
	fwrite(reinterpret_cast<void const *>(&this->fst.entries[0]), sizeof(FSTEntry), 1, this->isoStreamOut);
	//for(auto entry : this->fst.entries) fwrite(reinterpret_cast<void *>(&entry), sizeof(FSTEntry), 1, this->isoStreamOut); //TODO proper offsets
	
}

void DVDStream::dumpFiles(std::string const &outPath)
{
	Navigator navigator{outPath};
	uint32_t filesCompleted = 0, total = this->fst.root.numEntries;
	
	for(auto const &entry : this->fst.entries)
	{
		printf("[%u%%] - ", static_cast<uint32_t>((static_cast<float>(filesCompleted) / static_cast<float>(total)) * 100));
		
		if(entry.flag == 0)
		{
			auto file = this->readFile(entry);
			if(!file.empty())
			{
				std::string path = navigator.get() + entry.name;
				printf("%s\n", path.data());
				FILE *out = fopen(path.data(), "wb");
				fwrite(file.data(), file.size(), 1, out);
				fclose(out);
			}
		}
		else //TODO fix nested directories
		{
			std::vector<std::string> newPath;
			newPath.emplace_back(entry.name);
			uint32_t po = entry.parentOffset;
			do
			{
				FSTEntry pEntry = this->fst.entries[po];
				if(newPath.back() != pEntry.name) newPath.emplace_back(pEntry.name);
				po = pEntry.parentOffset;
			} while(po != 0);
			std::reverse(newPath.begin(), newPath.end());
			navigator.set(outPath);
			for(auto const &dir : newPath) navigator.go(dir);
			printf("New dir: %s\n", navigator.get().data());
			mkdir(navigator.get().data());
		}
		filesCompleted++;
	}
}

void flipEndianness(Header &header)
{
	header.magicWord = _byteswap_ulong(header.magicWord);
	header.dhBinOffset = _byteswap_ulong(header.dhBinOffset);
	header.addrDHBin = _byteswap_ulong(header.addrDHBin);
	header.mainDOLOffset = _byteswap_ulong(header.mainDOLOffset);
	header.fstOffset = _byteswap_ulong(header.fstOffset);
	header.fstSize = _byteswap_ulong(header.fstSize);
	header.maxFSTSize= _byteswap_ulong(header.maxFSTSize);
	header.userPos = _byteswap_ulong(header.userPos);
	header.userLen = _byteswap_ulong(header.userLen);
	header.unknown = _byteswap_ulong(header.unknown);
	header.padding3 = _byteswap_ulong(header.padding3);
}

void flipEndianness(HeaderInfo &headerInfo)
{
	headerInfo.dhBinSize = _byteswap_ulong(headerInfo.dhBinSize);
	headerInfo.simulatedMemorySize = _byteswap_ulong(headerInfo.simulatedMemorySize);
	headerInfo.argOffset = _byteswap_ulong(headerInfo.argOffset);
	headerInfo.debugFlag = _byteswap_ulong(headerInfo.debugFlag);
	headerInfo.trackLocation = _byteswap_ulong(headerInfo.trackLocation);
	headerInfo.trackSize = _byteswap_ulong(headerInfo.trackSize);
	headerInfo.countryCode = _byteswap_ulong(headerInfo.countryCode);
	headerInfo.unknown = _byteswap_ulong(headerInfo.unknown);
}

void flipEndianness(Apploader &apploader)
{
	apploader.entrypoint = _byteswap_ulong(apploader.entrypoint);
	apploader.size = _byteswap_ulong(apploader.size);
	apploader.trailerSize = _byteswap_ulong(apploader.trailerSize);
}

void flipEndianness(FSTEntry &fstEntry)
{
	fstEntry.fileOffset = _byteswap_ulong(fstEntry.fileOffset);
	fstEntry.fileLength = _byteswap_ulong(fstEntry.fileLength);
}

void flipEndianness(FST &fst)
{
	for(auto &entry : fst.entries) flipEndianness(entry);
}
