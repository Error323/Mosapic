project (hexapic)
cmake_minimum_required (VERSION 2.8)


#-------------------------------------------------------------------------------
# General Settings
#-------------------------------------------------------------------------------
set (CMAKE_COLOR_MAKEFILE ON)
set (CMAKE_CXX_FLAGS "-Wall -Wextra ")
set (CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG")
set (CMAKE_CXX_FLAGS_RELEASE "-O3")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DDEBUG")

set (ENABLE_PROFILING FALSE CACHE BOOL "Enable/Disable profiling")
set (ENABLE_CUDA TRUE CACHE BOOL "Enable/Disable cuda")

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "Release")
endif (NOT CMAKE_BUILD_TYPE)
if (ENABLE_PROFILING)
  add_definitions (-DENABLE_PROFILING)
endif (ENABLE_PROFILING)


#-------------------------------------------------------------------------------
# Extract and set the version
#-------------------------------------------------------------------------------
execute_process (
	COMMAND git describe --tags
	OUTPUT_VARIABLE GIT_VERSION
	ERROR_VARIABLE GIT_ERROR
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (GIT_VERSION)
	set (VERSION ${GIT_VERSION})
else (NOT GIT_VERSION)
	set (VERSION "0.0.0")
endif (GIT_VERSION)

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
find_package (OpenCV COMPONENTS opencv_core REQUIRED)

if (ENABLE_CUDA)
  find_package (OpenCV COMPONENTS opencv_gpu REQUIRED)
  add_definitions (-DENABLE_CUDA_DEVICE)
endif (ENABLE_CUDA)

include_directories (
	${Boost_INCLUDE_DIRS}
	${OpenCV_INCLUDE_DIR}
)


#-------------------------------------------------------------------------------
# Define executable and link libraries
#-------------------------------------------------------------------------------
add_executable (${CMAKE_PROJECT_NAME} ${hexapic_SOURCE})

target_link_libraries (${CMAKE_PROJECT_NAME}
	${Boost_LIBRARIES}
	${OpenCV_LIBS}
)

#-------------------------------------------------------------------------------
# Status report
#-------------------------------------------------------------------------------
message (STATUS "")
message (STATUS " C/C++:")
message (STATUS "   C++ Compiler:"               ${CMAKE_CXX_COMPILER})
message (STATUS "   C++ flags (Release):"        ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE})
message (STATUS "   C++ flags (Debug):"          ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG})
message (STATUS "   C++ flags (RelWithDebInfo):" ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
message (STATUS "   C Compiler:"                 ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1})
message (STATUS "   C flags (Release):"          ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE})
message (STATUS "   C flags (Debug):"            ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG})
message (STATUS "   C flags (RelWithDebInfo):"   ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELWITHDEBINFO})
message (STATUS "")
message (STATUS " CMakeflags:")
message (STATUS "   CMAKE_BUILD_TYPE "           ${CMAKE_BUILD_TYPE})
message (STATUS "   ENABLE_CUDA      "           ${ENABLE_CUDA})
message (STATUS "   ENABLE_PROFILING "           ${ENABLE_PROFILING})
message (STATUS "")
