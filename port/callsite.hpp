#ifndef HOUND_CALLSITE_H__
#define HOUND_CALLSITE_H__

#if defined(_WIN32)

USHORT
( 
 NTAPI
 * PlugHeap::RtlCaptureStackBackTrace)(
                                       ULONG FramesToSkip,
                                       ULONG FramesToCapture,
                                       PVOID *BackTrace,
                                       PULONG BackTraceHash
                                       );

void InitCaptureStackBackTrace();

inline unsigned short CaptureStackBackTrace(int skip,int STACK_DEPTH,void ** BackTrace,unsigned long *BackTraceHash) {
  return RtlCaptureStackBackTrace(skip,STACK_DEPTH,BackTrace,BackTraceHash);
}

#else

#include "capture_callsite.h"
#include "CallStack.hpp"

void InitCaptureStackBackTrace();

inline unsigned short CaptureStackBackTrace(int skip,int STACK_DEPTH,void ** BackTrace,unsigned long *BackTraceHash) {
  return get_callsite(skip,STACK_DEPTH,BackTrace,BackTraceHash);
}

#endif

inline ULONG StackHash(VOID *Frames[], int MaxFrames) {
	ULONG ret = 5381;

	for(int i = 0; i < MaxFrames; i++) {
		ret = ((ret << 5) + ret) + (unsigned long)Frames[i];
	}

	return ret;
}

#endif
