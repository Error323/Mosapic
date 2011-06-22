#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"

#include <iostream>
#include <sstream>

#define TILE_SIZE 100
#define ROWS_PER_FILE 1000
void HexaCrawler::Crawl(rcString inSrcDir, rcString inDstDir) {
	mDstDir = inDstDir;
	if (!boost::filesystem::exists(inDstDir))
	{
		std::cout << "Directory `" << inDstDir << "' doesn't exist yet, creating..." << std::endl;
		boost::filesystem::create_directory(inDstDir);
	}
	OpenFS();
	Crawl(inSrcDir);
	CloseFS();
	fs.open(mDstDir + "/rawdata.yml", cv::FileStorage::WRITE);
	fs << "num_files" << mFileCount;
	fs << "num_images" << mImgCount;
	fs.release();
	std::cout << std::endl << "Processed " << mImgCount << " images." << std::endl;
	std::cout << std::endl << "Files " << mFileCount << "." << std::endl;
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

void HexaCrawler::OpenFS() {
	std::stringstream s;
	s << mFileCount;
	fs.open(mDstDir + "/rawdata_" + s.str() + ".yml", cv::FileStorage::WRITE);
	mFileCount++;
}

void HexaCrawler::CloseFS() {
	int remaining_images = mImgCount % ROWS_PER_FILE;
	fs << "rows" << ((remaining_images == 0) ? ROWS_PER_FILE : remaining_images);
	fs << "cols" << TILE_SIZE*TILE_SIZE*3;
	fs << "file" << mFileCount;
	fs.release();
}

void HexaCrawler::Resize(cv::Mat& outImg) {
	int min = std::min<int>(outImg.rows, outImg.cols);
	cv::Mat img_tmp;
	cv::Size size(min,min);
	cv::Point2f center(outImg.cols/2.0f, outImg.rows/2.0f);
	cv::getRectSubPix(outImg, size, center, img_tmp);
	cv::resize(img_tmp, outImg, cv::Size(TILE_SIZE,TILE_SIZE));
}

void HexaCrawler::Process(rcString inImgName) {
	std::cout << "Processing `" << inImgName << "'..." << std::flush;

	cv::Mat img_color = cv::imread(inImgName, 1);
	if (img_color.data == NULL || img_color.rows < TILE_SIZE || img_color.cols < TILE_SIZE)
	{
		std::cout << "[failed]" << std::endl;
		return;
	}

	HexaCrawler::Resize(img_color);

	img_color = img_color.reshape(1, 1);
	std::stringstream s;
	s << mImgCount;
	fs << "img_" + s.str() << img_color;
	mImgCount++;
	if (mImgCount % ROWS_PER_FILE == 0)
	{
		CloseFS();
		OpenFS();
	}
	std::cout << "[done]" << std::endl;
}
