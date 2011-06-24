#include <iostream>
#include <string>
#include <sstream>

#include <opencv2/opencv.hpp>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

void Resize(cv::Mat& inImg) {
	int min = std::min<int>(inImg.rows, inImg.cols);
	cv::Mat img_tmp;
	cv::Size size(min,min);
	cv::Point2f center(inImg.cols/2.0f, inImg.rows/2.0f);
	cv::getRectSubPix(inImg, size, center, img_tmp);
	cv::resize(img_tmp, inImg, cv::Size(100,100));
}

void crawl(const boost::filesystem::path &inPath, cv::Mat& outData) {
	static boost::match_results<std::string::const_iterator> what;
	static boost::regex img_ext(".*(bmp|BMP|jpg|JPG|jpeg|JPEG|png|PNG)");

	boost::filesystem::directory_iterator n;
	for (boost::filesystem::directory_iterator i(inPath); i != n; ++i) 
	{
		try
		{
			if (boost::filesystem::is_directory(i->status()))
				crawl(i->path(), outData);
			else 
			if (boost::filesystem::is_regular_file(i->status()))
			{
				if (boost::regex_match(i->path().leaf(), what, img_ext, boost::match_default))
				{
					std::cout << "Processing `" << i->path() << "'...";
					cv::Mat img_color = cv::imread(i->path().string(), 1);
					if (img_color.data == NULL || img_color.rows < 100 || img_color.cols < 100)
					{
						std::cout << "[failed]" << std::endl;
						continue;
					}
					Resize(img_color);
					img_color = img_color.reshape(1, 1);
					outData.create(outData.rows+1, img_color.cols, img_color.type());
					cv::Mat bottom_row = outData.row(outData.rows-1);
					img_color.copyTo(bottom_row);
					std::cout << "[done]" << std::endl;
				}
			}
		}
		catch (const std::exception &ex)
		{
			std::cerr << i->path() << " " << ex.what() << std::endl;
		}
	}
}

int main(int argc, char** argv) {
	if (argc != 3)
	{
		std::cout << "Usage: pca <data dir> <dimensions>" << std::endl << std::endl;
		std::cout << "  data dir\tthe data directory" << std::endl;
		std::cout << "  dimensions\tthe pca dimensions" << std::endl;
		return 1;
	}

	cv::Mat data;
	int num_files = 0, num_images = 0, rows = 0;
	int dimensions;
	std::string datadir;
	if (!boost::filesystem::exists(argv[1]))
	{
		std::cerr << argv[1] << " doesn't exist" << std::endl;
		return 1;
	}
	else
	{
		datadir = std::string(argv[1]);
		cv::FileStorage fs(datadir + "rawdata.yml", cv::FileStorage::READ);
		fs["num_files"] >> num_files;
		fs["num_images"] >> num_images;
		fs.release();

		cv::Mat tmp, roi;
		int current_row = 0;
		data.create(num_images, 100*100*3, CV_8UC1);
		for (int i = 0; i < num_files; i++)
		{
			std::stringstream s;
			s << i;
			std::string entry = datadir + "rawdata_" + s.str() + ".yml";
			std::cout << "Loading `" << entry << "'..." << std::flush;
			fs.open(entry, cv::FileStorage::READ);
			rows = 0;
			fs["rows"] >> rows;
			for (int j = 0; j < rows; j++)
			{
				std::stringstream ss;
				ss << j;
				fs["img_" + ss.str()] >> tmp;
				roi = data.row(current_row);
				tmp.copyTo(roi);
				current_row++;
			}
			fs.release();
			std::cout << "[done]" << std::endl;
		}
		dimensions = atoi(argv[2]);
	}

	std::cout << "Performing pca..." << std::flush;
	cv::PCA pca(data, cv::Mat(), CV_PCA_DATA_AS_ROW, dimensions);
	std::cout << "[done]" << std::endl;
	
	// Construct database
	cv::Mat database(num_images, dimensions, pca.eigenvectors.type());
	cv::Mat flat_img, compressed_img;
	compressed_img.create(1, dimensions, data.type());
	for (int i = 0; i < num_images; i++)
	{
		flat_img = data.row(i).clone();
		pca.project(flat_img, compressed_img);
		cv::Mat current_row = database.row(i);
		compressed_img.copyTo(current_row);
		std::cout << "Compressed " << i << "/" << rows << std::endl;
	}

	std::cout << "Saving data..." << std::flush;
	cv::FileStorage fs(datadir + "/database.yml", cv::FileStorage::WRITE);
	fs << "compression" << database;
	fs << "eigenvectors" << pca.eigenvectors;
	fs << "mean" << pca.mean;
	fs.release();
	std::cout << "[done]" << std::endl;

	cv::Mat compressed, reconstructed, img_color;
	compressed.create(1, dimensions, database.type());

	for (int i = 0; i < num_images; i++)
	{
		img_color = data.row(i);
		pca.project(img_color, compressed);
		pca.backProject(compressed, reconstructed);
		reconstructed = reconstructed.reshape(3, 100);
		std::stringstream s;
		s << i;
		std::string entry = "reconstruction-" + s.str() + ".jpg";
		cv::imwrite(entry, reconstructed);
	}

	for (int i = 0; i < dimensions; i++)
	{
		cv::Mat eigenvec;
		cv::normalize(pca.eigenvectors.row(i), eigenvec, 255, 0, cv::NORM_MINMAX);
		reconstructed = eigenvec.reshape(3, 100);
		std::stringstream s;
		s << i;
		std::string entry = "eigenvector-" + s.str() + ".jpg";
		cv::imwrite(entry, reconstructed);
	}


/*
	int dimensions = 32;
	cv::PCA pca(data, cv::Mat(), CV_PCA_DATA_AS_ROW, dimensions);
	cv::Mat mean = pca.mean.clone();
	mean = mean.reshape(3, 100);
	cv::imwrite("mean.jpg", mean);
	cv::Mat img_color = cv::imread(argv[2], 1);
	Resize(img_color);
	img_color = img_color.reshape(1, 1);
	cv::Mat compressed, reconstructed;
	compressed.create(1, dimensions, img_color.type());
	pca.project(img_color, compressed);
	pca.backProject(compressed, reconstructed);
	reconstructed = reconstructed.reshape(3, 100);
	cv::imwrite("reconstruction.jpg", reconstructed);
*/
	return 0;
}
