#include "Fullscreen.h"

#include <iostream>

using namespace std;

#ifdef _WIN32

BOOL CALLBACK MonitorEnumProc(
	HMONITOR hMonitor,
	HDC hdcMonitor,
	LPRECT lprcMonitor,
	LPARAM dwData
	)
{
	std::vector<RECT>* monitors = (std::vector<RECT>*)dwData;

	monitors->push_back(*lprcMonitor);

	return TRUE;
}

bool enumMonitors(std::vector<RECT> &rect)
{
	cout << "Enumerating monitors...";
	if(EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM) &rect) == 0)
	{
		cout << "failed" << endl;
		return false;
	}
	cout << "ok" << endl;
	return true;
}

bool fullScreen(RECT &monitor, const std::string windowName)
{
	const LONG width = monitor.right - monitor.left;
	const LONG height = monitor.bottom - monitor.top;

	HWND hwnd = FindWindowA(NULL, windowName.c_str());
	if(hwnd == NULL) {
		return false;
	}

	SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
	SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

	SetWindowPos(hwnd, HWND_TOPMOST, monitor.left, monitor.top, width, height, SWP_SHOWWINDOW);

	ShowWindow(hwnd, SW_MAXIMIZE);

	return true;
}

bool getMonitorRect(int monitor, RECT &monitorRect)
{
	std::vector<RECT> rect;
	if(!enumMonitors(rect))
		return false;

	if (monitor < 0 || monitor >= (int)rect.size())
		return false;

	monitorRect = rect[monitor];

	return true;
}

bool printMonitors()
{
	std::vector<RECT> monitors;
	if(!enumMonitors(monitors))
	{
		return false;
	}

	cout << endl;

	size_t num = 0;
	for (std::vector<RECT>::iterator it = monitors.begin();
		it != monitors.end();
		++it)
	{
		RECT *monitor = &(*it);

		cout << "Monitor " << num << ":" << endl;
		cout << "Left/Right: " << monitor->left << " - " << monitor->right << endl;
		cout << "Top/Bottom: " << monitor->top << " - " << monitor->bottom << endl << endl;

		++num;
	}

	return true;
}
#else

bool enumMonitors(std::vector<RECT> &rect) {
	/* Not implemented for other platforms */
	return false;
}

bool fullScreen(RECT &monitor, const std::string windowName) {
	/* Not implemented for other platforms */
	return false;
}

bool printMonitors() {
	/* Not implemented for other platforms */ 
	return false;
}

bool getMonitorRect(int monitor, RECT &monitorRect) {
	/* Not implemented for other platforms */ 
	return false;
}

#endif
