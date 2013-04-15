#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <QtCore>
#include <QtGui>

#include <Eigen/Dense>

using namespace Eigen;

class HexaMosaic
{
public:
  HexaMosaic(
    QImage &image,
    const QDir &database,
    const int width,
    const int radius,
    const int dimensions,
    const float colorbalance
  );

  void Create();

private:
  QImage &mSourceImg;
  const QDir &mDatabaseDir;
  const int mWidth;
  const int mRadius;
  const int mDimensions;
  const float mColorBalance;

  int mHeight;
  int mTileSize;
  int mPixelWidth;
  int mPixelHeight;

  QVector<QFileInfo> mDatabase;
  QVector<QPoint> mCoordinates;
  QVector<int> mIndices;

  void Crawl(const QDir &dir);

  /*
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
  */
};

#endif // HEXAMOSAIC_HDR
