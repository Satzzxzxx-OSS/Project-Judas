#pragma once
#include <DetourTileCacheBuilder.h>
#include <cstring>
// Identity compression keeps the pinned cache format self-contained.
struct NavigationCompressor final:dtTileCacheCompressor {
 int maxCompressedSize(int n)override{return n;}
 dtStatus compress(const unsigned char* a,int n,unsigned char* b,int max,int* size)override{if(n<0||n>max)return DT_FAILURE;std::memcpy(b,a,n);*size=n;return DT_SUCCESS;}
 dtStatus decompress(const unsigned char* a,int n,unsigned char* b,int max,int* size)override{return compress(a,n,b,max,size);}
};
