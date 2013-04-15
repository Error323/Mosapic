#include "HexaMosaic.hpp"

#include "../utils/pca/PCA.hpp"
#include "../utils/Debugger.hpp"
#include "../utils/Verbose.hpp"

#include <algorithm>
#include <queue>
#include <cmath>

HexaMosaic::HexaMosaic(QImage &image,
    const QDir &database,
    const int width,
    const int dimensions,
    const float colorbalance):
  mSourceImg(image),
  mDatabaseDir(database),
  mWidth(width),
  mDimensions(dimensions),
  mColorBalance(colorbalance)
{
  LoadDatabase(mDatabaseDir);
  if (mDatabase.empty())
    FatalLine("Error: Database is empty");

  QImage img(mDatabase.front().absoluteFilePath());

  if (img.isNull() || img.width() != img.height())
    FatalLine("Error: Corrupt database");

  mTileSize = img.width();

  // Find mHeight such that the ratio is closest to original
  float orig_ratio = mSourceImg.width() / float(mSourceImg.height());
  mPixelWidth = mWidth * mTileSize;
  mHeight = 1;
  float best_ratio = std::numeric_limits<float>::max();
  while (true)
  {
    mPixelHeight = mHeight * mTileSize;
    float cur_ratio = mPixelWidth / float(mPixelHeight);

    if (fabs(orig_ratio - best_ratio) < fabs(orig_ratio - cur_ratio))
    {
      mHeight--;
      mPixelHeight = mHeight * mTileSize;
      break;
    }

    best_ratio = cur_ratio;
    mHeight++;
  }

  for (int x = 0; x < mWidth; x++)
  {
    for (int y = 0; y < mHeight; y++)
    {
      mIndices.append(mCoordinates.size());
      mCoordinates.append(QPoint(x*mTileSize, y*mTileSize));
    }
  }

  srand(0);
  std::random_shuffle(mIndices.begin(), mIndices.end());

  DebugLine("Dabase size:        " << mDatabase.size());
  DebugLine("Tile size:          " << mTileSize);
  DebugLine("Input image size:   " << mSourceImg.width() << "x" << mSourceImg.height());
  DebugLine("Input image ratio:  " << orig_ratio);
  DebugLine("Input image bytes:  " << mSourceImg.numBytes());
  DebugLine("Output image size:  " << mPixelWidth << "x" << mPixelHeight);
  DebugLine("Output image ratio: " << best_ratio);
  DebugLine("Output image tiles: " << mWidth << "x" << mHeight);
}

void HexaMosaic::Create()
{
  /*****************************************************************************
   * 1. Resize source image to output image. Convert source image into tiles,
   *    put each tile as row in a matrix
   ****************************************************************************/
  mSourceImg = mSourceImg.scaled(mWidth*mTileSize, mHeight*mTileSize,
                                 Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
  PCA pca(mCoordinates.size(), mTileSize*mTileSize*4);
  for (int i = 0; i < mWidth; i++)
  {
    for (int j = 0; j < mHeight; j++)
    {
      QImage img = mSourceImg.copy(i*mTileSize, j*mTileSize,
                                   mTileSize, mTileSize);
      Map<Matrix<uchar, 1, Dynamic> > row(img.bits(), img.byteCount());
      pca.AddRow(row.cast<float>());
    }
  }

  /*****************************************************************************
   * 2. Perform principal component analysis and visualize eigenvectors
   ****************************************************************************/
  Notice("Performing pca...");
  pca.Solve(mDimensions);
#ifndef NDEBUG
  RowVectorXf eigenvector;
  for (int i = 0; i < mDimensions; i++)
  {
    pca.GetEigenVector(i, eigenvector);
    eigenvector.array() -= eigenvector.minCoeff();
    eigenvector.array() /= eigenvector.maxCoeff();
    eigenvector.array() *= 255.0f;
    Matrix<uchar, 1, Dynamic> v = eigenvector.cast<uchar>();
    QImage vec_img(v.data(), mTileSize, mTileSize, mSourceImg.format());
    vec_img.save(QString("eigenvec-")+QString::number(i)+".jpg");
  }
#endif // NDEBUG
  NoticeLine("[done]");

  /*****************************************************************************
   * 3. Project source image onto eigenvectors as our basis, reconstruct source
   *    image with reduced data as visualization
   ****************************************************************************/
  Notice("Project source image...");
  MatrixXf projected_src;
  pca.GetProjectedData(projected_src);
#ifndef NDEBUG
  MatrixXf reduced_row;
  for (int i = 0, idx = 0; i < mWidth; i++)
  {
    for (int j = 0; j < mHeight; j++, idx++)
    {
      pca.BackProject(projected_src.row(idx), reduced_row);
      Matrix<uchar, Dynamic, Dynamic> data = reduced_row.cast<uchar>();
      QImage img(data.data(), mTileSize, mTileSize, mSourceImg.format());
      for (int x = 0; x < mTileSize; x++)
        for (int y = 0; y < mTileSize; y++)
          mSourceImg.setPixel(i*mTileSize+x, j*mTileSize+y, img.pixel(x,y));
    }
  }
  mSourceImg.save(QString("reduced-")+QString::number(mDimensions)+".jpg");
#endif
  NoticeLine("[done]");

  /*****************************************************************************
   * 4. Project the entire database for comparison later
   ****************************************************************************/
  Notice("Project database...");
  MatrixXf projected_db(mDatabase.size(), mDimensions);
  MatrixXf projected_row;
  for (int i = 0; i < mDatabase.size(); i++)
  {
    QImage db_img(mDatabase[i].absoluteFilePath());
    Map<Matrix<uchar, 1, Dynamic> > row(db_img.bits(), db_img.byteCount());
    pca.Project(row.cast<float>(), projected_row);
    projected_db.row(i) = projected_row;
  }
  NoticeLine("[done]");

  /*****************************************************************************
   * 5. Construct and save the mosaic using K-nearest neighbour with K=1, make
   *    sure we use an image from the database at most once
   ****************************************************************************/
  Notice("Construct mosaic...");
  QVector<int> used_ids;
  for (int i = 0; i < mIndices.size(); i++)
  {
    const QPoint &p = mCoordinates[mIndices[i]];

    std::priority_queue<Match> KNN;
    Match m;
    for (int j = 0; j < mDatabase.size(); j++)
    {
      m.id = j;
      m.val = Distance(projected_src.row(mIndices[i]), projected_db.row(j));
      KNN.push(m);
    }

    int best_id = KNN.top().id;
    while (!KNN.empty())
    {
      // Stop if the image is new
      if (std::find(used_ids.begin(), used_ids.end(), best_id) == used_ids.end())
        break;

      KNN.pop();
      best_id = KNN.top().id;
    }
    used_ids.push_back(best_id);
    QImage best_img(mDatabase[best_id].absoluteFilePath());
    for (int x = 0; x < mTileSize; x++)
      for (int y = 0; y < mTileSize; y++)
        mSourceImg.setPixel(p.x()+x, p.y()+y, best_img.pixel(x,y));
  }
  NoticeLine("[done]");

  Notice("Save image...");
  QString file_name("size:");
  file_name += QString::number(mWidth) + "x" + QString::number(mHeight);
  file_name += "-pca:" + QString::number(mDimensions);
  file_name += "-db:" + mDatabaseDir.dirName();
  file_name += "-cbr:" + QString::number(mColorBalance);
  file_name += ".tiff";
  mSourceImg.save(file_name);
  NoticeLine("[done]");
  NoticeLine("\nImage stored as: " << qPrintable(file_name));
}

float HexaMosaic::Distance(const RowVectorXf &a, const RowVectorXf &b)
{
  ASSERT(a.cols() == b.cols());
  return sqrtf((a.array()-b.array()).square().sum());
}

void HexaMosaic::LoadDatabase(const QDir &dir)
{
  QFileInfoList list = dir.entryInfoList();

  for (int i = 0; i < list.size(); i++)
  {
    QFileInfo &info = list[i];
    if (info.fileName() == "." || info.fileName() == "..")
      continue;

    if (info.isDir())
      LoadDatabase(info.absoluteFilePath());
    else
      mDatabase.append(info);
  }
}
