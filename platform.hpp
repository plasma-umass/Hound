#ifndef HOUND_PLATFORM_H
#define HOUND_PLATFORM_H

#undef ELIDE_FREE // 0x698baf14

// from heap layers
#if defined(_MSC_VER)

// Microsoft Visual Studio
#pragma inline_depth(255)
#define INLINE __forceinline
//#define inline __forceinline
#define NO_INLINE __declspec(noinline)
#pragma warning(disable: 4530)
#define MALLOC_FUNCTION
#define RESTRICT

#elif defined(__GNUC__)

// GNU C

#define NO_INLINE       __attribute__ ((noinline))
#define INLINE          inline
#define MALLOC_FUNCTION __attribute__((malloc))
#define RESTRICT        __restrict__

#else

// All others

#define NO_INLINE
#define INLINE inline
#define MALLOC_FUNCTION
#define RESTRICT

#endif


// Create win32 type alises for GNU C
#ifndef _WIN32
typedef unsigned long ULONG;
typedef void VOID;
typedef VOID * PVOID;
typedef long LONG;
typedef bool BOOL;
typedef ULONG DWORD;
typedef const PVOID LPCVOID;
typedef unsigned int UINT;
typedef char * PCHAR;
typedef unsigned short USHORT;
typedef ULONG * PULONG;
#endif

#if defined(_WIN32)
// XXX: Fixme (4530 only)
#pragma warning(disable:4530)
#pragma warning(disable:4996)

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
#include <windows.h>

// use Rockall as base allocator
#ifndef COMPILING_ROCKALL_LIBRARY
#define COMPILING_ROCKALL_LIBRARY 1
#endif
#include "RockallFrontEnd.hpp"
typedef FAST_HEAP BaseHeapType;

#ifdef COMPILING_PLUGLEAKS_DLL
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif

static USHORT
( 
 NTAPI
 * RtlCaptureStackBackTrace)(
                             ULONG FramesToSkip,
                             ULONG FramesToCapture,
                             PVOID *BackTrace,
                             PULONG BackTraceHash
                             );

#else // __GNU_C__
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include "localmallocheap.h"
#include "heaps/utility/localmallocheap.h"
//#include "heaps/general/leamallocheap.h"
#include "RockallAdaptor.hpp"
//#include "port/phkmallocheap.h"
// typedef RockallAdaptor<HL::LeaMallocHeap> BaseHeapType;
typedef RockallAdaptor<HL::LocalMallocHeap> BaseHeapType;
//typedef RockallAdaptor<HL::PhkMallocHeap> BaseHeapType;

#include "largeheap.h"
#include "combineheap.h"

//typedef RockallAdaptor<CombineHeap<HL::PhkMallocHeap,LargeHeap, 1024*64> > BaseHeapType;

#define DECLSPEC
#define UNREFERENCED_PARAMETER(foo)

#include <stdio.h>
inline void OutputDebugString(const char * str) {
  fputs(str,stderr);
}

#define sprintf_s snprintf
#define _ASSERTE(foo) assert(foo)

#endif

#ifdef PROFILE_FREES
#include "free_profiler.hpp"
#endif

#endif // __PLATFORM_H__
