#ifndef PCA_H
#define PCA_H

#include <Eigen/Dense>
#include <opencv2/opencv.hpp>

using namespace Eigen;

class PCA
{
public:
  /// @brief Const: create datamatrix rows * cols, rows <= cols
  PCA(const int rows, const int cols);

  /// @brief Add a new data row
  void AddRow(const RowVectorXf &row);
  void AddRow(const cv::Mat &row);

  /// @brief Compute eigenvectors, only when data matrix is filled
  void Solve(const int dimensions);

  /// @brief Project: Proj = (Data - Mean) * Eigen^T
  void Project(const MatrixXf &data, MatrixXf &projected);
  void Project(const cv::Mat &data, cv::Mat &projected);

  /// @brief BackProject: Reduced = Proj * Eigen - Mean
  void BackProject(const MatrixXf &projected, MatrixXf &reduced);
  void BackProject(const cv::Mat &projected, cv::Mat &reduced);

  /// @brief Return eigenvector i
  void GetEigenVector(const int i, RowVectorXf &eigenvector);
  void GetEigenVector(const int i, cv::Mat &eigenvector);


private:
  void CvMat2EigRow(const cv::Mat &in, RowVectorXf &out);
  void CvMat2EigMat(const cv::Mat &in, MatrixXf &out);
  void EigMat2CvMat(const MatrixXf &in, cv::Mat &out);
  void EigRow2CvMat(const RowVectorXf &in, cv::Mat &out);

  /// @brief Compute the covariance matrix C = (D-1)^-1 * X*X^T
  void CovarianceMatrix(const MatrixXf &data, MatrixXf &cov);

  /// @brief Compute the eigenvectors
  void EigenVectors(const MatrixXf &data, MatrixXf &eigenvectors);

  int mRows; ///< Number of rows in the data
  int mCols; ///< Number of columns in the data
  int mCurRow; ///< Current row of data added
  int mDimensions; ///< Number of principal components to use

  MatrixXf mData; ///< mData = X = data centered around mean
  MatrixXf mE; ///< Eigenvectors of C = (D-1)^-1 * X*X^T
  MatrixXf mEigen; ///< Eigenvectors of the data

  RowVectorXf mMean; ///< Mean of the data, mean(X)
  VectorXf mS; ///< Eigenvalues of C = (D-1)^-1 * X*X^T
};

#endif // PCA_H
