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
		return 0;
	}
	return 1;
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
				return p[0] << 16 | p[1] << 8 | p[2];
			else
				return p[0] | p[1] << 8 | p[2] << 16;
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

bool Image::Resize(cInt inSize) {
	ASSERT(mSurface != NULL);

	if (Width() < inSize || Height() < inSize)
		return false;

	pSDL_Surface surface = SDL_CreateRGBSurface(SDL_SRCCOLORKEY, inSize, inSize, 24, 0, 0, 0, 0);
	SDL_LockSurface(mSurface);

	cInt min = Width() < Height() ? Width() : Height();
	cInt step_size = min / inSize;
	cInt start_x = (Width() - min) / 2;
	cInt start_y = (Height() - min) / 2;

	for (int j = 0; j < inSize; j++)
	{
		cInt y = start_y + j*step_size;
		for (int i = 0; i < inSize; i++)
		{
			cInt x = start_x + i*step_size;
			cUint32 color = Average(x, y, step_size, step_size);
			PutPixel(surface, i, j, color);
		}
	}

	SDL_UnlockSurface(mSurface);
	SDL_FreeSurface(mSurface);
	mSurface = surface;

	return true;
}

void Image::GetRgb(cInt inX, cInt inY, pUint8 r, pUint8 g, pUint8 b) {
	cUint32 color = GetPixel(inX, inY);
	*r = (color >> 16) & 0xFF;
	*g = (color >> 8) & 0xFF;
	*b = color & 0xFF;
}

Uint32 Image::Average(cInt inXStart, cInt inYStart, cInt inXEnd, cInt inYEnd) {
	Uint32 r_sum, g_sum, b_sum;
	r_sum = g_sum = b_sum = 0;
	int i = 0;
	for (int x = inXStart; x < (inXStart+inXEnd); x++) {
		for (int y = inYStart; y < (inYStart+inYEnd); y++) {
			Uint8 r, g, b;
			GetRgb(x, y, &r, &g, &b);
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
		return r << 16 | g << 8 | b;
	else
		return r | g << 8 | b << 16;
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
