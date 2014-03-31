project(fileex)

enable_language(C)
enable_language(CXX)

cmake_minimum_required(VERSION 2.6)

set (SOURCES unrar/unpack.cpp
             unrar/suballoc.cpp
             unrar/rarvmtbl.cpp
             unrar/unpack20.cpp
             unrar/rarvm.cpp
             unrar/model.cpp
             unrar/unrar_open.cpp
             unrar/archive.cpp
             unrar/arcread.cpp
             unrar/getbits.cpp
             unrar/coder.cpp
             unrar/encname.cpp
             unrar/crc.cpp
             unrar/unpack15.cpp
             unrar/rawread.cpp
             unrar/unrar.cpp
             unrar/extract.cpp
             unrar/unrar_misc.cpp
             unrar/unicode.cpp
             fex/Gzip_Reader.cpp
             fex/fex.cpp
             fex/Gzip_Extractor.cpp
             fex/blargg_common.cpp
             fex/File_Extractor.cpp
             fex/blargg_errors.cpp
             fex/Data_Reader.cpp
             fex/Zlib_Inflater.cpp
             fex/Zip7_Extractor.cpp
             fex/Rar_Extractor.cpp
             fex/Zip_Extractor.cpp
             fex/Binary_Extractor.cpp
             7z_C/7zStream.c
             7z_C/LzFindMt.c
             7z_C/7zHeader.c
             7z_C/CpuArch.c
             7z_C/Lzma86Enc.c
             7z_C/Bra.c
             7z_C/Bra86.c
             7z_C/Lzma86Dec.c
             7z_C/Ppmd7Enc.c
             7z_C/Lzma2Enc.c
             7z_C/Ppmd7.c
             7z_C/7zCrcOpt.c
             7z_C/LzmaLib.c
             7z_C/Lzma2Dec.c
             7z_C/posix/Threads.c
             7z_C/7zFile.c
             7z_C/7zBuf2.c
             7z_C/LzmaEnc.c
             7z_C/7zIn.c
             7z_C/7zDec.c
             7z_C/MtCoder.c
             7z_C/7zItem.c
             7z_C/7zExtract.c
             7z_C/7zAlloc.c
             7z_C/Bcj2.c
             7z_C/LzFind.c
             7z_C/LzmaDec.c
             7z_C/Ppmd7Dec.c
             7z_C/7zBuf.c
             7z_C/Alloc.c
             7z_C/7zCrc.c
             7z_C/7zDecode.c
             7z_C/BraIA64.c
             7z_C/Delta.c)

include_directories(${PROJECT_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

add_library(fileex STATIC ${SOURCES})
