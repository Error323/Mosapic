#include "HexaMosaic.hpp"

#include "Debugger.hpp"
#include "Image.hpp"
#include <cmath>
#include <SDL/SDL_image.h>
#include <fstream>
#include <queue>
#include <algorithm>
#include <sstream>
#include <iostream>

#define WEIGHT_RED 0.299f
#define WEIGHT_GREEN 0.587f
#define WEIGHT_BLUE 0.114f
#define HALF_HEXAGON_WIDTH sinf(M_PI / 3.0f)
#define HEXAGON_WIDTH (2.0f * HALF_HEXAGON_WIDTH)

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
	cFloat unit_dx    = HEXAGON_WIDTH;
	cFloat unit_dy    = 1.5f;
	cFloat unit_ratio = unit_dx / unit_dy;
	cFloat radius     = 50.0f;

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
	vFloat src_data;
	vFloat db_data;

	std::ifstream data("data/database.dat", std::ios::in);
	String line;
	vString record;
	int record_count = 0;
	while(!std::getline(data, line).eof()) {
		Split(record, line, ',');
		for (unsigned int i = 1; i < record.size(); i++) {
			db_data.push_back(atof(record[i].c_str()));
		}
		record_count++;
	}

	Image tile_img;
	vInt ids;
	std::vector<std::pair<int,int> > coords;
	for (int j = 0; j < mHeight; j++)
		for (int i = 0; i < mWidth; i++)
			coords.push_back(std::pair<int,int>(i,j));
	
	random_shuffle(coords.begin(), coords.end());

	for (size_t k = 0; k < coords.size(); k++)
	{
		int i = coords[k].first;
		int j = coords[k].second;
		cInt src_y = roundf(start_y + j * dy);
		cFloat dst_y = unit_dy * radius * (j + 0.5f);
		cInt src_x = roundf(start_x + i * dx + ((j % 2) * (dx / 2.0f)));
		cFloat dst_x = unit_dx * radius * (i + 0.5f + ((j % 2) / 2.0f));
		src_data.clear();
		HexaMosaic::ExtractInfo(src_img, src_x, src_y, radius, src_data);
		// match with database
		std::priority_queue<Match> KNN; // All nearest neighbours
		for (int img_id = 0; img_id < record_count; img_id++)
		{
			float dist = 0.0f;
			for (int dim = 0; dim < 3; dim++)
			{
				dist += fabs(db_data[img_id*3+dim] - src_data[dim]);
			}
			KNN.push(Match(img_id, dist));
		}

		// Make sure we never use the same image twice
		std::stringstream s;
		int best_id = KNN.top().id;
		while (!KNN.empty() && find(ids.begin(), ids.end(), best_id) != ids.end())
		{
			KNN.pop();
			best_id = KNN.top().id;
		}
		ids.push_back(best_id);
		s << best_id;
		tile_img.Read(std::string("data/") + s.str() + ".bmp");
		FillHexagon(tile_img, dst_img, dst_x, dst_y, radius);
	}

	dst_img.Write(mDestImage);
}

void HexaMosaic::FillHexagon(
	rImage inImgSrc, 
	rImage inImgDst, 
	cFloat inX, 
	cFloat inY, 
	cFloat inRadius
) {
	cFloat radius = inRadius + 0.5f;
	for (int j = -radius; j < radius; j++)
	{
		for (int i = roundf(-radius * HALF_HEXAGON_WIDTH); i < roundf(radius * HALF_HEXAGON_WIDTH); i++)
		{
			if (HexaMosaic::InHexagon(i, j, radius))
			{
				cInt x = i+(radius*HALF_HEXAGON_WIDTH);
				cInt y = j+radius;
				cUint32 color = inImgSrc.GetPixel(x,y);
				inImgDst.PutPixel(i+inX, j+inY, color);
			}
		}
	}
}

inline bool HexaMosaic::InHexagon(cFloat inX, cFloat inY, cFloat inRadius) {
	// NOTE: inRadius is defined from the hexagon's center to a corner
	return   fabs(inX) < (inY + inRadius) * HEXAGON_WIDTH &&
			-fabs(inX) > (inY - inRadius) * HEXAGON_WIDTH;
}

void HexaMosaic::ExtractInfo(rImage inImg, cInt inX, cInt inY, cFloat inRadius, rvFloat outData) {
	cFloat radius = inRadius;
	float r_avg, g_avg, b_avg;
	r_avg = g_avg = b_avg = 0.0f;
	int count = 0;
	for (int j = -radius; j < radius; j++)
	{
		for (int i = roundf(-radius * HALF_HEXAGON_WIDTH); i < roundf(radius * HALF_HEXAGON_WIDTH); i++)
		{
			if (HexaMosaic::InHexagon(i, j, radius))
			{
				Uint8 r, g, b;
				if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
					inImg.GetRgb(i+inX, j+inY, &r, &g, &b);
				else
					inImg.GetRgb(i+inX, j+inY, &b, &g, &r);
				r_avg += WEIGHT_RED * r;
				g_avg += WEIGHT_GREEN * g;
				b_avg += WEIGHT_BLUE * b;
				count++;
			}
		}
	}
	r_avg /= count;
	g_avg /= count;
	b_avg /= count;
	outData.push_back(r_avg);
	outData.push_back(g_avg);
	outData.push_back(b_avg);
}

int HexaMosaic::Split(rvString outSplit, rcString inString, char inChar) {
	outSplit.clear();
	std::string::const_iterator s = inString.begin();
	while (true) {
		std::string::const_iterator begin = s;

		while (*s != inChar && s != inString.end())
			++s;

		outSplit.push_back(std::string(begin, s));

		if (s == inString.end())
				break;

		if (++s == inString.end()) {
				outSplit.push_back("");
				break;
		}
	}
	ASSERT(outSplit.size() == 4);
	return outSplit.size();
}
