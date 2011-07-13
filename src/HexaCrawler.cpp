#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"

#include <iostream>
#include <sstream>

void HexaCrawler::Crawl(rcString inSrcDir, rcString inDstDir, cInt inHexHeight) {
	mDstDir    = inDstDir;
	mHexHeight = inHexHeight;
	mHexWidth  = roundf(mHexHeight/2.0f*HEXAGON_WIDTH);
	mImgCount  = 0;

	if (!boost::filesystem::exists(inDstDir))
	{
		std::cout << "Directory `" << inDstDir << "' doesn't exist yet, creating..." << std::endl;
		boost::filesystem::create_directory(inDstDir);
	}
	Crawl(inSrcDir);
	fs.open(mDstDir + DATABASE_NAME, cv::FileStorage::WRITE);
	fs << "num_images" << mImgCount;
	fs << "hex_width" << mHexWidth;
	fs << "hex_height" << mHexHeight;
	fs.release();
	std::cout << std::endl << "Processed " << mImgCount << " images." << std::endl;
}

void HexaCrawler::Crawl(const boost::filesystem::path &inPath) {
	static boost::match_results<std::string::const_iterator> what;
	static boost::regex img_ext(".*(bmp|BMP|jpg|JPG|jpeg|JPEG|png|PNG)");

	boost::filesystem::directory_iterator n;
	for (boost::filesystem::directory_iterator i(inPath); i != n; ++i) 
	{
		try
		{
			if (boost::filesystem::is_directory(i->status()))
				Crawl(i->path());
			else 
				if (boost::filesystem::is_regular_file(i->status()))
				{
					if (boost::regex_match(i->path().leaf(), what, img_ext, boost::match_default))
						Process(i->path().string());
				}
		}
		catch (const std::exception &ex)
		{
			std::cerr << i->path() << " " << ex.what() << std::endl;
		}
	}
}

void HexaCrawler::Resize(cv::Mat& outImg) {
	int min = std::min<int>(outImg.rows, outImg.cols);
	int width  = min - (min % mHexWidth);
	int height = min - (min % mHexHeight);
	cv::Mat img_tmp;
	cv::Size size(width, height);
	cv::Point2f center(outImg.cols/2, outImg.rows/2);
	cv::getRectSubPix(outImg, size, center, img_tmp);
	cv::resize(img_tmp, outImg, cv::Size(mHexWidth, mHexHeight));
}

void HexaCrawler::Process(rcString inImgName) {
	std::cout << "Processing `" << inImgName << "'..." << std::flush;

	cv::Mat img_color = cv::imread(inImgName, 1);
	if (img_color.data == NULL || img_color.rows < mHexHeight || img_color.cols < mHexHeight)
	{
		std::cout << "[failed]" << std::endl;
		return;
	}

	Resize(img_color);

	std::stringstream s;
	s << mImgCount;
	cv::imwrite(mDstDir + IMAGE_PREFIX + s.str() + IMAGE_EXT, img_color);
	mImgCount++;
	std::cout << "[done]" << std::endl;
}
