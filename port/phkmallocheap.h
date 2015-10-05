#ifndef HOUND_PHKMALLOCHEAP_H
#define HOUND_PHKMALLOCHEAP_H

extern "C" {
	void * phkmalloc(size_t);
	void phkfree(void *);
  char phk_tryfree(void *);
	//void * phkrealloc(void *, size_t);
	size_t phkmalloc_usable_size(void *);
  void * phkmalloc_normalize(void *);
}

namespace HL {

class PhkMallocHeap {
public:
	inline void * malloc(size_t sz) {
	  void * ret = phkmalloc(sz);
	  assert(ret);
	  return ret;
	}

	inline bool free(void * ptr) {
	  if(!phk_tryfree(ptr)) {
	    //fprintf(stderr,"WARNING: phkmalloc can't free unknown pointer: %p\n",ptr);
	    return false;
	  }
	  return true;
	}
/*
	inline void * realloc(void * ptr, size_t sz) {
		return phkrealloc(ptr, sz);
	}
*/
	inline size_t getSize(void * ptr) {
    size_t ret = phkmalloc_usable_size(ptr);
		return ret;
	}
};	//end of class PhkMallocHeap

};	//end of namespace HL

#endif
