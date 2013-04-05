#ifndef HEXACRAWLER_HDR
#define HEXACRAWLER_HDR

#include <QtCore>
#include <QImage>

class HexaCrawler
{
public:
  HexaCrawler():
    mImgCount(0),
    mExistCount(0),
    mFailedCount(0),
    mClashCount(0) {}
  ~HexaCrawler() {}

  void Crawl(const QDir &input, const QDir &output, const int tileSize);
  void Resize(QImage &image);

private:
  int mImgCount;
  int mExistCount;
  int mFailedCount;
  int mClashCount;
  int mTileSize;
  QDir mDstDir;

  void Crawl(const QDir &dir);
  void Process(const QFileInfo &info);
  bool Exists(const QImage &a, const QImage &b);
};

#endif // HEXACRAWLER_HDR
