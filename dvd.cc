#include "include/gcdvd/dvd.hh"

DVDStream::DVDStream(std::string const &isoPath)
{
	this->isoStream = fopen(isoPath.data(), "rb");
	if(!this->isoStream) return;
	
	fseek(this->isoStream, 0, SEEK_END);
	long end = ftell(this->isoStream);
	rewind(this->isoStream);
	
	uint8_t *headerRaw = new uint8_t[0x0440];
	fread(headerRaw, 0x0440, 1, this->isoStream);
	this->dvdHeader = *reinterpret_cast<DVDHeader *>(headerRaw);
	delete[] headerRaw;
	
	uint8_t *headerInfoRaw = new uint8_t[0x2000];
	fread(headerInfoRaw, 0x2000, 1, this->isoStream);
	this->dvdHeaderInformation = *reinterpret_cast<DVDHeaderInformation *>(headerInfoRaw);
	delete[] headerInfoRaw;
	
	uint8_t *apploaderRaw = new uint8_t[0x001b];
	fread(apploaderRaw, 0x001b, 1, this->isoStream);
	this->apploader = *reinterpret_cast<Apploader *>(apploaderRaw);
	delete[] apploaderRaw;
	this->apploaderCode.resize(static_cast<size_t>(this->apploader.size));
	fread(this->apploaderCode.data(), static_cast<size_t>(this->apploader.size), 1, this->isoStream);
	
	uint8_t *fstRootEntryRaw = new uint8_t[fstEntrySize];
	fread(fstRootEntryRaw, fstEntrySize, 1, this->isoStream);
	this->fst.rootDirectoryEntry = *reinterpret_cast<FSTEntry *>(fstRootEntryRaw);
	delete[] fstRootEntryRaw;
	//TODO parse fst root entry and read the rest of FST into its vector, read string table into its vector
	
	this->initialized = true;
}

DVDStream::~DVDStream()
{
	fclose(this->isoStream);
}

std::unique_ptr<DVDStream> DVDStream::create(std::string const &isoPath)
{
	return std::make_unique<DVDStream>(isoPath);
}

std::vector<uint8_t>&& DVDStream::readFile(FSTEntry const &file)
{
	std::vector<uint8_t> out;
	
	return std::move(out);
}

std::vector<uint8_t>&& DVDStream::readFile(std::string const &fileName)
{
	std::vector<uint8_t> out;
	
	return std::move(out);
}
