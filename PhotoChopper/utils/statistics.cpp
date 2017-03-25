#include "statistics.h"

namespace pc
{
namespace utils
{
	
Statistics::Statistics()
{
	Reset();
}

Statistics::Info::Info(const std::string& file, const std::string& msg)
	: filename(file), message(msg)
{}

Statistics::Info::Info()
{}

void Statistics::Reset()
{
	m_success.clear();
	m_warning.clear();
	m_fail.clear();
}

void Statistics::AddSuccess(const Info& file)
{
	m_success.push_back(file);
}

void Statistics::AddWarning(const Info& file)
{
	m_warning.push_back(file);
}
void Statistics::AddFail(const Info& file, FailType type)
{
	m_fail[type].push_back(file);
}

int Statistics::GetTotalProcessedCount()
{
	return GetSuccessCount() + GetTotalFailCount();
}

int Statistics::GetSuccessCount()
{
	return m_success.size();
}

int Statistics::GetWarningsCount()
{
	return m_warning.size();
}

int Statistics::GetTotalFailCount()
{
	int count = 0;

	for each(const auto& t in m_fail)
	{
		count += t.second.size();
	}

	return count;
}

int Statistics::GetFailCount(FailType failType)
{
	TFailedInfos::const_iterator it = m_fail.find(failType);

	if (it != m_fail.end())
		return it->second.size();
	else
		return 0;
}

const Statistics::TInfoVec& Statistics::GetFails(FailType failType)
{
	TFailedInfos::const_iterator it = m_fail.find(failType);
	if (it != m_fail.end())
		return it->second;
	else
		throw std::invalid_argument("elements for specified failType not found");
}

const Statistics::TFailedInfos& Statistics::GetFails()
{
	return m_fail;
}

float Statistics::GetTotalTime()
{
	// not implemented yet
	return -1.f;
}

}
}