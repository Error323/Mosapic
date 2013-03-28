#include "PCA.hpp"

#include "../utils/Debugger.hpp"
#include "../utils/Verbose.hpp"

PCA::PCA(const int rows, const int cols):
  mRows(rows),
  mCols(cols),
  mCurRow(0),
  mDimensions(0)
{
  ASSERT(mRows <= mCols);

  mData.resize(mRows, mCols);
  mMean.resize(mCols);
  mMean.setZero();
}

void PCA::AddRow(const RowVectorXf &row)
{
  ASSERT(row.cols() == mCols);

  mData.row(mCurRow) = row;
  mMean += row / mRows;

  mCurRow++;
}

void PCA::AddRow(const cv::Mat &row)
{
  RowVectorXf e_row;
  CvMat2EigRow(row, e_row);
  AddRow(e_row);
}

void PCA::Solve(const int dimensions)
{
  mDimensions = dimensions;
  ASSERT(mDimensions > 0 && mDimensions < mRows);
  ASSERT(mCurRow == mRows);

  mEigen.resize(mDimensions, mCols);
  mE.resize(mRows, mRows);
  mS.resize(mRows);

  // Center data around mean
  for (int i = 0; i < mRows; i++)
    mData.row(i) -= mMean;

  // Compute covariance matrix
  MatrixXf cov = (1.0f / (mCols - 1)) * (mData * mData.transpose());

  // Compute eigenvectors and eigenvalues of the covariance matrix
  SelfAdjointEigenSolver<MatrixXf> solver(cov);
  ASSERT(mS.rows() == solver.eigenvalues().rows());
  ASSERT(mS.cols() == solver.eigenvalues().cols());
  mS = solver.eigenvalues();

  ASSERT(mE.rows() == solver.eigenvectors().rows());
  ASSERT(mE.cols() == solver.eigenvectors().cols());
  mE = solver.eigenvectors();

  // Compute real eigenvectors
  for (int i = 0; i < mDimensions; i++)
  {
    const float alpha = 1.0f / sqrtf((mCols - 1) * mS(mRows-i-1));
    mEigen.row(i) = alpha * (mData.transpose() * mE.col(mRows-i-1));
  }

  // Destroy data
  mData.resize(0, 0);
  mE.resize(0, 0);
  mS.resize(0);
}

void PCA::Project(const MatrixXf &data, MatrixXf &projected)
{
  ASSERT(mDimensions > 0 && mDimensions < mRows);
  ASSERT(data.cols() == mCols);

  projected = (data.rowwise() - mMean) * mEigen.transpose();
}

void PCA::Project(const cv::Mat &data, cv::Mat &projected)
{
  MatrixXf e_data, e_proj;
  CvMat2EigMat(data, e_data);
  Project(e_data, e_proj);
  EigMat2CvMat(e_proj, projected);
}

void PCA::BackProject(const MatrixXf &projected, MatrixXf &reduced)
{
  ASSERT(mDimensions > 0 && mDimensions < mRows);
  ASSERT(projected.cols() == mDimensions);

  reduced = (projected * mEigen).rowwise() + mMean;
}

void PCA::BackProject(const cv::Mat &projected, cv::Mat &reduced)
{
  MatrixXf e_proj, e_reduced;
  CvMat2EigMat(projected, e_proj);
  BackProject(e_proj, e_reduced);
  EigMat2CvMat(e_reduced, reduced);
}

void PCA::GetEigenVector(const int i, RowVectorXf &eigenvector)
{
  ASSERT(i >= 0 && i < mDimensions);
  eigenvector = mEigen.row(i);
}

void PCA::GetEigenVector(const int i, cv::Mat &eigenvector)
{
  RowVectorXf e;
  GetEigenVector(i, e);
  EigRow2CvMat(e, eigenvector);
}

void PCA::EigRow2CvMat(const RowVectorXf &in, cv::Mat &out)
{
  out.create(1, in.cols(), CV_32FC1);
  for (int j = 0; j < in.cols(); j++)
    out.at<float>(0,j) = in(j);
}

void PCA::CvMat2EigRow(const cv::Mat &in, RowVectorXf &out)
{
  cv::Mat tmp;
  in.convertTo(tmp, CV_32FC1);
  out.resize(tmp.cols);
  for (int j = 0; j < out.cols(); j++)
    out(j) = tmp.at<float>(0, j);
}

void PCA::CvMat2EigMat(const cv::Mat &in, MatrixXf &out)
{
  cv::Mat tmp;
  in.convertTo(tmp, CV_32FC1);
  out.resize(tmp.rows, tmp.cols);
  for (int i = 0; i < in.rows; i++)
    for (int j = 0; j < in.cols; j++)
      out(i, j) = tmp.at<float>(i, j);
}

void PCA::EigMat2CvMat(const MatrixXf &in, cv::Mat &out)
{
  out.create(in.rows(), in.cols(), CV_32FC1);
  for (int i = 0; i < in.rows(); i++)
    for (int j = 0; j < in.cols(); j++)
      out.at<float>(i, j) = in(i,j);
}
