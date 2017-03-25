#pragma once

#include <string>
#include <vector>

#include "iLog.h"

namespace pc
{
namespace utils
{

struct Parameters
{
	// data
	std::string outputDirectory;
	std::string inputDirectory;
	std::string extensionPattern;
	bool checkSubdirectories;

	bool saveFiles;

	bool log;
	pc::LogType logType;
	pc::LogLevel logLevel;
	std::string logFilename;

	bool GUI;

	bool drawLineBetweenEyes;
	bool drawExtrapolationLineBetweenEyes;
	float drawLineBetweenEyesExtrapolationParam;
	bool drawHorizontalEyesLineOriginal;
	bool drawHorizontalEyesLineResult;

	bool drawDetectedRegionsWhenException; // when warning or error
	bool drawEyesRegions;
	bool drawFacesRegions;

	bool silent; // TODO

	bool rotateImage;
	int rotateDegree; // need to be odd to 90

	bool needEyeHorizontalCorrection;
	bool needCrop;

	bool copyOriginalImageToResultWhenFailed;

	bool needToCopyResultImageWhenFailed;
	bool alsoCopyOriginalImageToFailedFolder;
	std::string copyResultImageWhenFailedFolder;

	float aspectRatio;
	float cropRelativeScaleX;
	float cropRelativeScaleY;

	int	  siholetteBrightnessThreshold;

	// TODO разные размеры границы для X и Y координатах
	bool  needToMakeImageBorder;
	bool  imageBorderSizeIsRelative;
	std::vector<float> imageBorderSize;
	std::vector<float> imageBorderMinSize;

	bool backupBeforeCrop;
	std::string backupBeforeCropPath;
	std::string backupBeforeCropFilenameTemplate;

	bool backupBeforeRotate;
	std::string backupBeforeRotatePath;
	std::string backupBeforeRotateFilenameTemplate;

	std::string cascadeFrontalFaceTemplate;
	std::string cascadeEyeTemplate;

	std::string configFilename;

	//коэффициент отступа от подбородка
	float cropRelativeScaleYDownFactor;

	// methods
	Parameters();

	void ResetToDefaults();
	void ReadFromFile(const std::string& filename);
	bool NeedToHaveDisplayedImage();
};

}
}