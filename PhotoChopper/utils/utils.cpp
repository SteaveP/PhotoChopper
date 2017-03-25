#include "utils.h"

#include <chrono>
#include <iomanip>
#include <sstream>

#include <opencv2/highgui.hpp>

namespace // anonymous
{
	std::tm gettm()
	{
		std::chrono::time_point<std::chrono::system_clock> now =
			std::chrono::system_clock::now();

		std::time_t now_time = std::chrono::system_clock::to_time_t(now);
		return *std::localtime(&now_time);
	}
} // namespace anonymous

namespace pc
{
namespace utils
{

std::string& to_lower(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

std::string GetTime()
{
	std::tm tm = gettm();
	std::ostringstream ss;

	ss << std::put_time(&tm, "%H:%M:%S");
	return ss.str();
}

std::string GetDate()
{
	std::tm tm = gettm();
	std::ostringstream ss;

	ss << std::put_time(&tm, "%d.%m.%Y");
	return ss.str();
}

int WaitForKeyOpenCV()
{
	return cv::waitKey(0);
}

void CloseAllOpenCVWindows()
{
	cv::destroyAllWindows();
}

float lerp(float a, float b, float param)
{
	return a + (b - a) * param;
}

}
}