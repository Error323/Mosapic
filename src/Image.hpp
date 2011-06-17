#ifndef IMAGE_HDR
#define IMAGE_HDR

#include <string>
#include <SDL/SDL_image.h>

class Image {
public:
	Image();
	Image(const int inWidth, const int inHeight);
	~Image();

	int Read(const std::string& inFileName);
	void Write(const std::string& inFileName);

	Uint32 GetPixel(const int inX, const int inY);
	void PutPixel(const int inX, const int inY, const Uint32 inColor);

	int Width();
	int Height();

private:
	SDL_Surface *mSurface;
};

#endif // IMAGE_HDR
