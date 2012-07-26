#include "stdafx.h"
#define NOMINMAX


#include <opencv2/opencv.hpp>

#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

#include <sstream>

using namespace cv;
using namespace std;

//
//-- Constants
//

struct Settings {
	// Command line settings
	int monitor;
	bool fullscreen;
	bool displayBGR;
	int sandPlaneDistanceInMM;
	std::string colorFile;

	// Derived from settings
	RECT monitorRect;

	int beamerXres;
	int beamerYres;

	int maxSandDepthInMM;
	int maxSandHeightInMM;

	uint16_t boxBottomDistanceInMM;

} settings;


#ifdef _WIN32

BOOL CALLBACK MonitorEnumProc(
	__in  HMONITOR hMonitor,
	__in  HDC hdcMonitor,
	__in  LPRECT lprcMonitor,
	__in  LPARAM dwData
	)
{
	std::vector<RECT>* monitors = (std::vector<RECT>*)dwData;

	monitors->push_back(*lprcMonitor);

	return TRUE;
}

bool enumMonitors(std::vector<RECT> &rect)
{
	cout << "Enumerating monitors...";
	if(EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM) &rect) == 0)
	{
		cout << "failed" << endl;
		return false;
	}
	cout << "ok" << endl;
	return true;
}

bool fullScreen(RECT &monitor, const std::string windowName)
{
	HWND hwnd = FindWindowA(NULL, windowName.c_str());
	if(hwnd == NULL) {
		return false;
	}

	SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
	SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

	SetWindowPos(hwnd, HWND_TOPMOST, monitor.left, monitor.top, monitor.right - monitor.left, monitor.bottom - monitor.top, SWP_SHOWWINDOW);

	ShowWindow(hwnd, SW_MAXIMIZE);

	return true;
}

bool getMonitorRect(int monitor, RECT &monitorRect)
{
	std::vector<RECT> rect;
	if(!enumMonitors(rect))
		return false;

	if (monitor < 0 || monitor >= rect.size())
		return false;

	monitorRect = rect[monitor];

	return true;
}

bool printMonitors()
{
	std::vector<RECT> monitors;
	if(!enumMonitors(monitors))
	{
		return false;
	}

	cout << endl;

	size_t num = 0;
	for (std::vector<RECT>::iterator it = monitors.begin();
		it != monitors.end();
		++it)
	{
		RECT *monitor = &(*it);

		cout << "Monitor " << num << ":" << endl;
		cout << "Left/Right: " << monitor->left << " - " << monitor->right << endl;
		cout << "Top/Bottom: " << monitor->top << " - " << monitor->bottom << endl << endl;

		++num;
	}

	return true;
}
#else

bool enumMonitors(std::vector<RECT> &rect) {
	/* Not implemented for other platforms */
	return false;
}

bool fullScreen(RECT &monitor, const std::string windowName) {
	/* Not implemented for other platforms */
	return false;
}

bool printMonitors() {
	/* Not implemented for other platforms */ 
	return false;
}

bool getMonitorRect(int monitor, RECT &monitorRect) {
	/* Not implemented for other platforms */ 
	return false;
}

#endif

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



void calibrationMouseClickHandler(int event, int x, int y, int flags, void* ptr)
{
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		vector<Point2f> *calibPoints = (vector<Point2f>*)ptr;
		cout << "Calibration point at x: " << x << " y: " << y << endl;
		calibPoints->push_back(Point2f(x,y));
	}
}

bool getManualCalibrationRectangleCorners(VideoCapture &capture, vector<Point2f> &calibPoints)
{
	calibPoints.clear();
	const std::string CALIB_BGR_WND = "Calibration";

	namedWindow(CALIB_BGR_WND, CV_WINDOW_KEEPRATIO);
	setMouseCallback(CALIB_BGR_WND, calibrationMouseClickHandler, &calibPoints);
	
	cout << "Get calibration points" << endl;
	while (calibPoints.size() < 4)
	{
		Mat bgrImage;
		if (!capture.grab())
		{
			cerr << "Failed to grab frame" << endl;
			return false;
		}

		if (!capture.retrieve(bgrImage, CV_CAP_OPENNI_BGR_IMAGE))
		{
			cerr << "Failed to retrieve" << endl;
			return false;
		}

		
		for (vector<Point2f>::iterator it = calibPoints.begin();
			it != calibPoints.end();
			++it)
		{
			circle(bgrImage, Point( it->x, it->y ), 5,  Scalar(0,0,255,0), 2, 8, 0 );
		}

		putText(bgrImage, "Please click the edges of the beamer area clockwise starting top left", Point(5,15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,0,255,0));
		imshow(CALIB_BGR_WND, bgrImage);

		if( waitKey( 30 ) >= 0 )
			break;

	}

	cv::destroyWindow(CALIB_BGR_WND);

	if (calibPoints.size() < 4)
	{
		cout << "Aborted calibration" << endl;
		return false;
	}

	return true;
}

bool getHomography(VideoCapture &capture, Mat &homography)
{
	vector<Point2f> calibPoints;
	if(!getManualCalibrationRectangleCorners(capture, calibPoints))
	{
		cerr << "Calibration failed" << endl;
		return false;
	}

	vector<Point2f> realPoints;

	realPoints.push_back(Point2f(0,0)); // Top left
	realPoints.push_back(Point2f(settings.beamerXres,0)); // Top right
	realPoints.push_back(Point2f(settings.beamerXres, settings.beamerYres)); // Bottom right
	realPoints.push_back(Point2f(0, settings.beamerYres)); // Bottom left

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


/**
 * @brief Custom MedianFilter class
 */

class MedianFilter {
private:
	const size_t m_width;
	const size_t m_height;
	const size_t m_depth;
	const size_t m_skip;

	vector<cv::Mat> m_state;
	size_t m_insertionPoint;

public:
	MedianFilter(size_t width, size_t height, size_t depth, size_t skip)
		: m_width(width)
		, m_height(height)
		, m_depth(depth)
		, m_skip(skip)
		, m_insertionPoint(0)
	{
		m_state.reserve(m_depth);
	}
	
	void addImage(cv::Mat &mat)
	{
		// Insert into ring buffer
		if (m_state.size() < m_depth)
			m_state.push_back(mat);
		else
			m_state[m_insertionPoint] = mat;

		++m_insertionPoint;

		if (m_insertionPoint >= m_depth)
			m_insertionPoint = 0;
	}

	void getMedianImage(cv::Mat &mat)
	{
		// Calculate Median for images in skip intervals
		vector<uint16_t> buf;
		buf.resize(m_depth / m_skip);

		mat = cv::Mat(m_height, m_width, CV_16UC1);

		const size_t curDepth = m_state.size();

		for (size_t y = 0; y < m_height; ++y)
		{
			for (size_t x = 0; x < m_width; ++x)
			{
				size_t n = 0;
				for (size_t i = 0; i < curDepth; i += m_skip)
				{
					buf[n] = m_state[i].at<uint16_t>(Point(x, y));
					++n;
				}
				sort(buf.begin(), buf.end());

				mat.at<uint16_t>(Point(x, y)) = buf[buf.size() / 2];
			}
		}
	}
};

bool parseSettingsFromCommandline(int argc, char **argv, bool &quit)
{
	const char *keys =
		"{f|fullscreen|true|If true fullscreen is used for output window}"
		"{m|monitor|0|Monitor to use for fullscreen}"
		"{e|enumerate|false|Enumerate monitors and quit}"
		"{d|depth|90|Maximum sand depth below plane in mm}"
		"{t|top|200|Maximum sand height above plane in mm}"
		"{g|ground|-1|Distance of the sand plane to the sensor. (-1 for automatic calibration.)}"
		"{c|colors|NONE|Colorband to use for coloring. NONE for greyscale}"
		"{b|bgr|false|If true BGR color view is displayed}"
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

	settings.fullscreen = clp.get<bool>("f");
	settings.monitor = clp.get<int>("m");
	settings.displayBGR = clp.get<bool>("b");

	settings.sandPlaneDistanceInMM = clp.get<int>("g");
	settings.maxSandDepthInMM = clp.get<int>("d");
	settings.maxSandHeightInMM = clp.get<int>("t");

	settings.colorFile = clp.get<std::string>("c");
	if (settings.colorFile == "NONE") settings.colorFile.clear(); // No color file


	if (!getMonitorRect(settings.monitor, settings.monitorRect))
	{
		cerr << "Failed to get information on monitor " << settings.monitor << endl;
		cerr << "Use the -e option to enumerate available monitors" << endl;
		quit = true;
		return false;
	}

	settings.beamerXres = settings.monitorRect.right - settings.monitorRect.left;
	settings.beamerYres = settings.monitorRect.bottom - settings.monitorRect.top;

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

	cout << "Loading color file " << colorFile << "..." << endl;

	colors = cv::imread(colorFile, 1);
	if (colors.data == NULL) {
		cout << "failed" << endl;
		return false;
	}

	if(colors.rows != 1)
	{
		cout << "The colorband must only have one row" << endl;
		return false;
	}

	//TODO: Add ability to scale this if needed
	const uint16_t fullRange = settings.maxSandDepthInMM + settings.maxSandHeightInMM;
	if (colors.cols != fullRange)
	{
		cout << "For given sand depth and height limits colorband has to cover " << fullRange << "mm, covers " << colors.rows << "mm (px) as columns." << endl;
		return false;
	}
	
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

int main( int argc, char* argv[] )
{
	bool quit;
	if(!parseSettingsFromCommandline(argc, argv, quit))
		return 1;
	
	if (quit)
		return 0;
	
	Mat colors;
	if (!loadColorFile(settings.colorFile, colors))
		return 1;

	if (colors.data == NULL)
	{
		cout << "Displaying continous grey scale" << endl;
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
	const std::string SAND_NORMALIZED = "Normalized Sand";
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

	//MedianFilter mf(settings.beamerXres, settings.beamerYres, 30, 10);
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

	Stopwatch timer;
	size_t frames = 0;

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

		warpPerspective(depthMap, depthWarped, homography, Size(settings.beamerXres, settings.beamerYres));

		//mf.addImage(depthWarped);

		//Mat depthWarpFiltered;
		//mf.getMedianImage(depthWarpFiltered);

		sandboxNormalizeAndColor(depthWarped, depthWarpedNormalized, settings.boxBottomDistanceInMM, colors);

		imshow(SAND_NORMALIZED, depthWarpedNormalized);

		if( waitKey( 1 ) >= 0 ) // Needed for event processing in OpenCV
			break;

		++frames;
	}

	return 0;
}



