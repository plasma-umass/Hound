#ifndef __AOBLOCK_H__
#define __AOBLOCK_H__

#include "platform.hpp"
#include "PageBlock.hpp"
#include "constants.h"

#include <bitset>

#include "VCFragManager.hpp"
#include "PageReuseFragManager.hpp"
#include "FragManagerFilter.hpp"
#include "AOHeap.hpp"
#include "AOMergeable.hpp"
#include "AOMergedBlock.hpp"

#include <unistd.h>
#include <sys/mman.h>

#include "config.h"

#ifndef MREMAP_FIXED
#define MREMAP_FIXED 2
#endif

extern int total_pages;
extern uint64_t global_allocs;
//extern unsigned int blockCount;

extern int saved_pages;

// This is the header for a block.
template <size_t N>
class AOBlock : public PageBlock, public AOMergeable<N> {
  friend class AOMergedBlock<N>;

  using AOMergeable<N>::_fragListNode;

protected:
  static const unsigned int NUM_SLOTS = N;
  static const unsigned int OBJECT_SIZE = AO_BLOCK_SIZE/N;
  static const size_t LOG_OBJECT_SIZE = StaticLog<OBJECT_SIZE>::VALUE;

public:
  AOBlock() 
    : PageBlock(), _aliased(false) {
  }

  BOOL Details(PVOID ptr, int * Space) {
    *Space = OBJECT_SIZE;
    return true;
  }

  virtual unsigned int getSizeClass() const { return LOG_OBJECT_SIZE - LOG_MIN_ALLOC; }
  
  virtual size_t getAllocs() const { return AOMergeable<N>::_pop; }

  virtual void mergeInto(AOMergedBlock<N> * merged) {
    assert(_magic == MAGIC_NUMBER);
    
    AOBlock<N> * target = merged->getTarget();

    _aliased = true;
    
    if(target) {
      //if(target->getPopulation() == 0) fprintf(stderr,"POPULATION OF TARGET IS 0\n");

      //fprintf(stderr,"block %p aliased to %p\n",_start,target->_start);

      for(unsigned int i = 0; i < N; i++) {
        if(BitmapBase<N>::_bitmap.test(i)) {
          memcpy(target->_start+i*OBJECT_SIZE,_start+i*OBJECT_SIZE,OBJECT_SIZE);
        }
      }

      // NB: You try to do this in a better way.
      // *vomit*
      typedef void * (mremap5_fun)(void *,size_t,size_t,int,void*);
      mremap5_fun * my_mremap = (mremap5_fun*)(mremap);

      // This removes the current mapping at _start and makes the VA _start point to the PA referenced by target->_start
      // NB: THIS DOES NOT CHANGE target->_start's PAGE MAPPING!
      // YES THIS IS CONFUSING
      void * result = my_mremap(target->_start, 0, 4096, MREMAP_MAYMOVE | MREMAP_FIXED, _start);
      if(result == (void*)-1 || result != _start) {
	perror("mremap");
	abort();
      }

      // We inherit protection modes of aliased page... so if it's protected,
      // we need to unprotect.
      if(!isProtected()
	 && target->isProtected()) {
	unprotect();
	//fprintf(stderr,"protected target page bug trigger\n");
      }
      
      //fprintf(stderr,"pointed %p at %p\n",_start,target->_start);
    }
  }

  virtual AOMergedBlock<N> * merge(AOMergeable<N> * rhs) {
    return rhs->mergeWith(this);
  }

  virtual AOMergedBlock<N> * mergeWith(AOBlock<N> * rhs) {
    return new AOMergedBlock<N>(rhs,this);
  }

  virtual void reportStats() {
    fprintf(stderr,"%d/%d %p\n",BitmapBase<N>::_pop,NUM_SLOTS,_start);
  }

protected:
  virtual ~AOBlock() {
    if(AOCommon::isProtected())
      unprotect();

    // We need to get restore a fresh page if we're currently aliased
    if(_aliased) {
      //fprintf(stderr,"remapping aliased page %x\n",_start);
      munmap(_start,AO_BLOCK_SIZE);
      // someone could map this out from under us here if we race.  but in that case the memory has still been recycled, so we're ok
      if((void*)MAP_FAILED == mmap(_start,AO_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) {
	fprintf(stderr,"MUNMAP RACE!\n");
	assert(false);
	return;
      }
    }

    BlockSourceHeap().free(_start);

    //fprintf(stderr,"removing block %p\n",_start);
  }

  inline UINT indexOf(PVOID ptr) const {
    return ((char*)ptr-(char*)_start)>>LOG_OBJECT_SIZE;
  }

  // have we been aliased?
  BOOL _aliased;

private:
  ULONG findObject(PVOID ptr) const {
    if(ptr < _start || ptr >= _end) return 0;

    return (indexOf(ptr)<<LOG_OBJECT_SIZE) + (ULONG)_start;
  }

};


#endif
