#include "include/gcdvd/dvd.hh"

#include <dir.h> //TODO mingw specific, mingw doesnt have filesystem implemented correctly yet :(
#include <algorithm>

Navigator::Navigator(std::string const &path)
{
	this->path = path;
}

void Navigator::set(std::string const &newPath)
{
	this->path = newPath;
}

void Navigator::go(char const *folderName, size_t len)
{
	if(this->path[this->path.size() - 1] != '/') this->path += '/';
	if(folderName[len - 1] != '/')
	{
		this->path += folderName;
		this->path += '/';
	}
	else this->path += folderName;
}

void Navigator::go(std::string &folderName)
{
	if(this->path[this->path.size() - 1] != '/') this->path += '/';
	if(folderName[folderName.size() - 1] != '/') this->path += folderName + '/';
	else this->path += folderName;
}

void Navigator::back()
{
	this->path = this->path.substr(0, this->path.find_last_of('/'));
}

void Navigator::backto(std::string const &folderName)
{
	if(folderName[folderName.size() - 1] != '/') this->path = this->path.substr(0, this->path.find_last_of(folderName + '/') + (folderName.size() + 1));
	else this->path = this->path.substr(0, this->path.find_last_of(folderName) + (folderName.size()));
}

std::string Navigator::get()
{
	if(this->path[this->path.size() - 1] != '/') this->path += "/";
	return this->path;
}

Navigator navigator{""};

DVDStream::DVDStream(std::string const &isoPathIn)
{
	this->isoStreamIn = fopen(isoPathIn.data(), "rb");
	if(!this->isoStreamIn) return;
	
	fread(&this->header, headerSize, 1, this->isoStreamIn);
	flipEndianness(this->header);
	
	fread(&this->headerInfo, headerInformationSize, 1, this->isoStreamIn);
	flipEndianness(this->headerInfo);
	
	fread(&this->apploader, apploaderSize, 1, this->isoStreamIn);
	flipEndianness(this->apploader);
	
	this->apploaderCode.resize(static_cast<size_t>(this->apploader.size));
	fread(this->apploaderCode.data(), static_cast<long>(this->apploader.size), 1, this->isoStreamIn);
	long apploaderEnd = ftell(this->isoStreamIn);
	this->apploaderPadding.resize(static_cast<size_t>(this->header.fstOffset - apploaderEnd), 0);
	fseek(this->isoStreamIn, static_cast<long>(this->apploaderPadding.size()), SEEK_CUR);
	this->apploaderTrailer.resize(this->apploader.trailerSize);
	fread(this->apploaderTrailer.data(), this->apploader.trailerSize, 1, this->isoStreamIn);
	
	fseek(this->isoStreamIn, static_cast<long>(this->header.fstOffset), SEEK_SET);
	{
		FSTEntry root{};
		fread(&root.flag, 1, 1, this->isoStreamIn);
		fread(&root.filenameOffset, 3, 1, this->isoStreamIn);
		fread(&root.fileOffset, 4, 1, this->isoStreamIn);
		fread(&root.fileLength, 4, 1, this->isoStreamIn);
		flipEndianness(root);
		this->fst.entries.push_back(root);
	}
	
	for(uint32_t i = 0; i < this->fst.entries[0].numEntries - 1; i++)
	{
		FSTEntry entry{};
		fread(&entry.flag, 1, 1, this->isoStreamIn);
		fread(&entry.filenameOffset, 3, 1, this->isoStreamIn);
		fread(&entry.fileOffset, 4, 1, this->isoStreamIn);
		fread(&entry.fileLength, 4, 1, this->isoStreamIn);
		flipEndianness(entry);
		this->fst.entries.push_back(entry);
	}
	
	fseek(this->isoStreamIn, static_cast<long>(this->header.fstOffset + (this->fst.entries[0].numEntries * fstEntrySize)), SEEK_SET);
	size_t strTableLen = static_cast<size_t>(this->header.fstSize - (this->fst.entries[0].numEntries * fstEntrySize));
	this->fst.stringTable.resize(strTableLen);
	fread(this->fst.stringTable.data(), strTableLen, 1, this->isoStreamIn);
	for(auto &entry : this->fst.entries) entry.name = std::string{this->fst.stringTable.data() + ((entry.filenameOffset[0] << 16) | (entry.filenameOffset[1] << 8) | (entry.filenameOffset[2]))};
	this->fst.entries[0].name = "root";
	this->initialized = true;
}

DVDStream::~DVDStream()
{
	fclose(this->isoStreamIn);
}

std::unique_ptr<DVDStream> DVDStream::create(std::string const &isoPathIn)
{
	return std::make_unique<DVDStream>(isoPathIn);
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

/*void DVDStream::writeHeader()
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
	//for(auto entry : this->fst.entries) fwrite(reinterpret_cast<void *>(&entry), sizeof(FSTEntry), 1, this->isoStreamOut);
	
}*/

void DVDStream::dumpFiles(std::string const &outPath)
{
	navigator.set(outPath);
	navigator.go("sys", 3);
	mkdir(navigator.get().data());
	printf("[0%%] - New dir: %s\n", navigator.get().data());
	FILE *out;
	uint32_t filesCompleted = 0, total = this->fst.entries[0].numEntries + 4;
	{//boot.bin
		std::string path = navigator.get() + "boot.bin";
		out = fopen(path.data(), "wb");
		fwrite(reinterpret_cast<void *>(&this->header), headerSize, 1, out);
		fclose(out);
		filesCompleted++;
		printf("[%u%%] - ", static_cast<uint32_t>((static_cast<float>(filesCompleted) / static_cast<float>(total)) * 100));
		printf("%s\n", path.data());
	}
	
	{//bi2.bin
		std::string path = navigator.get() + "bi2.bin";
		out = fopen(path.data(), "wb");
		fwrite(reinterpret_cast<void *>(&this->headerInfo), headerInformationSize, 1, out);
		fclose(out);
		filesCompleted++;
		printf("[%u%%] - ", static_cast<uint32_t>((static_cast<float>(filesCompleted) / static_cast<float>(total)) * 100));
		printf("%s\n", path.data());
	}
	
	{//apploader.bin
		std::string path = navigator.get() + "apploader.img";
		out = fopen(path.data(), "wb");
		fwrite(reinterpret_cast<void *>(&this->apploader), apploaderSize, 1, out);
		fwrite(this->apploaderCode.data(), this->apploaderCode.size(), 1, out);
		fwrite(this->apploaderTrailer.data(), this->apploaderTrailer.size(), 1, out);
		fclose(out);
		filesCompleted++;
		printf("[%u%%] - ", static_cast<uint32_t>((static_cast<float>(filesCompleted) / static_cast<float>(total)) * 100));
		printf("%s\n", path.data());
	}
	
	{//fst.bin
		std::string path = navigator.get() + "fst.bin";
		out = fopen((navigator.get() + "fst.bin").data(), "wb");
		for(auto const &entry : this->fst.entries) fwrite(&entry.flag, 12, 1, out);
		fwrite(this->fst.stringTable.data(), this->fst.stringTable.size(), 1, out);
		fclose(out);
		filesCompleted++;
		printf("[%u%%] - ", static_cast<uint32_t>((static_cast<float>(filesCompleted) / static_cast<float>(total)) * 100));
		printf("%s\n", path.data());
	}
	
	{//main.dol
		std::string path = navigator.get() + "main.dol";
		out = fopen(path.data(), "wb");
		fseek(this->isoStreamIn, static_cast<long>(this->header.mainDOLOffset), SEEK_SET); //TODO move this to constructor/store main.dol data in DVDStream?
		//TODO parse DOL header to find out how big it is
		
		fclose(out);
		filesCompleted++;
		printf("[%u%%] - ", static_cast<uint32_t>((static_cast<float>(filesCompleted) / static_cast<float>(total)) * 100));
		printf("%s\n", path.data());
	}
	
	navigator.set(outPath);
	
	for(auto const &entry : this->fst.entries)
	{
		if(!entry.name.empty())
		{
			printf("[%u%%] - ", static_cast<uint32_t>((static_cast<float>(filesCompleted) / static_cast<float>(total)) * 100));
			if(entry.flag == 0)
			{
				std::vector<uint8_t> file = this->readFile(entry);
				if(!file.empty())
				{
					std::string path = navigator.get() + entry.name;
					printf("%s\n", path.data());
					out = fopen(path.data(), "wb");
					fwrite(file.data(), file.size(), 1, out);
					fclose(out);
				}
				else printf("\nWarning: attempted to write empty file to %s\n", (navigator.get() + entry.name).data());
			}
			else
			{
				navigator.set(outPath);
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
				for(auto &dir : newPath) navigator.go(dir);
				printf("New dir: %s\n", navigator.get().data());
				mkdir(navigator.get().data());
			}
			filesCompleted++;
		}
		else
		{
			if(entry.flag == 0) printf("\nWarning: empty filename was encountered\n");
			else printf("\nWarning: empty directory name was encountered\n");
		}
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
