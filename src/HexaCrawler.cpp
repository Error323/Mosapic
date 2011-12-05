#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"

#include <iostream>
#include <sstream>

void HexaCrawler::Crawl(rcString inSrcDir, rcString inDstDir, cInt inTileSize)
{
  char c = inDstDir.at(inDstDir.size()-1);
  mDstDir = inDstDir;
  if (c != '/')
    mDstDir += "/";
  mTileSize = inTileSize;
  mImgCount = 0;

  if (!boost::filesystem::exists(inDstDir))
  {
    std::cout << "Directory `" << inDstDir << "' doesn't exist yet, creating..." << std::endl;
    boost::filesystem::create_directory(inDstDir);
  }

  Crawl(inSrcDir);
  std::cout << std::endl << "Processed " << mImgCount << " images." << std::endl;
}

void HexaCrawler::Crawl(const boost::filesystem::path &inPath)
{
  static boost::match_results<std::string::const_iterator> what;
  static boost::regex img_ext(".*(bmp|BMP|jpg|JPG|jpeg|JPEG|png|PNG)");

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
      std::cerr << i->path() << " " << ex.what() << std::endl;
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
  std::cout << "Processing `" << inImgName << std::flush;

  cv::Mat img_color = cv::imread(inImgName);

  if (img_color.data == NULL || img_color.rows < mTileSize || img_color.cols < mTileSize)
  {
    std::cout << " [failed]" << std::endl;
    return;
  }

  Resize(img_color);

  std::string img_dst = mDstDir + inImgName.substr(inImgName.find_last_of('/')+1);
  cv::imwrite(img_dst, img_color);
  mImgCount++;
  std::cout << " -> " << img_dst << " [done]" << std::endl;
}
