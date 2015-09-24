#ifndef HOUND_PLUG_HEAP_H
#define HOUND_PLUG_HEAP_H

#pragma warning(disable:4251) // non-DLL linkage warning

#include "global_metadata_heap.hpp"

#include "AOHeap.hpp"
#include "metadata_map.hpp"
#include "callsite.hpp"
#include "output.hpp"
#include "pinstubs.hpp"

extern uint64_t global_allocs;

#define SILENT

// "Pin" the address in unprotected memory
// Prevents a race between unprotecting a page and calling into the kernel with a handle to that page
DECLSPEC void Pin(LPCVOID ptr);
DECLSPEC void Unpin(LPCVOID ptr);

#define PLUG_SYNCH Guard<SPINLOCK> __SYNCH_THING(_mutex);

template<class PerCallsiteHeap>
class PlugHeap {
public:
  PlugHeap() : AppHeap(4<<20,true,false,false), _sitemap(), _locked(0), _tid(0) { }
  
  ~PlugHeap();
  
  PVOID New(size_t sz, int * Space = 0, BOOL Zero = false) {
    PLUG_SYNCH;
  
    global_allocs++;
  
    PVOID functions[STACK_DEPTH];
  
    for(int i = 0; i < STACK_DEPTH; i++) {
      functions[i] = 0;
    }
  
    HASH_TYPE hash;
  
    CaptureStackBackTrace(2, STACK_DEPTH, functions, &hash);

    //fprintf(stderr,"hash is %x\n",hash);
  
    Site<PerCallsiteHeap> * site = _sitemap.getSite(hash,functions);
  
    PVOID ret = NULL;
  
    // if we're sampling for this site, then do the custom alloc
    if(site->getAllocator()) {
      ret = site->getAllocator()->New(sz,Space,Zero);
    } else {
      site->doAlloc(sz);
    
#if (ADD_HEADER == 1)
      HEADER * hptr = (HEADER*)AppHeap.New(sz + sizeof(HEADER),Space,Zero);
      //fprintf(stderr,"allocated 0x%x, sz %d (+ %d)\n",hptr,sz,sizeof(HEADER));
    
      if(hptr) {
	// adjust reported space
	if(Space) 
	  *Space -= sizeof(HEADER);
      
	hptr->_site = hash;
      
	ret = &hptr[1];
      }
#else
      ret = AppHeap.New(sz,Space,Zero);
#endif
    }

    //fprintf(stderr,"allocated %p (%d)\n",ret,sz);

    plug_object_allocated(ret);
    return ret;
  }

  BOOL Delete(PVOID ptr) {
    PLUG_SYNCH;

    HEADER * hptr = (HEADER*)ptr;
#if (ADD_HEADER == 1)
    hptr--;
#endif

    if(ptr == NULL) return false;

#if defined(PROFILE_FREES) || defined(ELIDE_FREE)
    HASH_TYPE hash;
    PVOID functions[STACK_DEPTH];

    if(_profileFrees || _elideHash) {
	  
      for(int i = 0; i < STACK_DEPTH; i++) {
	functions[i] = 0;
      }
	  
      CaptureStackBackTrace(1, STACK_DEPTH, functions, &hash);
    }
#endif

#ifdef PROFILE_FREES
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
      int sz;
      Details(ptr,&sz);
	  
      _fprof.free(ptr,sz,hash,functions);
    }
#endif

#ifdef ELIDE_FREE
    if(_elideHash && hash == _elideHash) {
      //fprintf(stderr,"eliding free of %p\n",ptr);
      return false;
    }
#endif

#if defined(_WIN32)
    /* There are a variety of paths here */
    /* 1. The AppHeap (most objects) */
    /* 2. An AOHeap (uncommon case, so not checked first) */
    /* 3. ??? CRT? (bail out) */

    if( AppHeap.KnownArea(hptr) ) {
      HASH_TYPE site = hptr->_site;
      _sitemap.doFree(site);
      return AppHeap.Delete(hptr);
    } else {
      AOCommon * bl = AOCommon::fromPtr(ptr);

      if(bl)
	return bl->Delete(ptr);
      else {
	printf("couldn't delete ptr: 0x%x\n",ptr);
	// The great unknown
	return false;
      }
    }	
#else
    AOCommon * bl = AOCommon::fromPtr(ptr);
  
    if(bl) {
      plug_object_freed(ptr);
      return bl->Delete(ptr);
    }
    else {    
#if (ADD_HEADER == 1)
      HASH_TYPE site = hptr->_site;
      _sitemap.doFree(site);
#endif
      //fprintf(stderr,"deleting 0x%x (passed 0x%x)\n",hptr,ptr);
      if(!AppHeap.Delete(hptr)) {
	// the great unknown: we didn't allocate it, so it can't have a header.
	fprintf(stderr,"trying to free pointer through libc...\n");
	if(!_localbuf) LocalHeap.malloc(1);
	LocalHeap.free(ptr);
      } else {
	plug_object_freed(ptr);
      }
    }
#endif

  }

  // This is handled by wrapper.cpp
  // NB XXX NoDelete is not honored.
  PVOID Resize(PVOID ptr, size_t sz, int Move, int *Space, bool NoDelete, bool Zero) {
    PLUG_SYNCH;

    assert(false);

    if(ptr == NULL) {
      return New(sz,Space,Zero);
    } else if(sz == 0) {
      Delete(ptr);
      return NULL;
    }

    int o_sz;

    // don't realloc ptrs we don't own
    if(!Details(ptr,&o_sz)) return NULL;

    // if we're already big enough...
    if((size_t)o_sz >= sz) return ptr;

    // This reenters New above, and MAY or MAY NOT give the object a header
    // But it is transparent to us.
    PVOID n_ptr = New(sz,Space,Zero);

    memcpy(n_ptr,ptr,o_sz);

    Delete(ptr);

    return n_ptr;
  }

  BOOL KnownArea(PVOID ptr) {
    PLUG_SYNCH;

    assert(false);

    BOOL ret;
    ret = AppHeap.KnownArea(ptr);

    if(!ret) {
      AOCommon * bl = AOCommon::fromPtr(ptr);
      if(bl) {
	return true;
      }
    }
    return ret;
  }

  BOOL Details(PVOID ptr, int *Space) {
    PLUG_SYNCH;

    HEADER * hptr = (HEADER *)ptr;

#if (ADD_HEADER == 1) 
    hptr--;
#endif

#if defined(_WIN32)
    if(AppHeap.Details(hptr,Space)) {
#if (ADD_HEADER == 1)
      if(Space)
	*Space -= sizeof(HEADER);
#endif
      return true;
    } else {
      AOCommon * bl = AOCommon::fromPtr(ptr);
      if(bl) {
	return bl->Details(ptr,Space);
      }
    }
#else // linux
    AOCommon * bl = AOCommon::fromPtr(ptr);
    if(bl) {
      return bl->Details(ptr,Space);
    }
    else {
      if(Space) {
	*Space = AppHeap.getSize(hptr);

	if(*Space == 0) return false;

#if (ADD_HEADER == 1)
	*Space -= sizeof(HEADER);
#endif
      }
      return true;
    }
      
#endif

    return false;
  }

  void triage() {
    char buf[1024];

    OpenStatFile();

    //OutputStats("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
    OutputStats("<heap>\n");

    {
      char exe[256];
      int count=readlink("/proc/self/exe", exe, 255);
      if (count <= 0)
	{
	  printf( "Failed to get current executable file name\n" );
	  exit(1);
	}
      exe[count] = '\0';
      snprintf(buf,1024,"<exe>%s</exe>\n",exe);
      OutputStats(buf);
    }
	

#ifdef _WIN32
    {
      char fname[80];
	  
      GetModuleFileName(NULL, fname, 80);
	  
      sprintf_s(buf,1024,"<process>%s</process>\n",fname);
      OutputStats(buf);
    }
#endif

    _sitemap.triage();
    OutputStats("</heap>\n");

    CloseStatFile();
  }

  void reportStats() {
    //  fprintf(stderr,"got stats...");
    _sitemap.reportStats();
#ifdef PROFILE_FREES
    _fprof.dumpStats();
#endif
  }
  
  struct HEADER {
    HASH_TYPE _site;
  };
  
private:
  BaseHeapType AppHeap;
  HL::LocalMallocHeap LocalHeap;
  void * _localbuf;
  // XXX: AOHeap doesn't support large objects, so AppHeap can't be one.
  //AOHeap AppHeap;
  
  SPINLOCK _mutex;
  int _locked;
  DWORD _tid;
  
  MetadataMap<PerCallsiteHeap> _sitemap;
  
  typedef ULONG HASH_TYPE;
  
#ifdef PROFILE_FREES
  FreeProfiler _fprof;
  bool _profileFrees;
  bool _initProf;
#endif
  
#ifdef ELIDE_FREE
  uint32_t _elideHash;
#endif
  
};

#endif //__PLUG_HEAP_H__
