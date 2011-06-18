#ifndef HEXACRAWLER_HDR
#define HEXACRAWLER_HDR

#include "Types.hpp"
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <fstream>

DECLARE_CLASS(HexaCrawler);
DECLARE_CLASS(Image);

class HexaCrawler {
public:
	HexaCrawler(): mImgCount(0) {}
	~HexaCrawler() {}

	void Crawl(rcString inSrcDir, rcString inDstDir);

private:
	int mImgCount;
	String mDstDir;
	std::ofstream mDatabase;

	void Crawl(const boost::filesystem::path &inPath);
	void Process(rcString inImgName);
};

#endif // HEXACRAWLER_HDR
