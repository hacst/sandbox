#ifndef HISTORY_BUFFER
#define HISTORY_BUFFER

#include <opencv2\opencv.hpp>

class HistoryBuffer {
public:
	HistoryBuffer(const size_t depth);

	void addFrame(cv::Mat &frame, bool clone = true);

protected:
	std::vector<cv::Mat>& getHistory();

private:
	std::vector<cv::Mat> m_state;
	unsigned int m_insertionPoint;
	const size_t m_depth;
};

#endif // HISTORY_BUFFER