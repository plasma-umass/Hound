// -*- C++ -*-

#ifndef HOUND_COMBINEHEAP_H
#define HOUND_COMBINEHEAP_H

template <class SmallHeap, class BigHeap, size_t Cutoff>
class CombineHeap {
public:
  virtual ~CombineHeap(void) {
  }

  inline void *malloc(size_t sz) {
    void *ptr;
    // XXX
    if (sz > Cutoff) {
      ptr = _big.malloc(sz);
    } else {
      ptr = _small.malloc(sz);
    }
    return ptr;
  }

  inline bool free(void *ptr) {
    if (_small.free(ptr)) {
      return true;
    } else {
      return _big.free(ptr);
    }
  }

  inline size_t getSize(void *ptr) {
    size_t sz = _small.getSize(ptr);
    if (sz == 0) {
      sz = _big.getSize(ptr);
    }
    return sz;
  }

private:
  SmallHeap _small;
  BigHeap _big;
};

#endif
