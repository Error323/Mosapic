project (hexapic CXX C)
cmake_minimum_required (VERSION 2.8)


#-------------------------------------------------------------------------------
# General Settings
#-------------------------------------------------------------------------------
set (CMAKE_CXX_FLAGS "-Wall -Wextra")
set (CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

option (ENABLE_OPENMP "Enable/disable openmp (used by Eigen3)" OFF)
option (ENABLE_PROFILING "Enable/disable profiling" OFF)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "Release")
endif (NOT CMAKE_BUILD_TYPE)

if (ENABLE_OPENMP)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif (ENABLE_OPENMP)

if (ENABLE_PROFILING)
  set (CMAKE_BUILD_TYPE "Release")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pg -fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls -fno-default-inline -fno-inline")
endif (ENABLE_PROFILING)


#-------------------------------------------------------------------------------
# Extract and set the version
#-------------------------------------------------------------------------------
execute_process (
	COMMAND git describe --tags --always
	OUTPUT_VARIABLE VERSION
	ERROR_VARIABLE GIT_ERROR
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file (
	"${hexapic_SOURCE_DIR}/cmake/Version.hpp.in"
	"${hexapic_BINARY_DIR}/Version.hpp"
)


#-------------------------------------------------------------------------------
# Include sources
#-------------------------------------------------------------------------------
include (cmake/sources.cmake)
include_directories (${hexapic_BINARY_DIR})


#-------------------------------------------------------------------------------
# Find 3rd party libraries and include their headers
#-------------------------------------------------------------------------------
find_package (Boost COMPONENTS system filesystem regex program_options REQUIRED)
find_package (OpenCV COMPONENTS core highgui imgproc REQUIRED)
find_package (Eigen3 REQUIRED)
find_package (Qt4 COMPONENTS QtCore QtGui REQUIRED)

include_directories (
	${Boost_INCLUDE_DIRS}
	${OpenCV_INCLUDE_DIR}
  ${EIGEN3_INCLUDE_DIR}
  ${QT_INCLUDE_DIR}
  ${QT_QTCORE_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
)


#-------------------------------------------------------------------------------
# Define executable and link libraries
#-------------------------------------------------------------------------------
add_executable (hexacrawler ${hexacrawler_SOURCE})
target_link_libraries (hexacrawler
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
)

add_executable (hexamosaic ${hexamosaic_SOURCE})
target_link_libraries (hexamosaic
  ${Boost_LIBRARIES}
  ${OpenCV_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
)

#-------------------------------------------------------------------------------
# Status report
#-------------------------------------------------------------------------------
message (STATUS "")
message (STATUS " C/C++:")
message (STATUS "   C++ Compiler:               ${CMAKE_CXX_COMPILER}")
message (STATUS "   C++ flags (Release):        ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
message (STATUS "   C++ flags (Debug):          ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
message (STATUS "   C++ flags (RelWithDebInfo): ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message (STATUS "")
message (STATUS " CMakeflags (${CMAKE_PROJECT_NAME}):")
message (STATUS "   CMAKE_BUILD_TYPE            ${CMAKE_BUILD_TYPE}")
message (STATUS "   ENABLE_PROFILING            ${ENABLE_PROFILING}")
message (STATUS "   ENABLE_OPENMP               ${ENABLE_OPENMP}")
message (STATUS "")
