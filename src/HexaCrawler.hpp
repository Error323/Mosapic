#ifndef HEXACRAWLER_HDR
#define HEXACRAWLER_HDR

#include <QtCore>
#include <QImage>

class HexaCrawler
{
public:
  HexaCrawler();
  ~HexaCrawler() {}

  void Crawl(const QDir &input, const QDir &output, const int size);

private:
  int mImgCount;
  int mExistCount;
  int mFailedCount;
  int mClashCount;
  int mTileSize;

  QDir mDstDir;

  void Process(const QFileInfo &info);
  void Crawl(const QDir &dir);
  void Crop(QImage &image);
  void Resize(QImage &image);
  void GammaCorrect(QImage &image, const float gamma);
  bool IsEqual(const QImage &a, const QImage &b);
};

#endif // HEXACRAWLER_HDR
