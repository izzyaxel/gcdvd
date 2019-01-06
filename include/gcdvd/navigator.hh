#pragma once

#include <string>

struct Navigator
{
	explicit Navigator(std::string const &path);
	void set(std::string const &newPath);
	void go(std::string const &folderName);
	void back();
	void backto(std::string const &folderName);
	std::string get();

private:
	std::string path;
};
