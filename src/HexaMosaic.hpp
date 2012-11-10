#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <string>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "Types.hpp"

DECLARE_CLASS(HexaMosaic)

class HexaMosaic
{
public:
  HexaMosaic(
    rcString inSourceImage,
    rcString inDatabase,
    cInt inWidth,
    cInt inHeight,
    cBool inGrayscale,
    cInt inDimensions,
    cInt inMaxRadius
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

  String mSourceImage;
  String mDatabaseDir;

  int mWidth;
  int mHeight;
  bool mUseGrayscale;
  int mDimensions;
  int mMinRadius;
  int mNumImages;
  int mHexWidth;
  int mHexHeight;
  int mHexRadius;

  cv::Mat mDatabase;
  cv::Mat mHexMask;

  std::vector<cv::Point2i> mCoords;
  vInt mIndices;
  vString mImages;
};

#endif // HEXAMOSAIC_HDR
