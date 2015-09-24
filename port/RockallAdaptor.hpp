#ifndef HOUND_ROCKALL_ADAPTOR_H__
#define HOUND_ROCKALL_ADAPTOR_H__

#include <string.h>
#include <assert.h>

#include <stdio.h>

template <class BaseHeap>
class RockallAdaptor : public BaseHeap {
public:
  inline RockallAdaptor() {}
  inline RockallAdaptor(size_t sz, bool,bool,bool) {}

  inline PVOID New(size_t sz, int* Space = NULL, BOOL Zero = false) {
    PVOID ret = BaseHeap::malloc(sz);
    if(Space)
      *Space = sz;

    if(Zero)
      memset(ret,0,sz);

    //fprintf(stderr,"dlmalloc %p\n",ret);

    return ret;
  }

  inline BOOL Delete(PVOID p) {
    //fprintf(stderr,"dlmalloc freeing %p\n",p);
    //XXX 
    //return BaseHeap::free(p);
    BaseHeap::free(p);
    return true;
  }

  inline PVOID Resize(PVOID p, size_t sz, int Move = 1, int *Space = NULL, 
                      bool NoDelete = false, bool Zero = false) {
    assert(Move);

    if(p == NULL) {
      return New(sz,Space,Zero);
    }

    if(sz == 0) {
      Delete(p);
      return NULL;
    }

    size_t objSize = BaseHeap::getSize(p);
    if(objSize == sz) {
      return p;
    }

    PVOID buf = New(sz);

    size_t minSize = (objSize < sz) ? objSize : sz;
    if(buf) {
      memcpy(buf,p,minSize);
    }

    if(!NoDelete) {
      BaseHeap::free(p);
    }

    return buf;
  }

  // XXX: Assume we own this pointer
  inline BOOL Details(PVOID p, int *Space) {
    if(Space) {
      *Space = BaseHeap::getSize(p);
    }
    return true;
  }

  // A generic heap layer doesn't know whether or not it owns a given pointer.
  inline BOOL KnownArea(PVOID p) {
    assert(false);
    return false;
  }
                      
};

#endif
