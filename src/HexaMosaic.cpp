#include "HexaMosaic.hpp"

#include "Debugger.hpp"
#include "Image.hpp"

#include <cmath>
#include <fstream>
#include <queue>
#include <algorithm>
#include <sstream>
#include <iostream>

#include <SDL/SDL_image.h>

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

	// Load database
	std::ifstream data((mDatabase + "/" + DATABASE_FILENAME).c_str(), std::ios::in);
	String line;
	vString record;
	int record_count = 0;
	while(!std::getline(data, line).eof()) 
	{
		Split(record, line, ',');
		ASSERT(record.size() == 22);
		for (unsigned int i = 1; i < record.size(); i++) 
			db_data.push_back(atof(record[i].c_str()));

		record.clear();
		record_count++;
	}

	// Shuffle input coordinates
	std::vector<std::pair<int,int> > coords;
	for (int j = 0; j < mHeight; j++)
		for (int i = 0; i < mWidth; i++)
			coords.push_back(std::pair<int,int>(i,j));
	
	random_shuffle(coords.begin(), coords.end());

	// Construct hexa picture
	Image tile_img;
	vInt ids;
	int percentage_done = 0;
	for (int k = 0, n = coords.size(); k < n; k++)
	{
		int i = coords[k].first;
		int j = coords[k].second;
		cInt src_y = roundf(start_y + j * dy);
		cFloat dst_y = unit_dy * radius * (j + 0.5f);
		cInt src_x = roundf(start_x + i * dx + ((j % 2) * (dx / 2.0f)));
		cFloat dst_x = unit_dx * radius * (i + 0.5f + ((j % 2) / 2.0f));
		src_data.clear();
		int dimensions = HexaMosaic::ExtractInfo(src_img, src_x, src_y, radius, src_data);

		// match with database
		std::priority_queue<Match> KNN; // All nearest neighbours
		for (int img_id = 0; img_id < record_count; img_id++)
		{
			float dist = 0.0f;
			for (int dim = 0; dim < dimensions; dim++)
			{
				dist += fabs(db_data[img_id*dimensions+dim] - src_data[dim]);
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
		tile_img.Read(mDatabase + "/" + s.str() + ".bmp");
		FillHexagon(tile_img, dst_img, dst_x, dst_y, radius);
		int percentage = floorf((k/float(n))*100.0f);
		if (percentage > percentage_done)
		{
			percentage_done = percentage;
			printf("%05d/%05d %d%%\n", k, n, percentage_done);
		}
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

int HexaMosaic::ExtractInfo(rImage inImg, cInt inX, cInt inY, cFloat inRadius, rvFloat outData) {
	static std::vector<std::pair<int,int> > sample_coordinates, circle_coordinates;
	static vFloat weights;

	// Cache coordinates
	cInt half_radius = inRadius / 2.0f;
	if (sample_coordinates.empty())
	{
		// Extract 6 points from image on half radius with 120 degrees space
		float x = 0.0f;
		float y = half_radius;
		float theta = 0.0f;
		for (int i = 0; i < 6; i++)
		{
			theta += M_PI / 3.0f;
			x = half_radius*sinf(theta);
			y = half_radius*cosf(theta);
			sample_coordinates.push_back(std::pair<int,int>(int(roundf(x)), int(roundf(y))));
		}

		// Extract center coordinate
		sample_coordinates.push_back(std::pair<int,int>(0, 0));

		// Extract all coordinates in a circle
		float sigma = half_radius / 2.0f;
		float norm = HexaMosaic::GaussDens(0.0f, 0.0f, sigma);
		for (int y = -half_radius; y <= half_radius; y++)
		{
			for (int x = -half_radius; x <= half_radius; x++)
			{
				cFloat r = sqrtf(x*x + y*y);
				if (r <= half_radius)
				{
					circle_coordinates.push_back(std::pair<int,int>(x,y));
					weights.push_back(HexaMosaic::GaussDens(r, 0.0f, sigma)/norm);
				}
			}
		}

#ifdef DEBUG
		Image sample(100,100);
		vUint32 colors;
		colors.push_back(0xFF0000);
		colors.push_back(0x00FF00);
		colors.push_back(0x0000FF);
		colors.push_back(0xFF00FF);
		colors.push_back(0x00FFFF);
		colors.push_back(0xFFFF00);
		colors.push_back(0xFFFFFF);
		for (int k = 0, n = sample_coordinates.size(); k < n; k++)
		{
			cInt sample_x = sample_coordinates[k].first;
			cInt sample_y = sample_coordinates[k].second;
			for (int l = 0, m = circle_coordinates.size(); l < m; l++)
			{
				cInt circle_x = circle_coordinates[l].first;
				cInt circle_y = circle_coordinates[l].second;
				cInt x = sample_x + circle_x + inRadius;
				cInt y = sample_y + circle_y + inRadius;
				Uint8 r, g, b;
				SDL_GetRGB(colors[k], sample.GetFormat(), &r, &g, &b);
				r *= weights[l];
				g *= weights[l];
				b *= weights[l];
				sample.PutPixel(x, y, r << 16 | g << 8 | b);
			}
		}
		sample.Write("sample");
#endif
	}

	// For each sample coordinate compute the average pixelcolor of a circle
	Uint8 r, g, b;
	float r_avg, g_avg, b_avg;
	for (int k = 0, n = sample_coordinates.size(); k < n; k++)
	{
		r_avg = g_avg = b_avg = 0.0f;

		cInt sample_x = sample_coordinates[k].first;
		cInt sample_y = sample_coordinates[k].second;
		for (int l = 0, m = circle_coordinates.size(); l < m; l++)
		{
			cInt x = sample_x + circle_coordinates[l].first  + inX;
			cInt y = sample_y + circle_coordinates[l].second + inY;
			inImg.GetRgb(x, y, &r, &g, &b);
			r_avg += weights[l] * r;
			g_avg += weights[l] * g;
			b_avg += weights[l] * b;
		}

		r_avg *= WEIGHT_RED;
    	g_avg *= WEIGHT_GREEN;
    	b_avg *= WEIGHT_BLUE;

		r_avg /= circle_coordinates.size();
		g_avg /= circle_coordinates.size();
		b_avg /= circle_coordinates.size();

		outData.push_back(r_avg);
		outData.push_back(g_avg);
		outData.push_back(b_avg);
	}

	return sample_coordinates.size() * 3;
}

int HexaMosaic::Split(rvString outSplit, rcString inString, char inSplitChar) {
	std::string::const_iterator s = inString.begin();
	while (true) 
	{
		std::string::const_iterator begin = s;

		while (*s != inSplitChar && s != inString.end())
			++s;

		outSplit.push_back(std::string(begin, s));

		if (s == inString.end())
				break;

		if (++s == inString.end()) 
		{
				outSplit.push_back("");
				break;
		}
	}
	return outSplit.size();
}

float HexaMosaic::GaussDens(cFloat inX, cFloat inMu, cFloat inSigma) {
	cFloat a = 1.0f / (inSigma * sqrtf(2.0f * M_PI));
	cFloat b = expf(-(((inX - inMu) * (inX - inMu)) / (2.0f * inSigma * inSigma)));
	return (a * b);
}
