#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "Types.hpp"
#include "Version.hpp"
#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"

namespace po = boost::program_options;

int main(int argc,char **argv) {
	po::options_description generic("Generic options");
	generic.add_options()
		("version,v", "print version string")
		("help,h", "produce help message")
	;

	po::options_description crawl("Crawler options");
	crawl.add_options()
		("image-dir,i", po::value<String>(), "image directory")
		("output-dir,o", po::value<String>(), "cache directory and database")
	;

	po::options_description hexapic("Hexapic options");
	hexapic.add_options()
		("input-image", po::value<String>(), "source image")
		("output-image", po::value<String>(), "output image")
		("database", po::value<String>(), "database directory")
		("width", po::value<int>(), "width in tile size")
		("height", po::value<int>(), "height in tile size")
	;

	po::options_description cmdline_options;
	cmdline_options.add(generic).add(crawl).add(hexapic);

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
	po::notify(vm);

	if (vm.count("version"))
	{
		std::cout << HUMAN_NAME << std::endl;
	}
	else
	if (vm.count("image-dir") && vm.count("output-dir"))
	{
		cString image_dir  = vm["image-dir"].as<String>();
		cString output_dir = vm["output-dir"].as<String>();

		if (!boost::filesystem::exists(image_dir))
		{
			std::cerr << "Image directory doesn't exist." << std::endl;
			return 1;
		}

		HexaCrawler hc;
		hc.Crawl(image_dir, output_dir);
	}
	else
	if (vm.count("input-image") && vm.count("output-image") && 
		vm.count("database") && vm.count("width") && vm.count("height"))
	{
		cString input_image  = vm["input-image"].as<String>();
		cString output_image = vm["output-image"].as<String>();
		cString database     = vm["database"].as<String>();
		cInt width           = vm["width"].as<int>();
		cInt height          = vm["height"].as<int>();

		if (!boost::filesystem::exists(input_image))
		{
			std::cerr << "Source image doesn't exist." << std::endl;
			return 1;
		}

		if (!boost::filesystem::exists(database + "/" + DATABASE_FILENAME))
		{
			std::cerr << "Database doesn't exist." << std::endl;
			return 1;
		}

		HexaMosaic hm(input_image, output_image, database, width, height);
		hm.Create();
	}
	else
	{
		std::cout << HUMAN_NAME << std::endl;
		std::cout << "Constructs a hexagonal mosaic from images." << std::endl;
		std::cout << std::endl << generic << std::endl;
		std::cout << std::endl << crawl << std::endl;
		std::cout << std::endl << hexapic << std::endl;
		return 1;
	}

	return 0;
}
