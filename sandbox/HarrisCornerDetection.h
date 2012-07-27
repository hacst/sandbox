#ifndef HARRISCORNERDETECTION_H
#define HARRISCORNERDETECTION_H

#include <opencv2/opencv.hpp>
#include <vector>

bool getAutoCalibrationRectangleCornersHarris(cv::VideoCapture &capture, std::vector<cv::Point2f> &calibPoints, cv::vector<cv::Point2f> &realPoints);

#endif // HARRISCORNERDETECTION_H