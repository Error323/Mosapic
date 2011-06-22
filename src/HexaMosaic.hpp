#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <string>
#include <opencv2/opencv.hpp>
#include "Types.hpp"

#define DATABASE_FILENAME "database.dat"

DECLARE_CLASS(HexaMosaic);
DECLARE_CLASS(Image);

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
	static void PrincipalComponents(
		const cv::Mat& inImg,
		const int inDimensions,
		rvFloat outData
	);
	
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

	String mSourceImage;
	String mDestImage;
	String mDatabase;
	int mWidth;
	int mHeight;

	int Split(
		rvString outSplitted, 
		rcString inString, 
		char inSplitChar
	);
};

#endif // HEXAMOSAIC_HDR
