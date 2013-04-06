#ifndef HEXACRAWLER_HDR
#define HEXACRAWLER_HDR

#include <QtCore>
#include <QImage>

#include <Eigen/Dense>

using namespace Eigen;

class HexaCrawler
{
public:
  HexaCrawler();
  ~HexaCrawler() {}

  void Crawl(const QDir &input, const QDir &output, const int tileSize);

private:
  int mImgCount;
  int mExistCount;
  int mFailedCount;
  int mClashCount;
  int mTileSize;
  std::vector<float> mKernel;
  VectorXf mXKernel;
  VectorXf mYKernel;
  MatrixXf mFullKernel;
  MatrixXf mWindowRed;
  MatrixXf mWindowGreen;
  MatrixXf mWindowBlue;

  QDir mDstDir;

  void Crawl(const QDir &dir);
  void Process(const QFileInfo &info);
  void Crop(QImage &image);
  void Resize(QImage &image);
  void GammaCorrect(QImage &image, const float gamma);
  QRgb InterpolateLanczos(const QImage &image, const int sx, const int sy, const float xfrac, const float yfrac);
  bool IsEqual(const QImage &a, const QImage &b);
};

#endif // HEXACRAWLER_HDR
