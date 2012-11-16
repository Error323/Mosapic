#ifndef MATIO_H
#define MATIO_H

#include <string>
#include <opencv2/opencv.hpp>

// Save matrix to binary file
bool SaveMat( const std::string &filename, const cv::Mat &M);

// Read matrix from binary file
bool ReadMat( const std::string &filename, cv::Mat &M);

#endif // MATIO_H
