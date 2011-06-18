#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <string>
#include <SDL/SDL_image.h>

class Image;
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
	void FillHexagon(Image& inImg, const float inX, const float inY, const float inRadius, const Uint32 inColor);
};

#endif // HEXAMOSAIC_HDR
