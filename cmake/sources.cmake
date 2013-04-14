#-------------------------------------------------------------------------------
# Source listing
#-------------------------------------------------------------------------------

SET(hexacrawler_SOURCE
	src/crawler/Main.cpp
	src/crawler/HexaCrawler.cpp
  src/utils/Verbose.cpp
  src/utils/Timer.cpp
  src/utils/Types.hpp
  src/utils/Debugger.hpp
  src/utils/cuda/init.cu
  src/utils/cuda/init.h
)

SET(hexamosaic_SOURCE
	src/mosaic/Main.cpp
	src/mosaic/HexaMosaic.cpp
  src/utils/pca/PCA.cpp
  src/utils/Verbose.cpp
  src/utils/Timer.cpp
  src/utils/Types.hpp
  src/utils/Debugger.hpp
)
