project(fileex)

enable_language(C)
enable_language(CXX)

cmake_minimum_required(VERSION 2.6)

set (SOURCES fex/fex.cpp
             fex/blargg_common.cpp
             fex/File_Extractor.cpp
             fex/blargg_errors.cpp
             fex/Data_Reader.cpp
             fex/Binary_Extractor.cpp)

include_directories(${PROJECT_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

add_library(fileex STATIC ${SOURCES})
