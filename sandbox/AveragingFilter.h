#ifndef AVERAGING_FILTER_H
#define AVERAGING_FILTER_H

#include "HistoryBuffer.h"

class AveragingFilter : public HistoryBuffer {
public:
	AveragingFilter(const size_t depth, const size_t stepsize = 1);

	void getFiltered(cv::Mat &result);

private:
	const size_t m_stepsize;

};

#endif // AVERAGING_FILTER_H