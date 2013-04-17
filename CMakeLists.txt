project (mosapic CXX C)
cmake_minimum_required (VERSION 2.8)


#-------------------------------------------------------------------------------
# General Settings
#-------------------------------------------------------------------------------
set (CMAKE_CXX_FLAGS "-Wall -Wextra")
set (CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set (CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DQT_NO_DEBUG")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

option (ENABLE_OPENMP "Enable/disable openmp (used by Eigen3)" ON)
option (ENABLE_PROFILING "Enable/disable profiling" OFF)
option (ENABLE_CUDA "Enable/disable cuda" ON)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "Release")
endif (NOT CMAKE_BUILD_TYPE)

if (ENABLE_OPENMP)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif (ENABLE_OPENMP)

if (ENABLE_PROFILING)
  set (CMAKE_BUILD_TYPE "Release")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pg -fno-omit-frame-pointer")
endif (ENABLE_PROFILING)

if (ENABLE_CUDA)
  find_package (CUDA)
  add_definitions(-DENABLE_CUDA)
endif (ENABLE_CUDA)


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
	"${mosapic_SOURCE_DIR}/cmake/Version.hpp.in"
	"${mosapic_BINARY_DIR}/Version.hpp"
)


#-------------------------------------------------------------------------------
# Include sources
#-------------------------------------------------------------------------------
include (cmake/sources.cmake)
include_directories (${mosapic_BINARY_DIR})


#-------------------------------------------------------------------------------
# Find 3rd party libraries and include their headers
#-------------------------------------------------------------------------------
find_package (Eigen3 REQUIRED)
find_package (Qt4 COMPONENTS QtCore QtGui REQUIRED)

include_directories (
  ${EIGEN3_INCLUDE_DIR}
  ${QT_INCLUDE_DIR}
  ${QT_QTCORE_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
)


#-------------------------------------------------------------------------------
# Define executable and link libraries
#-------------------------------------------------------------------------------
if (CUDA_FOUND)
  cuda_add_executable (mosacrawler ${mosacrawler_SOURCE})
else (CUDA_FOUND)
  add_executable (mosacrawler ${mosacrawler_SOURCE})
endif (CUDA_FOUND)

target_link_libraries (mosacrawler
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
)

add_executable (mosapic ${mosapic_SOURCE})
target_link_libraries (mosapic
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
message (STATUS "   ENABLE_CUDA                 ${ENABLE_CUDA}")
message (STATUS "")
