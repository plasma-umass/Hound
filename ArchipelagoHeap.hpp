#ifndef HOUND_ARCHIPELAGO_HEAP_H
#define HOUND_ARCHIPELAGO_HEAP_H

#include "constants.h"

#include "util/guard.h"

#include "AOCommon.hpp"
#include "BlockList.hpp"

// Non-size-segregated block list foo.

template<class BlockType, class LargeObjectType>
class ArchipelagoHeap : public HeapBase {
public:
  void * malloc(size_t sz) {
    SYNCHRONIZED(this);

    void * ret;

    // round up to nearest multiple of the allocation grain
    sz = ((sz + MIN_ALLOC - 1) & ~(MIN_ALLOC-1));
	
    if(sz == 0) sz = MIN_ALLOC;

    //_alloc_ct++;

    if(sz >= PAGE_SIZE) {
      ret = allocLargeObject(sz);	
    } else {
      BlockType * block = allocNewBlock();
      
      ret = block->New(sz);
    }

    assert(ret != 0);
    //fprintf(stderr,"arch heap allocd %p\n",ret);
      
    return ret;
  }

  
  bool free(void * ptr) {
     AOCommon * bl = AOCommon::fromPtr(ptr);
  
     if(bl) {
       return bl->Delete(ptr);
     }

     assert(false);
  }

  size_t getSize(void * ptr) {
    AOCommon * bl = AOCommon::fromPtr(ptr);
  
    if(bl) {
      int sz;
      bl->Details(ptr,&sz);
      return sz;
    }
  }

  BlockType * allocNewBlock() {
    BlockType * ret = new BlockType();
    ret->_heap = this;
    BlockListImpl<AOCommon>::registerBlock(&ret->_heapListNode);
    return ret;
  }

  void * allocLargeObject(size_t sz) {
    LargeObjectType * block = new LargeObjectType();
    block->_heap = this;
    BlockListImpl<AOCommon>::registerBlock(&block->_heapListNode);
    return block->New(sz,0,false);
  }
  
  SPINLOCK lock;
};

#endif
