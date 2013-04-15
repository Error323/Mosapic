#include "HexaMosaic.hpp"

#include "../utils/pca/PCA.hpp"
#include "../utils/Debugger.hpp"
#include "../utils/Verbose.hpp"

#include <algorithm>

HexaMosaic::HexaMosaic(QImage &image,
    const QDir &database,
    const int width,
    const int radius,
    const int dimensions,
    const float colorbalance):
  mSourceImg(image),
  mDatabaseDir(database),
  mWidth(width),
  mRadius(radius),
  mDimensions(dimensions),
  mColorBalance(colorbalance)
{
  Crawl(mDatabaseDir);
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
  float prev_ratio = std::numeric_limits<float>::max();
  while (true)
  {
    mPixelHeight = mHeight * mTileSize;
    float cur_ratio = mPixelWidth / float(mPixelHeight);

    if (fabs(orig_ratio - prev_ratio) < fabs(orig_ratio - cur_ratio))
    {
      mHeight--;
      mPixelHeight = mHeight * mTileSize;
      break;
    }

    prev_ratio = cur_ratio;
    mHeight++;
  }

  for (int i = 0; i < mWidth; i++)
  {
    for (int j = 0; j < mHeight; j++)
    {
      mCoordinates.append(QPoint(i, j));
      mIndices.append(mCoordinates.size());
    }
  }

  srand(0);
  std::random_shuffle(mIndices.begin(), mIndices.end());

  DebugLine("Dabase size:        " << mDatabase.size());
  DebugLine("Tile size:          " << mTileSize);
  DebugLine("Input image size:   " << mSourceImg.width() << "x" << mSourceImg.height());
  DebugLine("Input image ratio:  " << orig_ratio);
  DebugLine("Input image bytes:  " << mSourceImg.numBytes());
  DebugLine("Input image format: " << mSourceImg.format());
  DebugLine("Output image size:  " << mPixelWidth << "x" << mPixelHeight);
  DebugLine("Output image ratio: " << prev_ratio);
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
}

void HexaMosaic::Crawl(const QDir &dir)
{
  QFileInfoList list = dir.entryInfoList();

  for (int i = 0; i < list.size(); i++)
  {
    QFileInfo &info = list[i];
    if (info.fileName() == "." || info.fileName() == "..")
      continue;

    if (info.isDir())
      Crawl(info.absoluteFilePath());
    else
      mDatabase.append(info);
  }
}

/*
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <queue>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <limits>
#include <opencv/highgui.h>

// Unit hexagon (i.e. edge length = 1) with its corners facing north and south
#define HALF_HEXAGON_WIDTH sinf(M_PI / 3.0f)
#define HEXAGON_WIDTH      (2.0f * HALF_HEXAGON_WIDTH)
#define HEXAGON_HEIGHT     2.0f

#define COUNTER_START_VAL 10
#define INIT_COUNTER(c) int c = COUNTER_START_VAL

#define COUNT_DOWN(i, c, v)                       \
  do {                                            \
    if (i % (v/COUNTER_START_VAL) == 0 && c >= 0) \
    {                                             \
      Notice(c << " ");                           \
      c--;                                        \
    }                                             \
  } while(0)                                      \

HexaMosaic::HexaMosaic(
  rcString inSourceImage,
  rcString inDatabase,
  cInt inWidth,
  cInt inHeight,
  cInt inDimensions,
  cInt inMinRadius,
  cFloat inCBRatio
):
  mSourceImage(inSourceImage),
  mWidth(inWidth),
  mHeight(inHeight),
  mDimensions(inDimensions),
  mMinRadius(inMinRadius),
  mCBRatio(inCBRatio),
  mNumImages(0)
{
  ASSERT(mCBRatio >= 0.0f && mCBRatio <= 1.0f);

  mDatabaseDir = inDatabase.at(inDatabase.size() - 1) == '/' ? inDatabase : inDatabase + '/';
  Crawl(mDatabaseDir);

  ASSERT_MSG(!mImages.empty(), "Database `%s' doesn't contain images",
             mDatabaseDir.c_str());
  cv::Mat first = cv::imread(mImages.front(), 1);
  ASSERT_MSG(first.data != NULL && first.rows == first.cols && first.rows > 0,
             "First image `%s' is not valid", mImages.front().c_str());

  mHexHeight = first.rows;
  mHexRadius = mHexHeight / 2.0f;
  mHexWidth  = roundf(mHexRadius * HEXAGON_WIDTH);

  mSrcImg = cv::imread(mSourceImage, 1);
  ASSERT_MSG(mSrcImg.data != NULL && mSrcImg.rows > 0 && mSrcImg.cols > 0, "Invalid input image");


  // Find mHeight such that the ratio is closest to original
  float orig_ratio = mSrcImg.cols / float(mSrcImg.rows);
  mDstWidth = mWidth * mHexWidth + mWidth * .25;
  float prev_ratio = 100.0f;

  int height = 1;

  while (true)
  {
    mDstHeight = height * mHexHeight * .75 + mHexHeight * .25 + height * .25;
    float cur_ratio = mDstWidth / float(mDstHeight);

    if (fabs(orig_ratio - prev_ratio) < fabs(orig_ratio - cur_ratio))
    {
      mHeight = height - 1;
      mDstHeight = mHeight * mHexHeight * .75 + mHexHeight * .25 + mHeight * .25;
      break;
    }

    prev_ratio = cur_ratio;
    height++;
  }

  DebugLine("Original(" << mSrcImg.cols << "x" << mSrcImg.rows
            << ") Tiles(" << mWidth << "x" << mHeight << ") Final("
            << mDstWidth << "x" << mDstHeight << ")");

  // Cache coordinates, so we can e.g. randomize
  for (int y = 0; y < mHeight; y++)
  {
    for (int x = 0; x < mWidth; x++)
    {
      if (y % 2 == 1 && x == mWidth - 1)
        continue;

      mIndices.push_back(mCoords.size());
      mCoords.push_back(cv::Point2i(x, y));
    }
  }
  srand(0);
  random_shuffle(mIndices.begin(), mIndices.end());

  // Precache hexagon mask
  mHexMask.create(mHexHeight, mHexWidth, CV_8UC1);
  mHexMask.setTo(cv::Scalar(0));

  for (int j = -mHexRadius; j < mHexRadius; j++)
  {
    for (int i = roundf(-mHexRadius * HALF_HEXAGON_WIDTH); i < roundf(mHexRadius * HALF_HEXAGON_WIDTH); i++)
    {
      if (HexaMosaic::InHexagon(i, j, mHexRadius))
      {
        cInt x = i + (mHexRadius * HALF_HEXAGON_WIDTH);
        cInt y = j + mHexRadius;
        mHexMask.at<Uint8>(y, x) = 255;
        cv::Point2i p(x, y);
        mHexCoords.push_back(p);
      }
    }
  }

#ifndef NDEBUG
  cv::imwrite("hexmask.jpg", mHexMask);
#endif // NDEBUG
}

void HexaMosaic::Im2HexRow(const cv::Mat &in, cv::Mat &out)
{
  out.create(1, mHexCoords.size(), CV_8UC3);

  for (int i = 0, n = mHexCoords.size(); i < n; i++)
  {
    cv::Point2i &p = mHexCoords[i];
    out.at<cv::Vec3b>(0, i) = in.at<cv::Vec3b>(p.y, p.x);
  }

  out = out.reshape(1, 1);
}

void HexaMosaic::Create()
{
  // unit dimensions of hexagon facing upwards
  cFloat unit_dx = HEXAGON_WIDTH;
  cFloat unit_dy = HEXAGON_HEIGHT * (3.0f / 4.0f);

  // Compute pca input data from source image
  cv::Mat pca_input(mCoords.size(), mHexCoords.size() * 3, CV_8UC1);
  PCA pca(mCoords.size(), mHexCoords.size() * 3);
  float dx = mSrcImg.cols / float(mWidth);
  float dy = mSrcImg.rows / float(mHeight);

  for (int i = 0, n = mCoords.size(); i < n; i++)
  {
    cInt x = mCoords[i].x;
    cInt y = mCoords[i].y;
    cInt src_y = (y * dy);
    cInt src_x = (x * dx + ((y % 2) * (dx / 2.0f)));
    cv::Rect roi(src_x, src_y, roundf(dx), roundf(dy));
    cv::Mat data_row, patch_resized, patch = mSrcImg(roi);
    cv::resize(patch, patch_resized, cv::Size(mHexWidth, mHexHeight));
    Im2HexRow(patch_resized, data_row);
    cv::Mat pca_input_row = pca_input.row(i);
    data_row.copyTo(pca_input_row);
    pca.AddRow(data_row);
  }

  Notice("Performing pca...");
  pca.Solve(mDimensions);
#ifndef NDEBUG

  // Construct eigenvector images for debugging
  for (int i = 0; i < mDimensions; i++)
  {
    cv::Mat eigenvec;
    cv::Mat correct;
    pca.GetEigenVector(i, eigenvec);
    cv::normalize(eigenvec, eigenvec, 255, 0, cv::NORM_MINMAX);
    eigenvec.convertTo(correct, CV_8UC3);
    HexRow2Im(correct, eigenvec);
    std::stringstream s;
    s << i;
    std::string entry = "eigenvector-" + s.str() + ".jpg";
    cv::imwrite(entry, eigenvec);
  }

#endif // DEBUG
  NoticeLine("[done]");

  // Compress original image data
  Notice("Compress source image...");
  cv::Mat compressed_src_img(pca_input.rows, mDimensions, CV_32FC1);
  pca.Project(pca_input, compressed_src_img);
  NoticeLine("[done]");

  // Compress database image data
  Notice("Compress database...");
  INIT_COUNTER(compress);
  cv::Mat compressed_database(mNumImages, mDimensions, CV_32FC1);
  cv::Mat entry, compressed_entry;
  for (int i = 0; i < mNumImages; i++)
  {
    cv::Mat data_row;
    LoadImage(mImages[i], data_row);
    compressed_entry = compressed_database.row(i);
    pca.Project(data_row, compressed_entry);
    COUNT_DOWN(i, compress, mNumImages);
  }
  NoticeLine("[done]");

  // Construct mosaic
  INIT_COUNTER(mosaic);
  Notice("Construct mosaic...");

  // prepare destination image
  cv::Mat dst_img(mDstHeight, mDstWidth, CV_8UC3);
  cv::Mat dst_img_gray(mDstHeight, mDstWidth, CV_8UC1, cv::Scalar(0));

  dx = mHexRadius * unit_dx;
  dy = mHexRadius * unit_dy;
  cv::Mat src_entry, tmp_entry, dst_patch, dst_patch_gray, src_patch;
  vInt ids;
  std::vector<cv::Point2i> locations;

  for (int i = 0, n = mCoords.size(); i < n; i++)
  {
    const cv::Point2i &loc = mCoords[mIndices[i]];
    src_entry = compressed_src_img.row(mIndices[i]);
    // Match with database
    std::priority_queue<Match> KNN; // All nearest neighbours

    for (int k = 0; k < mNumImages; k++)
    {
      tmp_entry = compressed_database.row(k);
      float dist = GetDistance(src_entry, tmp_entry);
      KNN.push(Match(k, dist));
    }

    int best_id = KNN.top().id;

    while (!KNN.empty())
    {
      // Stop if the image is new
      if (find(ids.begin(), ids.end(), best_id) == ids.end())
        break;

      // Stop if the image has no duplicates in a certain radius
      bool is_used = false;

      for (int k = 0, n = locations.size(); k < n; k++)
      {
        cInt x = locations[k].x - loc.x;
        cInt y = locations[k].y - loc.y;
        cInt r = int(ceil(sqrt(x * x + y * y)));

        if (r <= mMinRadius && ids[k] == best_id)
        {
          is_used = true;
          break;
        }
      }

      if (!is_used)
        break;

      KNN.pop();
      best_id = KNN.top().id;
    }

    ids.push_back(best_id);
    locations.push_back(loc);

    // Copy hexagon to destination
    cInt src_y = (loc.y * dy);
    cInt src_x = (loc.x * dx + ((loc.y % 2) * (dx / 2.0f)));
    cv::Rect roi(src_x, src_y, mHexWidth, mHexHeight);
    dst_patch = dst_img(roi);
    dst_patch_gray = dst_img_gray(roi);
    src_patch = cv::imread(mImages[best_id], 1);
    cv::getRectSubPix(src_patch, cv::Size(mHexWidth, mHexHeight),
                      cv::Point2f(src_patch.cols / 2.0f, src_patch.rows / 2.0f), entry);
    ColorBalance(entry, pca_input.row(mIndices[i]));
    entry.copyTo(dst_patch, mHexMask);
    mHexMask.copyTo(dst_patch_gray, mHexMask);
#ifndef NDEBUG
    std::string img_name = mImages[best_id].substr(mImages[best_id].find_last_of('/') + 1);
    cv::putText(dst_img, img_name,
                cv::Point(src_x + dx / 3.0f - mHexWidth/2, src_y + dy / 1.5f),
                CV_FONT_HERSHEY_PLAIN, 0.8,
                cv::Scalar(255, 0, 255),
                2);
#endif // NDEBUG
    COUNT_DOWN(i, mosaic, n);
  }

  // Stich edges with neighbouring pixel on x-axis
  cv::threshold(dst_img_gray, dst_img_gray, 0.0, 255.0, CV_THRESH_BINARY_INV);

#ifndef NDEBUG
  cv::imwrite("binary.png", dst_img_gray);
#endif // NDEBUG

  for (int y = 0; y < dst_img_gray.rows; y++)
  {
    for (int x = 1; x < dst_img_gray.cols; x++)
    {
      if (dst_img_gray.at<Uint8>(y, x) > 0)
      {
        while (x < dst_img_gray.cols && dst_img_gray.at<Uint8>(y, x) > 0)
        {
          dst_img.at<cv::Vec3b>(y, x) = dst_img.at<cv::Vec3b>(y, x - 1);
          x++;
        }
      }
    }
  }


  // Write image to disk
  int p = mDatabaseDir.substr(0, mDatabaseDir.size() - 1).find_last_of('/') + 1;
  std::string database = mDatabaseDir.substr(p);
  p = mSourceImage.find_last_of('/') + 1;
  std::string source = mSourceImage.substr(p, mSourceImage.size() - p - 4);
  std::transform(source.begin(), source.end(), source.begin(), ::tolower);
  std::stringstream s;
  s << "source:" << source
    << "-mosaic:" << mWidth << "x" << mHeight
    << "-pca:" << mDimensions
    << "-hexdims:"  << mHexWidth << "x" << mHexHeight
    << "-minradius:" << mMinRadius
    << "-db:" << database.substr(0, database.size() - 1)
    << "-cbr:" << mCBRatio
    << ".tiff";

  cv::imwrite(s.str(), dst_img);
  NoticeLine("[done]");
  NoticeLine("Resulting image: " << s.str());
}

void HexaMosaic::LoadImage(rcString inImageName, cv::Mat &out)
{
  cv::Mat entry, img;
  img = cv::imread(inImageName, 1);
  cv::getRectSubPix(img, cv::Size(mHexWidth, mHexHeight),
                    cv::Point2f(img.cols / 2.0f, img.rows / 2.0f), entry);
  Im2HexRow(entry, out);
}

void HexaMosaic::Crawl(const boost::filesystem::path &inPath)
{
  static boost::match_results<std::string::const_iterator> what;
  static boost::regex img_ext(".*(bmp|BMP|jpg|JPG|jpeg|JPEG|png|PNG|tiff|TIFF)");

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
      ErrorLine(i->path() << " " << ex.what());
    }
  }
}

void HexaMosaic::HexRow2Im(const cv::Mat &in, cv::Mat &out)
{
  out.create(mHexHeight, mHexWidth, CV_8UC3);
  out.setTo(cv::Scalar(0));

  for (int i = 0, n = mHexCoords.size(); i < n; i++)
  {
    cv::Point2i &p = mHexCoords[i];
    out.at<cv::Vec3b>(p.y, p.x) = in.at<cv::Vec3b>(0, i);
  }
}

void HexaMosaic::ColorBalance(cv::Mat &ioSrc, const cv::Mat &inDst)
{
  cv::Mat dst_lab;
  HexRow2Im(inDst, dst_lab);
  cvtColor(dst_lab, dst_lab, CV_RGB2Lab);
  cv::Scalar dst_lab_mean = cv::mean(dst_lab, mHexMask);

  cv::Mat src_lab;
  cvtColor(ioSrc, src_lab, CV_RGB2Lab);
  cv::Scalar src_lab_mean = cv::mean(src_lab, mHexMask);

  // Calculate deltas
  cv::Scalar deltas;

  for (int i = 0; i < 3; i++)
    deltas[i] = mCBRatio * (dst_lab_mean[i] - src_lab_mean[i]);

  // Translate X,Y by deltas
  cv::add(src_lab, deltas, src_lab);

  // X,Y to rgb
  cvtColor(src_lab, ioSrc, CV_Lab2RGB);
}


void HexaMosaic::Process(rcString inImgName)
{
  mImages.push_back(inImgName);
  mNumImages++;
}


bool HexaMosaic::InHexagon(cFloat inX, cFloat inY, cFloat inRadius)
{
  // NOTE: inRadius is defined from the hexagon's center to a corner
  return   fabs(inX) < (inY + inRadius) * HEXAGON_WIDTH &&
           -fabs(inX) > (inY - inRadius) * HEXAGON_WIDTH;
}

float HexaMosaic::GetDistance(const cv::Mat &inSrcRow, const cv::Mat &inDataRow)
{
  cv::Mat tmp;
  cv::subtract(inSrcRow, inDataRow, tmp);
  cv::pow(tmp, 2.0, tmp);
  return sqrtf(cv::sum(tmp)[0]);
}
*/
