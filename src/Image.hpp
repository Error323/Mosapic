#ifndef IMAGE_HDR
#define IMAGE_HDR

#include <string>
#include "Types.hpp"

DECLARE_CLASS(Image);
DECLARE_CLASS(SDL_Surface);

class Image {
public:
	Image();
	Image(cInt inWidth, cInt inHeight);
	~Image();

	int Read(rcString inFileName);
	void Write(rcString inFileName);

	Uint32 GetPixel(cInt inX, cInt inY);
	void PutPixel(cInt inX, cInt inY, cUint32 inColor);

	int Width();
	int Height();

private:
	pSDL_Surface mSurface;
};

#endif // IMAGE_HDR
