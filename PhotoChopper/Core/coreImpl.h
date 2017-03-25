#pragma once

#include "../utils/statistics.h"
#include "../utils/parameters.h"
#include "../utils/box.h"

#include <opencv2/objdetect.hpp>

namespace Exiv2
{
	class ExifData;
}

namespace pc
{
	
// internal class
class ProcessorImpl
{
	typedef std::auto_ptr<Exiv2::ExifData> TExifDataPtr;
	typedef std::vector<cv::Rect> TRegions;

public:
	ProcessorImpl(const utils::Parameters& params = utils::Parameters());
	~ProcessorImpl();

	void Open(const std::string& filename);
	void Close();

	void Process(utils::Parameters* params = nullptr);

	void Save();
	void SaveAs(const std::string& newFilename);

	bool IsValid();

	void ShowOrigin();
	void ShowResult();

	inline utils::Statistics& GetStatistics() { return m_stats; }
	
private:

	void  reset();
		
	void  processImpl();
	void  processOriginalImage();

	float rotate(std::vector<cv::Point2f>& eyeCenters);
	void  crop(cv::Mat& face);
	void  contours(int hight);

	void  horizontalRatio(pc::utils::Box& data, cv::Mat& resizedImage);
	void  calcAspectRatio(pc::utils::Box& data, cv::Mat& resizedImage, int concurrentHight);

	void  saveExif(const std::string &newFilename);
	void  applyExifOrientation(int orientation);

	int   findFaceBottom(cv::Mat& grayScaleImg, int startY);
	int   findFaceBottomMirror(cv::Mat& grayScaleImg);
	int   findLips();

	int   findChin(double middleValueForAll, int x, int y, cv::Mat grayScaleImg);
	void  tryToSaveResultImageToDisplayToFile();
	
	void  applyRotation();
	void  applyAndResetExifRotation(const std::string &filename);

	cv::Point findEyeCenter(cv::Mat face, cv::Rect eye, std::string debugWindow);
	std::vector<cv::Point2f> detectEyes();

	int   proofOrientation();
	
private:
	utils::Parameters m_params;
	utils::Statistics m_stats;
	TExifDataPtr m_exifData;

	std::string m_filename;
	utils::Box m_box;

	cv::Ptr<cv::CascadeClassifier> m_cascadeFrontalFace;
	cv::Ptr<cv::CascadeClassifier> m_cascadeEye;

	cv::Mat m_originImage;

	cv::Mat m_resizedImage;
	cv::Mat m_resizedImageGrayscale;

	bool	m_isOpen;
	bool	m_success;

	bool	m_needToDelayedCopyResultImageWhenFail;

	TRegions m_faces;
	TRegions m_eyes;

	// for display
	// TOOD refactor
	cv::Mat m_displayOrigin;
	cv::Mat m_displayResult;
};

}