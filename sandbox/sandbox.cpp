#include "stdafx.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include <sstream>

using namespace cv;
using namespace std;

//
//-- Constants
//

// 1024x768 Beamer resolution
const int BEAMER_MAX_X = 1024;
const int BEAMER_MAX_Y = 768;

// Sandbox constants
const int MAX_SAND_DEPTH_IN_MM = 90;
const int MAX_SAND_HEIGHT_IN_MM = 200;



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
	const std::string CALIB_BGR_WND = "Calibration (please click the edges of the beamer area clockwise starting top left)";

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
	realPoints.push_back(Point2f(BEAMER_MAX_X,0)); // Top right
	realPoints.push_back(Point2f(BEAMER_MAX_X, BEAMER_MAX_Y)); // Bottom right
	realPoints.push_back(Point2f(0, BEAMER_MAX_Y)); // Bottom left

	homography = getPerspectiveTransform(calibPoints, realPoints);
	return true;
}

bool getDepthCorrection(VideoCapture &capture, Mat &homography, unsigned short &boxBottomMM)
{
	if(!capture.grab())
		return false;

	Mat rawDepthInMM;
	if(!capture.retrieve(rawDepthInMM, CV_CAP_OPENNI_DEPTH_MAP))
		return false;

	Mat depthWarpedInMM;
	warpPerspective(rawDepthInMM, depthWarpedInMM, homography, Size(BEAMER_MAX_X, BEAMER_MAX_Y));
	
	uint64_t val = 0;
	unsigned int num = 0;
	// Sample a few points to estimate ground level
	for (int x = 0; x < BEAMER_MAX_X; x += BEAMER_MAX_X/16)
	{
		for (int y = 0; y < BEAMER_MAX_Y; y += BEAMER_MAX_Y/16)
		{
			val += depthWarpedInMM.at<unsigned short>(Point(x,y));
			++num;
		}
	}

	val = val / num;
	cout << "Sandbox ground level estimated at: " << val << endl;
	val += MAX_SAND_DEPTH_IN_MM;
	cout << "Sandbox deepest stop estimated at: " << val << endl;
	
	boxBottomMM = static_cast<unsigned short>(val);

	//TODO: Perspective correction doesn't help with 3D issues. Should use 4 edge points for reconstructing 3d base plane but that can wait.

	// Create depth correction as level
	//Mat baseLayer = Mat(BEAMER_MAX_X, BEAMER_MAX_Y, depthWarpedInMM.type(), val); // Idealized sandbox base
	//cv::subtract(baseLayer, depthWarpedInMM, depthCorrection);
	return true;
}

bool sandboxNormalize(Mat &depthWarped, Mat& depthWarpedNormalized, unsigned short boxBottomMM)
{
	const size_t rows = depthWarped.rows;
	const size_t cols = depthWarped.cols;
	depthWarpedNormalized = Mat(rows, cols, depthWarped.type());

	const uint16_t topOrig = boxBottomMM - MAX_SAND_DEPTH_IN_MM - MAX_SAND_HEIGHT_IN_MM;
	const uint16_t range = boxBottomMM - topOrig;

	for (size_t row = 0; row < rows; ++row)
	{
		for (size_t col = 0; col < cols; ++col)
		{
			uint16_t val = depthWarped.at<uint16_t>(Point(col, row));
			// Clip
			if (val > boxBottomMM) val = boxBottomMM;
			else if (val < topOrig) val = topOrig;

			// Shift and invert
			val = boxBottomMM - val;

			// Scale and save
			const uint16_t fac = numeric_limits<uint16_t>::max() / range;
			const uint16_t color = val * fac;
			depthWarpedNormalized.at<uint16_t>(Point(col, row)) = color;
		}
	}


	return true;
}

//
// Snippets
//

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

	Mat homography;
	if(!getHomography(capture, homography))
		return 1;

	unsigned short boxBottomMM;
	if(!getDepthCorrection(capture, homography, boxBottomMM))
		return 1;

	const std::string BGR_IMAGE = "Bgr Image";
	const std::string DEPTH_MAP = "Depth Map";
	const std::string BGR_WARPED = "Warped BGR Image";
	const std::string DEPTH_WARPED = "Warped Depth Image";

	namedWindow(BGR_IMAGE, CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
	namedWindow(DEPTH_MAP, CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
	namedWindow(BGR_WARPED, CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
	namedWindow(DEPTH_WARPED, CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);


	cout << "Enter mainloop" << endl;

	for (;;)
	{
		Mat bgrImage;
		Mat depthMap;

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

		Mat bgrWarped;		
		warpPerspective(bgrImage, bgrWarped, homography, Size(BEAMER_MAX_X, BEAMER_MAX_Y));

		Mat depthWarped;
		warpPerspective(depthMap, depthWarped, homography, Size(BEAMER_MAX_X, BEAMER_MAX_Y));

		Mat depthWarpedNormalized;
		sandboxNormalize(depthWarped, depthWarpedNormalized, boxBottomMM);

		imshow("sandnorm", depthWarpedNormalized);
		//unsigned short centerval = depthWarped.at<unsigned short>(Point(BEAMER_MAX_X/2, BEAMER_MAX_Y/2));
		//cout << centerval << endl;

		imshow(BGR_WARPED, bgrWarped);
		imshow(DEPTH_WARPED, depthWarped);
		imshow(BGR_IMAGE, bgrImage);
		imshow(DEPTH_MAP, depthMap);

		if( waitKey( 30 ) >= 0 )
			break;
	}

	return 0;
}



