#ifndef HOUND_HEAP_BASE_H
#define HOUND_HEAP_BASE_H

#include "port/Spinlock.hpp"

#include "global_metadata_heap.hpp"

class AOCommon;

class HeapBase : public BlockListImpl<AOCommon> {
public:
  SPINLOCK lock;

  void * operator new(size_t sz) {
    return GlobalMetadataHeap::getInstance()->New(sz,NULL,true);
  }

  void * operator new(size_t sz,void * ptr) {
    return ptr;
  }
  
  void operator delete(void * ptr) {
    GlobalMetadataHeap::getInstance()->Delete(ptr);
  }
};

#endif
