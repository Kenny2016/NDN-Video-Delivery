cmake_minimum_required(VERSION 2.8)

find_package(LibXml2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)

include_directories(include)

file(GLOB_RECURSE libdash_source *.cpp)

set(LIBLOG_LIBRARIES "../liblog/lib/liblog.so.1.0.1" CACHE PATH "Path to liblog.so")
#set(LIBLOG_INCLUDES "../liblog/include/" CACHE PATH "Path to liblog includes")
#include_directories(${LIBLOG_INCLUDES})
#link_directories(${LIBLOG_LIBRARIES})
MESSAGE("CMAKE LOL " ${CURL_LIBRARIES})
MESSAGE("CMAKE L2OL " ${LIBLOG_LIBRARIES})
MESSAGE("G++ FLAGS : " ${CMAKE_CXX_FLAGS})
MESSAGE("LINKER FLAGS : " ${CMAKE_BINARY_DIR})
add_library(dash SHARED ${libdash_source})
add_library(log SHARED IMPORTED)
set_property(TARGET log PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/../liblog/lib/liblog.so.1.0.1)
target_link_libraries(dash ${CURL_LIBRARIES} ${ZLIB_LIBRARIES} ${LIBXML2_LIBRARIES} log)
