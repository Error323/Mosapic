#include "Image.hpp"
#include "Debugger.hpp"
#include <iostream>

Image::Image() {
	mSurface = NULL;
}

Image::Image(const int inWidth, const int inHeight) {
	mSurface = SDL_CreateRGBSurface(SDL_SRCCOLORKEY, inWidth, inHeight, 24, 0, 0, 0, 0);
}

Image::~Image() {
	SDL_FreeSurface(mSurface);
}

int Image::Read(const std::string& inFileName) {
	mSurface = IMG_Load(inFileName.c_str());
	if (mSurface == NULL) 
	{
		fprintf(stderr, "Couldn't load %s: %s\n", inFileName.c_str(), SDL_GetError());
		return 1;
	}
	return 0;
}

void Image::Write(const std::string& inFileName) {
	ASSERT(mSurface != NULL);
	SDL_SaveBMP(mSurface, inFileName.c_str());
}

Uint32 Image::GetPixel(const int inX, const int inY) {
	ASSERT(mSurface != NULL);
	if (inX < 0 || inX >= Width() || inY < 0 || inY >= Height())
		return 0;

    int bpp = mSurface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)mSurface->pixels + inY * mSurface->pitch + inX * bpp;

    switch(bpp) {
			case 1: return *p;
			case 2: return *(Uint16 *)p;
			case 3:
			{
				if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
					return p[2] << 16 | p[1] << 8 | p[0];
				else
					return p[0] << 16 | p[1] << 8 | p[2];
			}
			case 4: return *(Uint32 *)p;
			default: return 0; /* shouldn't happen, but avoids warnings */
    }
}

void Image::PutPixel(const int inX, const int inY, const Uint32 inColor) {
	ASSERT(mSurface != NULL);

	if (inX < 0 || inX >= Width() || inY < 0 || inY >= Height())
		return;

    int bpp = mSurface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)mSurface->pixels + inY * mSurface->pitch + inX * bpp;

    switch(bpp) {
    case 1:
        *p = inColor;
        break;
    case 2:
        *(Uint16 *)p = inColor;
        break;
    case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
		{
			p[0] = (inColor >> 16) & 0xFF;
			p[1] = (inColor >> 8) & 0xFF;
			p[2] = inColor & 0xFF;
		}
		else
		{
			p[0] = inColor & 0xFF;
			p[1] = (inColor >> 8) & 0xFF;
			p[2] = (inColor >> 16) & 0xFF;
		}
        break;
    case 4:
        *(Uint32 *)p = inColor;
        break;
    }
}

int Image::Width() { 
	ASSERT(mSurface != NULL);
	return mSurface->w; 
}

int Image::Height() {
	ASSERT(mSurface != NULL);
	return mSurface->h; 
}
