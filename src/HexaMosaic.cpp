#include "HexaMosaic.hpp"
#include "Debugger.hpp"

#include <cmath>
#include <fstream>
#include <queue>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <limits>

#define HALF_HEXAGON_WIDTH sinf(M_PI / 3.0f)
#define HEXAGON_WIDTH (2.0f * HALF_HEXAGON_WIDTH)

HexaMosaic::HexaMosaic(
	rcString inSourceImage,
	rcString inDatabase,
	cInt inWidth,
	cInt inHeight,
	cInt inDimensions,
	cInt inMaxRadius
	):
	mSourceImage(inSourceImage),
	mDatabaseFileName(inDatabase),
	mWidth(inWidth),
	mHeight(inHeight),
	mDimensions(inDimensions),
	mMaxRadius(inMaxRadius)
{
}

void HexaMosaic::Create() {
	// Load the database (raw images)
//	LoadDatabase(mDatabaseFileName + "/rawdata.yml", mDatabase);
	mTileSize = 100;
	// Randomize image coordinates, gives more natural result
	std::vector<cv::Point2i> coordinates;
	for (int y = 0; y < mHeight; y++)
		for (int x = 0; x < mWidth; x++)
			coordinates.push_back(cv::Point2i(x,y));

	// unit dimensions of hexagon facing upwards
	cFloat unit_dx    = HEXAGON_WIDTH;
	cFloat unit_dy    = 1.5f;
	cFloat radius     = mTileSize / 2.0f;

	// Precache hexagon coordinates
	std::vector<cv::Point2i> hex_coords;
	cv::Mat hex_mask(mTileSize-1, radius*unit_dx, CV_8UC1, cv::Scalar(0));
	for (int j = -radius; j < radius; j++)
	{
		for (int i = roundf(-radius * HALF_HEXAGON_WIDTH); i < roundf(radius * HALF_HEXAGON_WIDTH); i++)
		{
			if (HexaMosaic::InHexagon(i, j, radius))
			{
				cInt x = i+(radius*HALF_HEXAGON_WIDTH);
				cInt y = j+radius-1;
				hex_coords.push_back(cv::Point2i(x,y));
				hex_mask.at<Uint8>(y,x) = 255;
			}
		}
	}
	cv::imwrite("hexmask.jpg",hex_mask);

	// Load source image
	cv::Mat src_img = cv::imread(mSourceImage, 1);
	cv::Mat src_img_scaled;
	cv::resize(src_img,
			   src_img_scaled,
			   cv::Size(int(roundf(mWidth*radius*unit_dy)),
						int(roundf(mHeight*radius*unit_dy))
						)
			   );
	cv::Mat dst_img(int(roundf(mWidth*radius*unit_dy)),
					int(roundf(mHeight*radius*unit_dy)),
					CV_8UC3,
					cv::Scalar(0,0,0)
					);

	// Compute pca input data from source image
	cFloat dx = src_img_scaled.cols / float(mWidth);
	cFloat dy = src_img_scaled.rows / float(mHeight);
	for (int i = 0, n = coordinates.size(); i < n; i++)
	{
		cInt x = coordinates[i].x;
		cInt y = coordinates[i].y;
		cInt src_y = (y * dy);
		cInt src_x = (x * dx + ((y % 2) * (dx / 2.0f)));
		cv::Rect roi(src_x, src_y, hex_mask.cols, hex_mask.rows);
		if (roi.x+hex_mask.cols >= src_img_scaled.cols || roi.y+hex_mask.rows >= src_img_scaled.rows)
			continue;
		cv::Mat src_patch = src_img_scaled(roi);
		cv::Mat dst_patch = dst_img(roi);
		src_patch.copyTo(dst_patch, hex_mask);
	}
	cv::imwrite("dst_img.jpg",dst_img);
/*
	// Precache destination image
	cv::Mat dst_img(int(roundf(mWidth*radius*unit_dx)),
					int(roundf(mHeight*radius*unit_dx)),
					CV_8UC3
					);

	// Compute pca input data from source image
	cv::Mat pca_input(mWidth*mHeight, mTileSize*mTileSize*3, CV_8UC1);
	for (int i = 0, n = coordinates.size(); i < n; i++)
	{
		cInt x = coordinates[i].x;
		cInt y = coordinates[i].y;
		cInt src_y = roundf(start_y + y * dy);
		cInt src_x = roundf(start_x + x * dx + ((y % 2) * (dx / 2.0f)));
		cv::Mat data_row, pca_input_row = pca_input.row(i);
		DataRow(src_x, src_y, src_img, hex_mask_src, data_row);
		data_row.copyTo(pca_input_row);
	}

	std::cout << "Performing pca..." << std::flush;
	cv::PCA pca(pca_input, cv::Mat(), CV_PCA_DATA_AS_ROW, mDimensions);
	#ifdef DEBUG
	// Construct eigenvector images for debugging
	for (int i = 0; i < mDimensions; i++)
	{
		cv::Mat eigenvec;
		cv::normalize(pca.eigenvectors.row(i), eigenvec, 255, 0, cv::NORM_MINMAX);
		eigenvec = eigenvec.reshape(3, mTileSize);
		std::stringstream s;
		s << i;
		std::string entry = "eigenvector-" + s.str() + ".jpg";
		cv::imwrite(entry, eigenvec);
	}
	#endif
	std::cout << "[done]" << std::endl;

	// Compress original image data
	std::cout << "Compress source image..." << std::flush;
	cv::Mat compressed_src_img;
	CompressData(pca, pca_input, compressed_src_img);
	std::cout << "[done]" << std::endl;

	// Compress database image data
	std::cout << "Compress database..." << std::flush;
	cv::Mat compressed_database;
	CompressData(pca, mDatabase, compressed_database);
	std::cout << "[done]" << std::endl;

	// Construct mosaic
	std::cout << "Construct mosaic..." << std::flush;
	random_shuffle(coordinates.begin(), coordinates.end());
	cv::Mat src_entry, tmp_entry, dst_patch, src_patch;
	vInt ids;
	std::vector<cv::Point2i> locations;
	cv::Rect roi_patch(0,0,mTileSize,mTileSize);
	for (int i = 0, n = coordinates.size(); i < n; i++)
	{
		const cv::Point2i& loc = coordinates[i];
		src_entry = compressed_src_img.row(loc.y*mWidth+loc.x);
		// Match with database
		std::priority_queue<Match> KNN; // All nearest neighbours
		for (int k = 0; k < mNumImages; k++)
		{
			tmp_entry = compressed_database.row(k);
			float dist = GetDistance(src_entry, tmp_entry);
			KNN.push(Match(k, dist));
		}
		int best_id = KNN.top().id;
		while (!KNN.empty())
		{
			// Stop if the image is new
			if (find(ids.begin(), ids.end(), best_id) == ids.end())
				break;

			// Stop if the image has no duplicates in a certain radius
			cInt max_radius = 5;
			bool is_used = false;
			for (int k = 0, n = locations.size(); k < n; k++)
			{
				int x = locations[k].x-loc.x;
				int y = locations[k].y-loc.y;
				int radius = int(ceil(sqrt(x*x+y*y)));
				if (radius <= max_radius && ids[k] == best_id)
				{
					is_used = true;
					break;
				}
			}

			if (!is_used)
				break;
			KNN.pop();
			best_id = KNN.top().id;
		}
		ids.push_back(best_id);
		locations.push_back(loc);

		// Copy hexagon to destination
		roi_patch.x = unit_dx * radius * (loc.x + 0.5f + ((loc.y % 2) / 2.0f));
		roi_patch.y = unit_dy * radius * (loc.y + 0.5f);
		dst_patch = dst_img(roi_patch);
		src_patch = mDatabase.row(best_id);
		src_patch = src_patch.reshape(3, mTileSize);
		src_patch.copyTo(dst_patch, hex_mask);
	}

	std::stringstream s;
	s << "mosaic-pca" << mDimensions
	  << "-tilesize"  << mTileSize
	  << "-maxradius" << mMaxRadius
	  << ".jpg";

	cv::imwrite(s.str(), dst_img);
	std::cout << "[done]" << std::endl;
	std::cout << "Resulting image: " << s.str() << std::endl;
	*/
}

void HexaMosaic::DataRow(cInt inX, cInt inY, const cv::Mat &inSrcImg, const cv::Mat &inMask, cv::Mat &outDataRow) {
	cv::Rect roi(inX, inY, inMask.cols, inMask.rows);
	cv::Mat patch = inSrcImg(roi);
	patch.copyTo(outDataRow, inMask);
	outDataRow = outDataRow.reshape(1,1);
}

void HexaMosaic::LoadDatabase(rcString inFileName, cv::Mat& outDatabase) {
	int num_files = 0, rows = 0;

	// Load database
	cv::FileStorage fs(inFileName, cv::FileStorage::READ);
	fs["num_files"] >> num_files;
	fs["num_images"] >> mNumImages;
	fs["tile_size"] >> mTileSize;
	fs.release();

	cv::Mat tmp, roi;
	int current_row = 0;
	outDatabase.create(mNumImages, mTileSize*mTileSize*3, CV_8UC1);
	for (int i = 0; i < num_files; i++)
	{
		std::stringstream s;
		s << i;
		std::string entry = mDatabaseFileName + "rawdata_" + s.str() + ".yml";
		std::cout << "Loading `" << entry << "'..." << std::flush;
		fs.open(entry, cv::FileStorage::READ);
		rows = 0;
		fs["rows"] >> rows;
		for (int j = 0; j < rows; j++)
		{
			std::stringstream ss;
			ss << j;
			fs["img_" + ss.str()] >> tmp;
			roi = outDatabase.row(current_row);
			tmp.copyTo(roi);
			current_row++;
		}
		fs.release();
		std::cout << "[done]" << std::endl;
	}
}

void HexaMosaic::CompressData(const cv::PCA& inPca, const cv::Mat& inUnCompressed, cv::Mat& outCompressed) {
	outCompressed.create(inUnCompressed.rows, mDimensions, inPca.eigenvectors.type());
	cv::Mat entry, compressed_entry;
	for (int i = 0; i < inUnCompressed.rows; i++)
	{
		entry = inUnCompressed.row(i);
		compressed_entry = outCompressed.row(i);
		inPca.project(entry, compressed_entry);
	}
}

bool HexaMosaic::InHexagon(cFloat inX, cFloat inY, cFloat inRadius) {
	// NOTE: inRadius is defined from the hexagon's center to a corner
	return   fabs(inX) < (inY + inRadius) * HEXAGON_WIDTH &&
			-fabs(inX) > (inY - inRadius) * HEXAGON_WIDTH;
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
