#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif


enum CalibrationModes {
	MANUAL,
	AUTO_HOUGH,
	AUTO_HARRIS,
	CALIBRATION_MODE_MAX
};
//
//-- Constants
//

extern struct Settings {
	// Command line settings
	int monitor;
	bool fullscreen;
	bool displayBGR;
	int sandPlaneDistanceInMM;
	std::string colorFile;

	std::string treasureFile;
	std::string treasureSound;

	CalibrationModes calibrationMode;

	// Averaging filter settings
	size_t averagingDepth;
	size_t averagingStepsize;

	// Median filter settings
	size_t medianDepth;
	size_t medianStepsize;

	// Derived from settings
	RECT monitorRect;

	int beamerXres;
	int beamerYres;

	int maxSandDepthInMM;
	int maxSandHeightInMM;

	uint16_t boxBottomDistanceInMM;

} settings;

#endif // SETTINGS_H