#include <iostream>

#include "core/core.h"
#include "utils/statistics.h"
#include "utils/filesystem.h"
#include "utils/utils.h"
#include "utils/Log.h"

#include <vector>

void printUsage(const char* programName)
{
	std::cout << " Usage: " << programName << " [-i=<input dir>] [-o=<output_dir>] [-s=<path to settings file>]" << std::endl;
}

void initialize_log(const pc::utils::Parameters& params)
{
	pc::Log::init(params.logType);
	pc::Log::get().SetLogLevel(params.logLevel);
	pc::Log::get().SetEnabled(params.log);

	// TODO refactor this hack
	if (auto filelog = dynamic_cast<pc::FileLog*>(&pc::Log::get()))
	{
		filelog->SetFile(params.logFilename);
	}
}

int initialize_params(int argc, char** argv, pc::utils::Parameters &params)
{
	std::string inputDirectory;
	std::string outputDirectory;
	std::string settingsFile;

	bool setOutputDirectoryFromArguments = false;
	bool setInputDirectoryFromArguments = false;
	bool setSettingsFileFromArguments = false;

	for (int i = 1; i < argc; ++i)
	{
		char* argument = argv[i];
		if (std::strncmp(argument, "-i=", 3) == 0)
		{
			setInputDirectoryFromArguments = true;
			inputDirectory = std::string(argument).substr(3);
			pc::utils::filesystem::trim_dir_name(inputDirectory);
		}
		else if (std::strncmp(argument, "-o=", 3) == 0)
		{
			setOutputDirectoryFromArguments = true;
			outputDirectory = std::string(argument).substr(3);
			pc::utils::filesystem::trim_dir_name(outputDirectory);
		}
		else if (std::strncmp(argument, "-s=", 3) == 0)
		{
			setSettingsFileFromArguments = true;
			settingsFile = std::string(argument).substr(3);
		}
		else if (std::strncmp(argument, "-h", 2) == 0)
		{
			return -1;
		}
		else
		{
			// TODO throw exception
			std::cout << "WARNING: unknown argument " << i << " (" << argument << ")" << std::endl;
		}
	}

	if (setSettingsFileFromArguments)
		params.configFilename = settingsFile;

	params.ReadFromFile(params.configFilename);

	if (setOutputDirectoryFromArguments)
		params.outputDirectory = outputDirectory;

	if (setInputDirectoryFromArguments)
		params.inputDirectory = inputDirectory;
	
	pc::utils::filesystem::createDir(params.outputDirectory);

	if (params.needToCopyResultImageWhenFailed || params.alsoCopyOriginalImageToFailedFolder)
		pc::utils::filesystem::createDir(params.copyResultImageWhenFailedFolder);

	return 0;
}

bool processFile(const tinydir_file &file, pc::Processor &processor, const pc::utils::Parameters &params, bool& abortedByUser)
{
	try
	{
		processor.Open(file.path);
		processor.Process();

		if (params.saveFiles)
		{
			processor.SaveAs(params.outputDirectory + "/" + file.name);
		}

		processor.Close();
	}
	catch (const std::exception&)
	{
		processor.Close();
	}

	if (params.GUI)
	{
		try
		{

			processor.ShowOrigin();
			processor.ShowResult();

			if (pc::utils::WaitForKeyOpenCV() == pc::utils::KeyEscape)
				abortedByUser = true;
		}
		catch (std::exception&)
		{ }
	}

	return !abortedByUser;
}

void cleanupGUI(const pc::utils::Parameters &params)
{
	if (params.GUI)
	{
		pc::utils::CloseAllOpenCVWindows();
	}
}

void printStatistics(pc::Processor &processor, size_t processedCount, size_t expectedCount, bool abortedByUser)
{
	if (processedCount != expectedCount)
	{
		pc::Log::get().Write("", pc::LogLevel::Note);
		pc::Log::get().Write("process aborted, but not all files have been processed ("
			+ std::to_string(processedCount) + "/" + std::to_string(expectedCount) + ")",
			pc::LogLevel::Warning);
	}

	pc::utils::Statistics& stats = processor.GetStatistics();

	if (abortedByUser == false)
	{
		// check correctness
		bool correct = stats.GetTotalProcessedCount() == expectedCount;
		if (!correct)
		{
			pc::Log::get().Write("", pc::LogLevel::Note);
			pc::Log::get().Write("Info in statistics doesn't match info in processed files! Total processed files in Statistics = "
				+ std::to_string(stats.GetTotalProcessedCount()) + ", and in system = " + std::to_string(expectedCount),
				pc::LogLevel::Error);
		}
	}

	// print statistics
	MSG_WRITE("\nResults (input files count " + std::to_string(expectedCount) + "):");
	MSG_WRITE("  Processed files:  " + std::to_string(stats.GetTotalProcessedCount()));
	MSG_WRITE("  Success count:    " + std::to_string(stats.GetSuccessCount()));
	MSG_WRITE("  Warnings count:   " + std::to_string(stats.GetWarningsCount()));
	MSG_WRITE("  Total fail count: " + std::to_string(stats.GetTotalFailCount()));

	// TODO print detailed infos for all warnings and errors
	if (stats.GetWarningsCount() > 0)
	{
		// TODO
	}

	if (stats.GetTotalFailCount() > 0)
	{
		for each(const auto& f in stats.GetFails())
		{
			// TODO print error info
		}
	}
}

int main(int argc, char* argv[])
{
try
{
	pc::utils::Parameters params;
	if (initialize_params(argc, argv, params) < 0)
	{
		printUsage(argv[0]);

		// Wait for a keystroke in the window
		system("pause");
		return 1;
	}

	initialize_log(params);

	MSG_WRITE("\n\n===========================================\n"
				" started new operation " + pc::utils::GetTime() + " " + pc::utils::GetDate() +
				"\n===========================================\n");
		
	pc::utils::filesystem::TFiles files;
	pc::utils::filesystem::getFilesInDirectory(params.inputDirectory, files, params.extensionPattern, params.checkSubdirectories);

	// TODO always use absolute path here
	MSG_WRITE(std::string("try to process ") + std::to_string(files.size()) + " files in " + params.inputDirectory);
	MSG_WRITE(std::string("Output directory is ") + (params.outputDirectory.empty() ? pc::utils::filesystem::getCurrentDirectory() : params.outputDirectory) + "\n");

	if (files.empty() == false)
	{
		pc::Processor processor(params);

		size_t index = 1u;
		size_t total = files.size();

		bool abortedByUser = false;
		for (const auto& file : files)
		{
			MSG_WRITE(" :: " + std::to_string(index) + "/" + std::to_string(total) + " file: " + file.path + " :: ");

			++index;

			if (processFile(file, processor, params, abortedByUser) == false)
				break;
		}

		cleanupGUI(params);
		printStatistics(processor, index - 1u, total, abortedByUser);
	}
	else
	{
		MSG_WRITE("Nothing found in specified directory \"" + params.inputDirectory + "\"");
	}

	MSG_WRITE(	"\n==========================================\n"
				" ended operation " + pc::utils::GetTime() + " " + pc::utils::GetDate() +
				"\n==========================================");
}
catch (const std::exception& ex)
{
	std::string msg = "\ncatch exception at main level\n  Message: " + std::string(ex.what()) + "\n";

	std::cout << msg;
	pc::Log::get().Write(msg, pc::LogLevel::Error);
}

system("pause");
return 0;	
}