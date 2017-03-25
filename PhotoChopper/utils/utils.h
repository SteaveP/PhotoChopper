#pragma once

#include <string>

namespace pc
{
namespace utils
{

enum KeyCode {
	KeyEscape = 27
};

std::string& to_lower(std::string& str);

std::string GetTime();
std::string GetDate();

int WaitForKeyOpenCV();
void CloseAllOpenCVWindows();

float lerp(float a, float b, float param);

}
}