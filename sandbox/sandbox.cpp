#define NOMINMAX

#ifdef _WIN32
#include <windows.h>
#include <WinUser.h>
#endif

#include <opencv2/opencv.hpp>

#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include <sstream>

#include "Settings.h"
#include "Fullscreen.h"
#include "AveragingFilter.h"
#include "MedianFilter.h"
#include "HarrisCornerDetection.h"
#include "HoughCornerDetection.h"
#include "ManualCornerDetection.h"
#include "Sound.h"

using namespace cv;
using namespace std;


bool initializeCapture(VideoCapture &capture)
{
	cout << "Opening device...";
	capture.open(CV_CAP_OPENNI_ASUS);
	if (!capture.isOpened())
	{
		cout << "failed" << endl;
		return false;
	}
	cout << "ok" << endl;

	cout << "Setting capture mode...";
	if(!capture.set( CV_CAP_OPENNI_IMAGE_GENERATOR_OUTPUT_MODE, CV_CAP_OPENNI_VGA_30HZ ))
	{
		cout << "failed" << endl;
		return false;
	}

	cout << "ok" << endl;
	cout << "Settings: " << endl <<
            left << setw(20) << "FRAME_WIDTH" << capture.get( CV_CAP_PROP_FRAME_WIDTH ) << endl <<
            left << setw(20) << "FRAME_HEIGHT" << capture.get( CV_CAP_PROP_FRAME_HEIGHT ) << endl <<
            left << setw(20) << "FRAME_MAX_DEPTH" << capture.get( CV_CAP_PROP_OPENNI_FRAME_MAX_DEPTH ) << " mm" << endl <<
            left << setw(20) << "FPS" << capture.get( CV_CAP_PROP_FPS ) << endl;

	return true;
}

bool grabAndStore(VideoCapture &capture, const std::string &prefix = std::string())
{
	if(!capture.grab())
		return false;

	Mat img;
	if(!capture.retrieve(img, CV_CAP_OPENNI_BGR_IMAGE))
		return false;

	if(!imwrite(prefix + "bgr.png", img))
		return false;

	return true;
	if(!capture.retrieve(img, CV_CAP_OPENNI_GRAY_IMAGE))
		return false;

	if(!imwrite(prefix + "gray.png", img))
		return false;

	if(!capture.retrieve(img, CV_CAP_OPENNI_DEPTH_MAP))
		return false;

	if(!imwrite(prefix + "depth.png", img))
		return false;

	return true;
}

bool grabAndStoreMany(VideoCapture &capture, const int n = 30, const std::string &prefix = std::string())
{
	for (int i = 0; i < n; ++i)
	{
		cout << "Snap " << i << endl;
		stringstream ss;
		ss << prefix << i << "_";
		if(!grabAndStore(capture, ss.str()))
		{
			cout << "Failed" << endl;
			return false;
		}

		if( waitKey( 30 ) >= 0 )
			return false;
	}

	return true;
}

bool getHomography(VideoCapture &capture, Mat &homography)
{
	vector<Point2f> calibPoints;
	vector<Point2f> realPoints;

	if (settings.calibrationMode == AUTO_HARRIS) {
		if(!getAutoCalibrationRectangleCornersHarris(capture, calibPoints, realPoints))
		{
			cerr << "Auto harris calibration failed, starting manual calibration" << endl;
			settings.calibrationMode = MANUAL;
		}
	}
	else if (settings.calibrationMode == AUTO_HOUGH) {
		if(!getAutoCalibrationRectangleCornersHough(capture, calibPoints, realPoints))
		{
			cerr << "Auto calibration failed, starting manual calibration" << endl;
			settings.calibrationMode = MANUAL;
		}
	}


	if (settings.calibrationMode == MANUAL) {
		if(!getManualCalibrationRectangleCorners(capture, calibPoints, realPoints))
		{
			cerr << "Manual calibration failed" << endl;
			return false;
		}
	}

	homography = getPerspectiveTransform(calibPoints, realPoints);
	return true;
}

bool getDepthCorrection(VideoCapture &capture, Mat &homography, uint16_t &boxBottomDistanceInMM)
{
	if (settings.sandPlaneDistanceInMM >= 0) {
		cout << "Using manual settings for depth correction" << endl;
		cout << "Sandbox sand level set to: " << settings.sandPlaneDistanceInMM << "mm" << endl;
		boxBottomDistanceInMM = settings.sandPlaneDistanceInMM + settings.maxSandDepthInMM;
		cout << "Sandbox box bottom level estimated at: " << boxBottomDistanceInMM << "mm" << endl;
		return true; // Depth correction already set manually
	}

	if(!capture.grab())
		return false;

	Mat rawDepthInMM;
	if(!capture.retrieve(rawDepthInMM, CV_CAP_OPENNI_DEPTH_MAP))
		return false;

	Mat depthWarpedInMM;
	warpPerspective(rawDepthInMM, depthWarpedInMM, homography, Size(settings.beamerXres, settings.beamerYres));
	
	uint64_t val = 0;
	unsigned int num = 0;
	// Sample a few points to estimate ground level
	for (int x = 0; x < settings.beamerXres; x += settings.beamerXres/16)
	{
		for (int y = 0; y < settings.beamerYres; y += settings.beamerYres/16)
		{
			val += depthWarpedInMM.at<unsigned short>(Point(x,y));
			++num;
		}
	}

	val = val / num;
	cout << "Sandbox sand level estimated at: " << val << "mm" << endl;
	val += settings.maxSandDepthInMM;
	cout << "Sandbox box bottom level estimated at: " << val << "mm" << endl;
	
	boxBottomDistanceInMM = static_cast<uint16_t>(val);

	//TODO: Perspective correction doesn't help with 3D issues. Should use 4 edge points for reconstructing 3d base plane but that can wait.

	return true;
}

/**
 * @brief Combined normalize and colorization of the depth map.
 * Somewhat optimized version of normalization and colorization (single loop, less branches etc.) for
 * 7% more overall performance with somewhat reduced redability....
 *
 * @param depthWarped Source depth map
 * @param depthWarpedNormalized Destination image for colorized result.
 * @param boxBottomDistanceInMM Distance to box bottom from sensor in mm
 * @param colorBand Color band to use for mapping, empty for grayscale.
 * @return True if successfull
 */
bool sandboxNormalizeAndColor(Mat &depthWarped, Mat& depthWarpedNormalized, uint16_t boxBottomDistanceInMM, Mat colorBand)
{
	const bool colored = (colorBand.data != NULL);

	const size_t rows = depthWarped.rows;
	const size_t cols = depthWarped.cols;
	depthWarpedNormalized = Mat(rows, cols, colored ? colorBand.type() : depthWarped.type());

	const uint16_t topOrig = boxBottomDistanceInMM - settings.maxSandDepthInMM - settings.maxSandHeightInMM;
	const uint16_t range = boxBottomDistanceInMM - topOrig;

	if (colored)
	{
		uint16_t value;
		uint8_t *target = depthWarpedNormalized.data;
		for(uint16_t *current = (uint16_t*)depthWarped.data; current != (uint16_t*)depthWarped.dataend; ++current)
		{
			// Clip
			value = boxBottomDistanceInMM - std::min<uint16_t>(boxBottomDistanceInMM, std::max<uint16_t>(topOrig + 1, *current));
			memcpy(target, colorBand.data + (value * 3), 3);

			target += 3;
		}
	}
	else
	{
		uint16_t value;
		uint16_t *target = (uint16_t*) depthWarpedNormalized.data;
		const uint16_t scale = std::numeric_limits<uint16_t>::max() / range;

		for(uint16_t *current = (uint16_t*)depthWarped.data; current != (uint16_t*)depthWarped.dataend; ++current)
		{
			// Clip
			value = boxBottomDistanceInMM - std::min<uint16_t>(boxBottomDistanceInMM, std::max<uint16_t>(topOrig + 1, *current));
			*target = value * scale;

			++target;
		}
	}


	return true;
}

bool parseSettingsFromCommandline(int argc, char **argv, bool &quit)
{
	const char *keys =
		"{full|fullscreen|true|If true fullscreen is used for output window}"
		"{m|monitor|0|Monitor to use for fullscreen}"
		"{e|enumerate|false|Enumerate monitors and quit}"
		"{d|depth|90|Maximum sand depth below plane in mm}"
		"{t|top|200|Maximum sand height above plane in mm}"
		"{g|ground|-1|Distance of the sand plane to the sensor. (-1 for automatic calibration.)}"
		"{c|colors|NONE|Prefix for colorbands to use for coloring (-c col -> col0.png - col9.png). NONE for greyscale}"
		"{b|bgr|false|If true BGR color view is displayed}"
		"{cal|calibration|1|Calibration mode. (0 for manual, 1 for hough circles, 2 for harris corners)}"
		"{avgd|averagingdepth|0|Averaging filter depth in frames. (0 = off)}"
		"{avgs|averagingstepsize|1|Averaging filter step size.}"
		"{medd|mediandepth|0|Median filter depth in frames. (0 = off)}"
		"{meds|medianstepsize|1|Median filter step size.}"
		"{east|eastereggshhhh|NONE|Nothing really, doesn't take the name without extension for a small png and a wav either}"
		"{h|help|false|Print help}";

	CommandLineParser clp(argc, argv, keys);

	if (clp.get<bool>("h"))
	{
		cout << "Usage: " << argv[0] << " [options]" << endl;
		clp.printParams();
		quit = true;
		return true;
	}

	if (clp.get<bool>("e"))
	{
		if (printMonitors())
		{
			quit = true;
			return true;
		}
		else
		{
			quit = true;
			return false;
		}
	}

	settings.fullscreen = clp.get<bool>("full");
	settings.monitor = clp.get<int>("m");
	settings.displayBGR = clp.get<bool>("b");

	settings.sandPlaneDistanceInMM = clp.get<int>("g");
	settings.maxSandDepthInMM = clp.get<int>("d");
	settings.maxSandHeightInMM = clp.get<int>("t");

	settings.colorFile = clp.get<std::string>("c");
	if (settings.colorFile == "NONE") settings.colorFile.clear(); // No color file

	settings.treasureFile = clp.get<std::string>("east");
	if (settings.treasureFile == "NONE")
	{
		settings.treasureFile.clear(); // No treasure file
	}
	else
	{
		settings.treasureSound = settings.treasureFile + ".wav";
		settings.treasureFile = settings.treasureFile + ".png";
	}
	


	if (!getMonitorRect(settings.monitor, settings.monitorRect))
	{
		cerr << "Failed to get information on monitor " << settings.monitor << endl;
		cerr << "Use the -e option to enumerate available monitors" << endl;
		quit = true;
		return false;
	}

	settings.beamerXres = settings.monitorRect.right - settings.monitorRect.left;
	settings.beamerYres = settings.monitorRect.bottom - settings.monitorRect.top;
	settings.calibrationMode = (CalibrationModes)clp.get<int>("cal");

	if (settings.calibrationMode < 0 || settings.calibrationMode >= CALIBRATION_MODE_MAX)
	{
		cerr << "Unknown calibration mode " << settings.calibrationMode << endl;
		quit = true;
		return false;
	}

	settings.averagingDepth = static_cast<size_t>(std::max(0, clp.get<int>("avgd")));
	settings.averagingStepsize = static_cast<size_t>(std::max(1, clp.get<int>("avgs")));

	if (settings.averagingDepth > 0)
	{
		cout << "Using average filter with depth " << settings.averagingDepth
			 << " and stepsize " << settings.averagingStepsize << endl;
	}

	settings.medianDepth = static_cast<size_t>(std::max(0, clp.get<int>("medd")));
	settings.medianStepsize = static_cast<size_t>(std::max(1, clp.get<int>("meds")));

	if (settings.medianDepth > 0)
	{
		if (settings.averagingDepth > 0)
		{
			cout << "WARNING: Median and averaging filter cannot be used at the same time. Disabled median." << endl;
			settings.medianDepth = 0;
		}
		else
		{
			cout << "Using median filter with depth " << settings.medianDepth
				 << " and stepsize " << settings.medianStepsize << endl;
		}
	}

	quit = false;
	return true;
}

bool loadColorFile(const std::string &colorFile, Mat &colors)
{
	if (colorFile.empty()) {
		cout << "No colorband given" << endl;
		colors = Mat(); // Empty material
		return true;
	}

	cout << "Loading color file " << colorFile << "...";

	colors = cv::imread(colorFile, 1);
	if (colors.data == NULL) {
		cout << "failed" << endl;
		return false;
	}

	if(colors.rows != 1)
	{
		cout << "failed" << endl << "The colorband must only have one row" << endl;
		return false;
	}

	//TODO: Add ability to scale this if needed
	const uint16_t fullRange = settings.maxSandDepthInMM + settings.maxSandHeightInMM;
	if (colors.cols != fullRange)
	{
		cout << "failed" << endl << "For given sand depth and height limits colorband has to cover " << fullRange << "mm, covers " << colors.rows << "mm (px) as columns." << endl;
		return false;
	}
	
	cout << "ok" << endl;
	return true;
}

class Stopwatch {
	const double m_freq;
	int64 m_startTicks;

public:
	Stopwatch()
		: m_freq(cv::getTickFrequency())
		, m_startTicks(cv::getTickCount()) {}

	double getTime()
	{
		return static_cast<double>(cv::getTickCount() - m_startTicks) / m_freq;
	}

	double reset()
	{
		const int64 cur = cv::getTickCount();
		double result =  static_cast<double>(cv::getTickCount() - m_startTicks) / m_freq;
		m_startTicks = cur;

		return result;
	}

};

void renderInfo(const std::string &window, Mat &infoMat, double fps)
{
	memset(infoMat.data, 255, infoMat.dataend - infoMat.data);

	putText(infoMat, "Select this window and press ESC to quit", Point(5,15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,0,255,0));
	stringstream ss;
	ss.precision(5);
	if (fps <= 0) ss << "FPS: ?";
	else ss << "FPS: " << fps;

	putText(infoMat, ss.str(), Point(5,100), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0,0,0,0));

	imshow(window, infoMat);
}

bool huntTreasure(Mat &depthMap, Mat &treasure, Mat &display, int left, int top, int depth = 50, double threshold = 0.7)
{
	const int right = left + treasure.cols;
	const int bottom = top + treasure.rows;

	const uint16_t topOrig = settings.boxBottomDistanceInMM - settings.maxSandDepthInMM - settings.maxSandHeightInMM;

	assert(depthMap.type() == CV_16UC1);
	assert(display.type() == CV_8UC3);
	assert(display.type() == CV_8UC3);

	size_t foundPx = 0;

	for (int x = left; x < right; ++x)
	{
		for (int y = top; y < bottom; ++y)
		{
			const uint16_t val = settings.boxBottomDistanceInMM - std::min<uint16_t>(settings.boxBottomDistanceInMM, std::max<uint16_t>(topOrig + 1, depthMap.at<uint16_t>(Point(x,y))));
			if (val <= depth)
			{
				// Found pixel of treasure
				Vec3b pval(treasure.at<Vec3b>(Point(x - left, y - top)));
				if (pval != Vec3b(255,0,255))
					display.at<Vec3b>(Point(x, y)) = pval;

				++foundPx;
			}
		}
	}

	const double allPx = treasure.cols * treasure.rows;
	return (foundPx / allPx >= threshold);
}

void invertMat(Mat &mat)
{
	for (uchar *cur = mat.data; cur < mat.dataend; ++cur)
	{
		*cur = 255 - *cur;
	}
}

int main( int argc, char* argv[] )
{
	bool quit;
	if(!parseSettingsFromCommandline(argc, argv, quit))
		return 1;
	
	if (quit)
		return 0;

	vector<Mat> colors;
	for (size_t i = 0; i < 10; ++i)
	{
		stringstream ss;
		ss << settings.colorFile << i << ".png";

		Mat cf;
		if(loadColorFile(ss.str(), cf))
			colors.push_back(cf);
	}

	if (colors.empty())
	{
		cout << "Displaying continous grey scale" << endl;
		colors.push_back(Mat());
	}

	VideoCapture capture;
	if (!initializeCapture(capture))
		return 1;

	//grabAndStoreMany(capture, 10, "white");

	Mat homography;
	if(!getHomography(capture, homography))
		return 1;

	if(!getDepthCorrection(capture, homography, settings.boxBottomDistanceInMM))
		return 1;

	const std::string BGR_IMAGE = "Bgr Image";
	const std::string BGR_WARPED = "Warped BGR Image";
	const std::string SAND_NORMALIZED = "Normalized Sand Image";
	const std::string INFO_VIEW = "Runtime information";

	if (settings.displayBGR) {
		namedWindow(BGR_IMAGE, CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
		namedWindow(BGR_WARPED, CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
	}

	namedWindow(SAND_NORMALIZED);

	if (settings.fullscreen)
	{
		if(!fullScreen(settings.monitorRect, SAND_NORMALIZED))
		{
			cerr << "Failed to fullscreen output window on monitor " << settings.monitor << endl;
		}
		else
		{
			cout << "Entered fullscreen on monitor " << settings.monitor << endl;
		}
	}

	if (!settings.displayBGR)
	{
		cout << "BGR color view disabled, enable with -b if needed" << endl;
	}

	cout << "Enter mainloop" << endl;

	// Define loop variables here to prevent unneeded allocations
	Mat infoMat(200,400, CV_8UC3);

	// Render dummy info
	renderInfo(INFO_VIEW, infoMat, -1);

	Mat depthMap;
	Mat depthWarped;
	Mat depthWarpedNormalized;

	Mat bgrImage;
	Mat bgrWarped;

	AveragingFilter avgFilter(settings.averagingDepth, settings.averagingStepsize);
	MedianFilter medFilter(settings.medianDepth, settings.medianStepsize);

	Stopwatch timer;
	size_t frames = 0;

	Mat filteredDepthmap;

	Mat treasure;
	int treasureX;
	int treasureY;
	bool foundTreasure = false;
	size_t winningShuffle = 0;

	size_t currentColor = 0;

	if (!settings.treasureFile.empty())
	{
		treasure = imread(settings.treasureFile);
		if (treasure.type() != CV_8UC3)
		{
			cerr << "Indegestible" << endl;
			settings.treasureFile = std::string();
		}
		srand((unsigned int)(cv::getTickCount() & 0xFFFFFFFF));
		treasureX = rand() % (settings.beamerXres - treasure.cols);
		treasureY = rand() % (settings.beamerYres - treasure.rows);

		cout << "Find the treasure " << endl;
		// DEBUGGING ONLY cout << treasureX << " " << treasureY << endl;
	}

	for (;;)
	{
		if (!capture.grab())
		{
			cerr << "Failed to grab frame" << endl;
			return 1;
		}

		if (timer.getTime() > 2.)
		{
			// Update info display every 2 seconds
			const double took = timer.reset();
			const double fps = frames / took;
			frames = 0;
			renderInfo(INFO_VIEW, infoMat, fps);
		}

		if (settings.displayBGR) {
			if (!capture.retrieve(bgrImage, CV_CAP_OPENNI_BGR_IMAGE))
			{
				cerr << "Failed to retrieve" << endl;
				return 1;
			}

			warpPerspective(bgrImage, bgrWarped, homography, Size(settings.beamerXres, settings.beamerYres));
			
			imshow(BGR_WARPED, bgrWarped);
			imshow(BGR_IMAGE, bgrImage);
		}

		if (!capture.retrieve(depthMap, CV_CAP_OPENNI_DEPTH_MAP))
		{
			cerr << "Failed to retrieve valid depth mask" << endl;
			return 1;
		}


		if (settings.averagingDepth > 0)
		{
			avgFilter.addFrame(depthMap);
			avgFilter.getFiltered(filteredDepthmap);
		}
		else if (settings.medianDepth > 0)
		{
			medFilter.addFrame(depthMap);
			medFilter.getFiltered<uint16_t>(filteredDepthmap);
		}
		else
		{
			filteredDepthmap = depthMap;
		}

		warpPerspective(filteredDepthmap, depthWarped, homography, Size(settings.beamerXres, settings.beamerYres));

		sandboxNormalizeAndColor(depthWarped, depthWarpedNormalized, settings.boxBottomDistanceInMM, colors[currentColor]);

		if (!settings.treasureFile.empty())
		{
			if(huntTreasure(depthWarped, treasure, depthWarpedNormalized, treasureX, treasureY))
			{
				if (!foundTreasure)
				{
					cout << "You found the treasure, press t to hide it again" << endl;
					foundTreasure = true;
					winningShuffle = 30;
					if (!settings.treasureFile.empty())
						doPlaySound(settings.treasureSound);
				}
			}
		}

		if (winningShuffle > 0)
		{
			if (winningShuffle % 10 < 5)
			{
				invertMat(depthWarpedNormalized);
			}
			--winningShuffle;
		}

		imshow(SAND_NORMALIZED, depthWarpedNormalized);

		const int key = waitKey(1);  // Needed for event processing in OpenCV
		if (key == 't')
		{
			treasureX = rand() % (settings.beamerXres - treasure.cols);
			treasureY = rand() % (settings.beamerYres - treasure.rows);
			cout << "Find the treasure ";
			// DEBUGGING ONLY!!!! // cout << treasureX << " " << treasureY << endl;

			foundTreasure = false;
		}
		else if (key >= '0' && key <= '9')
		{
			size_t num = key - '0';
			if (num < colors.size())
			{
				cout << "Switching to color profile " << num << endl;
				currentColor = num;
			}
		}
		else if( key == 27 )
		{
			break;
		}

		++frames;
	}

	return 0;
}



