/* a Heap Layers wrapper for a Rockall-interface heap */

#ifndef __ROCKLAYER_H__
#define __ROCKLAYER_H__

#include <cstdio>

template <class ROCK_HEAP>
class RockLayer : public ROCK_HEAP {
public:
  inline void * malloc(size_t sz) {
    return ROCK_HEAP::New(sz);
  }

  inline void * calloc(size_t ct, size_t sz) {
    return ROCK_HEAP::New(ct*sz,NULL,true);
  }

  inline bool free(void * ptr) {
    return ROCK_HEAP::Delete(ptr);
  }

  inline size_t getSize(void * ptr) {
    int sz;
    if(ptr && ROCK_HEAP::Details(ptr,&sz)) {
      return sz;
    } else {
      return 0;
    }
  }
};

#endif
