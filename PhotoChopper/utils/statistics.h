#pragma once

#include <string>
#include <vector>
#include <map>

namespace pc
{

namespace utils
{

class Statistics
{
public:

	struct Info
	{
		std::string filename;
		std::string message;

		Info();
		Info(const std::string& file, const std::string& msg);
	};

	enum class FailType
	{
		OpenFile,
		SaveFile,
		FaceDetection,
		Other
	};

	typedef std::vector<Info> TInfoVec;
	typedef std::map<FailType, TInfoVec> TFailedInfos;

	Statistics();
	void Reset();

	void AddSuccess(const Info& file);
	void AddWarning(const Info& file);
	void AddFail(const Info& file, FailType type);

	int  GetTotalProcessedCount();
	int	 GetSuccessCount();
	int  GetWarningsCount();
	int  GetTotalFailCount();
	int  GetFailCount(FailType failType);
	const TFailedInfos& GetFails();
	const TInfoVec& GetFails(FailType failType);

	float GetTotalTime();
protected:
	TFailedInfos	m_fail;
	TInfoVec		m_warning;
	TInfoVec		m_success;
};

}
}