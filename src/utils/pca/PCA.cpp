#include "PCA.hpp"

#include "../Debugger.hpp"
#include "../Verbose.hpp"

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

void PCA::Solve(const int dimensions)
{
  mDimensions = dimensions;
  ASSERT(mDimensions > 0 && mDimensions < mRows);
  ASSERT(mCurRow == mRows);

  mEigen.resize(mDimensions, mCols);
  mE.resize(mRows, mRows);
  mS.resize(mRows);

  // Center data around mean
  mData.rowwise() -= mMean;

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

void PCA::BackProject(const MatrixXf &projected, MatrixXf &reduced)
{
  ASSERT(mDimensions > 0 && mDimensions < mRows);
  ASSERT(projected.cols() == mDimensions);

  reduced = (projected * mEigen).rowwise() + mMean;
}

void PCA::GetEigenVector(const int i, RowVectorXf &eigenvector)
{
  ASSERT(i >= 0 && i < mDimensions);
  eigenvector = mEigen.row(i);
}
