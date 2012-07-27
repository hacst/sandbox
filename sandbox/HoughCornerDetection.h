#ifndef HOUGHCORNERDETECTION_H
#define HOUGHCORNERDETECTION_H

#include <opencv2/opencv.hpp>
#include <vector>

bool getAutoCalibrationRectangleCornersHough(cv::VideoCapture &capture, std::vector<cv::Point2f> &calibPoints, std::vector<cv::Point2f> &realPoints);

#endif // HOUGHCORNERDETECTION_H