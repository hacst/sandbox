#include "HarrisCornerDetection.h"

using namespace cv;
using namespace std;

#include "Fullscreen.h"
#include "Settings.h"

bool getAutoCalibrationRectangleCornersHarris(VideoCapture &capture, vector<Point2f> &calibPoints, vector<Point2f> &realPoints){
	calibPoints.clear();
	const std::string CALIB_BGR_WND_2 = "Harris Calibration";
	const std::string CALIB_BGR_WND_3 = "Harris Extra";
	namedWindow(CALIB_BGR_WND_2);


	realPoints.push_back(Point2f(0,0)); // Top left
	realPoints.push_back(Point2f(1024,0)); // Top right
	realPoints.push_back(Point2f(1024, 768)); // Bottom right
	realPoints.push_back(Point2f(0, 768)); // Bottom left

	if (settings.fullscreen)
	{
		if(!fullScreen(settings.monitorRect, CALIB_BGR_WND_2))
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

	imshow(CALIB_BGR_WND_2, m);


	for(;;)
	{
		calibPoints.clear();
		

		waitKey(100);

		if(!capture.grab())
		{
			cerr << "Failed to grab" << endl;
			cv::destroyWindow(CALIB_BGR_WND_2);
			return false;
		}

		Mat mat, mat2;
		if(!capture.retrieve(mat, CV_CAP_OPENNI_BGR_IMAGE))
		{
			cerr << "Failed to retrieve" << endl;
			cv::destroyWindow(CALIB_BGR_WND_2);
			return false;
		}
	
	
		/// Convert it to gray
		Mat src_gray;
		cvtColor( mat, src_gray, CV_BGR2GRAY );
	
		int blockSize = 4;
		int apertureSize = 3;
		double k = 0.01;

		Mat chImg, chImgNorm, chImgNormScaled;
		cornerHarris(src_gray, chImg, blockSize, apertureSize, k, BORDER_DEFAULT);
		normalize( chImg, chImgNorm, 0, 255, NORM_MINMAX, CV_32FC1, Mat() );
		convertScaleAbs( chImgNorm, chImgNormScaled );

		/// erkannte Ecken in tempPoints abspeichern	
		int thresh = 70;
		vector<Point> points;
		for( int j = 0; j < chImgNorm.rows ; j++ ){
			for( int i = 0; i < chImgNorm.cols; i++ ){
				if( (int) chImgNorm.at<float>(j,i) > thresh ){
					points.push_back(Point(i, j));
				}
			}
		}
		/// nahliegende Eckpunkte als ein punkt betrachten  
		int dist = 10;
		for(int i = 0; i < points.size(); i++){
			//cout << points[i].x << "," << points[i].y << endl;
			calibPoints.push_back(points[i]);
			//realPoints.push_back(points[i]);
			circle( chImgNormScaled, points[i], 5,  Scalar(0), 2, 8, 0 );
			for(int j = i + 1; j < points.size(); j++){
				if( (points[j].x > (points[i].x - dist) & points[j].x < (points[i].x + dist)) 
					&& (points[j].y > (points[i].y - dist) & points[j].y < (points[i].y + dist)) ){
						i++;
				} else {
					break;
				}
			}
		}
		imshow(CALIB_BGR_WND_3, chImgNormScaled);

		if (calibPoints.size() == 4) {
			cerr << "Harris calibration done" << endl;
			waitKey(1000);
			break;
		}

		if( waitKey( 30 ) >= 0 )
		{
			cerr << "Harris calibration aborted" << endl;
			cv::destroyWindow(CALIB_BGR_WND_2);
			cv::destroyWindow(CALIB_BGR_WND_3);
			return false;
		}
	}

	waitKey(2000);
	cv::destroyWindow(CALIB_BGR_WND_3);

	cv::destroyWindow(CALIB_BGR_WND_2);
	return true;

}