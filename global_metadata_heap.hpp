#ifndef __GLOBAL_METADATA_HEAP_H__
#define __GLOBAL_METADATA_HEAP_H__

#include "platform.hpp"

// class SuperHyperGlobalMetaHeap
class GlobalMetadataHeap {
public:
  static BaseHeapType * getInstance() {
    static char buf[sizeof(BaseHeapType)];
    static BaseHeapType * heap = new (buf) BaseHeapType(4<<20,true,false,false);
    return heap;
  }
};

#endif
