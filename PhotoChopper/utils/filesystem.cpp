#include "filesystem.h"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <cassert>

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#include <windows.h>

#include "utils.h"

namespace // anonymous
{
	bool _createDir(const std::string& path)
	{
		BOOL result = CreateDirectoryA(path.c_str(), 0);

		if (result != 0)
			return true;

		DWORD errcode = GetLastError();

		if (errcode == ERROR_ALREADY_EXISTS)
			return true;

		if (pc::utils::filesystem::dirExists(path))
			return true;

		return false;
	}
} // namespace anonymous

namespace pc
{
namespace utils
{
namespace filesystem
{

File::File()
	: m_fid(-1)
{}

File::File(const std::string& file)
	: m_fid(-1)
{
	// NOTE exceptions in constructor are bad!
	try
	{
		Open(file);
	}
	catch (...)
	{
		Close();
	}
}

File::~File()
{
	Close();
}

void File::Open(const std::string& file)
{
	if (_isValid())
		Close();

	// TODO _O_U8TEXT?
	int res = _sopen_s(&m_fid, file.c_str(), _O_WRONLY | _O_CREAT | _O_APPEND | _O_TEXT, _SH_DENYWR, _S_IWRITE | _S_IREAD);

	if (res != 0)
	{
		if (res == EACCES)
		{
			throw std::exception((std::string("cant open file ") + file + " for writing. Maybe there is some process lock the file.").c_str());
		}
		else
			throw std::exception((std::string("cant open file ") + file + " for unknown reason").c_str());
	}

	// all good
	m_path = file;
}

void File::Close()
{
	if (_isValid())
	{
		_close(m_fid);

		m_fid = -1;
		m_path.clear();
	}
}

void File::Write(const std::string& data, bool endOfLine /* = true */)
{
	if (_isValid() == false)
	{
		return;
	}

	int res = _write(m_fid, data.c_str(), data.size());

	if (res != data.size())
	{
		// TODO try again?
	}
	else if (res < 0)
	{
		// error!
		throw std::exception((std::string("file write error! ") + m_path).c_str());
	}
}

bool File::_isValid()
{
	return m_fid >= 0;
}

std::string getFilename(const std::string& path, std::string* dir)
{
	std::string::size_type a = path.find_last_of('\\');
	std::string::size_type b = path.find_last_of('/');

	std::string::size_type pos = /*std::*/max((a == std::string::npos ? 0 : a), (b == std::string::npos ? 0 : b));

	if (pos == std::string::npos)
		pos = 0;

	assert(pos + 1u < path.size());

	if (dir != nullptr)
		*dir = path.substr(0, pos);

	// TODO failsafe here! 
	return path.substr(pos + 1u);
}

std::string getExtension(const std::string& path, std::string* filenameWithoutExtension)
{
	std::string extension;

	std::string::size_type dotPos = path.find_last_of('.');

	if (dotPos != std::string::npos)
	{
		if (filenameWithoutExtension != nullptr)
			*filenameWithoutExtension = path.substr(0, dotPos);

		if (dotPos != path.length() - 1)
			extension = path.substr(dotPos + 1);
	}

	return extension;
}

bool copyFile(const std::string& src, const std::string& dst, bool failIfExist /*= false*/)
{
	std::ifstream source(src, std::ios::binary);
	std::ofstream dest(dst, std::ios::binary);
	
	if (!source || !dest)
		return false;

	std::istreambuf_iterator<char> begin_source(source);
	std::istreambuf_iterator<char> end_source;
	std::ostreambuf_iterator<char> begin_dest(dest);
	std::copy(begin_source, end_source, begin_dest);

	if (!source || !dest)
		return false;

	return true;
}

bool createDir(const std::string& path)
{
	bool result = _createDir(path);

	if (result)
		return result;

	std::string rpath = path + "/";

	std::string::iterator it = rpath.begin();

	// if there is network path, no need to replace first two backslashes
	if (rpath.length() > 2 && rpath.substr(0, 2) == "\\\\")
		it += 2;

	std::replace(it, rpath.end(), '\\', '/');


	auto p = rpath.find_first_of('/');
	while (p != std::string::npos)
	{
		std::string tmp = rpath.substr(0, p);

		if (tmp.empty() == false)
		{
			bool r = _createDir(tmp);

			if (r == false)
				return false;
		}

		p = rpath.find_first_of('/', p + 1);
	}

	return true;
}

std::string getCurrentDirectory()
{
	const int len = 1024 * 4;
	char buf[len];
	GetCurrentDirectoryA(len, buf);

	return buf;
}

bool dirExists(const std::string& path)
{
	if (isUNCServer(path) || isUncServerShare(path))
	{
		// если сетевой путь, по умолчанию считаем, что он есть?
		return true;
	}

	DWORD ftyp = GetFileAttributesA(path.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

bool fileExists(const std::string& path)
{
	DWORD ftyp = GetFileAttributesA(path.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return false;   // this is a directory!

	return true;    // this is not a directory!
}

bool isUNCServer(const std::string& path)
{
	if (path.length() > 2 && path.substr(0, 2) == "\\\\")
	{
		std::string trimmed = path.substr(2);
		std::replace(trimmed.begin(), trimmed.end(), '\\', '/');

		int count = std::count(trimmed.begin(), trimmed.end(), '/');

		if (count <= 1)
			return true;
	}

	return false;
}

bool isUncServerShare(const std::string& path)
{
	if (path.length() > 2 && path.substr(0, 2) == "\\\\")
	{
		std::string trimmed = path.substr(2);
		std::replace(trimmed.begin(), trimmed.end(), '\\', '/');

		int count = std::count(trimmed.begin(), trimmed.end(), '/');

		if ((count == 1 && trimmed[trimmed.length() - 1] != '/')
			|| (count == 2 && trimmed[trimmed.length() - 1] == '/'))
			return true;
	}

	return false;
}

std::string& trim_dir_name(std::string& dir)
{
	if (dir.size() != 0 && dir[dir.size() - 1] == '\\')
	{
		dir[dir.size() - 1] = '\0';
		dir.resize(dir.size() - 1);
	}

	return dir;
}

void getFilesInDirectory(const std::string& path, TFiles& files,
	const std::string& extensionFilter, bool recursive)
{
	bool checkExtension = extensionFilter.empty() == false;

	tinydir_dir dir;
	if (tinydir_open(&dir, path.c_str()) < 0)
		return;

	while (dir.has_next)
	{
		tinydir_file file;
		if (tinydir_readfile(&dir, &file) < 0)
			continue;

		if (strcmp(".", file.name) != 0 && strcmp("..", file.name) != 0)
		{
			if (file.is_dir && recursive)
				return getFilesInDirectory(file.path, files, extensionFilter, recursive);
			else
			{
				bool success = true;

				if (checkExtension)
					success = pc::utils::to_lower(std::string(file.extension)) == extensionFilter;

				if (success)
					files.push_back(file);
			}
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
}

}
}
}
