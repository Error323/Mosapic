#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"

#include "utils/Debugger.hpp"
#include "utils/Verbose.hpp"

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

void HexaCrawler::Resize(QImage &image)
{
  int min = std::min<int>(image.width(), image.height());
  int width  = min - (min % mTileSize);
  int height = min - (min % mTileSize);
  int x = image.width() / 2 - width / 2;
  int y = image.height() / 2 - height / 2;

  image = image.copy(x, y, width, height).scaled(mTileSize, mTileSize);
}

bool HexaCrawler::Exists(const QImage &a, const QImage &b)
{
  if (a.width() != b.width() || a.height() != b.height())
    return false;

  for (int i = 0; i < a.numBytes(); i++)
    if (a.bits()[i] != b.bits()[i])
      return false;

  return true;
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

  Resize(image);

  QFileInfo file(mDstDir.absolutePath() + "/" + info.baseName() + ".tiff");

  int clash_count = 0;
  while (file.exists())
  {
    QImage existing(file.absoluteFilePath());
    if (Exists(existing, image))
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
