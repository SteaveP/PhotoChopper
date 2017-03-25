#pragma once

#include <string>

namespace pc
{

enum LogLevel
{
	Note = 0,
	None,
	Error,
	Warning,
	Info
};

enum LogType
{
	LogToFile,
	LogToConsole,
	LogToConsoleAndFile
};

std::string LogLevelToString(LogLevel loglevel);
LogLevel LogLevelFromString(const std::string& loglevelstr);

std::string LogTypeToString(LogType logtype);
LogType LogTypeFromString(const std::string& logtypestr);

}