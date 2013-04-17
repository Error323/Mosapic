#include "MosaCrawler.hpp"

#include "Constants.hpp"
#include "../utils/Debugger.hpp"
#include "../utils/Verbose.hpp"

#ifdef ENABLE_CUDA
#include "../utils/cuda/init.h"
#include "../utils/cuda/process.h"
#endif

#include <math.h>

#define CLAMP(v, a, b) std::min(std::max(v, a), b)
#define ROUND_CLAMP(v, a, b) roundf(CLAMP(v, a, b))
#define PIXEL(p) ROUND_CLAMP(p, 0.0f, 255.0f)

float sinc(float x)
{
  x *= M_PI;
  return sinf(x) / x;
}

float lanczos(const float x, const float a)
{
  if (x == 0.0f)
    return 1.0f;

  return sinc(x) * sinc(x/a);
}

MosaCrawler::MosaCrawler():
  mImgCount(0),
  mExistCount(0),
  mFailedCount(0),
  mClashCount(0),
  mCudaIsInitialized(false)
{
#ifdef ENABLE_CUDA
  mCudaIsInitialized = gpu::initCuda();
#endif
}

void MosaCrawler::Crawl(const QDir &input, const QDir &output, const int size, const bool fast, const float gamma)
{
  mDstDir = output;

  mTileSize = size;
  mImgCount = 0;
  mExistCount = 0;
  mFailedCount = 0;
  mClashCount = 0;
  mGamma = gamma;
  mFastResizing = fast;

  if (fast)
    DebugLine("Using linear interpolation as resize algorithm");
  else
  {
    DebugLine("Using lanczos interpolation as resize algorithm");
    DebugLine("Lanczos side lobes:     " << LANCZOS_WIDTH);
  }
  DebugLine("Gamma correction value: " << gamma);

  Crawl(input);
  NoticeLine("");
  NoticeLine("Failed    " << mFailedCount << " images");
  NoticeLine("Existing  " << mExistCount << " images");
  NoticeLine("Clashed   " << mClashCount << " images");
  NoticeLine("Processed " << mImgCount << " images");
}

void MosaCrawler::Crawl(const QDir &dir)
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
      Process(info);
  }
}

void MosaCrawler::Crop(QImage &image)
{
  if (image.width() == image.height())
    return;

  int min = std::min(image.width(), image.height());
  int x = image.width() / 2 - min / 2;
  int y = image.height() / 2 - min / 2;
  image = image.copy(x, y, min, min);
}

bool MosaCrawler::IsEqual(const QImage &a, const QImage &b)
{
  if (a.isNull() || b.isNull())
    return false;

  if (a.width() != b.width() || a.height() != b.height())
    return false;

  for (int i = 0; i < a.numBytes(); i++)
    if (a.bits()[i] != b.bits()[i])
      return false;

  return true;
}

void MosaCrawler::GammaCorrect(QImage &image, const float gamma)
{
  for (int i = 0; i < image.numBytes(); i++)
    image.bits()[i] = roundf(powf(image.bits()[i] / 255.0f, gamma) * 255.0f);
}

void MosaCrawler::Resize(const QImage &image, QImage &resized)
{
  ASSERT(image.width() == image.height());
  if (mTileSize == image.width())
  {
    resized = image;
    return;
  }

  float factor = mTileSize / float(image.width());
  float scale = float(image.width()) / mTileSize;
  float support = scale * LANCZOS_WIDTH;
  float phase, density;

  float r,g,b;
  std::vector<float> kernel_x(int(2.0f*support+3.0f));
  std::vector<float> kernel_y(int(2.0f*support+3.0f));
  for (int ty = 0; ty < mTileSize; ty++)
  {
    float center_y = (ty + 0.5f) / factor;
    int start_y = std::max(int(center_y-support+0.5f), 0);
    int stop_y = std::min(int(center_y+support+0.5f), image.height());
    int n_y = stop_y-start_y;
    density = 0.0f;
    for (int i = 0; i < n_y; i++)
    {
      phase = float(start_y + i) - center_y + 0.5f;
      kernel_y[i] = lanczos(factor*phase, LANCZOS_WIDTH);
      density += kernel_y[i];
    }
    for (int i = 0; i < n_y; i++)
      kernel_y[i] /= density;

    for (int tx = 0; tx < mTileSize; tx++)
    {
      float center_x = (tx + 0.5f) / factor;
      int start_x = std::max(int(center_x-support+0.5f), 0);
      int stop_x = std::min(int(center_x+support+0.5f), image.width());
      int n_x = stop_x-start_x;
      density = 0.0f;
      for (int i = 0; i < n_x; i++)
      {
        phase = float(start_x + i) - center_x + 0.5f;
        kernel_x[i] = lanczos(factor*phase, LANCZOS_WIDTH);
        density += kernel_x[i];
      }
      for (int i = 0; i < n_x; i++)
        kernel_x[i] /= density;

      r = g = b = 0.0f;
      QRgb p;
      float k_xy;
      for (int i = 0; i < n_y; i++)
      {
        for (int j = 0; j < n_x; j++)
        {
          p = image.pixel(start_x+j, start_y+i);
          k_xy = kernel_x[j] * kernel_y[i];
          r += k_xy * qRed(p);
          g += k_xy * qGreen(p);
          b += k_xy * qBlue(p);
        }
      }
      resized.setPixel(tx, ty, qRgb(PIXEL(r), PIXEL(g), PIXEL(b)));
    }
  }
}

void MosaCrawler::Process(const QFileInfo &info)
{
  QImage image(info.absoluteFilePath());


  Notice("Processing `" << qPrintable(info.fileName()) << "'...");
  if (image.isNull() || image.width() < mTileSize || image.height() < mTileSize)
  {
    ErrorLine("[failed]");
    mFailedCount++;
    return;
  }

  Crop(image);
  if (image.format() != QImage::Format_RGB32)
    image = image.convertToFormat(QImage::Format_RGB32);

  QImage resized(mTileSize, mTileSize, image.format());
#ifdef ENABLE_CUDA
  bool success = false;
  if (mCudaIsInitialized && !mFastResizing)
  {
    success =
      gpu::process(reinterpret_cast<uchar4*>(image.bits()), image.height(),
                   reinterpret_cast<uchar4*>(resized.bits()), resized.height(),
                   mGamma);
  }

  if (!success)
#endif
  {
    Debug("[cpu]");
    if (mGamma != 1.0f)
      GammaCorrect(image, mGamma);

    if (mFastResizing)
      resized = image.scaled(mTileSize, mTileSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else
      Resize(image, resized);

    if (mGamma != 1.0f)
      GammaCorrect(resized, 1.0f/mGamma);
  }

  QFileInfo file(mDstDir.absolutePath() + "/" + info.baseName() + ".tiff");

  int clash_count = 0;
  while (file.exists())
  {
    QImage existing(file.absoluteFilePath());
    if (IsEqual (existing, resized))
    {
      WarningLine("[exists]");
      mExistCount++;
      return;
    }
    else
    {
      Warning("[clash]");
      file.setFile(mDstDir.absolutePath() + "/" + info.baseName() + "-" +
                   QString::number(clash_count) + ".tiff");
      clash_count++;
      mClashCount++;
    }
  }

  resized.save(file.absoluteFilePath());
  mImgCount++;
  NoticeLine("[done]");
}
