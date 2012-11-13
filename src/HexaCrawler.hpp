#ifndef HEXACRAWLER_HDR
#define HEXACRAWLER_HDR

#include "utils/Types.hpp"
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>

DECLARE_CLASS(HexaCrawler)

class HexaCrawler
{
public:
  HexaCrawler(): mImgCount(0) {}
  ~HexaCrawler() {}

  void Crawl(rcString inSrcDir, rcString inDstDir, cInt inTileSize);
  void Resize(cv::Mat &outImg);

private:
  int mImgCount;
  int mTileSize;
  String mDstDir;

  void Crawl(const boost::filesystem::path &inPath);
  void Process(rcString inImgName);
};

#endif // HEXACRAWLER_HDR
