#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"

#include "utils/Debugger.hpp"
#include "utils/Verbose.hpp"

#include <iostream>
#include <sstream>

void HexaCrawler::Crawl(rcString inSrcDir, rcString inDstDir, cInt inTileSize)
{
  char c = inDstDir.at(inDstDir.size() - 1);
  mDstDir = inDstDir;

  if (c != '/')
    mDstDir += "/";

  mTileSize = inTileSize;
  mImgCount = 0;
  mExistCount = 0;
  mFailedCount = 0;
  mClashCount = 0;

  if (!boost::filesystem::exists(inDstDir))
  {
    NoticeLine("Directory `" << inDstDir << "' doesn't exist yet, creating...");
    boost::filesystem::create_directory(inDstDir);
  }

  Crawl(inSrcDir);
  NoticeLine("");
  NoticeLine("Failed    " << mFailedCount << " images");
  NoticeLine("Existing  " << mExistCount << " images");
  NoticeLine("Clashed   " << mClashCount << " images");
  NoticeLine("Processed " << mImgCount << " images");
}

void HexaCrawler::Crawl(const boost::filesystem::path &inPath)
{
  static boost::match_results<std::string::const_iterator> what;
  static boost::regex img_ext(".*(bmp|BMP|jpg|JPG|jpeg|JPEG|png|PNG|tiff|TIFF)");

  boost::filesystem::directory_iterator n;

  for (boost::filesystem::directory_iterator i(inPath); i != n; ++i)
  {
    try
    {
      if (boost::filesystem::is_directory(i->status()))
      {
        Crawl(i->path());
      }
      else
      {
        if (boost::filesystem::is_regular_file(i->status()) &&
            boost::regex_match(i->path().string(), what, img_ext, boost::match_default))
        {
          Process(i->path().string());
        }
      }
    }
    catch (const std::exception &ex)
    {
      ErrorLine(i->path() << " " << ex.what());
    }
  }
}

void HexaCrawler::Resize(cv::Mat &outImg)
{
  int min = std::min<int>(outImg.rows, outImg.cols);
  int width  = min - (min % mTileSize);
  int height = min - (min % mTileSize);
  cv::Mat img_sub;
  cv::Size size(width, height);
  cv::Point2f center(outImg.cols / 2, outImg.rows / 2);
  cv::getRectSubPix(outImg, size, center, img_sub);
  cv::Mat img_sub_blur;
  cv::blur(img_sub, img_sub_blur, cv::Size(3, 3));
  cv::resize(img_sub_blur, outImg, cv::Size(mTileSize, mTileSize));
}

void HexaCrawler::Process(rcString inImgName)
{
  Notice("Processing `" << inImgName << "'");

  size_t s_pos = inImgName.find_last_of('/') + 1;
  size_t e_pos = inImgName.find_last_of('.') - s_pos;
  std::string img_dst = mDstDir + inImgName.substr(s_pos, e_pos) + ".tiff";

  cv::Mat img_color = cv::imread(inImgName);

  if (img_color.data == NULL || img_color.rows < mTileSize || img_color.cols < mTileSize)
  {
    ErrorLine(" [failed]");
    mFailedCount++;
    return;
  }

  Resize(img_color);

  int clash_count = 0;
  while (boost::filesystem::exists(img_dst))
  {
    cv::Mat img_existing = cv::imread(img_dst);
    cv::Mat equal = (img_existing == img_color);
    int sum = cv::sum(equal)[0];
    if (sum == img_color.rows*img_color.cols*255)
    {
      WarningLine(" [exists]");
      mExistCount++;
      return;
    }
    else
    {
      Warning(" [clash]");
      std::stringstream ss;
      ss << clash_count;
      img_dst = mDstDir + inImgName.substr(s_pos, e_pos) + "-" + ss.str() + ".tiff";
      clash_count++;
      mClashCount++;
    }
  }

  cv::imwrite(img_dst, img_color);
  mImgCount++;
  NoticeLine(" -> " << img_dst << " [done]");
}
