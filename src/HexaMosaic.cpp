#include "HexaMosaic.hpp"

#include "pca/PCA.hpp"
#include "utils/Debugger.hpp"
#include "utils/Timer.hpp"
#include "utils/Verbose.hpp"
#include "utils/MatIO.hpp"

#include <cmath>
#include <fstream>
#include <queue>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <limits>
#include <opencv/highgui.h>

extern bool SaveMat( const std::string &filename, const cv::Mat &M);
extern bool ReadMat( const std::string &filename, cv::Mat &M);

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

#ifdef DEBUG
  cv::imwrite("hexmask.jpg", mHexMask);
#endif // DEBUG
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
#ifdef DEBUG

  // Construct eigenvector images for debugging
  for (int i = 0; i < mDimensions; i++)
  {
    cv::Mat eigenvec;
    cv::Mat correct;
    cv::normalize(pca.GetEigenVector(i), eigenvec, 255, 0, cv::NORM_MINMAX);
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
  cv::Mat dst_img_gray(mDstHeight, mDstWidth, CV_8UC1);

  dx = mHexRadius * unit_dx;
  dy = mHexRadius * unit_dy;
  random_shuffle(mIndices.begin(), mIndices.end());
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
#ifdef DEBUG
    std::string img_name = mImages[best_id].substr(mImages[best_id].find_last_of('/') + 1);
    cv::putText(dst_img, img_name,
                cv::Point(src_x + dx / 3.0f, src_y + dy / 1.5f),
                CV_FONT_HERSHEY_PLAIN, 2.0,
                cv::Scalar(255, 0, 255),
                2);
#endif // DEBUG
    COUNT_DOWN(i, mosaic, n);
  }

  // Stich edges with neighbouring pixel on x-axis
  cv::Mat dst_binary;
  cv::threshold(dst_img_gray, dst_binary, 0.0, 255.0, CV_THRESH_BINARY_INV);

  for (int y = 0; y < dst_binary.rows; y++)
  {
    for (int x = 1; x < dst_binary.cols; x++)
    {
      if (dst_binary.at<Uint8>(y, x) > 0)
      {
        while (x < dst_binary.cols && dst_binary.at<Uint8>(y, x) > 0)
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
  String img_name = inImageName + ".bin";

  if (ReadMat(img_name, out))
    return;

  cv::Mat entry, img;
  img = cv::imread(inImageName, 1);
  cv::getRectSubPix(img, cv::Size(mHexWidth, mHexHeight),
                    cv::Point2f(img.cols / 2.0f, img.rows / 2.0f), entry);
  Im2HexRow(entry, out);
  SaveMat(img_name, out);
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
