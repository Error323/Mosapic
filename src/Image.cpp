#include "Image.hpp"
#include "Debugger.hpp"

#include <iostream>
#include <cmath>
#include <SDL/SDL_image.h>

Image::Image() {
	mSurface = NULL;
}

Image::Image(cInt inWidth, cInt inHeight) {
	mSurface = SDL_CreateRGBSurface(SDL_SRCCOLORKEY, inWidth, inHeight, 24, 0, 0, 0, 0);
}

Image::~Image() {
	SDL_FreeSurface(mSurface);
}

int Image::Read(rcString inFileName) {
	mSurface = IMG_Load(inFileName.c_str());
	if (mSurface == NULL) 
	{
		fprintf(stderr, "Couldn't load %s: %s\n", inFileName.c_str(), SDL_GetError());
		return 1;
	}
	return 0;
}

void Image::Write(rcString inFileName) {
	ASSERT(mSurface != NULL);
	SDL_SaveBMP(mSurface, (inFileName + ".bmp").c_str());
}

Uint32 Image::GetPixel(cInt inX, cInt inY) {
	ASSERT(mSurface != NULL);
	if (inX < 0 || inX >= Width() || inY < 0 || inY >= Height())
		return 0;

    int bpp = mSurface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    pUint8 p = (pUint8)mSurface->pixels + inY * mSurface->pitch + inX * bpp;

    switch(bpp) {
		case 1: {
			return *p;
		}
		case 2: {
			return *(pUint16)p;
		}
		case 3: {
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
				return p[2] << 16 | p[1] << 8 | p[0];
			else
				return p[0] << 16 | p[1] << 8 | p[2];
		}
		case 4: {
			return *(pUint32)p;
		}
		default: { 
			ASSERT_MSG(false, "bpp = %d", bpp);
			return 0;
		}
    }
}

pSDL_PixelFormat Image::GetFormat() {
	return mSurface->format;
}

bool Image::Resize(int inX, int inY, cBool inDeform, cFloat inCropScalar) {
	ASSERT(mSurface != NULL);

	if (Width() < inX || Height() < inY)
		return false;

	if (!inDeform) {
		inX = inX < inY ? inX : inY;
		inY = inX;
	}

	pSDL_Surface surface = SDL_CreateRGBSurface(SDL_SRCCOLORKEY, inX, inY, 24, 0, 0, 0, 0);
	SDL_LockSurface(mSurface);

	int x, y;
	cInt X = Width();
	cInt Y = Height();

	float xSize = (1.0f-inCropScalar) * X;
	float ySize = (1.0f-inCropScalar) * Y;
	int xFactor = (int) roundf(xSize/inX);
	int yFactor = (int) roundf(ySize/inY);
	Uint32 p;

	for (int i = 0; i < inX; i++)
	{
		x = (int) roundf(((X - xSize) / 2) + (i * (xSize / inX)));
		for (int j = 0; j < inY; j++)
		{
			y = (int) roundf(((Y - ySize) / 2) + (j * (ySize / inY)));
			p = Average(x, y, xFactor, yFactor);
			PutPixel(surface, i, j, p);
		}
	}

	SDL_UnlockSurface(mSurface);
	SDL_FreeSurface(mSurface);
	mSurface = surface;

	return true;
}

Uint32 Image::Average(cInt inXStart, cInt inYStart, cInt inXEnd, cInt inYEnd) {
	Uint32 r_sum, g_sum, b_sum;
	r_sum = g_sum = b_sum = 0;
	int i = 0;
	for (int x = inXStart; x < (inXStart+inXEnd); x++) {
		for (int y = inYStart; y < (inYStart+inYEnd); y++) {
			Uint8 r, g, b;
			Uint32 p = GetPixel(x, y);
			SDL_GetRGB(p, mSurface->format, &r, &g, &b);
			r_sum += r;
			g_sum += g;
			b_sum += b;
			i++;
		}
	}
	Uint8 r = r_sum/i;
	Uint8 g = g_sum/i;
	Uint8 b = b_sum/i;

	if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
		return b << 16 | g << 8 | r;
	else
		return r << 16 | g << 8 | b;
}

void Image::PutPixel(cInt inX, cInt inY, cUint32 inColor) {
	PutPixel(mSurface, inX, inY, inColor);
}

void Image::PutPixel(pSDL_Surface inSurface, cInt inX, cInt inY, cUint32 inColor) {
	ASSERT(inSurface != NULL);

	if (inX < 0 || inX >= Width() || inY < 0 || inY >= Height())
		return;

    int bpp = inSurface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (pUint8)inSurface->pixels + inY * inSurface->pitch + inX * bpp;

    switch(bpp) {
		case 1: {
			*p = inColor;
			break;
		}
		case 2: {
			*(pUint16)p = inColor;
			break;
		}
		case 3: {
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
		}
		case 4: {
			*(pUint32)p = inColor;
			break;
		}
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
