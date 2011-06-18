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
	

private:
	String mSourceImage;
	String mDestImage;
	String mDatabase;
	int mWidth;
	int mHeight;

	void FillHexagon(
		rImage inImg, 
		cFloat inX, 
		cFloat inY, 
		float inRadius, 
		cUint32 inColor
	);
};

#endif // HEXAMOSAIC_HDR
