#pragma once

#include <string>
#include <vector>
#include <map>

#include <memory>

#include "../utils/parameters.h"

namespace pc
{

namespace utils
{
	class Statistics;
}

class ProcessorImpl;
class Processor
{		
private:
	std::unique_ptr<ProcessorImpl> m_impl;
public:

	Processor(const utils::Parameters& params = utils::Parameters());
	~Processor();

	void Open(const std::string& filename);
	void Close();

	void Process(utils::Parameters* params = nullptr);

	void Save();
	void SaveAs(const std::string& newFilename);

	void Reset();

	void ShowOrigin();
	void ShowResult();
		
	utils::Statistics& GetStatistics();
};

}