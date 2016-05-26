cmake_minimum_required(VERSION 2.8)

include_directories(libxml2/include)
include_directories(libdash/include)
include_directories(sdl/include)
include_directories(zlib/include)
include_directories(iconv/include)
include_directories(liblog/include)

#set(LIBLOG_LIBRARIES "../liblog/lib/" CACHE PATH "Path to liblog.so")
#set(LIBLOG_INCLUDES "../liblog/include/" CACHE PATH "Path to liblog includes")
#include_directories(${LIBLOG_INCLUDES})
#link_directories(${LIBLOG_LIBRARIES})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "-std=c++0x -g -DLOG_BUILD")
add_subdirectory(libdash)
add_subdirectory(libdash_networkpart_test)
#target_link_libraries(libdash liblog)
