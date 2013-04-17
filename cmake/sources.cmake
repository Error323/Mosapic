#-------------------------------------------------------------------------------
# Source listing
#-------------------------------------------------------------------------------

SET(mosacrawler_SOURCE
  src/crawler/Main.cpp
  src/crawler/MosaCrawler.cpp
  src/utils/Verbose.cpp
  src/utils/Timer.cpp
  src/utils/Types.hpp
  src/utils/Debugger.hpp
  src/utils/cuda/init.cu
  src/utils/cuda/init.h
  src/utils/cuda/process.h
  src/utils/cuda/process.cu
)

SET(mosapic_SOURCE
  src/mosaic/Main.cpp
  src/mosaic/Mosapic.cpp
  src/utils/pca/PCA.cpp
  src/utils/Verbose.cpp
  src/utils/Timer.cpp
  src/utils/Types.hpp
  src/utils/Debugger.hpp
)
