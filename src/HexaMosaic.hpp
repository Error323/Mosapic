#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <string>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "Types.hpp"

DECLARE_CLASS(HexaMosaic)

// Unit hexagon (i.e. edge length = 1) with its corners facing north and south
#define HALF_HEXAGON_WIDTH	sinf(M_PI / 3.0f)
#define HEXAGON_WIDTH		(2.0f * HALF_HEXAGON_WIDTH)
#define HEXAGON_HEIGHT		2.0f

#define DATABASE_NAME		"meta.yml"
#define IMAGE_PREFIX		"img_"
#define IMAGE_EXT			".jpg"

class HexaMosaic {
public:
	HexaMosaic(
		rcString inSourceImage,
		rcString inDatabase,
		cInt inWidth,
		cInt inHeight,
		cBool inGrayscale,
		cInt inDimensions,
		cInt inMaxRadius
		);

	void Create();
	
private:
	struct Match {
		Match(): id(-1), val(-1.0f) {}
		Match(int pid, float v): id(pid), val(v) {}
		int id;
		float val;
		bool operator< (const Match& m) const {
			return val > m.val;
		}
	};

	float GetDistance(
		const cv::Mat& inSrcRow,
		const cv::Mat& inDataRow
		);

	bool InHexagon(
		cFloat inX,
		cFloat inY,
		cFloat inRadius
		);

	String mSourceImage;
	String mDatabaseDir;

	int mWidth;
	int mHeight;
	bool mUseGrayscale;
	int mDimensions;
	int mMinRadius;
	int mNumImages;
	int mHexWidth;
	int mHexHeight;
	int mHexRadius;

	cv::Mat mDatabase;
	cv::Mat mHexMask;

	std::vector<cv::Point2i> mCoords;
	std::vector<int> mIndices;
};

#endif // HEXAMOSAIC_HDR
