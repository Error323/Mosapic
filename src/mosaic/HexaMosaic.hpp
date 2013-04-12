#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <string>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "../utils/Types.hpp"

class HexaMosaic
{
public:
  HexaMosaic(
    rcString inSourceImage,
    rcString inDatabase,
    cInt inWidth,
    cInt inHeight,
    cInt inDimensions,
    cInt inMinRadius,
    cFloat inCBRatio
  );

  void Create();

private:
  struct Match
  {
    Match(): id(-1), val(-1.0f) {}
    Match(int pid, float v): id(pid), val(v) {}
    int id;
    float val;
    bool operator< (const Match &m) const
    {
      return val > m.val;
    }
  };

  float GetDistance(
    const cv::Mat &inSrcRow,
    const cv::Mat &inDataRow
  );

  bool InHexagon(
    cFloat inX,
    cFloat inY,
    cFloat inRadius
  );

  void Crawl(const boost::filesystem::path &inPath);
  void Process(rcString inImgName);
  void ColorBalance(cv::Mat &ioSrc, const cv::Mat &inDst);
  void Im2HexRow(const cv::Mat &in, cv::Mat &out);
  void HexRow2Im(const cv::Mat &in, cv::Mat &out);
  void LoadImage(rcString inImageName, cv::Mat &out);

  String mSourceImage;
  String mDatabaseDir;

  int mWidth;
  int mHeight;
  bool mUseGrayscale;
  int mDimensions;
  int mMinRadius;
  float mCBRatio;
  int mNumImages;

  int mHexWidth;
  int mHexHeight;
  int mHexRadius;
  int mDstWidth;
  int mDstHeight;


  cv::Mat mHexMask;
  cv::Mat mSrcImg;

  std::vector<cv::Point2i> mCoords;
  std::vector<cv::Point2i> mHexCoords;
  vInt mIndices;
  vString mImages;
};

#endif // HEXAMOSAIC_HDR
