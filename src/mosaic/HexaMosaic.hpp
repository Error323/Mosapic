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
    const int dimensions,
    const float colorbalance
  );

  void Create();

private:
  struct Match
  {
    Match(): id(-1), val(0.0f) {}
    Match(int pid, float v): id(pid), val(v) {}
    int id;
    float val;
    bool operator < (const Match &m) const
    {
      return val > m.val;
    }
  };

  QImage &mSourceImg;
  const QDir &mDatabaseDir;
  const int mWidth;
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
  float Distance(const RowVectorXf &a, const RowVectorXf &b);
};

#endif // HEXAMOSAIC_HDR
