#include "MatIO.hpp"

#include <fstream>

bool SaveMat( const std::string &filename, const cv::Mat &M)
{
  if (M.empty())
  {
    return false;
  }

  std::ofstream out(filename.c_str(), std::ios::out | std::ios::binary);

  if (!out)
    return false;

  int cols = M.cols;
  int rows = M.rows;
  int chan = M.channels();
  int eSiz = (M.dataend - M.datastart) / (cols * rows * chan);

  // Write header
  out.write((char *)&cols, sizeof(cols));
  out.write((char *)&rows, sizeof(rows));
  out.write((char *)&chan, sizeof(chan));
  out.write((char *)&eSiz, sizeof(eSiz));

  // Write data.
  if (M.isContinuous())
  {
    out.write((char *)M.data, cols * rows * chan * eSiz);
  }
  else
  {
    return false;
  }

  out.close();
  return true;
}

/****************************************************************************/
// Read matrix from binary file
bool ReadMat( const std::string &filename, cv::Mat &M)
{
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);

  if (!in)
    return false;

  int cols;
  int rows;
  int chan;
  int eSiz;

  // Read header
  in.read((char *)&cols, sizeof(cols));
  in.read((char *)&rows, sizeof(rows));
  in.read((char *)&chan, sizeof(chan));
  in.read((char *)&eSiz, sizeof(eSiz));

  // Determine type of the matrix
  int type = 0;

  switch (eSiz)
  {
  case sizeof(char):
    type = CV_8UC(chan);
    break;
  case sizeof(float):
    type = CV_32FC(chan);
    break;
  case sizeof(double):
    type = CV_64FC(chan);
    break;
  }

  // Alocate Matrix.
  M = cv::Mat(rows, cols, type, cv::Scalar(1));

  // Read data.
  if (M.isContinuous())
  {
    in.read((char *)M.data, cols * rows * chan * eSiz);
  }
  else
  {
    return false;
  }

  in.close();
  return true;
}
