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

void CreateImages(rcString inDestDir) {
	Image img(100,100);
	for (int i = 0; i < 10000; i++)
	{
		cUint8 r = rand() % 255;
		cUint8 g = rand() % 255;
		cUint8 b = rand() % 255;
		cUint32 color = r << 16 | g << 8 | b;

		for (int y = 0; y < 100; y++)
			for (int x = 0; x < 100; x++)
				img.PutPixel(x, y, color);
		
		std::stringstream s;
		s << i;
		img.Write(inDestDir + "/" + s.str());
	}
}

int main(int argc,char **argv) {
	std::cout << HUMAN_NAME << std::endl;
	switch (argc)
	{
		case 2: {
			cString dest_dir = argv[1];
			CreateImages(dest_dir);
		}
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
