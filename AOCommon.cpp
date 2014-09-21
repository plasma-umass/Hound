#include "AOCommon.hpp"
#include "HeapBase.hpp"
#include "global_metadata_heap.hpp"
#include "mprotectwrapper.hpp"
#include "mmapwrapper_plug.h"

#include "pinstubs.hpp"

#include "util/guard.h"

#include <assert.h>
#include <stdio.h>

#if !defined(_WIN32)
#include "mmapwrapper.h"
#include "pagetable.hpp"
#endif

void * AOCommon::operator new(size_t sz) {
  return GlobalMetadataHeap::getInstance()->New(sz);
}

void AOCommon::operator delete(void * ptr) {
  GlobalMetadataHeap::getInstance()->Delete(ptr);
}

VOID AOCommon::pin() { 
	SYNCHRONIZED(_heap);

	if(!_pinned) {
	  assert(_protected);
	  unprotect();
	}

	_pinned++; 

	//fprintf(stderr,"pinned 0x%x (from %p), now %d\n",_start,__builtin_return_address(2),_pinned);

	if(false) {
		char buf[256];
		sprintf(buf,"pinned 0x%x\n",_start);
		OutputDebugString(buf);
	}
}

VOID AOCommon::unpin() {
  SYNCHRONIZED(_heap);
  _pinned--;

  //fprintf(stderr,"unpinned 0x%x, now %d (site %x)\n",_start,_pinned,_heap->getCallSite());

  if(_pinned == 0) {
    protect();
  }

  if(false) {
    char buf[256];
    sprintf(buf,"unpin 0x%x\n",_start);
    OutputDebugString(buf);
  }
}

// @returns: true if we handled the fault
BOOL AOCommon::faultHandler(DWORD VA) {
  PVOID start = (PVOID)(VA & ~(AO_BLOCK_SIZE-1));

  AOCommon * bl = AOCommon::fromPtr(start);
  // barrier() unprotects the page.
  if(bl) {
    plug_mmap(VA);
    assert(bl->isProtected());
    bl->barrier(VA);
    return true;
  }
  else
    return false;
}

VOID AOCommon::protect() {
  SYNCHRONIZED(_heap);
  
  if(_protected)
    return;
  
  _ASSERTE(!_pinned);
  
  protect_range(_start,_end-_start);
  _protected = true;
  _protTime = global_allocs;
  //fprintf(stderr,"* protected block 0x%x\n",_start);
  
  // Callback into PIN
  plug_mmap((ULONG)_start);
}

VOID AOCommon::unprotect() {
  // RecycledBlockFactory can merge virgin virtual pages to protected
  // pages, so it has to be able to unprotect blocks that are not yet
  // on a heap.  In this case, we're guaranteed to be thread-local so
  // we don't need to lock anything anyway.
  if(!_heap) {
    BOOL st = unprotect_range((PVOID)_start,_end-_start);
    _protected = false;
    return;
  } else { 
    SYNCHRONIZED(_heap);
    
    BOOL st = unprotect_range((PVOID)_start,_end-_start);
    //_ASSERTE(st);

    if(!st) {
      perror("******************** Failed to unprotect range");
      abort();
    }
    
    _protected = false;
    //printf("* unprotected block 0x%x\n",_start);
    plug_munmap((ULONG)_start);
  }
}

// this is called when the block has been filled and subsequently cleared
VOID AOCommon::doCleared() {
  //printf("cleared a block, now %d/%d\n",_live_blocks,_allocated_blocks);
  assert(_heapListNode.list() == _heap);
  _heap->removeBlock(&_heapListNode);
}

AOCommon::~AOCommon() {
  SYNCHRONIZED(_heap);
  
  assert(_magic == MAGIC_NUMBER);
  _magic = 0xcafebabe;
  _pinned = -42;
  
  BOOL st;
  
  //fprintf(stderr,"~AOCommon(): 0x%x\n",_start);
  
  //HL::MmapWrapper::unmap(_start,getSize());
  PageTable::getInstance()->clear(_start,getSize());  
}
