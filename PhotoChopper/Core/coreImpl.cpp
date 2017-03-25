#include "coreImpl.h"

#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <exiv2/exiv2.hpp>

#include "../utils/log.h"
#include "../utils/utils.h"
#include "../utils/errors.h"
#include "../utils/filesystem.h"

namespace pc
{

using stats = utils::Statistics;

ProcessorImpl::ProcessorImpl(const utils::Parameters& params)
	: m_params(params)
	, m_isOpen(false)
	, m_success(true)
	, m_needToDelayedCopyResultImageWhenFail(false)
	, m_exifData(nullptr)
	, m_cascadeFrontalFace(cv::makePtr<cv::CascadeClassifier>(params.cascadeFrontalFaceTemplate))
	, m_cascadeEye(cv::makePtr<cv::CascadeClassifier>(params.cascadeEyeTemplate))
{ }

ProcessorImpl::~ProcessorImpl()
{}

void ProcessorImpl::reset()
{
	m_isOpen = false;
	m_success = true;

	m_needToDelayedCopyResultImageWhenFail = false;

	m_filename.clear();
	m_faces.clear();
	m_eyes.clear();

	m_box.Reset();

	m_resizedImage = cv::Mat();
	m_resizedImageGrayscale = cv::Mat();
		
	// NOTE надо сбрасывать классифаеры тоже, ибо после распознования второго изображения данные не сбрасываются
	m_cascadeFrontalFace = cv::makePtr<cv::CascadeClassifier>(m_params.cascadeFrontalFaceTemplate);
	m_cascadeEye = cv::makePtr<cv::CascadeClassifier>(m_params.cascadeEyeTemplate);
}

void ProcessorImpl::Open(const std::string& filename)
{
	reset();

	try
	{
		m_filename = filename;

		m_originImage = cv::imread(m_filename, cv::IMREAD_COLOR);
		if (m_originImage.empty())
		{
			throw std::exception("opencv cant open image file to read");
		}

		m_isOpen = true;

  		if (m_params.needToMakeImageBorder)
  		{
 			// TODO рассчитать необходимое кол-во пикселей для поворота на 45 градусов максимум
  			// TODO учитывать размеры в ширину и высоту, минимальные и относительные значения
  			int borderSize = max(int(m_params.imageBorderMinSize[0]), int(max(m_originImage.cols, m_originImage.rows) * 0.09f));
    		cv::Mat copy = m_originImage;
   			cv::copyMakeBorder(copy, m_originImage, borderSize, borderSize, borderSize, borderSize, cv::BORDER_REPLICATE);
  		}

		applyAndResetExifRotation(filename);
		applyRotation();
			
		// TODO set min size for width and height
		cv::resize(m_originImage, m_resizedImage, cv::Size(), m_box.xScale, m_box.yScale, CV_INTER_LINEAR);

		if (m_params.GUI)
		{
			m_resizedImage.copyTo(m_displayOrigin);
		}
		if (m_params.NeedToHaveDisplayedImage())
		{
			m_resizedImage.copyTo(m_displayResult);
		}
	
	}
	catch (std::exception&)
	{
		m_success = false;

		// TODO use absolute filepath
		std::string msg = std::string("OpenCV: cant open file ") + filename;
		pc::Log::get().Write(msg, pc::LogLevel::Error);

		m_stats.AddFail(stats::Info(filename, msg), stats::FailType::OpenFile);
		throw std::exception(msg.c_str());
	}		
}

void ProcessorImpl::Close()
{
	if (m_isOpen == false)
		return;

	if (m_needToDelayedCopyResultImageWhenFail)
	{
		tryToSaveResultImageToDisplayToFile();
	}

	if (m_success)
	{
		m_stats.AddSuccess(stats::Info(m_filename, ""));
	}
	else
	{
		if (m_params.copyOriginalImageToResultWhenFailed && m_params.saveFiles)
		{
			std::string filenameFull;
			try
			{
				filenameFull = utils::filesystem::getFilename(m_filename);
				utils::filesystem::copyFile(m_filename, m_params.outputDirectory + "/" + filenameFull);
			}
			catch (std::exception&)
			{
				std::string msg = "failed when trying to save original image to result folder when failed (" + filenameFull + ")";

				pc::Log::get().Write(msg, pc::LogLevel::Warning);
				m_stats.AddWarning(stats::Info(m_filename, msg));
			}
		}
	}

	m_isOpen = false;
}

void ProcessorImpl::SaveAs(const std::string& newFilename)
{
	if (m_isOpen == false)
	{
		std::string msg = std::string("trying to save empty file to ") + newFilename;

		pc::Log::get().Write(msg, pc::LogLevel::Error);
		m_stats.AddFail(stats::Info(m_filename, msg), stats::FailType::SaveFile);
		throw std::exception(msg.c_str());
	}

	// process original image
	try
	{
		this->processOriginalImage();
	}
	catch (std::exception&)
	{
		m_success = false;
		m_needToDelayedCopyResultImageWhenFail = true;

		std::string msg = std::string("cant process fullsize original image");

		pc::Log::get().Write(msg, pc::LogLevel::Error);
		m_stats.AddFail(stats::Info(m_filename, msg), stats::FailType::Other);

		throw std::exception(msg.c_str());
	}

	if (m_success == false)
		return;

	// save image if success processed
	try
	{
		bool success = cv::imwrite(newFilename, m_originImage);
		if (!success)
			throw std::exception();

		saveExif(newFilename);
	}
	catch (std::exception&)
	{
		m_success = false;
		m_needToDelayedCopyResultImageWhenFail = true;

		std::string msg = std::string("cant write file ") + m_filename + " to file " + newFilename;

		pc::Log::get().Write(msg, pc::LogLevel::Error);
		m_stats.AddFail(stats::Info(m_filename, msg), stats::FailType::SaveFile);

		throw std::exception(msg.c_str());
	}
}

bool ProcessorImpl::IsValid()
{
	return m_isOpen && m_faces.size() >= 1 && m_eyes.size() >= 2;
}

void ProcessorImpl::ShowOrigin()
{
	if (m_params.GUI == false)
		return;

	static bool isInit = false;

	if (isInit == false)
	{
		// Create a window for display
		cv::namedWindow("Origin Window", cv::WINDOW_AUTOSIZE); 
		cv::moveWindow("Origin Window", 30, 30);

		isInit = true;
	}

	// Show our image inside it.
	cv::imshow("Origin Window", m_displayOrigin); 
	cv::setWindowTitle("Origin Window", m_filename + ", angle: " + (m_params.needEyeHorizontalCorrection ? std::to_string(m_box.angle) : ""));
}

void ProcessorImpl::ShowResult()
{
	if (m_params.GUI == false)
		return;
		
	static bool isInit = false;

	if (isInit == false)
	{
		// Create a window for display.
		cv::namedWindow("Result Window", cv::WINDOW_AUTOSIZE); 
		cv::moveWindow("Result Window", 500, 30);
		isInit = true;
	}

	// Show our image inside it.
	cv::imshow("Result Window", m_displayResult); 
}

void ProcessorImpl::Process(utils::Parameters* params)
{
	// NOTE все распознование образов и поиск границ делаем на уменьшенной
	// копии изображения. А потом масштабируем полученные координаты на исходный размер

	assert(m_isOpen);

	try
	{
		this->processImpl();
	}
	catch (const pc::DetectionException& ex)
	{
		m_success = false;
		m_needToDelayedCopyResultImageWhenFail = true;

		m_stats.AddFail(stats::Info(m_filename, ex.what()), stats::FailType::FaceDetection);
		throw std::exception(ex.what());
	}
	catch (const std::exception&)
	{
		m_success = false;
		m_needToDelayedCopyResultImageWhenFail = true;

		std::string msg = std::string("OpenCV: unknown error during face detection of file ") + m_filename;

		pc::Log::get().Write(msg, pc::LogLevel::Error);
		m_stats.AddFail(stats::Info(m_filename, msg), stats::FailType::FaceDetection);
			
		throw std::exception(msg.c_str());
	}
}

void ProcessorImpl::processImpl()
{				
	cv::cvtColor(m_resizedImage, m_resizedImageGrayscale, cv::COLOR_RGB2GRAY);

	// detect face
	m_cascadeFrontalFace->detectMultiScale(m_resizedImageGrayscale, m_faces,
		1.3, 5, 0, cv::Size(80, 80));
		 		
	int lipsY = findLips();
	int faceBottomY = findFaceBottom(m_resizedImageGrayscale, lipsY);
		
	bool faceDetectionFailed = m_faces.size() < 1;
	bool faceDetectionWarning = m_faces.size() > 1;

	bool needToDrawFaceRegions = m_params.drawFacesRegions || ((faceDetectionWarning || faceDetectionFailed) 
			&& m_params.drawDetectedRegionsWhenException && m_params.NeedToHaveDisplayedImage());

	bool needToDrawEyeRegions = false;
	bool eyesDetectionFailed = true;
	bool eyesDetectionWarning = false;

	std::vector<cv::Point2f> eyeCenters;
	std::vector<cv::Point2f> newEyeCenterPoints;

	if (m_faces.size() >= 1)
	{				
		newEyeCenterPoints = detectEyes();

		eyesDetectionFailed = m_eyes.size() < 2;
		eyesDetectionWarning = m_eyes.size() > 2;

		needToDrawEyeRegions = needToDrawFaceRegions || m_params.drawEyesRegions || ((eyesDetectionWarning || eyesDetectionFailed)
			&& m_params.drawDetectedRegionsWhenException && m_params.NeedToHaveDisplayedImage());

		needToDrawFaceRegions = needToDrawFaceRegions || needToDrawEyeRegions;
			
		for (size_t j = 0u; j < min(2u, m_eyes.size()); ++j)
		{
			cv::Rect& rt = m_eyes[j];
			rt.x += m_faces[0].x;
			rt.y += m_faces[0].y;
			eyeCenters.push_back(cv::Point2f(rt.x + rt.width * 0.5f, rt.y + rt.height * 0.5f));
		}

		if (needToDrawEyeRegions)
		{
			for (size_t j = 0; j < m_eyes.size(); j++)
			{
				const cv::Rect& rt = m_eyes[j];

				static cv::Scalar color(0, 232, 162);
				cv::Scalar currentColor = color / double(m_eyes.size() + 1);
				cv::rectangle(m_displayResult, rt, currentColor * double(m_eyes.size() - (j - 1)), 5);
			}
		}

		if (m_params.drawLineBetweenEyes && eyesDetectionFailed == false && m_params.NeedToHaveDisplayedImage())
		{
			static int thickness = 3;

			assert(eyeCenters.size() >= 2);

			static cv::Scalar lineColor(36, 28, 237);

			// draw line between eyes
			cv::line(m_displayResult, eyeCenters[0], eyeCenters[1], lineColor, thickness);

			if (m_params.drawExtrapolationLineBetweenEyes)
			{
				static cv::Scalar extrapolatedLineColor(163, 73, 164);

				float interpParam = m_params.drawLineBetweenEyesExtrapolationParam;

				cv::Point2f exp1(utils::lerp(eyeCenters[0].x, eyeCenters[1].x, interpParam),
					utils::lerp(eyeCenters[0].y, eyeCenters[1].y, interpParam));

				cv::line(m_displayResult, eyeCenters[0], exp1, extrapolatedLineColor, thickness);

				cv::Point2f exp2(utils::lerp(eyeCenters[1].x, eyeCenters[0].x, interpParam),
					utils::lerp(eyeCenters[1].y, eyeCenters[0].y, interpParam));

				cv::line(m_displayResult, eyeCenters[1], exp2, extrapolatedLineColor, thickness);

					
			}
		}

		if (m_params.drawHorizontalEyesLineOriginal && eyesDetectionFailed == false && m_params.NeedToHaveDisplayedImage())
		{
			static cv::Scalar horizontLineColor(232, 162, 0);

			assert(eyeCenters.size() >= 1);

			float horizont = eyeCenters[0].y;
			cv::line(m_displayResult, cv::Point2f(0.f, horizont), cv::Point2f(float(m_displayResult.cols), horizont), horizontLineColor, 1);
		}
	}
		
	if (needToDrawFaceRegions)
	{
		for (size_t i = 0; i < m_faces.size(); ++i)
		{
			static cv::Scalar color(181, 230, 29);
			cv::Scalar x = color / double(m_faces.size() + 1);
			cv::rectangle(m_displayResult, m_faces[i], x * double(m_faces.size() - (i - 1)), 5);
		}
	}

	if (m_params.needEyeHorizontalCorrection && faceDetectionFailed == false && eyesDetectionFailed == false)
	{			
		if (newEyeCenterPoints.size() == 2)
			m_box.angle = rotate(newEyeCenterPoints);
		else
			m_box.angle = rotate(eyeCenters);
			
		pc::Log::get().Write(std::string("Rotation angle is ") + std::to_string(m_box.angle), pc::LogLevel::Info);

		if (m_params.drawHorizontalEyesLineResult && m_params.NeedToHaveDisplayedImage())
		{
			static cv::Scalar horizontLineColor(0, 252, 255);

			assert(eyeCenters.size() >= 1);
			float horizont = eyeCenters[0].y;
			cv::line(m_displayResult, cv::Point2f(0.f, horizont), cv::Point2f(float(m_displayResult.cols), horizont), horizontLineColor, 1);
		}
	}

	if (faceDetectionWarning)
	{
		std::string msg = "founded more than 1 face (" + std::to_string(m_faces.size()) + ")";
		pc::Log::get().Write(msg, pc::LogLevel::Warning);
		m_stats.AddWarning(stats::Info(m_filename, msg));

		m_needToDelayedCopyResultImageWhenFail = true;
	}

	if (eyesDetectionWarning)
	{
		std::string msg = "founded more than 2 eyes (" + std::to_string(m_eyes.size()) + ")";
		pc::Log::get().Write(msg, pc::LogLevel::Warning);
		m_stats.AddWarning(stats::Info(m_filename, msg));
			
		m_needToDelayedCopyResultImageWhenFail = true;
	}

	if (faceDetectionFailed)
	{
		const char* msg = "founded less than 1 face";
		pc::Log::get().Write(msg, pc::LogLevel::Error);

		throw DetectionException(msg);
	}

	if (eyesDetectionFailed)
	{
		std::string msg = "founded ";
		msg += (m_eyes.size() > 2 ? "more" : "less");
		msg += " than 2 eyes (" + std::to_string(m_eyes.size()) + ")";
		pc::Log::get().Write(msg, pc::LogLevel::Error);

		throw DetectionException(msg.c_str());
	}
		
	// TODO draw only if corresponding flag is turned on
	//lines of lips and face bottom
	cv::line(m_displayOrigin, cv::Point2f((float)m_box.minx, (float)lipsY),
		cv::Point2f(255.f, (float)lipsY), cv::Scalar(255.f, 255.f, 255.f), 2);

	cv::line(m_displayOrigin, cv::Point2f((float)m_box.minx, (float)faceBottomY),
		cv::Point2f(255.f, (float)faceBottomY), cv::Scalar(255.f, 255.f, 255.f), 2);

	if (m_params.needCrop && m_faces.empty() == false)
	{
		contours(faceBottomY);
	}
}

void ProcessorImpl::applyRotation()
{
	// apply rotation image readed from config

	try
	{
		if (m_params.rotateImage)
		{
			int orient = 1;

			int degree = (m_params.rotateDegree >= 0 ? 1 : -1) * std::abs(m_params.rotateDegree) % 360;
			switch (degree)
			{
			case 90:
				orient = 6; break;
			case -90:
				orient = 8; break;
			case 180:
			case -180:
				orient = 3; break;
			case 270:
				orient = 3; applyExifOrientation(orient); orient = 6; break;
			case -270:
				orient = 3; applyExifOrientation(orient); orient = 8; break;
			default:
				break;
			}

			applyExifOrientation(orient);
		}
	}
	catch (const std::exception&)
	{
		const char* msg = "fail when trying to rotation taken from settings file";
		pc::Log::get().Write(msg, pc::LogLevel::Warning);
		m_stats.AddWarning(stats::Info(m_filename, msg));
	}
}

int ProcessorImpl::proofOrientation()
{
	// check if image has invalid meta orientation tag
	// and have been rotated to a wrong degree
	if (m_originImage.rows < m_originImage.cols)
	{
		cv::Mat hsv = m_originImage.clone();

		cv::cvtColor(m_originImage, hsv, CV_BGR2HSV);

		cv::Mat binary;
		cv::inRange(hsv, cv::Scalar(0, 0, 0), cv::Scalar(255, 30, 255), binary);

		int lenght = 0;
		for (int i = 0; i < binary.cols; i++)
		{
			uchar pix = binary.at<uchar>(binary.rows / 2, i);
			if (pix == 255)
			{
				lenght++;
			}
			else
				break;
		}

		if (lenght > binary.rows * 12 / 100)
			return 1; // turn right

		lenght = 0;
		for (int i = binary.cols - 1; i >= 0; i--)
		{
			uchar pix = binary.at<uchar>(binary.rows / 2, i);
			if (pix == 255)
			{
				lenght++;
			}
			else
				break;
		}
		if (lenght > binary.rows * 12 / 100)
			return -1; // turn left			
	}
	return 0;
}

std::vector<cv::Point2f> ProcessorImpl::detectEyes()
{
	static const int faceIndex = 0;
	assert(faceIndex < m_faces.size());

	cv::Mat imageFace(m_resizedImageGrayscale, m_faces[faceIndex]);
	m_cascadeEye->detectMultiScale(imageFace, m_eyes, 1.3, 7, 0, cv::Size(50, 50));

	std::vector<cv::Point2f> points;

	if (m_eyes.empty())
		return points;
		
	for (size_t i = 0; i < m_eyes.size(); ++i)
	{
		cv::Rect eyeRect(
			m_eyes[i].x + m_eyes[i].x * 15 / 100,
			m_eyes[i].y + m_eyes[i].y * 27 / 100,
			m_eyes[i].width - m_eyes[i].x * 15 / 100,
			m_eyes[i].height - m_eyes[i].height * 27 / 100
		);

		eyeRect.x += m_faces[faceIndex].x;
		eyeRect.y += m_faces[faceIndex].y;

		cv::Mat eye = m_resizedImage(eyeRect);
		cv::cvtColor(eye, eye, CV_BGR2HSV);

		cv::Mat binaryEye;
		cv::inRange(eye, cv::Scalar(0, 0, 0), cv::Scalar(254, 255, 20), binaryEye);
			
		cv::dilate(binaryEye, binaryEye, cv::Mat(), cv::Point(-1, -1), 4, 1, 1);

		// Find all contours
		std::vector<std::vector<cv::Point> > contours;
		cv::findContours(binaryEye.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

		// Fill holes in each contour
		cv::drawContours(binaryEye, contours, -1, CV_RGB(255, 255, 255), -1);

		size_t maxSizeID = -1;
		double prevAreaSize = 0.0;
		for (size_t j = 0; j < contours.size(); j++)
		{
			// Blob area
			double area = cv::contourArea(contours[j]);		

			if (prevAreaSize < area)
				maxSizeID = j;
		}
		if (maxSizeID == -1)
			return points;

		cv::Rect rect = cv::boundingRect(contours[maxSizeID]);
		rect.x += eyeRect.x;
		rect.y += eyeRect.y;

		//cv::rectangle(m_displayOrigin, rect, cv::Scalar(0, 0, 255), 2);

		points.push_back(cv::Point(rect.x + rect.width / 2, rect.y + rect.height / 2));
	}

	if (points.size() >= 2)
	{
// 			cv::line(m_displayOrigin, points[0],
// 				points[1], cv::Scalar(0, 0, 255), 2);
	}

	return points;
}

void ProcessorImpl::processOriginalImage()
{
	if (IsValid() && m_params.needEyeHorizontalCorrection)
	{
		cv::Mat rotMat = cv::getRotationMatrix2D(
			cv::Size2f(
				m_originImage.size().width * 0.5f,
				m_originImage.size().height * 0.5f
			),
			m_box.angle, 1.f);

		// TODO maybe need to rotate around face center instead of image center?
		cv::Mat tmp = m_originImage;
		cv::warpAffine(tmp, m_originImage, rotMat, m_originImage.size(), cv::INTER_LINEAR,
			cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
	}

	if (IsValid() && m_params.needCrop)
	{
		crop(m_originImage);
	}
}

float ProcessorImpl::rotate(std::vector<cv::Point2f>& eyeCenters)
{
	assert(eyeCenters.size() >= 2);

	static const float PI = 3.14159265358979323846F;

	// всегда относительно левого глаза ищем угол, иначе изображние может переворачиваться
	int indx2 = eyeCenters[0].x >= eyeCenters[1].x;
	int indx1 = 1 - indx2;

	float angleRad = std::atan2(eyeCenters[indx1].y - eyeCenters[indx2].y,
		eyeCenters[indx1].x - eyeCenters[indx2].x);

	float angle = (angleRad * 180.f) / PI;

	cv::Mat rotationMat = cv::getRotationMatrix2D(
		cv::Size2f(m_resizedImage.size().width * 0.5f, m_resizedImage.size().height * 0.5f),
		angle, 1.f);

	cv::Mat tmp = m_resizedImage;

	// TODO maybe need to rotate around face center instead of image center?
	cv::warpAffine(tmp, m_resizedImage, rotationMat, m_resizedImage.size(), cv::INTER_LINEAR,
		cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

	tmp = m_displayResult;
	// TODO maybe need to rotate around face center instead of image center?
	cv::warpAffine(tmp, m_displayResult, rotationMat, m_displayResult.size(), cv::INTER_LINEAR,
		cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

	// update gray-scale image
	cv::cvtColor(m_resizedImage, m_resizedImageGrayscale, cv::COLOR_RGB2GRAY);

	return angle;
}

void ProcessorImpl::contours(int hight)
{
	if (m_faces.size() < 1 || m_eyes.size() < 2)
		return;

	int newHeight = m_resizedImageGrayscale.cols;

	// TODO в зависимости от настройки cделать по разному вычисления newHeight
	// case 1: нижняя граница определенного лица
	newHeight = m_faces[0].y + m_faces[0].height;
	// case 2: половина расстояния между нижней границы лица и нижней границы нижнего глаза
	{
		int lowEyeBorder = max(m_eyes[0].y + m_eyes[0].height,
			m_eyes[1].y + m_eyes[1].height);

		newHeight = lowEyeBorder + int(float(newHeight - lowEyeBorder) * 0.5f);
	}		

	// обрезаем нижнюю часть (обычно ниже подбородка все срезается, но может и чуть выше)
	cv::Mat tmp = m_resizedImageGrayscale(
		cv::Rect(0, 0, m_resizedImageGrayscale.cols, newHeight));

	cv::Mat threshMat;
	cv::threshold(tmp, threshMat, m_params.siholetteBrightnessThreshold, 255, CV_THRESH_BINARY_INV);
				
	int& minx = m_box.minx;
	int& maxx = m_box.maxx;
	int& miny = m_box.miny;

	minx = threshMat.cols;
	maxx = 0;
	miny = threshMat.rows;

	// ищем мин и макс границы слева, справа, сверху
	for (int i = 0; i < threshMat.rows; ++i)
	{
		size_t elemSize = threshMat.elemSize();
		uchar* r = (uchar*)(threshMat.data + i * elemSize * threshMat.cols);
			
		for (int j = 0; j < threshMat.cols; ++j)
		{
			if (r[j] > 0)
			{
				minx = min(minx, j);
				miny = min(miny, i);

				// break Не делаем из-за того, что нужно найти верхнюю границу
			}
		}

		for (int j = threshMat.cols - 1; j >= 0; --j)
		{
			if (r[j] > 0)
			{
				maxx = max(maxx, j);
				miny = min(miny, i);

				// нашли границу, выходим
				break;
			}
		}
	}

	if (maxx == 0)
		maxx = threshMat.cols;
		
	int stepx = int(float(threshMat.cols) * m_params.cropRelativeScaleX);
	int stepy = int(float(threshMat.rows) * m_params.cropRelativeScaleY);

	minx = max(0, minx - stepx);
	maxx = min(threshMat.cols, maxx + stepx);
	miny = max(0, miny - stepy);

		
	// calc image size with respect of aspect ration
	calcAspectRatio(m_box, m_resizedImageGrayscale, hight);
	horizontalRatio(m_box, m_resizedImageGrayscale);

	assert(minx >= 0 && minx < maxx);
	assert(maxx <= threshMat.cols);
	assert(miny >= 0 && miny < threshMat.rows);
	// check aspect ratio
	//assert(minx + maxx);
		
	m_box.maxy = m_box.miny + int(float(m_box.width()) * m_params.aspectRatio);

	// correcting params if photo's new bottom was detected
	if (m_box.maxy < hight)
	{
		m_box.maxy = hight;
		int H = m_box.height();
		int W = (int)((float)H / m_params.aspectRatio);
		int deltaW = (W - m_box.width()) / 2;
		m_box.minx -= deltaW;
		m_box.maxx += deltaW;
	}
		
	m_resizedImage = m_resizedImage(cv::Rect(minx, miny, m_box.width(), m_box.height()));

	// update grayscale image
	m_resizedImageGrayscale = m_resizedImageGrayscale(cv::Rect(minx, miny, m_box.width(), m_box.height()));
	m_displayResult = m_displayResult(cv::Rect(minx, miny, m_box.width(), m_box.height()));						
}

void ProcessorImpl::crop(cv::Mat& image)
{
	cv::Mat tmp = image;

	float xScaleReverse = 1.0f / m_box.xScale;
	float yScaleReverse = 1.0f / m_box.yScale;

	// crop image
	int x = m_box.minx;
	int y = m_box.miny;

	x = int(float(x) * xScaleReverse);
	y = int(float(y) * yScaleReverse);

	int width = m_box.maxx - m_box.minx;
	int height = int((float)width * m_params.aspectRatio);

	width = int(float(width)	* xScaleReverse);
	height = int(float(height)	* yScaleReverse);

	// TODO проверять выход за границы картинки, причем сохраняя соотношение сторон
	width = min(width, image.cols - x);
	height = min(height, image.rows - y);

	assert(x >= 0 && (x + width) <= image.cols);
	assert(y >= 0 && (y + height) <= image.rows);

	image = tmp(cv::Rect(x, y, width, height));
}

void ProcessorImpl::applyExifOrientation(int orientation)
{
	// 1	top	left side
	// 2	top	right side
	// 3	bottom	right side
	// 4	bottom	left side
	// 5	left side	top
	// 6	right side	top
	// 7	right side	bottom
	// 8	left side	bottom

	//	   1        2       3      4         5            6           7          8
	//	
	//	888888  888888      88  88      8888888888  88                  88  8888888888
	//	88          88      88  88      88  88      88  88          88  88      88  88
	//	8888      8888    8888  8888    88          8888888888  8888888888          88
	//	88          88      88  88
	//	88          88  888888  888888

	enum exifOrient
	{
		topleft = 1,
		topright,
		bottomright,
		bottomleft,
		lefttop,
		righttop,
		rightbottom,
		leftbottom
	};

	cv::Mat copy = m_originImage;

	switch (orientation)
	{
	case topleft:		// 1
		// do nothing
		break;
	case topright:		// 2
		// mirror vertically
		cv::flip(copy, m_originImage, 1);
		break;
	case bottomright:	// 3
		// mirror vertically and horizontally
		cv::flip(copy, m_originImage, -1);
		break;
	case bottomleft:	// 4
		// mirror horizontally
		cv::flip(copy, m_originImage, 0);
		break;

	case righttop:	// 6
		// rotate clockwise
		cv::flip(m_originImage, copy, 0);
		cv::transpose(copy, m_originImage);
		break;
	case leftbottom:	// 8
		// rotate counter-clockwise
		cv::flip(m_originImage, copy, 1);
		cv::transpose(copy, m_originImage);
		break;

	case rightbottom:	// 7
	case lefttop:		// 5
		// transpose
		cv::transpose(copy, m_originImage);
		break;
	default:
		break;
	}
}
	
void ProcessorImpl::applyAndResetExifRotation(const std::string &filename)
{
	// читаем  метаданные exif(в jpeg это jfif). 
	// Находим ориентацию и вращаем изображение
	// попутно сбрасывая этот флаг в новом файле

	try
	{
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.c_str());
		assert(image.get() != 0);

		image->readMetadata();

		m_exifData.reset(new Exiv2::ExifData(image->exifData()));

		if (m_exifData.get() && m_exifData->empty() == false)
		{
			Exiv2::ExifData::iterator it = m_exifData->findKey(Exiv2::ExifKey("Exif.Image.Orientation"));

			if (it != m_exifData->end())
			{
				Exiv2::Exifdatum& data = *it;

				long orientation = data.value().toLong();

				if (orientation != 1)
					pc::Log::get().Write("exif orientation is " + std::to_string(orientation), Info);

				applyExifOrientation(orientation);

				// TODO check that photo is really turned like we want

				// set orientation to default
				data = uint16_t(1);
			}
		}
	}
	catch (Exiv2::AnyError& er)
	{
		// TODO get rid from magic number
		static const int magic_number = 9;

		const char* msg = nullptr;
		if (er.code() == magic_number)
			msg = "Exiv2: cant open file to read exif metadata";
		else
			msg = "Exiv2: cant change rotation metadata or rotate image recording to rotation metadata";

		pc::Log::get().Write(msg, pc::LogLevel::Warning);
		m_stats.AddWarning(stats::Info(m_filename, msg));
	}
	catch (std::exception&)
	{
		const char* msg = "fail when trying to rotation taken from exif metadata";
		pc::Log::get().Write(msg, pc::LogLevel::Warning);
		m_stats.AddWarning(stats::Info(m_filename, msg));
	};
}

void ProcessorImpl::saveExif(const std::string &newFilename)
{
	// TODO need to update width and height in exif metadata?

	if (m_exifData.get())
	{
		try
		{
			// write exif metadata to image
			Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(newFilename.c_str());
			assert(image.get() != nullptr);

			image->setExifData(*m_exifData.get());
			image->writeMetadata();
		}
		catch (std::exception&)
		{
			std::string msg = "failed when trying to write exif metadata to new file " + newFilename;
			pc::Log::get().Write(msg, pc::LogLevel::Warning);
			m_stats.AddWarning(stats::Info(m_filename, msg));
		}
	}
}

void ProcessorImpl::calcAspectRatio(pc::utils::Box& data, cv::Mat& resizedImage, int concurrentHight)
{
	int width = data.maxx - data.minx;
	int height = int(float(width) * m_params.aspectRatio);
		
	bool shrinkVertically = height > resizedImage.rows;
	bool needToShrinkHorizontal = concurrentHight > height;		

	if (shrinkVertically)
	{
		pc::Log::get().Write("Need to shrink image for correct aspect ratio", pc::LogLevel::Info);

		data.miny = 0;
		height = resizedImage.rows;
		int newWidth = int(float(height) / m_params.aspectRatio);

		// no need to abs delta because new width can be bigger or less than old one
		int deltaX = int(float(newWidth - width) * 0.5f);

		data.minx -= deltaX;
		data.maxx += deltaX;

		bool needToShrinkVertically = data.minx < 0 || data.maxx > resizedImage.cols;

		data.minx = max(0, data.minx);
		data.maxx = min(resizedImage.cols, data.maxx);

		// TODO shrink vertically if there is shrink horizontally (needToShrinkVertically is true)
	}
	else
	{
		if (data.miny + height > resizedImage.rows)
			data.miny = resizedImage.rows - height;

		cv::line(m_displayOrigin, cv::Point2f((float)m_box.minx, (float)data.miny),
			cv::Point2f(255.f, (float)data.miny), cv::Scalar(0.f, 0.f, 255.f), 2);
		cv::line(m_displayOrigin, cv::Point2f((float)m_box.minx, (float)data.maxy),
			cv::Point2f(255.f, (float)data.maxy), cv::Scalar(0.f, 0.f, 255.f), 2);
	}		
}

void ProcessorImpl::horizontalRatio(pc::utils::Box& data, cv::Mat& resizedImage)
{
	// TODO
}
	
void ProcessorImpl::tryToSaveResultImageToDisplayToFile()
{
	if (m_params.needToCopyResultImageWhenFailed || m_params.alsoCopyOriginalImageToFailedFolder)
	{
		static const char* suffix = "_result";

		std::string filenameFull;
		std::string filename;
		std::string extension;
			
		filenameFull = pc::utils::filesystem::getFilename(m_filename);
		extension = pc::utils::filesystem::getExtension(filenameFull, &filename);

		pc::utils::filesystem::createDir(m_params.copyResultImageWhenFailedFolder);

		if (m_params.needToCopyResultImageWhenFailed)
		{
			std::string newFilePath = m_params.copyResultImageWhenFailedFolder + "/"
				+ filename + suffix + (extension.size() > 0 ? "." + extension : "");

			try
			{
				std::string msg = "save processed image with highlighted face detection result in " + newFilePath;
				pc::Log::get().Write(msg, pc::LogLevel::Info);

				bool result = cv::imwrite(newFilePath, m_displayResult);

				if (result == false)
					throw std::exception();
			}
			catch (std::exception&)
			{
				std::string msg = "failed when trying to save displayed image to file " + newFilePath;

				pc::Log::get().Write(msg, pc::LogLevel::Warning);
				m_stats.AddWarning(stats::Info(m_filename, msg));
			}
		}

		// TODO just copy file here using system default command
		if (m_params.alsoCopyOriginalImageToFailedFolder)
		{
			std::string originFilePath = m_params.copyResultImageWhenFailedFolder + "/" + filenameFull;

			try
			{
				bool result = pc::utils::filesystem::copyFile(m_filename, originFilePath, false);
			}
			catch (std::exception&)
			{ }
		}
	}
}
	
int ProcessorImpl::findFaceBottom(cv::Mat& grayScaleImg, int startY)
{
	double middleValueForAll = 0;
		
	int startRow = grayScaleImg.rows * 10 / 100;
	int endRow = grayScaleImg.rows * 60 / 100;

	int startCol = grayScaleImg.cols * 20 / 100;
	int endCol = grayScaleImg.cols * 80 / 100;

	//подсчитываем среднюю тональность в сером изображении
	for (int i = startRow; i < endRow; i++)
	{
		for (int j = startCol; j < endCol; j++)
		{		
			try
			{
				uchar bgrPixel = grayScaleImg.at<uchar>(i, j);
				middleValueForAll += bgrPixel;
			}
			catch (std::exception ex)
			{
				//TODO
			}
		}
	}
		
	middleValueForAll /= grayScaleImg.cols * grayScaleImg.rows;


	int newHeight = m_resizedImageGrayscale.cols * 70 / 100;			

	// обрезаем нижнюю часть (обычно ниже подбородка все срезается, но может и чуть выше)
	cv::Mat tmp = m_resizedImageGrayscale(cv::Rect(0, 0, m_resizedImageGrayscale.cols, newHeight));

	//поиск нижней границы выделения лица(копипаст из contours();)
	cv::Mat threshMat;
	cv::threshold(tmp, threshMat, m_params.siholetteBrightnessThreshold, 255, CV_THRESH_BINARY_INV);

	int& minx = m_box.minx;
	int& maxx = m_box.maxx;
	int& miny = m_box.miny;

	minx = threshMat.cols;
	maxx = 0;
	miny = threshMat.rows;

	// ищем мин и макс границы слева, справа, сверху
	for (int i = 0; i < threshMat.rows; ++i)
	{
		size_t xx = threshMat.elemSize();
		uchar* r = (uchar*)(threshMat.data + i * xx * threshMat.cols);

		for (int j = 0; j < threshMat.cols; ++j)
		{
			if (r[j] > 0)
			{
				minx = min(minx, j);
				miny = min(miny, i);

				// break Не делаем из-за того, что нужно найти
				// верхнюю границу
			}
		}

		for (int j = threshMat.cols - 1; j >= 0; --j)
		{
			if (r[j] > 0)
			{
				maxx = max(maxx, j);
				miny = min(miny, i);

				// нашли границу, выходим
				break;
			}
		}
	}

	if (maxx == 0)
		maxx = threshMat.cols;

	int stepx = int(float(threshMat.cols) * m_params.cropRelativeScaleX);
	int stepy = int(float(threshMat.rows) * m_params.cropRelativeScaleY);

	minx = max(0, minx - stepx);
	maxx = min(threshMat.cols, maxx + stepx);
	miny = max(0, miny - stepy);
		
	assert(minx >= 0 && minx < maxx);
	assert(maxx <= threshMat.cols);
	assert(miny >= 0 && miny < threshMat.rows);

	m_box.maxy = m_box.miny + int(float(m_box.width()) * m_params.aspectRatio);

	//поиск подбородка.
	return findChin(middleValueForAll, grayScaleImg.cols / 2, startY, grayScaleImg);				
}

int ProcessorImpl::findChin(double middleValueForAll, int medianaX, int startY, cv::Mat grayScaleImg)
{		
	bool isFound = false;
		
	int hight = 0;
	cv::line(m_displayResult, cv::Point2f((float)m_box.minx, (float)startY),
		cv::Point2f((float)m_box.maxx, (float)startY), cv::Scalar(0.f, 255.f, 0.f), 2);

	//поиск темной области подбородка, которые темнее средней тональности по серому фото		
	for (int i = startY; i < grayScaleImg.rows; i++)
	{		
		cv::line(m_displayOrigin, cv::Point2f(float(medianaX - 1), float(i)),
			cv::Point2f(float(medianaX + 1), float(i)), cv::Scalar(0.f, 0.f, 255.f), 2);

		try
		{
			hight = i;
			uchar bgrPixel = grayScaleImg.at<uchar>(i, medianaX);
			int pxl = bgrPixel;
			if (pxl <= middleValueForAll)
			{
				isFound = true;
				break;
			}
				
		}
		catch (std::exception ex)
		{
			//TODO
		}
	}
				
	int lowPartUnderMouth = grayScaleImg.rows - startY;
	int lowPartUnderFB = -1 * ((grayScaleImg.rows - hight) - lowPartUnderMouth);

	//hardcode factors for finding out situation is needed to correct bottom face position
	double LOW = 0.15;
	double MID = 0.26;
	double HIGH = 0.38;

	double otn = (double)lowPartUnderFB / (double)lowPartUnderMouth;

	if (otn < LOW || otn > HIGH)
	{
		hight = startY + (int)(MID * (double)lowPartUnderMouth);
	}

	hight += (int)((MID * (double)lowPartUnderMouth) / m_params.cropRelativeScaleYDownFactor);
		
	if (hight > m_resizedImage.rows * 90 / 100)
		hight = 0;

	return hight;
}
	
int ProcessorImpl::findLips()
{	
	cv::Mat hsv = m_resizedImage.clone();
	cv::cvtColor(m_resizedImage, hsv, CV_BGR2HSV);		
		
	cv::Mat binary;
	cv::inRange(hsv, cv::Scalar(128, 128, 113), cv::Scalar(255, 230, 172), binary);
				
	cv::Mat res = binary;
	cv::dilate(binary, res, cv::Mat(), cv::Point(-1, -1), 8, 1, 1);
				
	int startY = res.rows * 55 / 100;
	int startX = res.cols * 30 / 100;
	int endX = res.cols - startX;

	int lipsY = 0;

	bool exDetected = false;
	bool isRun = true;

	for (int i = startY; i < res.rows; i++)
	{
		if (!isRun)
			break;

		lipsY = i;
		for (int j = startX; j < endX; j++)
		{
			uchar bgrPixel = res.at<uchar>(i, j);
			if (bgrPixel == 255)
			{					
				exDetected = true;
				break;
			}
			else if (j == endX - 1 && exDetected)
			{
				isRun = false;
			}
		}
	}

	return lipsY;
}

}