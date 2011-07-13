#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <string>
#include <opencv2/opencv.hpp>
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

	void LoadDatabase(
		rcString inFileName,
		cv::Mat& outDatabase
		);

	float GetDistance(
		const cv::Mat& inSrcRow,
		const cv::Mat& inDataRow
		);

	void CompressData(
		const cv::PCA& inPca,
		const cv::Mat& inUnCompressed,
		cv::Mat& outCompressed
		);

	bool InHexagon(
		cFloat inX,
		cFloat inY,
		cFloat inRadius
		);

	void DataRow(
		cInt inX,
		cInt inY,
		const cv::Mat& inSrcImg,
		const cv::Mat& inMask,
		cv::Mat& outDataRow
		);

	String mSourceImage;
	String mDatabaseDir;

	int mWidth;
	int mHeight;
	int mDimensions;
	int mMaxRadius;
	int mNumImages;
	int mHexWidth;
	int mHexHeight;

	cv::Mat mDatabase;
};

#endif // HEXAMOSAIC_HDR
