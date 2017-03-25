#pragma once

#include <exception>

namespace pc
{

class DetectionException : public std::exception
{
public:
	DetectionException(const char* msg)
		: std::exception(msg)
	{}
};

}