#include "AveragingFilter.h"

using namespace cv;
using namespace std;

AveragingFilter::AveragingFilter(const size_t depth, const size_t stepsize)
	: HistoryBuffer(depth)
	, m_stepsize(stepsize)
{

}

void AveragingFilter::getFiltered(cv::Mat &result)
{
	std::vector<cv::Mat>& history = getHistory();

	assert(history.size() > 0);

	if(result.data == NULL)
	{
		// Reserve storage if not available
		result = Mat(history[0].rows, history[0].cols, history[0].type());
	}

	memset(result.data, 0, result.dataend - result.data);

	double avgWeight = 1. / ceil((static_cast<double>(history.size()) / m_stepsize));

	for (size_t pos = 0; pos < history.size(); pos += m_stepsize)
	{
		assert(history[pos].type() == result.type());
		assert(history[pos].cols == result.cols);
		assert(history[pos].rows == result.rows);

		addWeighted(history[pos], avgWeight, result, 1.0, 0.0, result);
	}
}
