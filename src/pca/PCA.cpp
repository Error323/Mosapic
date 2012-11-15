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

#ifndef ENABLE_CUDA_DEVICE
  mUseDevice = false;
#else
  mUseDevice = true;
#endif

  InitDevice();
}

void PCA::AddRow(const cv::Mat &row)
{
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

  // Compute svd
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

  cv::Mat d, mean;
  data.convertTo(d, CV_32FC1);
  cv::repeat(mMean, data.rows, 1, mean);
  d -= mean;
  projected = d * mEigen.t();
}

void PCA::BackProject(const cv::Mat &projected, cv::Mat &reduced)
{
  PROFILE_FUNCTION();
  ASSERT(mDimensions > 0 && mDimensions < mRows);
  ASSERT(projected.cols == mDimensions);
  cv::Mat mean;
  cv::repeat(mMean, projected.rows, 1, mean);

  reduced = projected * mEigen;
  reduced += mean;
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

  for (int i = 0; i < mDimensions; i++)
  {
    float alpha = 1.0f/sqrtf((mCols-1)*mS.at<float>(i));
    cv::Mat x = mData.t() * mE.col(i);
    x *= alpha;
    mEigen.row(i) = x.t();
  }
}

void PCA::CovarianceGold(const cv::Mat &data, cv::Mat &cov)
{
  PROFILE_FUNCTION();

  cov = data * data.t();
  cov *= 1.0f/(mCols-1);
}
