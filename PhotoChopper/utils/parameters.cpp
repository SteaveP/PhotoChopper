#include "parameters.h"

#include "iLog.h"
#include "filesystem.h"

#include <libconfig.h++>
#include <iostream>


namespace pc
{

namespace utils
{

Parameters::Parameters()
{
	ResetToDefaults();
}

void Parameters::ReadFromFile(const std::string& filename)
{
	using namespace libconfig;

	configFilename = filename;

	Config cfg;

	try
	{
		cfg.setOptions(cfg.getOptions() | libconfig::Setting::OptionAutoConvert);
		cfg.readFile(configFilename.c_str());

		Setting& settings = cfg.getRoot();
		if (settings.exists("global"))
		{
			Setting& gGlobal = settings["global"];

			gGlobal.lookupValue("outputDirectory", outputDirectory);
			filesystem::trim_dir_name(outputDirectory);

			gGlobal.lookupValue("inputDirectory", inputDirectory);
			filesystem::trim_dir_name(inputDirectory);

			gGlobal.lookupValue("checkSubdirectories", checkSubdirectories);
			gGlobal.lookupValue("extensionPattern", extensionPattern);

			gGlobal.lookupValue("saveFiles", saveFiles);

			gGlobal.lookupValue("GUI", GUI);

			gGlobal.lookupValue("drawLineBetweenEyes", drawLineBetweenEyes);
			gGlobal.lookupValue("drawExtrapolationLineBetweenEyes", drawExtrapolationLineBetweenEyes);
			gGlobal.lookupValue("drawLineBetweenEyesExtrapolationParam", drawLineBetweenEyesExtrapolationParam);

			gGlobal.lookupValue("drawHorizontalEyesLineOriginal", drawHorizontalEyesLineOriginal);
			gGlobal.lookupValue("drawHorizontalEyesLineResult", drawHorizontalEyesLineResult);

			gGlobal.lookupValue("drawDetectedRegionsWhenException", drawDetectedRegionsWhenException);

			gGlobal.lookupValue("drawEyesRegions", drawEyesRegions);
			gGlobal.lookupValue("drawFacesRegions", drawFacesRegions);

			gGlobal.lookupValue("silent", silent);

			gGlobal.lookupValue("rotateImage", rotateImage);
			gGlobal.lookupValue("rotateDegree", rotateDegree);

			gGlobal.lookupValue("log", log);

			gGlobal.lookupValue("cropRelativeScaleYDownFactor", cropRelativeScaleYDownFactor);

			std::string val;
			gGlobal.lookupValue("logType", val);
			if (val.empty() == false)
				logType = pc::LogTypeFromString(val);

			val.clear();
			gGlobal.lookupValue("logLevel", val);
			if (val.empty() == false)
				logLevel = pc::LogLevelFromString(val);

			gGlobal.lookupValue("logFilename", logFilename);
			gGlobal.lookupValue("needEyeHorizontalCorrection", needEyeHorizontalCorrection);
			gGlobal.lookupValue("needCrop", needCrop);
			gGlobal.lookupValue("cascadeFrontalFaceTemplate", cascadeFrontalFaceTemplate);
			gGlobal.lookupValue("cascadeEyeTemplate", cascadeEyeTemplate);
			//gGlobal.lookupValue("configFilename", configFilename);

			gGlobal.lookupValue("copyOriginalImageToResultWhenFailed", copyOriginalImageToResultWhenFailed);
			gGlobal.lookupValue("needToCopyResultImageWhenFailed", needToCopyResultImageWhenFailed);
			gGlobal.lookupValue("alsoCopyOriginalImageToFailedFolder", alsoCopyOriginalImageToFailedFolder);
			gGlobal.lookupValue("copyResultImageWhenFailedFolder", copyResultImageWhenFailedFolder);
			filesystem::trim_dir_name(copyResultImageWhenFailedFolder);

			gGlobal.lookupValue("aspectRatio", aspectRatio);
			gGlobal.lookupValue("cropRelativeScaleX", cropRelativeScaleX);
			gGlobal.lookupValue("cropRelativeScaleY", cropRelativeScaleY);

			gGlobal.lookupValue("siholetteBrightnessThreshold", siholetteBrightnessThreshold);


			gGlobal.lookupValue("needToMakeImageBorder", needToMakeImageBorder);
			gGlobal.lookupValue("imageBorderSizeIsRelative", imageBorderSizeIsRelative);

			if (gGlobal.exists("imageBorderSize"))
			{
				libconfig::Setting& setting = gGlobal["imageBorderSize"];
				if (setting.getType() == libconfig::Setting::Type::TypeArray && setting.getLength() >= 2)
				{
					imageBorderSize.clear();
					imageBorderSize.push_back(setting[0]);
					imageBorderSize.push_back(setting[1]);
				}

				// TODO else throw exception or write warning?
			}

			if (gGlobal.exists("imageBorderMinSize"))
			{
				libconfig::Setting& setting = gGlobal["imageBorderMinSize"];
				if (setting.getType() == libconfig::Setting::Type::TypeArray && setting.getLength() >= 2)
				{
					imageBorderMinSize.clear();
					imageBorderMinSize.push_back(setting[0]);
					imageBorderMinSize.push_back(setting[1]);
				}

				// TODO else throw exception or write warning?
			}

			gGlobal.lookupValue("backupBeforeCrop", backupBeforeCrop);
			gGlobal.lookupValue("backupBeforeCropPath", backupBeforeCropPath);
			filesystem::trim_dir_name(backupBeforeCropPath);
			gGlobal.lookupValue("backupBeforeCropFilenameTemplate", backupBeforeCropFilenameTemplate);

			gGlobal.lookupValue("backupBeforeRotate", backupBeforeRotate);
			gGlobal.lookupValue("backupBeforeRotatePath", backupBeforeRotatePath);
			filesystem::trim_dir_name(backupBeforeRotatePath);
			gGlobal.lookupValue("backupBeforeRotateFilenameTemplate", backupBeforeRotateFilenameTemplate);
		}
	}
	catch (libconfig::FileIOException&)
	{
		// NOTE cant message to log here because log is not initialized yet!
		std::cout << "WARNING: can't open settings file " << configFilename << std::endl;
		ResetToDefaults();
	}
	catch (std::exception&)
	{
		// NOTE cant message to log here because log is not initialized yet!
		std::cout << "WARNING: catch error when reading and apply settings from file " << configFilename << std::endl;
		ResetToDefaults();
	}
}

void Parameters::ResetToDefaults()
{
	log = true;
	logLevel = pc::LogLevel::Info;
	logType = pc::LogType::LogToConsole;
	logFilename = "./data/process.log";
	needEyeHorizontalCorrection = true;
	needCrop = true;
	cascadeFrontalFaceTemplate = "./data/haarcascade_frontalface_default.xml";
	cascadeEyeTemplate = "./data/haarcascade_eye.xml";

	configFilename = "settings.cfg";

	saveFiles = true;

	GUI = true;

	drawLineBetweenEyes = true;
	drawExtrapolationLineBetweenEyes = true;
	drawLineBetweenEyesExtrapolationParam = -0.3f;
	drawHorizontalEyesLineOriginal = true;
	drawHorizontalEyesLineResult = true;

	drawDetectedRegionsWhenException = true;
	drawEyesRegions = true;
	drawFacesRegions = true;

	silent = false;

	rotateImage = false;
	rotateDegree = 0;

	outputDirectory = "./result";
	inputDirectory = ".";

	extensionPattern = "jpg";
	checkSubdirectories = false;

	copyOriginalImageToResultWhenFailed = true;

	needToCopyResultImageWhenFailed = true;
	alsoCopyOriginalImageToFailedFolder = true;
	copyResultImageWhenFailedFolder = "./result/fails";

	aspectRatio = 4.0f / 3.0f;
	cropRelativeScaleX = 0.044f;
	cropRelativeScaleY = 0.033f;

	siholetteBrightnessThreshold = 140;

	needToMakeImageBorder = true;
	imageBorderSizeIsRelative = false;

	imageBorderSize.push_back(40);
	imageBorderSize.push_back(40);
	imageBorderMinSize.push_back(40);
	imageBorderMinSize.push_back(40);

	backupBeforeCrop = false;
	backupBeforeCropPath;
	backupBeforeCropFilenameTemplate;

	backupBeforeRotate = false;
	backupBeforeRotatePath;
	backupBeforeRotateFilenameTemplate;
}

bool Parameters::NeedToHaveDisplayedImage()
{
	return GUI || needToCopyResultImageWhenFailed;
}

}
}