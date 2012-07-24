#include "stdafx.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <iostream>
#include <iomanip>
#include <vector>

#include <sstream>

using namespace cv;
using namespace std;

static void colorizeDisparity( const Mat& gray, Mat& rgb, double maxDisp=-1.f, float S=1.f, float V=1.f )
{
    CV_Assert( !gray.empty() );
    CV_Assert( gray.type() == CV_8UC1 );

    if( maxDisp <= 0 )
    {
        maxDisp = 0;
        minMaxLoc( gray, 0, &maxDisp );
    }

    rgb.create( gray.size(), CV_8UC3 );
    rgb = Scalar::all(0);
    if( maxDisp < 1 )
        return;

    for( int y = 0; y < gray.rows; y++ )
    {
        for( int x = 0; x < gray.cols; x++ )
        {
            uchar d = gray.at<uchar>(y,x);
            unsigned int H = ((uchar)maxDisp - d) * 240 / (uchar)maxDisp;

            unsigned int hi = (H/60) % 6;
            float f = H/60.f - H/60;
            float p = V * (1 - S);
            float q = V * (1 - f * S);
            float t = V * (1 - (1 - f) * S);

            Point3f res;

            if( hi == 0 ) //R = V,  G = t,  B = p
                res = Point3f( p, t, V );
            if( hi == 1 ) // R = q, G = V,  B = p
                res = Point3f( p, V, q );
            if( hi == 2 ) // R = p, G = V,  B = t
                res = Point3f( t, V, p );
            if( hi == 3 ) // R = p, G = q,  B = V
                res = Point3f( V, q, p );
            if( hi == 4 ) // R = t, G = p,  B = V
                res = Point3f( V, p, t );
            if( hi == 5 ) // R = V, G = p,  B = q
                res = Point3f( q, p, V );

            uchar b = (uchar)(std::max(0.f, std::min (res.x, 1.f)) * 255.f);
            uchar g = (uchar)(std::max(0.f, std::min (res.y, 1.f)) * 255.f);
            uchar r = (uchar)(std::max(0.f, std::min (res.z, 1.f)) * 255.f);

            rgb.at<Point3_<uchar> >(y,x) = Point3_<uchar>(b, g, r);
        }
    }
}

static float getMaxDisparity( VideoCapture& capture )
{
    const int minDistance = 400; // mm
    float b = (float)capture.get( CV_CAP_OPENNI_DEPTH_GENERATOR_BASELINE ); // mm
    float F = (float)capture.get( CV_CAP_OPENNI_DEPTH_GENERATOR_FOCAL_LENGTH ); // pixels
    return b * F / minDistance;
}

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
	const std::string CALIB_BGR_WND = "Calibration (please click the edges of the beamer area)";

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
			circle(bgrImage, Point( it->x, it->y ), 5,  Scalar(0), 2, 8, 0 );
		}

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

// Snippets
	/*if(!grabAndStore(capture))
	{
		cout << "Failed to store set of images" << endl;
		return 1;
	}*/


int main( int argc, char* argv[] )
{
	VideoCapture capture;
	if (!initializeCapture(capture))
		return 1;

	vector<Point2f> calibPoints;
	if(!getManualCalibrationRectangleCorners(capture, calibPoints))
	{
		cerr << "Calibration failed" << endl;
		return 1;
	}

	const std::string BGR_IMAGE = "Bgr Image";
	const std::string DEPTH_MAP = "Depth Map";

	namedWindow(BGR_IMAGE, CV_WINDOW_KEEPRATIO);
	namedWindow(DEPTH_MAP, CV_WINDOW_KEEPRATIO);



	unsigned int n = 0;

	int h = 0;
	int s = 0;
	int v = 0;
	int h2 = 0;
	int s2 = 0;
	int v2 = 0;

	bool first = true;



	cout << "Enter mainloop" << endl;
	for (;;)
	{
		Mat bgrImage;
		Mat depthMap;
		Mat lastBgr;

		if (!capture.grab())
		{
			cerr << "Failed to grab frame" << endl;
			return 1;
		}

		if (!capture.retrieve(bgrImage, CV_CAP_OPENNI_BGR_IMAGE))
		{
			cerr << "Failed to retrieve" << endl;
			return 1;
		}

		if (!capture.retrieve(depthMap, CV_CAP_OPENNI_DEPTH_MAP))
		{
			cerr << "Failed to retrieve valid depth mask" << endl;
			return 1;
		}

		createTrackbar("H", "mask", &h, 180, NULL, NULL);
		createTrackbar("S", "mask", &s, 255, NULL, NULL);
		createTrackbar("V", "mask", &v, 255, NULL, NULL);
		createTrackbar("H2", "mask", &h2, 180, NULL, NULL);
		createTrackbar("S2", "mask", &s2, 255, NULL, NULL);
		createTrackbar("V2", "mask", &v2, 255, NULL, NULL);

		Scalar hsv_min = Scalar(h, s, v, 0);
		Scalar hsv_max = Scalar(h2, s2, v2, 0);

		Mat hsvImage;
		cvtColor(bgrImage, hsvImage,  CV_BGR2HSV);
		Mat hsvMask;
		inRange(hsvImage, hsv_min, hsv_max, hsvMask);
		imshow("mask", hsvMask);

		
		Mat grey;
		cvtColor(bgrImage, grey, CV_BGR2GRAY);
		imshow("grey", grey);
		Mat res;
  
		Mat bin;
		threshold(grey, bin, 160, 255, CV_THRESH_BINARY);
		imshow("bin", bin);


		int blockSize = 2;
		int apertureSize = 3;
		double k = 0.04;

		cornerHarris(bin, res, blockSize, apertureSize, k, BORDER_DEFAULT);
		Mat dst_norm;
		Mat dst_norm_scaled;
		normalize( res, dst_norm, 0, 255, NORM_MINMAX, CV_32FC1, Mat() );
		convertScaleAbs( dst_norm, dst_norm_scaled );

		int thresh = 200;

		/// Drawing a circle around corners
		int max = 0;
		for( int j = 0; j < dst_norm.rows ; j++ )
		{ for( int i = 0; i < dst_norm.cols; i++ )
		{
			if( (int) dst_norm.at<float>(j,i) > thresh )
			{
				circle( dst_norm_scaled, Point( i, j ), 5,  Scalar(0), 2, 8, 0 );
				++max;
				if (max > 20) {
					cout << "oo" << endl;
					break;
				}
			}
		}
		}

		imshow("Harris", dst_norm_scaled);
                    //Mat colorDisparityMap;
                    //colorizeDisparity( depthMap, colorDisparityMap, getMaxDisparity(capture));
                    //Mat validColorDisparityMap;
                    //colorDisparityMap.copyTo( validColorDisparityMap, disparityMap != 0 );

		imshow(BGR_IMAGE, bgrImage);
		imshow(DEPTH_MAP, depthMap);

		stringstream ss;
		ss << "recording/brimage" << n << ".png";
		/*if(!imwrite(ss.str(), bgrImage))
		{
			cerr << "Failed to write " << ss.str() << endl;
			return 1;
		}
		ss.str(string());
		ss << "recording/dimage" << n << ".png";
		if(!imwrite(ss.str(), depthMap))
		{
			cerr << "Failed to write " << ss.str() << endl;
			return 1;
		}*/
		//uchar d = depthMap.data[(depthMap.rows / 2)  * depthMap.cols + depthMap.cols / 2];
		//cout << (unsigned int)d << endl;
		cout << (unsigned int) depthMap.at<ushort>(depthMap.rows/ 2, depthMap.cols/2) << endl;
		++n;

        if( waitKey( 30 ) >= 0 )
            break;
	}
	return 0;
}