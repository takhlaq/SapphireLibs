cmake_minimum_required(VERSION 3.0.2)
project(Sapphire)

include_directories( "../../external/" )

if(UNIX)
  include_directories( "/usr/include/mysql/" )
  
else()
  include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/../../external/MySQL/" )
endif()

file( GLOB UTILS_PUBLIC_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*" )
file( GLOB UTILS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*" )

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )

add_library( xivdat ${UTILS_PUBLIC_INCLUDE_FILES} ${UTILS_SOURCE_FILES} )
target_include_directories( xivdat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
cotire( xivdat )
