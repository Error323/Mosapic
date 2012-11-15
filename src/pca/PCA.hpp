#ifndef PCA_H
#define PCA_H

#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>

class PCA
{
public:
  /// @brief Const: create datamatrix rows * cols, rows <= cols
  PCA(const int rows, const int cols);

  /// @brief Add a new data row
  void AddRow(const cv::Mat &row);

  /// @brief Compute eigenvectors, only when data matrix is filled
  void Solve(const int dimensions);

  /// @brief Project: Proj = (Data - Mean) * Eigen^T
  void Project(const cv::Mat &data, cv::Mat &projected);

  /// @brief BackProject: Reduced = Proj * Eigen - Mean
  void BackProject(const cv::Mat &projected, cv::Mat &reduced);

  /// @brief Return eigenvector i
  cv::Mat GetEigenVector(const int i);

private:
  /// @brief Compute the covariance matrix on the cpu C = (D-1)^-1 * X*X^T
  void CovarianceGold(const cv::Mat &data, cv::Mat &cov);

  /// @brief Compute eigenvectors on the cpu (mEigen)
  void EigenVectorsGold();

  /// @brief Compute the covariance matrix on the gpu C = (D-1)^-1 * X*X^T
  bool CovarianceDevice(const cv::Mat &data, cv::Mat &cov);

  /// @brief Compute eigenvectors on the gpu (mEigen)
  bool EigenVectorsDevice();

  /// @brief Initialize gpu computing device
  void InitDevice();

  int mRows; ///< Number of rows in the data
  int mCols; ///< Number of columns in the data
  int mCurRow; ///< Current row of data added
  int mDimensions; ///< Number of principal components to use
  bool mUseDevice; ///< Use gpu or not

  cv::Mat mData; ///< mData = X = data centered around mean
  cv::Mat mMean; ///< Mean of the data, mean(X)
  cv::Mat mEigen; ///< Eigenvectors of the data
  cv::Mat mE; ///< Eigenvectors of C = (D-1)^-1 * X*X^T
  cv::Mat mS; ///< Eigenvalues of C = (D-1)^-1 * X*X^T
};

#endif // PCA_H
