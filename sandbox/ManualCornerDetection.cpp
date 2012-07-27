#include "ManualCornerDetection.h"

using namespace cv;
using namespace std;

#include "Settings.h"



void calibrationMouseClickHandler(int event, int x, int y, int flags, void* ptr)
{
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		vector<Point2f> *calibPoints = (vector<Point2f>*)ptr;
		cout << "Calibration point at x: " << x << " y: " << y << endl;
		calibPoints->push_back(Point(x,y));
	}
}


bool getManualCalibrationRectangleCorners(VideoCapture &capture, vector<Point2f> &calibPoints, vector<Point2f> &realPoints)
{
	realPoints.push_back(Point(0,0)); // Top left
	realPoints.push_back(Point(settings.beamerXres,0)); // Top right
	realPoints.push_back(Point(settings.beamerXres, settings.beamerYres)); // Bottom right
	realPoints.push_back(Point(0, settings.beamerYres)); // Bottom left

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
			circle(bgrImage, Point2f( it->x, it->y ), 5,  Scalar(0,0,255,0), 2, 8, 0 );
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