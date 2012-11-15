#ifndef PCA_H
#define PCA_H

#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>

class PCA
{
public:
  PCA(const int rows, const int cols);

  void AddRow(const cv::Mat &row);
  void Solve(const int dimensions);

  void Project(const cv::Mat &data, cv::Mat &projected);
  void BackProject(const cv::Mat &projected, cv::Mat &reduced);

  cv::Mat mEigen;

private:
  void CovarianceGold(const cv::Mat &data, cv::Mat &cov);
  void EigenVectorsGold();

  bool CovarianceDevice(const cv::Mat &data, cv::Mat &cov);
  bool EigenVectorsDevice();
  void InitDevice();

  int mRows;
  int mCols;
  int mCurRow;
  int mDimensions;
  bool mUseDevice;

  cv::Mat mData;
  cv::Mat mMean;
  cv::Mat mE;
  cv::Mat mS;
};

#endif // PCA_H
