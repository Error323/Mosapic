#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>

#include <boost/program_options.hpp>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#define RAND_FLT()    (rand()/static_cast<float>(RAND_MAX))
#define RAND_INT(a,b) ((rand() % ((b)-(a) + 1)) + (a))

#define COUNTER_START_VAL 10
#define INIT_COUNTER(c) int c = COUNTER_START_VAL
#define COUNT_DOWN(i, c, v)                       \
  do {                                            \
    if (i % (v/COUNTER_START_VAL) == 0 && c >= 0) \
    {                                             \
      std::cout << c << " " << std::flush;        \
      c--;                                        \
    }                                             \
  } while(0)                                      \

namespace po = boost::program_options;

void drawLine(cv::Mat &img)
{
  cv::Point2i p1(RAND_INT(0, img.rows), RAND_INT(0, img.cols));
  cv::Point2i p2(RAND_INT(0, img.rows), RAND_INT(0, img.cols));
  int thickness = RAND_INT(1, 10);
  int color = RAND_INT(0,255);
  cv::line(img, p1, p2, cv::Scalar(color,color,color), thickness, CV_AA);
}

void save(const cv::Mat &img, const int i)
{
  std::stringstream ss;
  ss << "img-" << i << ".png";
  cv::imwrite(ss.str(), img);
}

int main(int argc, char *argv[])
{
  srand(time(0));
  int tile_size, iterations;

  po::options_description generate("Generation options");
  generate.add_options()
  ("help,h", "produce help message")
  ("tilesize,t", po::value<int>(&tile_size)->default_value(150), "tile size")
  ("iterations,i", po::value<int>(&iterations)->default_value(10000), "number of iterations")
  ;

  po::options_description cmdline_options;
  cmdline_options.add(generate);

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << generate << std::endl;
    return 0;
  }

  cv::Mat img(tile_size, tile_size, CV_8UC3);
  std::cout << "Generating..." << std::flush;
  INIT_COUNTER(num_img);
  for (int i = 0; i < iterations; i++)
  {
    img.setTo(cv::Scalar(255,255,255));

    int num_lines = RAND_INT(1, 3);
    for (int j = 0; j < num_lines; j++)
      drawLine(img);

    save(img, i);
    COUNT_DOWN(i, num_img, iterations);
  }
  std::cout << "[done]" << std::endl;

  return 0;
}
