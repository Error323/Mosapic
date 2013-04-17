#ifndef MOSACRAWLER_H
#define MOSACRAWLER_H

#include <QtCore>
#include <QImage>

class MosaCrawler
{
public:
  MosaCrawler();
  ~MosaCrawler() {}

  void Crawl(const QDir &input, const QDir &output, const int size, const bool fast=true, const float gamma=1.0f);

private:
  int mImgCount;
  int mExistCount;
  int mFailedCount;
  int mClashCount;
  int mTileSize;
  float mGamma;
  bool mFastResizing;
  bool mCudaIsInitialized;

  QDir mDstDir;

  void Process(const QFileInfo &info);
  void Crawl(const QDir &dir);
  void Crop(QImage &image);
  void Resize(const QImage &image, QImage &resized);
  void GammaCorrect(QImage &image, const float gamma);
  bool IsEqual(const QImage &a, const QImage &b);
};

#endif // MOSACRAWLER_H
