#include "HexaMosaic.hpp"
#include "Debugger.hpp"

#include <cmath>
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
      std::cout << c << " " << std::flush;        \
      c--;                                        \
		}                                             \
  } while(0)                                      \
 
HexaMosaic::HexaMosaic(
  rcString inSourceImage,
  rcString inDatabase,
  cInt inWidth,
  cInt inHeight,
  cBool inGrayscale,
  cInt inDimensions,
  cInt inMinRadius
):
  mSourceImage(inSourceImage),
  mWidth(inWidth),
  mHeight(inHeight),
  mUseGrayscale(inGrayscale),
  mDimensions(inDimensions),
  mMinRadius(inMinRadius),
  mNumImages(0)
{
  mDatabaseDir = inDatabase.at(inDatabase.size() - 1) == '/' ? inDatabase : inDatabase + '/';
  Crawl(mDatabaseDir);
  cv::Mat first = cv::imread(mImages.front());
  ASSERT(first.data != NULL && first.rows == first.cols && first.rows > 0);

  mHexHeight = first.rows;
  mHexRadius = mHexHeight / 2.0f;
  mHexWidth  = roundf(mHexRadius * HEXAGON_WIDTH);

  // Cache coordinates, so we can e.g. randomize
  for (int y = 0; y < mHeight; y++)
  {
    for (int x = 0; x < mWidth; x++)
    {
      if (y % 2 == 1 && x == mWidth - 1)
      {
        continue;
      }

      mIndices.push_back(mCoords.size());
      mCoords.push_back(cv::Point2i(x, y));
    }
  }

  // Precache hexagon coordinates
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
      }
    }
  }

#ifdef DEBUG
  cv::imwrite("hexmask.jpg", mHexMask);
#endif // DEBUG
}

void HexaMosaic::Create()
{
  // unit dimensions of hexagon facing upwards
  cFloat unit_dx = HEXAGON_WIDTH;
  cFloat unit_dy = HEXAGON_HEIGHT * (3.0f / 4.0f);

  // Load source image
  cv::Mat src_img = cv::imread(mSourceImage, (mUseGrayscale ? 0 : 1));
  cv::Mat dst_img(mHeight * mHexHeight * .75 + mHexHeight * .25 + mHeight * .25,
                  mWidth * mHexWidth + mWidth * .25,
                  (mUseGrayscale ? CV_8UC1 : CV_8UC3));
  cv::Mat dst_img_gray(mHeight * mHexHeight * .75 + mHexHeight * .25 + mHeight * .25,
      mWidth * mHexWidth + mWidth * .25,
      CV_8UC1);

  // Compute pca input data from source image
  cv::Mat pca_input(mCoords.size(), mHexHeight * mHexWidth*(mUseGrayscale ? 1 : 3), CV_8UC1);
  float dx = src_img.cols / float(mWidth);
  float dy = src_img.rows / float(mHeight);

  for (int i = 0, n = mCoords.size(); i < n; i++)
  {
    cInt x = mCoords[i].x;
    cInt y = mCoords[i].y;
    cInt src_y = roundf(y * dy);
    cInt src_x = roundf(x * dx + ((y % 2) * (dx / 2.0f)));
    cv::Rect roi(src_x, src_y, roundf(dx), roundf(dy));
    cv::Mat data_row, patch_resized, patch = src_img(roi);
    cv::resize(patch, patch_resized, cv::Size(mHexWidth, mHexHeight));
    patch_resized.copyTo(data_row, mHexMask);
    cv::Mat pca_input_row = pca_input.row(i);
    data_row = data_row.reshape(1, 1);
    data_row.copyTo(pca_input_row);
  }

  std::cout << "Performing pca..." << std::flush;
  cv::PCA pca(pca_input, cv::Mat(), CV_PCA_DATA_AS_ROW, mDimensions);
#ifdef DEBUG

  // Construct eigenvector images for debugging
  for (int i = 0; i < mDimensions; i++)
  {
    cv::Mat eigenvec;
    cv::normalize(pca.eigenvectors.row(i), eigenvec, 255, 0, cv::NORM_MINMAX);
    eigenvec = eigenvec.reshape((mUseGrayscale ? 1 : 3), mHexHeight);
    std::stringstream s;
    s << i;
    std::string entry = "eigenvector-" + s.str() + ".jpg";
    cv::imwrite(entry, eigenvec);
  }

#endif // DEBUG
  std::cout << "[done]" << std::endl;

  // Compress original image data
  std::cout << "Compress source image..." << std::flush;
  cv::Mat compressed_src_img(pca_input.rows, mDimensions, CV_32FC1);
  cv::Mat entry, compressed_entry;

  for (int i = 0; i < pca_input.rows; i++)
  {
    entry = pca_input.row(i);
    compressed_entry = compressed_src_img.row(i);
    pca.project(entry, compressed_entry);
  }

  std::cout << "[done]" << std::endl;

  // Compress database image data
  std::cout << "Compress database..." << std::flush;
  INIT_COUNTER(compress);
  cv::Mat compressed_database(mNumImages, mDimensions, CV_32FC1);
  cv::Mat img;
  for (int i = 0; i < mNumImages; i++)
  {
    img = cv::imread(mImages[i], (mUseGrayscale ? 0 : 1));

    cv::getRectSubPix(img, cv::Size(mHexWidth, mHexHeight),
                      cv::Point2f(img.cols/2.0f, img.rows/2.0f), entry);
    entry = entry.reshape(1,1);
    compressed_entry = compressed_database.row(i);
    pca.project(entry, compressed_entry);
    COUNT_DOWN(i, compress, mNumImages);
  }

  std::cout << "[done]" << std::endl;

  // Construct mosaic
  INIT_COUNTER(mosaic);
  std::cout << "Construct mosaic..." << std::flush;
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
      {
        break;
      }

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
      {
        break;
      }

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
    src_patch = cv::imread(mImages[best_id], (mUseGrayscale ? 0 : 1));
    cv::getRectSubPix(src_patch, cv::Size(mHexWidth, mHexHeight),
                      cv::Point2f(src_patch.cols/2.0f, src_patch.rows/2.0f), entry);
    entry.copyTo(dst_patch, mHexMask);
    mHexMask.copyTo(dst_patch_gray, mHexMask);
#ifdef DEBUG
    std::string img_name = mImages[best_id].substr(mImages[best_id].find_last_of('/')+1);
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
  std::vector<cv::Mat> split;
  cv::split(dst_img, split);
  std::vector<int> idx;
  idx.push_back(1);
  idx.push_back(-1);
  idx.push_back(2);
  idx.push_back(-2);

  for (int y = dy / 2.0f; y < dst_binary.rows - dy / 2.0f; y++)
  {
    for (int x = dx / 2.0f; x < dst_binary.cols - dx / 2.0f; x++)
    {
      if (dst_binary.at<Uint8>(y, x) > 0)
      {
        for (int i = 0, n = idx.size(); i < n; i++)
        {
          if (dst_binary.at<Uint8>(y, x + idx[i]) == 0)
          {
            split[0].at<Uint8>(y, x) = split[0].at<Uint8>(y, x + idx[i]);

            if (!mUseGrayscale)
            {
              split[1].at<Uint8>(y, x) = split[1].at<Uint8>(y, x + idx[i]);
              split[2].at<Uint8>(y, x) = split[2].at<Uint8>(y, x + idx[i]);
            }

            break;
          }
        }
      }
    }
  }

  cv::merge(split, dst_img);

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
    << "-grayscale:" << (mUseGrayscale ? "on" : "off")
    << IMAGE_EXT;

  cv::imwrite(s.str(), dst_img);
  std::cout << "[done]" << std::endl;
  std::cout << "Resulting image: " << s.str() << std::endl;
}

void HexaMosaic::Crawl(const boost::filesystem::path &inPath)
{
  static boost::match_results<std::string::const_iterator> what;
  static boost::regex img_ext(".*(bmp|BMP|jpg|JPG|jpeg|JPEG|png|PNG)");

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
            boost::regex_match(i->path().leaf(), what, img_ext, boost::match_default))
        {
          Process(i->path().string());
        }
      }
    }
    catch (const std::exception &ex)
    {
      std::cerr << i->path() << " " << ex.what() << std::endl;
    }
  }
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
