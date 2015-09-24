#ifndef HOUND_GLOBAL_METADATA_HEAP_H
#define HOUND_GLOBAL_METADATA_HEAP_H

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
