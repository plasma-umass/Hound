#ifndef HOUND_LARGEHEAP_H
#define HOUND_LARGEHEAP_H

#include <assert.h>

namespace HL {};

#include "mmapwrapper.h"
#include "mmapalloc.h"
#include "myhashmap.h"
#include "bumpalloc.h"
#include "freelistheap.h"

using namespace HL;

class LargeHeap {
public:

  void * malloc (size_t sz) {
    void * ptr = MmapWrapper::map (sz);
    set (ptr, sz);
    return ptr;
  }

  bool free (void * ptr) {
    // If we allocated this object, free it.
    size_t sz = get(ptr);
    if (sz > 0) {
      //fprintf(stderr,"largeheap unmapping %p\n",ptr);
      MmapWrapper::unmap (ptr, sz);
      clear (ptr);
      return true;
    } else {
      return false;
    }
  }

  size_t getSize (void * ptr) {
    size_t s = get(ptr);
    return s;
  }

private:

  // The heap from which memory comes for the Map's purposes.
  // Objects come from chunks via mmap, and we manage these with a free list.
  class SourceHeap :
  public HL::FreelistHeap<BumpAlloc<65536, MmapAlloc> > { };

  /// The map type, with all the pieces in place.
  typedef MyHashMap<void *, size_t, SourceHeap> mapType;

  mapType _objectSize;

  inline size_t get (void * ptr) {
    return _objectSize.get (ptr);
  }
  
  inline void set (void * ptr, size_t sz) {
    // Initialize a range with the actual size.
    size_t currSize = sz;
    int iterations = (sz + PAGE_SIZE - 1) / PAGE_SIZE;
    for (int i = 0; i < iterations; i++) {
      _objectSize.set ((char *) ptr + i * PAGE_SIZE, currSize);
      currSize -= PAGE_SIZE;
    }
  }
  
  inline void clear (void * ptr) {
    size_t sz = get (ptr);
    int iterations = (sz + PAGE_SIZE - 1) / PAGE_SIZE;
    for (int i = 0; i < iterations; i++) {
      _objectSize.erase ((void *) ((char *) ptr + i * PAGE_SIZE));
    }
  }

  enum { PAGE_SIZE = 4096 };

};


#endif
