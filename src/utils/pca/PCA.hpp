#ifndef PCA_H
#define PCA_H

#include <Eigen/Dense>

using namespace Eigen;

class PCA
{
public:
  /// @brief Const: create datamatrix rows * cols, rows <= cols
  PCA(const int rows, const int cols);

  /// @brief Add a new data row
  void AddRow(const RowVectorXf &row);

  /// @brief Compute eigenvectors, only when data matrix is filled
  void Solve(const int dimensions);

  /// @brief Project: Proj = (Data - Mean) * Eigen^T
  void Project(const MatrixXf &data, MatrixXf &projected);

  /// @brief BackProject: Reduced = Proj * Eigen - Mean
  void BackProject(const MatrixXf &projected, MatrixXf &reduced);

  /// @brief Return eigenvector i
  void GetEigenVector(const int i, RowVectorXf &eigenvector);

  /// @brief Return eigenvalue i
  float GetEigenValue(const int i);

  /// @brief Return the projected data: Proj = (Data - Mean) * Eigen^T
  void GetProjectedData(MatrixXf &projected);


private:
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
