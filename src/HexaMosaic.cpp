#include "HexaMosaic.hpp"

#include "Debugger.hpp"
#include "Image.hpp"
#include <cmath>

#define HALF_HEXAGON_HEIGHT sinf(M_PI / 3.0f)
#define HEXAGON_HEIGHT (2.0f * HALF_HEXAGON_HEIGHT)

HexaMosaic::HexaMosaic(
	rcString inSourceImage,
	rcString inDestImage,
	rcString inDatabase,
	cInt inWidth,
	cInt inHeight
):
	mSourceImage(inSourceImage),
	mDestImage(inDestImage),
	mDatabase(inDatabase),
	mWidth(inWidth),
	mHeight(inHeight) {
}

void HexaMosaic::Create() {
	Image src_img;
	src_img.Read(mSourceImage);

	// unit dimensions of hexagon facing upwards
	cFloat unit_dx    = 2.0f * HALF_HEXAGON_HEIGHT;
	cFloat unit_dy    = 1.5f;
	cFloat unit_ratio = unit_dx / unit_dy;
	cFloat radius     = 20.0f;

	// real ratios (from source image) of hexagon facing upwards
	cFloat dst_ratio  = (mWidth * unit_dx) / (mHeight * unit_dy);
	cFloat src_ratio  = (float(src_img.Width()) / src_img.Height());

	// source height is greater then destination height
	float dx, dy, start_x, start_y;
	if (src_ratio < dst_ratio)
	{
		dx = float(src_img.Width()) / mWidth;
		dy = dx / unit_ratio;
		start_x = dx / 2.0f;
		start_y = (src_img.Height() - mHeight * dy) / 2.0f;
	}
	// source width is greater then destination width
	else
	{
		dy = float(src_img.Height()) / mHeight;
		dx = dy * unit_ratio;
		start_y = dy / 2.0f;
		start_x = (src_img.Width() - mWidth * dx) / 2.0f;
	}

	Image dst_img(int(roundf(mWidth*radius*unit_dx)), int(roundf(mHeight*radius*unit_dy)));
	for (int j = 0; j < mHeight; j++)
	{
		cInt src_y = roundf(start_y + j * dy);
		cFloat dst_y = unit_dy * radius * (j + 0.5f);
		for (int i = 0; i < mWidth; i++)
		{
			cInt src_x = roundf(start_x + i * dx + ((j % 2) * (dx / 2.0f)));
			cFloat dst_x = unit_dx * radius * (i + 0.5f + ((j % 2) / 2.0f));
			cUint32 color = src_img.GetPixel(src_x, src_y);
			FillHexagon(dst_img, dst_x, dst_y, radius, color);
		}
	}

	dst_img.Write(mDestImage);
}

void HexaMosaic::FillHexagon(rImage inImg, cFloat inX, cFloat inY, float inRadius, cUint32 inColor) {

	// XXX: Removes black edges. Should be fixed with anti-aliasing
	inRadius += 1.0f;

	for (int j = -inRadius; j < inRadius; j++)
	{
		for (int i = roundf(-inRadius * HALF_HEXAGON_HEIGHT); i < roundf(inRadius * HALF_HEXAGON_HEIGHT); i++)
		{
			if ( abs(i) < (j + inRadius) * HEXAGON_HEIGHT && -abs(i) > (j - inRadius) * HEXAGON_HEIGHT )
			{
				inImg.PutPixel(i+inX, j+inY, inColor);
			}
		}
	}
}
