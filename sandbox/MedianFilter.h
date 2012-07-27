#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

#include "HistoryBuffer.h"

class MedianFilter : public HistoryBuffer {
public:
	MedianFilter(const size_t depth, const size_t stepsize)
		: HistoryBuffer(depth)
		, m_stepsize(stepsize) 
	{
	}

	template <typename T>
	void getFiltered(cv::Mat &result)
	{
		std::vector<cv::Mat>& history = getHistory();

		assert(history.size() > 0);

		if(result.data == NULL)
		{
			// Reserve storage if not available
			result = Mat(history[0].rows, history[0].cols, history[0].type());
		}

		const size_t ROWS = result.rows;
		const size_t COLS = result.cols;

		std::vector<T> buffer;
		buffer.resize(static_cast<size_t>(ceil((static_cast<double>(history.size()) / m_stepsize))));
		size_t n = 0;
		const size_t MIDDLEIDX = buffer.size() / 2;
		for (size_t row = 0; row < ROWS; ++row)
		{
			for (size_t col = 0; col < COLS; ++col)
			{
				n = 0;
				for (size_t pos = 0; pos < history.size(); pos += m_stepsize)
				{
					buffer[n] = history[pos].at<T>(Point(col, row));
					++n;
				}

				std::sort(buffer.begin(), buffer.end());

				result.at<T>(Point(col, row)) = buffer[MIDDLEIDX];
			}
		}
	}

private:
	const size_t m_stepsize;

};


#endif // MEDIAN_FILTER_H
