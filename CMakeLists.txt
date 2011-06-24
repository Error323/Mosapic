PROJECT(HEXAPIC)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#-------------------------------------------------------------------------------
# General Settings
# NOTE: -O2 breaks rendering without -fno-strict-aliasing
# NOTE: we need -rdynamic for backtrace() to work properly
#-------------------------------------------------------------------------------
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_CXX_FLAGS "-Wall -Wextra")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -rdynamic -DDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -rdynamic -DDEBUG")

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
	"${HEXAPIC_SOURCE_DIR}/cmake/Version.hpp.in"
	"${HEXAPIC_BINARY_DIR}/Version.hpp"
)


#-------------------------------------------------------------------------------
# Include sources
#-------------------------------------------------------------------------------
INCLUDE(cmake/sources.list)
INCLUDE_DIRECTORIES(${HEXAPIC_BINARY_DIR})


#-------------------------------------------------------------------------------
# Find 3rd party libraries and include their headers
#-------------------------------------------------------------------------------
FIND_PACKAGE(Boost COMPONENTS filesystem regex program_options REQUIRED)
FIND_PACKAGE(OpenCV REQUIRED)

INCLUDE_DIRECTORIES(
	${Boost_INCLUDE_DIRS}
	${OpenCV_INCLUDE_DIR}
)


#-------------------------------------------------------------------------------
# Define executable and link libraries
#-------------------------------------------------------------------------------
ADD_EXECUTABLE(hexapic ${HEXAPIC_SOURCE})
ADD_EXECUTABLE(pca ${HEXAPIC_SOURCE_DIR}/src/pca.cpp)

TARGET_LINK_LIBRARIES(hexapic
	${Boost_LIBRARIES}
	${OpenCV_LIBS}
)

TARGET_LINK_LIBRARIES(pca
	${Boost_LIBRARIES}
	${OpenCV_LIBS}
)
