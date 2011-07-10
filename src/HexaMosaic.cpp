#include "HexaMosaic.hpp"
#include "Debugger.hpp"

#include <cmath>
#include <fstream>
#include <queue>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <limits>

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
	cv::Mat database;
	int num_files = 0, num_images = 0, rows = 0, tile_size = 0;

	// Load database
	cv::FileStorage fs(mDatabase + "/rawdata.yml", cv::FileStorage::READ);
	fs["num_files"] >> num_files;
	fs["num_images"] >> num_images;
	fs["tile_size"] >> tile_size;
	fs.release();

	cv::Mat tmp, roi;
	int current_row = 0;
	database.create(num_images, tile_size*tile_size*3, CV_8UC1);
	for (int i = 0; i < num_files; i++)
	{
		std::stringstream s;
		s << i;
		std::string entry = mDatabase + "rawdata_" + s.str() + ".yml";
		std::cout << "Loading `" << entry << "'..." << std::flush;
		fs.open(entry, cv::FileStorage::READ);
		rows = 0;
		fs["rows"] >> rows;
		for (int j = 0; j < rows; j++)
		{
			std::stringstream ss;
			ss << j;
			fs["img_" + ss.str()] >> tmp;
			roi = database.row(current_row);
			tmp.copyTo(roi);
			current_row++;
		}
		fs.release();
		std::cout << "[done]" << std::endl;
	}

	// Compute pca components from source image
	cv::Mat src_img = cv::imread(mSourceImage, 1);
	cv::Mat src_img_scaled, pca_data;
	cv::resize(src_img, src_img_scaled, cv::Size(mWidth*tile_size, mHeight*tile_size), 0.0, 0.0, CV_INTER_CUBIC);
	pca_data.create(mHeight*mWidth, tile_size*tile_size*3, CV_8UC1);
	cv::Rect roi_patch(0, 0, tile_size, tile_size);
	cv::Mat src_patch;
	for (int i = 0; i < mHeight; i++)
	{
		roi_patch.y = i * tile_size;
		for (int j = 0; j < mWidth; j++)
		{
			roi_patch.x = j * tile_size;
			src_patch = src_img_scaled(roi_patch).clone();
			src_patch = src_patch.reshape(1, 1);
			cv::Mat entry = pca_data.row(i*mWidth+j);
			src_patch.copyTo(entry);
		}
	}

	cInt dimensions = 8;
	std::cout << "Performing pca..." << std::flush;
	cv::PCA pca(pca_data, cv::Mat(), CV_PCA_DATA_AS_ROW, dimensions);
	#ifdef DEBUG
	// Construct eigenvector images for debugging
	for (int i = 0; i < dimensions; i++)
	{
		cv::Mat eigenvec;
		cv::normalize(pca.eigenvectors.row(i), eigenvec, 255, 0, cv::NORM_MINMAX);
		eigenvec = eigenvec.reshape(3, tile_size);
		std::stringstream s;
		s << i;
		std::string entry = "eigenvector-" + s.str() + ".jpg";
		cv::imwrite(entry, eigenvec);
	}
	#endif
	std::cout << "[done]" << std::endl;

	// Compress original image data
	std::cout << "Compress source image..." << std::flush;
	cv::Mat flat_img, compressed_src_img, compressed_entry;
	compressed_src_img.create(mWidth*mHeight, dimensions, pca.eigenvectors.type());
	for (int i = 0; i < pca_data.rows; i++)
	{
		flat_img = pca_data.row(i);
		compressed_entry = compressed_src_img.row(i);
		pca.project(flat_img, compressed_entry);
	}
	pca_data.release();
	std::cout << "[done]" << std::endl;

	// Compress database image data
	std::cout << "Compress database..." << std::flush;
	cv::Mat compressed_database;
	compressed_database.create(num_images, dimensions, pca.eigenvectors.type());
	for (int i = 0; i < num_images; i++)
	{
		flat_img = database.row(i);
		compressed_entry = compressed_database.row(i);
		pca.project(flat_img, compressed_entry);
	}
	std::cout << "[done]" << std::endl;

	// Construct mosaic
	std::cout << "Construct mosaic..." << std::flush;
	cv::Mat src_entry, tmp_entry, dst_patch;
	vInt ids;
	for (int i = 0; i < mHeight; i++)
	{
		roi_patch.y = i * tile_size;
		for (int j = 0; j < mWidth; j++)
		{
			roi_patch.x = j * tile_size;
			src_entry = compressed_src_img.row(i*mWidth+j);
			// Match with database
			std::priority_queue<Match> KNN; // All nearest neighbours
			for (int k = 0; k < num_images; k++)
			{
				tmp_entry = compressed_database.row(k);
				float dist = GetDistance(src_entry, tmp_entry);
				KNN.push(Match(k, dist));
			}
			int best_id = KNN.top().id;
			while (!KNN.empty() && find(ids.begin(), ids.end(), best_id) != ids.end())
			{
				KNN.pop();
				best_id = KNN.top().id;
			}
			ids.push_back(best_id);
			src_patch = src_img_scaled(roi_patch);
			dst_patch = database.row(best_id);
			dst_patch = dst_patch.reshape(3, tile_size);
			dst_patch.copyTo(src_patch);
		}
	}
	cv::imwrite(mDestImage, src_img_scaled);
	std::cout << "[done]" << std::endl;
}

float HexaMosaic::GetDistance(const cv::Mat& inSrcRow, const cv::Mat& inDataRow) {
	cv::Mat diff, square;
	float sum, dist;
	cv::subtract(inSrcRow, inDataRow, diff);
	cv::pow(diff, 2.0, square);
	sum = cv::sum(square)[0];
	dist = sqrt(sum);
	return dist;
}
