#ifndef HEXACRAWLER_HDR
#define HEXACRAWLER_HDR

#include "Types.hpp"
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <opencv2/opencv.hpp>

DECLARE_CLASS(HexaCrawler)

class HexaCrawler {
public:
	HexaCrawler(): mImgCount(0) {}
	~HexaCrawler() {}

	void Crawl(rcString inSrcDir, rcString inDstDir, cInt inTileSize);
	void Resize(cv::Mat& outImg);

private:
	int mImgCount;
	int mHexWidth;
	int mHexHeight;
	String mDstDir;
	cv::FileStorage fs;

	void Crawl(const boost::filesystem::path &inPath);
	void Process(rcString inImgName);
};

#endif // HEXACRAWLER_HDR
