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
	rcString inDatabase,
	cInt inWidth,
	cInt inHeight,
	cInt inDimensions,
	cInt inMinRadius
	):
	mSourceImage(inSourceImage),
	mDatabaseDir(inDatabase),
	mWidth(inWidth),
	mHeight(inHeight),
	mDimensions(inDimensions),
	mMinRadius(inMinRadius)
{
	cv::FileStorage fs(mDatabaseDir + DATABASE_NAME, cv::FileStorage::READ);
	fs["num_images"] >> mNumImages;
	fs["hex_width"] >> mHexWidth;
	fs["hex_height"] >> mHexHeight;
	fs.release();
	mHexRadius = mHexHeight / 2.0f;

	// Cache coordinates, so we can e.g. randomize
	for (int y = 0; y < mHeight; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			if (y % 2 == 1 && x == mWidth-1)
				continue;
			mIndices.push_back(mCoords.size());
			mCoords.push_back(cv::Point2i(x,y));
		}
	}

	// Precache hexagon coordinates
	mHexMask.create(mHexHeight, mHexWidth, CV_8UC1);
	mHexMask.setTo(cv::Scalar(0));
	for (int j = -mHexRadius; j < mHexRadius; j++)
	{
		for (int i = roundf(-mHexRadius * HALF_HEXAGON_WIDTH); i < roundf(mHexRadius * HALF_HEXAGON_WIDTH); i++)
		{
			if (HexaMosaic::InHexagon(i, j, mHexRadius))
			{
				cInt x = i+(mHexRadius*HALF_HEXAGON_WIDTH);
				cInt y = j+mHexRadius;
				mHexMask.at<Uint8>(y,x) = 255;
			}
		}
	}

	#ifdef DEBUG
	cv::imwrite("hexmask.jpg",mHexMask);
	#endif // DEBUG
}

void HexaMosaic::Create() {
	// Load the database (raw images)
	LoadDatabase(mDatabase);

	// unit dimensions of hexagon facing upwards
	cFloat unit_dx = HEXAGON_WIDTH;
	cFloat unit_dy = HEXAGON_HEIGHT * (3.0f/4.0f);

	// Load source image
	cv::Mat src_img = cv::imread(mSourceImage, 1);
	cv::Mat dst_img(mHeight*mHexHeight*.75+mHexHeight*.25+mHeight*.25,
					mWidth*mHexWidth+mWidth*.25,
					CV_8UC3);

	// Compute pca input data from source image
	cv::Mat pca_input(mCoords.size(), mHexHeight*mHexWidth*3, CV_8UC1);
	float dx = src_img.cols / float(mWidth);
	float dy = src_img.rows / float(mHeight);
	for (int i = 0, n = mCoords.size(); i < n; i++)
	{
		cInt x = mCoords[i].x;
		cInt y = mCoords[i].y;
		cInt src_y = roundf(y * dy);
		cInt src_x = roundf(x * dx + ((y % 2) * (dx / 2.0f)));
		cv::Rect roi(src_x, src_y, roundf(dx), roundf(dy));
		cv::Mat data_row, patch = src_img(roi);
		cv::resize(patch, data_row, cv::Size(mHexWidth, mHexHeight));
		cv::Mat pca_input_row = pca_input.row(i);
		data_row = data_row.reshape(1,1);
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
		eigenvec = eigenvec.reshape(3, mHexHeight);
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
	dx = mHexRadius*unit_dx;
	dy = mHexRadius*unit_dy;
	random_shuffle(mIndices.begin(), mIndices.end());
	cv::Mat src_entry, tmp_entry, dst_patch, src_patch;
	vInt ids;
	std::vector<cv::Point2i> locations;
	for (int i = 0, n = mCoords.size(); i < n; i++)
	{
		const cv::Point2i& loc = mCoords[mIndices[i]];
		src_entry = compressed_src_img.row(mIndices[i]);
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
			bool is_used = false;
			for (int k = 0, n = locations.size(); k < n; k++)
			{
				cInt x = locations[k].x-loc.x;
				cInt y = locations[k].y-loc.y;
				cInt r = int(ceil(sqrt(x*x+y*y)));
				if (r <= mMinRadius && ids[k] == best_id)
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
		cInt src_y = (loc.y * dy);
		cInt src_x = (loc.x * dx + ((loc.y % 2) * (dx / 2.0f)));
		cv::Rect roi(src_x, src_y, mHexWidth, mHexHeight);
//		cv::rectangle(dst_img, roi, cv::Scalar(255,255,255));
//		cv::imshow("dst_img", dst_img);
//		cv::waitKey();
		dst_patch = dst_img(roi);
		std::stringstream s;
		s << best_id;
		src_patch = cv::imread(mDatabaseDir + "img_" + s.str() + ".jpg");
		src_patch.copyTo(dst_patch, mHexMask);
	}
	std::cout << "[done]" << std::endl;

	// TODO: anti-aliasing

	// Write image to disk
	std::stringstream s;
	s << "mosaic-" << mWidth << "x" << mHeight
	  << "-pca" << mDimensions
	  << "-hexdims"  << mHexWidth << "x" << mHexHeight
	  << "-minradius" << mMinRadius
	  << ".jpg";

	cv::imwrite(s.str(), dst_img);
	std::cout << "Resulting image: " << s.str() << std::endl;
}

void HexaMosaic::DataRow(cInt inX, cInt inY, const cv::Mat &inSrcImg, const cv::Mat &inMask, cv::Mat &outDataRow) {
	cv::Rect roi(inX, inY, inMask.cols, inMask.rows);
	cv::Mat patch = inSrcImg(roi);
	patch.copyTo(outDataRow, inMask);
	outDataRow = outDataRow.reshape(1,1);
}

void HexaMosaic::LoadDatabase(cv::Mat& outDatabase) {
	std::cout << "Loading " << mNumImages << " images..." << std::flush;
	cv::Mat roi, row;
	outDatabase.create(mNumImages, mHexWidth*mHexHeight*3, CV_8UC1);
	for (int i = 0; i < mNumImages; i++)
	{
		std::stringstream s;
		s << i;
		std::string entry = mDatabaseDir + IMAGE_PREFIX + s.str() + IMAGE_EXT;
		row = cv::imread(entry, 1).reshape(1,1);
		roi = outDatabase.row(i);
		row.copyTo(roi);
	}
	std::cout << "[done]" << std::endl;
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
	cv::Mat tmp;
	cv::subtract(inSrcRow, inDataRow, tmp);
	cv::pow(tmp, 2.0, tmp);
	return sqrtf(cv::sum(tmp)[0]);
}
