cmake_minimum_required(VERSION 2.6)
project(Sapphire)

include_directories("../../external/")
# todo: fix standalone build
if(UNIX)
  include_directories("/usr/include/mysql/")
  
else()
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../external/MySQL/")

  # ignore unchecked iterators warnings from msvc
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()

message(status "** CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

file(GLOB UTILS_PUBLIC_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*")
file(GLOB UTILS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_library(xivdat ${UTILS_PUBLIC_INCLUDE_FILES} ${UTILS_SOURCE_FILES})
