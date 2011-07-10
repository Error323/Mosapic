#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <string>
#include <opencv2/opencv.hpp>
#include "Types.hpp"

DECLARE_CLASS(HexaMosaic);

class HexaMosaic {
public:
	HexaMosaic(
		rcString inSourceImage,
		rcString inDatabase,
		cInt inWidth,
		cInt inHeight,
		cInt inDimensions
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

	float GetDistance(const cv::Mat& inSrcRow, const cv::Mat& inDataRow);

	String mSourceImage;
	String mDatabase;
	int mWidth;
	int mHeight;
	int mDimensions;
};

#endif // HEXAMOSAIC_HDR
