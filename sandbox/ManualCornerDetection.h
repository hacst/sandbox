#ifndef MANUALCORNERDETECTION_H
#define MANUALCORNERDETECTION_H

#include <opencv2/opencv.hpp>
#include <vector>

bool getManualCalibrationRectangleCorners(cv::VideoCapture &capture, std::vector<cv::Point2f> &calibPoints, std::vector<cv::Point2f> &realPoints);

#endif // MANUALCORNERDETECTION_H