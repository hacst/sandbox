#ifndef FULLSCREEN_H
#define FULLSCREEN_H

#ifdef _WIN32
#include <windows.h>
#endif

#include <vector>
#include <string>

bool enumMonitors(std::vector<RECT> &rect);
bool fullScreen(RECT &monitor, const std::string windowName);
bool getMonitorRect(int monitor, RECT &monitorRect);
bool printMonitors();

#endif FULLSCREEN_H