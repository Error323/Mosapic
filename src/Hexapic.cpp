#include <iostream>
#include <string>
#include <cstdlib>

#include "HexaMosaic.hpp"
#include "Version.hpp"

void PrintUsage() {
	printf("Usage: hexapic\n");
}

int main(int argc,char **argv) {
	printf("%s\n", HUMAN_NAME);
	switch (argc)
	{
		case 6: 
		{
			std::string source_image = argv[1];
			std::string dest_image   = argv[2];
			std::string database     = argv[3];
			int width                = atoi(argv[4]);
			int height               = atoi(argv[5]);
			HexaMosaic hm(source_image, dest_image, database, width, height);
			hm.Create();
			break;
		}

		default: 
		{
			PrintUsage();
			exit(1);
		}
	}
	return 0;
}
