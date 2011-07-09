#include "HexaMosaic.hpp"
#include "Debugger.hpp"

#include <cmath>
#include <fstream>
#include <queue>
#include <algorithm>
#include <sstream>
#include <iostream>

HexaMosaic::HexaMosaic(
	rcString inSourceImage,
	rcString inDestImage,
	rcString inDatabase,
	cInt inWidth,
	cInt inHeight
	):
	mSourceImage(inSourceImage),
	mDestImage(inDestImage),
	mDatabase(inDatabase),
	mWidth(inWidth),
	mHeight(inHeight) {
}

void HexaMosaic::Create() {
	cv::Mat src_img = cv::imread(mSourceImage, 1);
	cv::Mat dst_img(mHeight*100, mWidth*100, CV_8UC3);
	cv::Mat stitched_img(mHeight*100, mWidth*100, CV_8UC3);
	cInt scale_x = (src_img.cols-100) / mWidth;
	cInt scale_y = (src_img.rows-100) / mHeight;

	// Load database


	vInt ids;
	int percentage_done = 0;
	for (int i = 0; i < mHeight; i++)
	{
		for (int j = 0; j < mWidth; j++)
		{
			cv::Rect r(j*scale_x, i*scale_y, 100, 100);
			cv::Mat roi = src_img(r);
			vFloat src_data;
			HexaMosaic::PrincipalComponents(roi, dimensions, src_data);

			// match with database
			std::priority_queue<Match> KNN; // All nearest neighbours
			for (int img_id = 0; img_id < record_count; img_id++)
			{
				float dist = 0.0f;
				for (int dim = 0; dim < dimensions; dim++)
				{
					dist += powf(db_data[img_id*32+dim] - src_data[dim], 2.0f);
				}
				KNN.push(Match(img_id, sqrtf(dist)));
			}

			// Make sure we never use the same image twice
			int best_id = KNN.top().id;
			while (!KNN.empty() && find(ids.begin(), ids.end(), best_id) != ids.end())
			{
				KNN.pop();
				best_id = KNN.top().id;
			}
			ids.push_back(best_id);
			std::stringstream s;
			s << best_id;
			cv::Mat tile_img = cv::imread(mDatabase + "/" + s.str() + ".jpg", 1);
			cv::Rect r2(j*100, i*100, 100, 100);
			cv::Mat roi3 = stitched_img(r2);
			roi.copyTo(roi3);
			cv::Mat roi2 = dst_img(r2);
			tile_img.copyTo(roi2);
			cInt percentage = roundf((ids.size()/float(mWidth*mHeight))*100);
			if (percentage > percentage_done)
			{
				percentage_done = percentage;
				std::cout << percentage_done << "%" << std::endl;
			}
		}
	}
	cv::imwrite("stitched.jpg", stitched_img);
	cv::imwrite(mDestImage, dst_img);
	cv::Mat img_gray;
	cv::cvtColor(dst_img, img_gray, CV_RGB2GRAY);
	cv::imwrite("output_gray.jpg", img_gray);
}


void HexaMosaic::PrincipalComponents(const cv::Mat& inImg, const int inDimensions, rvFloat outData) {
	cv::Mat img_gray;
	cv::cvtColor(inImg, img_gray, CV_RGB2GRAY);
	cv::PCA pca(img_gray, cv::Mat(), CV_PCA_DATA_AS_ROW, inDimensions);
	for (int i = 0; i < pca.eigenvalues.rows; i++)
		outData.push_back(pca.eigenvalues.at<float>(0,i));
}


int HexaMosaic::Split(rvString outSplit, rcString inString, char inSplitChar) {
	std::string::const_iterator s = inString.begin();
	while (true)
	{
		std::string::const_iterator begin = s;

		while (*s != inSplitChar && s != inString.end())
			++s;

		outSplit.push_back(std::string(begin, s));

		if (s == inString.end())
			break;

		if (++s == inString.end())
		{
			outSplit.push_back("");
			break;
		}
	}
	return outSplit.size();
}
