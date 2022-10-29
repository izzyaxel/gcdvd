#include "dvd.hh"

#if defined(WINDOWS)
#include <codecvt>
#include <locale>
#include <libloaderapi.h>
#endif

std::string replaceAll(std::string const &input, char const &searchFor, char const &replaceWith)
{
	std::string out;
	for(auto &character : input)
	{
		if(character == searchFor) out.push_back(replaceWith);
		else out.push_back(character);
	}
	return out;
}

//TODO I don't have any way to check if the OSX path works
std::string getCWD()
{
	constexpr uint32_t const pathLen = 2048;
	#if defined(WINDOWS)
	wchar_t rawdir[pathLen];
	GetModuleFileNameW(nullptr, rawdir, pathLen);
	std::string exeDir = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(std::wstring(rawdir));
	exeDir = replaceAll(exeDir, '\\', '/');
	return exeDir.substr(0, exeDir.find_last_of('/') + 1);
	#elif defined(LINUX)
	char dir[pathLen];
	getcwd(dir, pathLen);
	return std::string{dir} + "/";
	#elif defined(OSX)
	uint32_t len = pathLen;
	char dir[pathLen];
	_NSGetExecutablePath(dir, &pathLen);
	return std::string{dir} + "/";
	#endif
}

int main()
{
	std::unique_ptr<DVDStream> dvd = DVDStream::create(getCWD() + "xxx");
	dvd->dumpFiles(getCWD() + "dump/");
	dvd.reset();
	return 0;
}
