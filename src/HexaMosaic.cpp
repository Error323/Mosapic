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
	cv::Mat raw_data;
	int num_files = 0, num_images = 0, rows = 0, tile_size = 0;

	// Load database
	cv::FileStorage fs(mDatabase + "/rawdata.yml", cv::FileStorage::READ);
	fs["num_files"] >> num_files;
	fs["num_images"] >> num_images;
	fs["tile_size"] >> tile_size;
	fs.release();

	cv::Mat tmp, roi;
	int current_row = 0;
	raw_data.create(num_images, tile_size*tile_size*3, CV_8UC1);
	for (int i = 0; i < num_files; i++)
	{
		std::stringstream s;
		s << i;
		std::string entry = mDatabase + "/rawdata_" + s.str() + ".yml";
		std::cout << "Loading `" << entry << "'..." << std::flush;
		fs.open(entry, cv::FileStorage::READ);
		rows = 0;
		fs["rows"] >> rows;
		for (int j = 0; j < rows; j++)
		{
			std::stringstream ss;
			ss << j;
			fs["img_" + ss.str()] >> tmp;
			roi = raw_data.row(current_row);
			tmp.copyTo(roi);
			current_row++;
		}
		fs.release();
		std::cout << "[done]" << std::endl;
	}

	// Compute pca components from source image
	cv::Mat src_img = cv::imread(mSourceImage, 1);
	for (int i = 0; i < mHeight; i++)
	{
		for (int j = 0; j < mWidth; j++)
		{

		}
	}

/*
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
*/
}
