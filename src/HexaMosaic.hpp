#ifndef HEXAMOSAIC_HDR
#define HEXAMOSAIC_HDR

#include <string>
#include "Types.hpp"

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
	
	static bool InHexagon(
		cFloat inX, 
		cFloat inY, 
		cFloat inRadius
	);

	static void ExtractInfo(
		rImage inImg, 
		cInt inX, 
		cInt inY, 
		cFloat inRadius, 
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

	void FillHexagon(
		rImage inImgSrc, 
		rImage inImgDst, 
		cFloat inX, 
		cFloat inY, 
		cFloat inRadius
	);

	int Split(rvString v, rcString str, char c);
};

#endif // HEXAMOSAIC_HDR
