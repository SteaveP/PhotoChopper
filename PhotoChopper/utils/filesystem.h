#pragma once

#include <string>
#include <vector>

#include "../external/tinydir/tinydir.h"

namespace pc
{
namespace utils
{
namespace filesystem
{

// class File for non buffered read & writes
class File
{
	int m_fid;
	std::string m_path;

public:

	File();
	File(const std::string& file);
	~File();

	void Write(const std::string& data, bool endOfLine = true);
	void Open(const std::string& file);
	void Close();

protected:

	bool _isValid();
};

typedef std::vector<tinydir_file> TFiles;

// возвращает полный путь
void getFilesInDirectory(const std::string& path, TFiles& files,
	const std::string& extensionFilter, bool recursive = false);

std::string getFilename(const std::string& path, std::string* dir = nullptr);
std::string getExtension(const std::string& path, std::string* filenameWithoutExtension = nullptr);
std::string& trim_dir_name(std::string& dir);

bool copyFile(const std::string& src, const std::string& dst, bool failIfExist = false);

bool fileExists(const std::string& path);
bool dirExists(const std::string& path);
bool createFile(const std::string& path, bool failIfExists = true);
bool createDir(const std::string& path);

bool isUNCServer(const std::string& path);
bool isUncServerShare(const std::string& path);

std::string getAbsolutePath(const std::string& relativePath);
std::string getRelativePath(const std::string& absolutePath);

std::string getCurrentDirectory();

}
}
}