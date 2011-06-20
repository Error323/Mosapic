#include "HexaCrawler.hpp"
#include "HexaMosaic.hpp"
#include "Image.hpp"

#include <iostream>
#include <sstream>

void HexaCrawler::Crawl(rcString inSrcDir, rcString inDstDir) {
	if (!boost::filesystem::exists(inDstDir))
	{
		std::cout << "Directory '" << inDstDir << "' doesn't exist yet, creating..." << std::endl;
		boost::filesystem::create_directory(inDstDir);
	}
	mDstDir = inDstDir;
	Crawl(inSrcDir);
	std::cout << "Processed " << mImgCount << " images." << std::endl;
}

void HexaCrawler::Crawl(const boost::filesystem::path &inPath) {
	static boost::match_results<std::string::const_iterator> what;
	static boost::regex img_ext(".*(bmp|BMP|jpg|JPG|jpeg|JPEG|png|PNG)");

	boost::filesystem::directory_iterator n;
	for (boost::filesystem::directory_iterator i(inPath); i != n; ++i) 
	{
		try
		{
			if (boost::filesystem::is_directory(i->status()))
			{
				Crawl(i->path());
			}
			else 
			if (boost::filesystem::is_regular_file(i->status()))
			{
				if (boost::regex_match(i->path().leaf(), what, img_ext, boost::match_default))
				{
					Process(i->path().string());
				}
			}
		}
		catch (const std::exception &ex)
		{
			std::cerr << i->path() << " " << ex.what() << std::endl;
		}
	}
}

void HexaCrawler::Process(rcString inImgName) {
	std::cout << "Processing '" << inImgName << "'...";
	static Image img;

	// Resize and store image
	if (!img.Read(inImgName) || !img.Resize(100))
	{
		std::cout << "[failed]" << std::endl;
		return;
	}
	std::stringstream s;
	s << mImgCount;
	img.Write(mDstDir + "/" + s.str());

	// Extract image info
	vFloat data;
	HexaMosaic::ExtractInfo(img, 50, 50, 50, data);
	
	// Store image info into database
	mDatabase.open((mDstDir + "/database.dat").c_str(), std::ios::app);
	mDatabase << mImgCount;
	for (size_t i = 0; i < data.size(); i++)
		mDatabase << "," << data[i];
	mDatabase << std::endl;
	mDatabase.flush();
	mDatabase.close();
	
	mImgCount++;
	std::cout << "[done]" << std::endl;
}
