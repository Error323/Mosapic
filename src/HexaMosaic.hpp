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
		rcString inDestImage,
		rcString inDatabase,
		cInt inWidth,
		cInt inHeight
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

	float GetDistance(const cv::Mat& a, const cv::Mat& b);

	String mSourceImage;
	String mDestImage;
	String mDatabase;
	int mWidth;
	int mHeight;
};

#endif // HEXAMOSAIC_HDR
