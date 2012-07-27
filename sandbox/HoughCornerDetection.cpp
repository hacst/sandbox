#include "HoughCornerDetection.h"

using namespace cv;
using namespace std;

#include "Settings.h"
#include "Fullscreen.h"

bool getAutoCalibrationRectangleCornersHough(VideoCapture &capture, vector<Point2f> &calibPoints, vector<Point2f> &realPoints)
{
	calibPoints.clear();
	const std::string CALIB_BGR_WND = "Auto Calibration";
	const std::string CALIB_BGR_WND_MONITOR = "Auto Calibration Monitor";

	namedWindow(CALIB_BGR_WND);
	namedWindow(CALIB_BGR_WND_MONITOR);

	const int CALIB_CIRCLE_RADIUS = 150;
	const int CALIB_CIRCLE_DISTANCE_BORDER = 25;

	cv::Point circle_top_left = cv::Point(CALIB_CIRCLE_RADIUS + CALIB_CIRCLE_DISTANCE_BORDER, CALIB_CIRCLE_RADIUS + CALIB_CIRCLE_DISTANCE_BORDER); // top left
	cv::Point circle_top_right = cv::Point(settings.beamerXres - (CALIB_CIRCLE_RADIUS + CALIB_CIRCLE_DISTANCE_BORDER), CALIB_CIRCLE_RADIUS + CALIB_CIRCLE_DISTANCE_BORDER); // top right
	cv::Point circle_bottom_left = cv::Point(CALIB_CIRCLE_RADIUS + CALIB_CIRCLE_DISTANCE_BORDER, settings.beamerYres - (CALIB_CIRCLE_RADIUS + CALIB_CIRCLE_DISTANCE_BORDER)); // bottom left
	cv::Point circle_bottom_right = cv::Point(settings.beamerXres - (CALIB_CIRCLE_RADIUS + CALIB_CIRCLE_DISTANCE_BORDER), settings.beamerYres -(CALIB_CIRCLE_RADIUS + CALIB_CIRCLE_DISTANCE_BORDER)); // bottom right

	realPoints.push_back(Point2f(circle_top_left)); // Top left
	realPoints.push_back(Point2f(circle_top_right)); // Top right
	realPoints.push_back(Point2f(circle_bottom_right)); // Bottom right
	realPoints.push_back(Point2f(circle_bottom_left)); // Bottom left

	if (settings.fullscreen)
	{
		if(!fullScreen(settings.monitorRect, CALIB_BGR_WND))
		{
			cerr << "Failed to fullscreen output window on monitor " << settings.monitor << endl;
		}
		else
		{
			cout << "Entered fullscreen on monitor " << settings.monitor << endl;
		}
	}

	Mat m(settings.beamerYres, settings.beamerXres, CV_8UC1);
	memset(m.data, 0xFF, m.dataend - m.data);
	Mat m2(settings.beamerYres, settings.beamerXres, CV_8UC1);
	memset(m2.data, 0xFF, m2.dataend - m2.data);
	Mat m3(settings.beamerYres, settings.beamerXres, CV_8UC1);
	memset(m3.data, 0xFF, m3.dataend - m3.data);
	Mat m4(settings.beamerYres, settings.beamerXres, CV_8UC1);
	memset(m4.data, 0xFF, m4.dataend - m4.data);

	cv::circle(m, circle_top_left, CALIB_CIRCLE_RADIUS, cv::Scalar(0), CV_FILLED);

	imshow(CALIB_BGR_WND, m);

	for(;;)
	{
		waitKey(100);

		if(!capture.grab())
		{
			cerr << "Failed to grab" << endl;
			cv::destroyWindow(CALIB_BGR_WND);
			cv::destroyWindow(CALIB_BGR_WND_MONITOR);
			return 1;
		}

		Mat mat, mat2;
		if(!capture.retrieve(mat, CV_CAP_OPENNI_BGR_IMAGE))
		{
			cerr << "Failed to retrieve" << endl;
			cv::destroyWindow(CALIB_BGR_WND);
			cv::destroyWindow(CALIB_BGR_WND_MONITOR);
			return 1;
		}
		
		putText(mat, "Please make sure that the sand surface is level", Point(5,15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,0,255,0));
		imshow(CALIB_BGR_WND_MONITOR, mat);

		// Convert to gray
		cvtColor(mat, mat2, CV_BGR2GRAY);

		// Reduce the noise so we avoid false circle detection
		GaussianBlur( mat2, mat2, Size(9, 9), 2, 2 );

		vector<Vec3f> circles;

		// Apply the Hough Transform to find the circles
		HoughCircles( mat2, circles, CV_HOUGH_GRADIENT, 1, mat2.rows/8, 200, 100, 0, 0 );

		if (circles.size() == 1) {
			cerr << "Circle detected!" << endl;

			// Draw the circles detected
			Point center(cvRound(circles[0][0]), cvRound(circles[0][1]));
			int radius = cvRound(circles[0][2]);

			// circle center
			circle( mat, center, 3, Scalar(0,255,0), -1, 8, 0 );

			// circle outline
			circle( mat, center, radius, Scalar(0,0,255), 3, 8, 0 );

			imshow(CALIB_BGR_WND_MONITOR, mat);

			waitKey(1000);

			switch (calibPoints.size()) {
			case 0:
				// top left
				calibPoints.push_back(cv::Point(center.x, center.y));

				// show second circle
				cv::circle(m2, circle_top_right, CALIB_CIRCLE_RADIUS, cv::Scalar(0), CV_FILLED);
				imshow(CALIB_BGR_WND, m2);
				break;
			case 1:
				// top right
				calibPoints.push_back(cv::Point(center.x, center.y));

				// show third circle
				cv::circle(m3, circle_bottom_right, CALIB_CIRCLE_RADIUS, cv::Scalar(0), CV_FILLED);
				imshow(CALIB_BGR_WND, m3);
				break;
			case 2:
				// bottom right
				calibPoints.push_back(cv::Point(center.x, center.y));

				// show fourth circle
				cv::circle(m4, circle_bottom_left, CALIB_CIRCLE_RADIUS, cv::Scalar(0), CV_FILLED);
				imshow(CALIB_BGR_WND, m4);
				break;
			case 3:
				// bottom left
				calibPoints.push_back(cv::Point(center.x, center.y));
				break;
			default:
				break;
			};
		}

		if (calibPoints.size() == 4) {
			cerr << "Automatic calibration done" << endl;
			waitKey(1000);
			break;
		}

		if( waitKey( 30 ) >= 0 )
		{
			cerr << "Automatic calibration aborted" << endl;
			cv::destroyWindow(CALIB_BGR_WND);
			cv::destroyWindow(CALIB_BGR_WND_MONITOR);
			return false;
		}

	}

	cv::destroyWindow(CALIB_BGR_WND);
	cv::destroyWindow(CALIB_BGR_WND_MONITOR);
	return true;
}