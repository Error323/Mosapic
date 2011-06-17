#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <string>

class HexaMosaic {
public:
	HexaMosaic(
		const std::string& inSourceImage,
		const std::string& inDestImage,
		const std::string& inDatabase,
		const int inWidth,
		const int inHeight
	);

	void Create();
	

private:
	std::string mSourceImage;
	std::string mDestImage;
	std::string mDatabase;
	int mWidth;
	int mHeight;
};

#endif // HEXAMOSAIC_HDR
