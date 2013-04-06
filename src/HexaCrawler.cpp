#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"

#include "utils/Debugger.hpp"
#include "utils/Verbose.hpp"

#define CLAMP(v, a, b) std::min(std::max(v, a), b)
#define ROUND_CLAMP(v, a, b) roundf(CLAMP(v, a, b))
#define LANCZOS_SPP (4000)
#define LANCZOS_WIDTH (3)
#define KERNEL_SIZE (LANCZOS_WIDTH * 2)
#define LANCZOS_SAMPLES (LANCZOS_SPP * (LANCZOS_WIDTH + 1))

float sinc(const float x)
{
  float y = x * M_PI;
  if (fabs(x) < 1.0f / LANCZOS_SPP)
    return 1.0f;
  else
    return sinf(y) / y;
}

HexaCrawler::HexaCrawler():
  mImgCount(0),
  mExistCount(0),
  mFailedCount(0),
  mClashCount(0)
{
  float a = float(LANCZOS_WIDTH);
  float x = 0.0f;
  float dx = a / float(LANCZOS_SAMPLES - 1);

  mXKernel.resize(KERNEL_SIZE);
  mYKernel.resize(KERNEL_SIZE);
  mFullKernel.resize(KERNEL_SIZE, KERNEL_SIZE);
  mWindowRed.resize(KERNEL_SIZE, KERNEL_SIZE);
  mWindowGreen.resize(KERNEL_SIZE, KERNEL_SIZE);
  mWindowBlue.resize(KERNEL_SIZE, KERNEL_SIZE);
  mKernel.resize(LANCZOS_SAMPLES);
  for (int i = 0; i < LANCZOS_SAMPLES; i++)
  {
    mKernel[i] = (fabs(x) < a) ? (sinc(x) * sinc(x / a)) : 0.0f;
    x += dx;
  }
}

void HexaCrawler::Crawl(const QDir &input, const QDir &output, const int tileSize)
{
  mDstDir = output;

  mTileSize = tileSize;
  mImgCount = 0;
  mExistCount = 0;
  mFailedCount = 0;
  mClashCount = 0;

  Crawl(input);
  NoticeLine("");
  NoticeLine("Failed    " << mFailedCount << " images");
  NoticeLine("Existing  " << mExistCount << " images");
  NoticeLine("Clashed   " << mClashCount << " images");
  NoticeLine("Processed " << mImgCount << " images");
}

void HexaCrawler::Crawl(const QDir &dir)
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

void HexaCrawler::Crop(QImage &image)
{
  if (image.width() == image.height())
    return;

  int min = std::min(image.width(), image.height());
  int x = image.width() / 2 - min / 2;
  int y = image.height() / 2 - min / 2;
  image = image.copy(x, y, min, min);
}

QRgb HexaCrawler::InterpolateLanczos(const QImage &image, const int sx, const int sy, const float xfrac, const float yfrac)
{
  float kx_sum = 0.0f;
  float ky_sum = 0.0f;
  int x_shift = int(xfrac * LANCZOS_SPP + 0.5f);
  int y_shift = int(yfrac * LANCZOS_SPP + 0.5f);
  float r, g, b;

  const int from = KERNEL_SIZE/2;
  const int to = -KERNEL_SIZE/2+1;
  for (int i = from; i >= to; i--)
  {
    int pos = i * LANCZOS_SPP;
    kx_sum += mXKernel(-to + i) = mKernel[abs(x_shift - pos)];
    ky_sum += mYKernel(-to + i) = mKernel[abs(y_shift - pos)];
  }

  mXKernel.array() /= kx_sum;
  mYKernel.array() /= ky_sum;

  for (int i = 0; i < KERNEL_SIZE; i++)
  {
    int y = CLAMP(i + sy + to, 0, image.height()-1);
    for (int j = 0; j < KERNEL_SIZE; j++)
    {
      int x = CLAMP(j + sx + to, 0, image.width()-1);
      mWindowRed(i,j) = qRed(image.pixel(x, y));
      mWindowGreen(i,j) = qGreen(image.pixel(x, y));
      mWindowBlue(i,j) = qBlue(image.pixel(x, y));
    }
  }

  mFullKernel = mYKernel * mXKernel.transpose();
  r = (mFullKernel.array() * mWindowRed.array()).sum();
  g = (mFullKernel.array() * mWindowGreen.array()).sum();
  b = (mFullKernel.array() * mWindowBlue.array()).sum();

  r = ROUND_CLAMP(r, 0.0f, 255.0f);
  g = ROUND_CLAMP(g, 0.0f, 255.0f);
  b = ROUND_CLAMP(b, 0.0f, 255.0f);
  return qRgb(r, g, b);
}

void HexaCrawler::Resize(QImage &image)
{
  ASSERT(image.width() == image.height());
  if (mTileSize == image.width())
    return;

  QImage target(mTileSize, mTileSize, QImage::Format_RGB32);
  image = image.scaled(mTileSize*1.3, mTileSize*1.3,
    Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  float ratio = image.width() / float(mTileSize);

  QRgb p;
  for (int ty = 0; ty < mTileSize; ty++)
  {
    float yfrac = ty * ratio;
    int sy = int(yfrac);
    yfrac -= sy;

    for (int tx = 0; tx < mTileSize; tx++)
    {
      float xfrac = tx * ratio;
      int sx = int(xfrac);
      xfrac -= sx;

      p = InterpolateLanczos(image, sx, sy, xfrac, yfrac);
      target.setPixel(tx, ty, p);
    }
  }

  image = target;
}

bool HexaCrawler::IsEqual(const QImage &a, const QImage &b)
{
  if (a.width() != b.width() || a.height() != b.height())
    return false;

  for (int i = 0; i < a.numBytes(); i++)
    if (a.bits()[i] != b.bits()[i])
      return false;

  return true;
}

void HexaCrawler::GammaCorrect(QImage &image, const float gamma)
{
  for (int i = 0; i < image.numBytes(); i++)
    image.bits()[i] = roundf(std::pow(image.bits()[i] / 255.0f, gamma) * 255.0f);
}

void HexaCrawler::Process(const QFileInfo &info)
{
  QImage image(info.absoluteFilePath());

  Notice("Processing `" << qPrintable(info.fileName()) << "'...");
  if (image.isNull() || image.width() < mTileSize || image.height() < mTileSize)
  {
    ErrorLine("[failed]");
    mFailedCount++;
    return;
  }

  GammaCorrect(image, 2.2f);
  Crop(image);
  Resize(image);
  GammaCorrect(image, 1.0f/2.2f);

  QFileInfo file(mDstDir.absolutePath() + "/" + info.baseName() + ".tiff");

  int clash_count = 0;
  while (file.exists())
  {
    QImage existing(file.absoluteFilePath());
    if (IsEqual (existing, image))
    {
      ErrorLine("[exists]");
      mExistCount++;
      return;
    }
    else
    {
      Warning("[clash]");
      file.setFile(mDstDir.absolutePath() + "/" + info.baseName() + "-" + QString::number(clash_count) + ".tiff");
      clash_count++;
      mClashCount++;
    }
  }

  image.save(file.absoluteFilePath());
  mImgCount++;
  NoticeLine("[done]");
}
