#include "include/gcdvd/navigator.hh"

Navigator::Navigator(std::string const &path)
{
	this->path = path;
}

void Navigator::set(std::string const &newPath)
{
	this->path = newPath;
}

void Navigator::go(std::string const &folderName)
{
	if(this->path[this->path.size() - 1] != '/') this->path += "/";
	if(folderName[folderName.size() - 1] != '/') this->path += folderName + "/";
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
