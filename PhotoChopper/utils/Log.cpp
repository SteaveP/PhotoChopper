#include "Log.h"

#include <iostream>

#include "utils.h"
#include "filesystem.h"

namespace pc
{

LogType Log::g_logType = LogToConsole;
	
Log::Log()
	: m_logLevel(LogLevel::Info), m_enabled(true)
{}
	
void Log::init(LogType type)
{
	g_logType = type;
}


Log& Log::get()
{
	static std::auto_ptr<Log> log;

	if (log.get() == nullptr)
	{
		// create new one
		switch (g_logType)
		{
		case LogToFile:
			log.reset(new FileLog());
			break;
		case LogToConsole:
			log.reset(new ConsoleLog());
			break;
		case LogToConsoleAndFile:
			log.reset(new ConsoleAndFileLog());
			break;
		default:
			throw std::exception("unknown log type");
			break;
		}
	}

	static auto& log_ref = *log.get();

	return log_ref;
}

void Log::SetLogLevel(LogLevel logLevel)
{
	m_logLevel = logLevel;
}

std::string Log::getStr(const std::string& message, LogLevel loglevel, bool insertNewLine /*= true*/)
{
	std::string result;

	switch (loglevel)
	{
	case pc::Error:
		result += "  Error: ";
		break;
	case pc::Warning:
		result += "  Warning: ";
		break;
	case pc::Info:
		result += "  Info: ";
		break;
	case pc::None:
	case pc::Note:
		// nothing;
		break;
	default:
		break;
	}

	result += message + (insertNewLine ? "\n" : "");

	return result;
}

bool Log::canWrite(LogLevel logLevel)
{
	// NOTE now m_enabled influence on all log levels
	//bool isEnabled = (logLevel == Note) ? true : m_enabled;
	bool isEnabled = m_enabled;
	return isEnabled && (logLevel <= m_logLevel && (logLevel != None && m_logLevel != None));
}

void Log::SetEnabled(bool enabled, bool showMessage)
{
	if (showMessage)
	{
		std::string str = "logging is "; str += (enabled ? "enabled" : "disabled");
		Write(str, LogLevel::Note);
	}

	m_enabled = enabled;
}
	
void FileLog::SetFile(const std::string& filename)
{
	try
	{
		if (utils::filesystem::fileExists(filename) == false)
		{
			std::string dir;
			std::string name = utils::filesystem::getFilename(filename, &dir);

			bool result = utils::filesystem::createDir(dir);
			if (!result)
				throw std::exception("cant create dir");
		}

		m_file.reset(new utils::filesystem::File());

		if (m_file.get() == nullptr)
			std::cerr << "cant open log file " << filename << std::endl;

		m_file->Open(filename);
	}
	catch (std::exception& ex)
	{
		m_file.reset(nullptr);
		std::cerr << "cant open log file " << filename << std::endl;
		std::cerr << "    message: " << ex.what() << std::endl;
	}
}

void FileLog::Write(const std::string& message, LogLevel loglevel, bool insertNewLine)
{
	if (m_file.get() && canWrite(loglevel))
	{
		m_file->Write( getStr(message, loglevel, insertNewLine) );
	}
}
	
void ConsoleLog::Write(const std::string& message, LogLevel loglevel, bool insertNewLine /* = true */)
{
	if (canWrite(loglevel))
		std::cout << getStr(message, loglevel, insertNewLine);
}


void ConsoleAndFileLog::Write(const std::string& message, LogLevel loglevel, bool insertNewLine)
{
	super::Write(message, loglevel, insertNewLine);
	m_consoleLog.Write(message, loglevel, insertNewLine);
}

void ConsoleAndFileLog::SetLogLevel(LogLevel logLevel)
{
	super::SetLogLevel(logLevel);
	m_consoleLog.SetLogLevel(logLevel);
}

void ConsoleAndFileLog::SetEnabled(bool enabled, bool showMessage)
{
	super::SetEnabled(enabled, showMessage);
	m_consoleLog.SetEnabled(enabled, false);
}

//////////////////////////////////////////////////////////////////////////

std::string LogLevelToString(LogLevel loglevel)
{
	switch (loglevel)
	{
	case pc::LogLevel::Note:
		return "Note";
		break;
	case pc::LogLevel::None:
		return "None";
		break;
	case pc::LogLevel::Error:
		return "Error";
		break;
	case pc::LogLevel::Warning:
		return "Warning";
		break;
	case pc::LogLevel::Info:
		return "Info";
		break;
	default:
		return "Unknown";
		break;
	}
}

LogLevel LogLevelFromString(const std::string& loglevelstr)
{
	std::string str = loglevelstr;
	utils::to_lower(str);

	if (str == "note")
		return LogLevel::Note;
	else if (str == "none")
		return LogLevel::None;
	else if (str == "error")
		return LogLevel::Error;
	else if (str == "warning")
		return LogLevel::Warning;
	else if (str == "info")
		return LogLevel::Info;
	else
	{
		// TODO return default value and write warning message?
		std::cout << "WARNING: value of LogLevel from settings file is unknown (" << (loglevelstr.empty() ? "<emtpy>" : loglevelstr) << "); set default value" << std::endl;
		return LogLevel::Info;
	}			
}

std::string LogTypeToString(LogType logtype)
{
	switch (logtype)
	{
	case pc::LogType::LogToFile:
		return "LogToFile";
		break;
	case pc::LogType::LogToConsole:
		return "LogToConsole";
		break;
	case pc::LogType::LogToConsoleAndFile:
		return "LogToConsoleAndFile";
		break;
	default:
		return "Unknown";
		break;
	}
}

LogType LogTypeFromString(const std::string& logtypestr)
{
	std::string str = logtypestr;
	utils::to_lower(str);

	if (str == "logtofile")
		return LogType::LogToFile;
	else if (str == "logtoconsole")
		return LogType::LogToConsole;
	else if (str == "logtoconsoleandfile")
		return LogType::LogToConsoleAndFile;
	else
	{
		// TODO return default value and write warning message?
		std::cout << "WARNING: value of LogType from settings file is unknown (" << (logtypestr.empty() ? "<emtpy>" : logtypestr) << "); set default value" << std::endl;
		return LogType::LogToConsole;
	}
}
}
