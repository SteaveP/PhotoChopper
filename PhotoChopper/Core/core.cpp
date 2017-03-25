#include "core.h"
#include "coreImpl.h"

namespace pc
{

Processor::Processor(const utils::Parameters& params /* = Parameters() */)
	: m_impl(new ProcessorImpl(params))
{}

Processor::~Processor()
{}

void Processor::Open(const std::string& filename)
{
	m_impl->Open(filename);
}

void Processor::Process(utils::Parameters* params)
{
	m_impl->Process(params);
}

void Processor::Close()
{
	m_impl->Close();
}

void Processor::SaveAs(const std::string& newFilename)
{
	m_impl->SaveAs(newFilename);
}

utils::Statistics& Processor::GetStatistics()
{
	return m_impl->GetStatistics();
}

void Processor::ShowOrigin()
{
	m_impl->ShowOrigin();
}

void Processor::ShowResult()
{
	m_impl->ShowResult();
}

}