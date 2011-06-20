#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>

#include "Types.hpp"
#include "Version.hpp"
#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"
#include "Image.hpp"

void PrintUsage() {
	std::cout << "Usage: hexapic" << std::endl;
}

int main(int argc,char **argv) {
	std::cout << HUMAN_NAME << std::endl;
	switch (argc)
	{
		case 3: {
			cString source_dir = argv[1];
			cString dest_dir   = argv[2];
			HexaCrawler hc;
			hc.Crawl(source_dir, dest_dir);
		}
		case 6: {
			cString source_image = argv[1];
			cString dest_image   = argv[2];
			cString database     = argv[3];
			cInt width           = atoi(argv[4]);
			cInt height          = atoi(argv[5]);
			HexaMosaic hm(source_image, dest_image, database, width, height);
			hm.Create();
			break;
		}

		default: {
			PrintUsage();
			exit(1);
		}
	}
	return 0;
}
