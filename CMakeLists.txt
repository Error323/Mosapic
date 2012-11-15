PROJECT(hexapic)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)


#-------------------------------------------------------------------------------
# General Settings
#-------------------------------------------------------------------------------
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_CXX_FLAGS "-Wall -Wextra")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=core2")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DDEBUG")

SET(ENABLE_PROFILING FALSE CACHE BOOL "Enable/Disable profiling")
IF (ENABLE_PROFILING)
  ADD_DEFINITIONS(-DENABLE_PROFILING)
ENDIF(ENABLE_PROFILING)


#-------------------------------------------------------------------------------
# Extract and set the version
#-------------------------------------------------------------------------------
EXECUTE_PROCESS(
	COMMAND git describe --tags
	OUTPUT_VARIABLE GIT_VERSION
	ERROR_VARIABLE GIT_ERROR
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

IF(GIT_VERSION)
	SET(VERSION ${GIT_VERSION})
ELSE(NOT GIT_VERSION)
	SET(VERSION "0.0.0")
ENDIF(GIT_VERSION)

CONFIGURE_FILE(
	"${hexapic_SOURCE_DIR}/cmake/Version.hpp.in"
	"${hexapic_BINARY_DIR}/Version.hpp"
)


#-------------------------------------------------------------------------------
# Include sources
#-------------------------------------------------------------------------------
INCLUDE(cmake/sources.cmake)
INCLUDE_DIRECTORIES(${hexapic_BINARY_DIR})


#-------------------------------------------------------------------------------
# Find 3rd party libraries and include their headers
#-------------------------------------------------------------------------------
FIND_PACKAGE(Boost COMPONENTS system filesystem regex program_options REQUIRED)
FIND_PACKAGE(OpenCV REQUIRED)
FIND_PACKAGE(CUDA)

IF(CUDA_FOUND)
  ADD_DEFINITIONS(-DENABLE_CUDA_DEVICE)
ENDIF(CUDA_FOUND)

INCLUDE_DIRECTORIES(
	${Boost_INCLUDE_DIRS}
	${OpenCV_INCLUDE_DIR}
)


#-------------------------------------------------------------------------------
# Define executable and link libraries
#-------------------------------------------------------------------------------
ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} ${hexapic_SOURCE})

TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME}
	${Boost_LIBRARIES}
	${OpenCV_LIBS}
)

#-------------------------------------------------------------------------------
# Status report
#-------------------------------------------------------------------------------
MESSAGE(STATUS " Compiling mode is '${CMAKE_BUILD_TYPE}'")
