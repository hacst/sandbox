#include "HistoryBuffer.h"

using namespace cv;
using namespace std;

HistoryBuffer::HistoryBuffer(const size_t depth)
	: m_depth(depth)
	, m_insertionPoint(0)
{
}

void HistoryBuffer::addFrame(cv::Mat &frame, bool clone)
{
	assert(frame.data != NULL);

	Mat cp = clone ? frame.clone() : frame;

	assert(cp.type() == frame.type());
	assert(cp.rows == frame.rows);
	assert(cp.cols == frame.cols);

	if (m_state.size() < m_depth)
	{
		m_state.push_back(cp);
	}
	else
	{
		m_state[m_insertionPoint] = cp;
	}

	++m_insertionPoint;

	if (m_insertionPoint >= m_depth)
		m_insertionPoint = 0;
}

std::vector<cv::Mat>& HistoryBuffer::getHistory() {
	return m_state;
}
