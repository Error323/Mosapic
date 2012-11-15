#include "PCA.hpp"

#include "../utils/Debugger.hpp"
#include "../utils/Timer.hpp"
#include "../utils/Verbose.hpp"

PCA::PCA(const int rows, const int cols):
  mRows(rows),
  mCols(cols),
  mCurRow(0),
  mDimensions(0)
{
  ASSERT(mRows <= mCols);

  mData.create(mRows, mCols, CV_32FC1);
  mMean.create(1, mCols, CV_32FC1);
  mMean.setTo(cv::Scalar(0.0f));

  size_t bytes = mCols * mRows * mData.elemSize();
  float mbytes = bytes / (1024.0f*1024.0f);
  WarningLine("Data(" << mRows << ", " << mCols << ") "<< mbytes << " MBytes");

#ifndef ENABLE_CUDA_DEVICE
  mUseDevice = false;
#else
  mUseDevice = false;
#endif

  InitDevice();
}

void PCA::AddRow(const cv::Mat &row)
{
  PROFILE_FUNCTION();
  ASSERT(row.cols == mCols);
  ASSERT(row.rows == 1);

  float f;
  for (int j = 0; j < mCols; j++)
  {
    f = static_cast<float>(row.at<unsigned char>(0, j));
    mData.at<float>(mCurRow,j) = f;
    mMean.at<float>(0,j) += f/mRows;
  }
  mCurRow++;
}

void PCA::Solve(const int dimensions)
{
  PROFILE_FUNCTION();

  ASSERT(dimensions > 0 && dimensions < mRows);
  mDimensions = dimensions;
  mEigen.create(mDimensions, mCols, CV_32FC1);
  mEigen.setTo(cv::Scalar(0.0f));

  ASSERT(mCurRow == mRows);

  // Center data around mean
  for (int i = 0; i < mRows; i++)
    mData.row(i) -= mMean;

  // Compute covariance matrix
  cv::Mat cov(mRows, mRows, CV_32FC1);
  if (!mUseDevice || !CovarianceDevice(mData, cov))
    CovarianceGold(mData, cov);

  // Compute eigenvectors and eigenvalues of the covariance matrix
  cv::eigen(cov, mS, mE);
  mE = mE.t();

  // Compute eigenvectors
  if (!mUseDevice || !EigenVectorsDevice())
    EigenVectorsGold();
}

void PCA::Project(const cv::Mat &data, cv::Mat &projected)
{
  PROFILE_FUNCTION();
  ASSERT(mDimensions > 0 && mDimensions < mRows);
  ASSERT(data.cols == mCols);

  // Convert data to floats
  cv::Mat data_converted;
  if (data.type() != CV_32FC1)
    data.convertTo(data_converted, CV_32FC1);
  else
    data_converted = data;

  // Subtract mean
  for (int i = 0; i < data.rows; i++)
    data_converted.row(i) -= mMean;

  // Project against eigenvectors
  cv::gemm(data_converted, mEigen, 1.0, cv::Mat(), 0.0, projected, cv::GEMM_2_T);
}

void PCA::BackProject(const cv::Mat &projected, cv::Mat &reduced)
{
  PROFILE_FUNCTION();
  ASSERT(mDimensions > 0 && mDimensions < mRows);
  ASSERT(projected.cols == mDimensions);

  cv::Mat mean;
  cv::repeat(mMean, projected.rows, 1, mean);

  // Reduced = Proj * Eigen + Mean
  cv::gemm(projected, mEigen, 1.0, mean, 1.0, reduced, 0);
}

cv::Mat PCA::GetEigenVector(const int i)
{
  ASSERT(i >= 0 && i < mDimensions);
  return mEigen.row(i);
}

void PCA::InitDevice()
{
  if (!mUseDevice)
    return;

  PROFILE_FUNCTION();

  try
  {
    Notice("Initializing " << cv::gpu::DeviceInfo().name() << "...");
    cv::gpu::DeviceInfo().freeMemory();
    NoticeLine("[done]");
  }
  catch(const cv::Exception& ex)
  {
    ErrorLine("Error: " << ex.what());
    WarningLine("Falling back to cpu");
    mUseDevice = false;
  }
}

bool PCA::CovarianceDevice(const cv::Mat &data, cv::Mat &cov)
{
  PROFILE_FUNCTION();

  try
  {
    cv::gpu::GpuMat a, c;
    a.upload(data);
    c.create(mRows, mRows, CV_32FC1);
    cv::gpu::gemm(a, a, 1.0f/(mCols-1), cv::gpu::GpuMat(), 0.0f, c, cv::GEMM_2_T);
    c.download(cov);
  }
  catch(const cv::Exception& ex)
  {
    ErrorLine("Error: " << ex.what());
    WarningLine("Falling back to cpu");
    return false;
  }

  return true;
}

bool PCA::EigenVectorsDevice()
{
  PROFILE_FUNCTION();

  cv::Mat host_e;
  try
  {
    cv::gpu::GpuMat a, u, e, t;
    a.upload(mData);
    u.upload(mE);
    e.create(mCols, mDimensions, CV_32FC1);

    for (int i = 0; i < mDimensions; i++)
    {
      float alpha = 1.0f/sqrtf((mCols-1)*mS.at<float>(i));
      t = e.col(i);
      cv::gpu::gemm(a, u.col(i), alpha, cv::gpu::GpuMat(), 0.0f, t, cv::GEMM_1_T);
    }
    e.download(host_e);
  }
  catch(const cv::Exception& ex)
  {
    ErrorLine("Error: " << ex.what());
    WarningLine("Falling back to cpu");
    return false;
  }

  mEigen = host_e.t();

  return true;
}

void PCA::EigenVectorsGold()
{
  PROFILE_FUNCTION();

  cv::Mat eigen(mCols, mDimensions, CV_32FC1), tmp;
  for (int i = 0; i < mDimensions; i++)
  {
    double alpha = 1.0/sqrt((mCols-1)*mS.at<float>(i));
    tmp = eigen.col(i);
    cv::gemm(mData, mE.col(i), alpha, cv::Mat(), 0.0, tmp, cv::GEMM_1_T);
  }
  mEigen = eigen.t();
}

void PCA::CovarianceGold(const cv::Mat &data, cv::Mat &cov)
{
  PROFILE_FUNCTION();
  double alpha = 1.0/(mCols-1);
  cv::gemm(data, data, alpha, cv::Mat(), 0.0, cov, cv::GEMM_2_T);
}
