#ifndef HOUND_PROFINJ_H
#define HOUND_PROFINJ_H

typedef unsigned long ULONG;
typedef void VOID;
typedef void* PVOID;

#include "oneheap.h"
#include "lockedheap.h"
#include "spinlock.h"
#include "reentrantheap.h"

typedef HL::OneHeap<HL::LocalMallocHeap> BaseHeapType;

#include "port/capture_callsite.h"
#include "callsite.hpp"
#include "free_profiler.hpp"

template<class Super>
class ProfInj : public Super {
public:
  void free(void * ptr) {
    
    unsigned long hash;
    PVOID functions[STACK_DEPTH];
    
    if(_profileFrees || _elideHash) {
      for(int i = 0; i < STACK_DEPTH; i++) {
	functions[i] = 0;
      }
      
      CaptureStackBackTrace(1, STACK_DEPTH, functions, &hash);
    }

    if(!_initProf) {
      _initProf = true;
      if(getenv("PROFILE_FREES")) {
	fprintf(stderr,"Free profiling enabled\n");
	_profileFrees = true;
      }
      if(getenv("ELIDE_FREE")) {
	_elideHash = strtoul(getenv("ELIDE_FREE"),NULL,16);
	fprintf(stderr,"eliding hash %x\n",_elideHash);
      }
    }
    
    if(_profileFrees) {
      int sz = Super::getSize(ptr);
      
      _fprof.free(ptr,sz,hash,functions);
    }
    
    if(_elideHash && hash == _elideHash) {
      //fprintf(stderr,"eliding free of %p\n",ptr);
      //fprintf(stdout,".");
      return;
    }

    //fprintf(stderr,"freed %p\n",ptr);

    Super::free(ptr);
  }

  void reportStats() {
    _fprof.dumpStats();
  }
  
  bool _initProf;
  unsigned long _elideHash;
  bool _profileFrees;

  FreeProfiler _fprof;
};

#endif
