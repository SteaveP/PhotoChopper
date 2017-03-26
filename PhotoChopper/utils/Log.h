#pragma once

#include "iLog.h"
#include "classutils.h"

#include <string>
#include <memory>

#define MSG_WRITE(x) pc::Log::get().Write(x, pc::LogLevel::Note)

namespace pc
{

namespace utils
{
namespace filesystem
{
	class File;
} // namespace filesystem
} // namespace utils

// interface
class Log : public utils::noncopyable
{
public:

	Log();

	static Log& get();
	static void init(LogType logType);
		
	virtual void SetLogLevel(LogLevel logLevel);
	virtual void SetEnabled(bool enabled, bool showMessage = true);

	virtual void Write(const std::string& message, LogLevel loglevel, bool insertNewLine = true) = 0;
	virtual ~Log() {};

protected:

	virtual std::string getStr(const std::string& message, LogLevel loglevel, bool insertNewLine = true);
	virtual bool canWrite(LogLevel logLevel);

	static LogType g_logType;

protected:

	LogLevel m_logLevel;
	bool	 m_enabled;
};

class ConsoleLog : public Log
{
public:
	virtual void Write(const std::string& message, LogLevel loglevel, bool insertNewLine /* = true */) override;
};

class FileLog : public Log
{
public:
	virtual void Write(const std::string& message, LogLevel loglevel, bool insertNewLine) override;

	void SetFile(const std::string& filename);

private:
	typedef std::unique_ptr<utils::filesystem::File> FilePtr;
	FilePtr m_file;
};

class ConsoleAndFileLog : public FileLog
{
	INIT_INHERITANCE(FileLog);

public:
	virtual void Write(const std::string& message, LogLevel loglevel, bool insertNewLine) override;

	virtual void SetLogLevel(LogLevel logLevel) override;
	virtual void SetEnabled(bool enabled, bool showMessage) override;
private:

	ConsoleLog	m_consoleLog;
};

}