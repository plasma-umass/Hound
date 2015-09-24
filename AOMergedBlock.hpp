
#ifndef HOUND_AO_MERGED_H
#define HOUND_AO_MERGED_H

#include "constants.h"
#include "AOMergeable.hpp"
#include "global_metadata_heap.hpp"
#include "FragManager.hpp"

#include "config.h"

extern int saved_pages;
extern int total_pages;
extern int _mergedblocks;

template <unsigned int N>
class AOMergedBlock : public AOMergeable<N>, public FragManager<N> {
  typedef BlockListImpl<AOMergeable<N> > MyList;
  typedef typename MyList::Node Node;

  using AOMergeable<N>::_fragListNode;

public:
  static const size_t OBJECT_SIZE = AO_BLOCK_SIZE/N;

  AOMergedBlock(AOBlock<N> * b1, AOBlock<N> * b2) 
    : _rc(0), closed(b1->isClosed() && b2->isClosed())
  { 
#if (DEBUG_MERGE == 1)
    fprintf(stderr,"creating new AOMergedBlock from %p (%d) and %p (%d), closed: %d\n",b1,b2,b1->isClosed(),b2->isClosed(),closed);
#endif
    
    // update this, we're going to increment by 2 during the merge but we really just save one page
    saved_pages--;

    merge(b1);
    merge(b2);

#if (DEBUG_MERGE == 1)
    sanityCheckSize();
#endif

    assert(_rc == 2);

    assert(b1->isClosed() || b2->isClosed());

    //fprintf(stderr,"%d + %d = %d\n",b1->getPopulation(),b2->getPopulation(),AOMergeable<N>::_pop);
    
    if(closed && AOMergeable<N>::_pop > 0 && AOMergeable<N>::_pop <= N/2) {
      FragManagerType<N>::Type::getInstance()->registerBlock(&_fragListNode);
    } else {
      //fprintf(stderr, "pop is %d/%d\n",AOMergeable<N>::_pop,N);
    }

    //fprintf(stderr,"flist is %p\n",AOMergeable<N>::_flist);
  }

  AOMergedBlock<N> * merge(AOMergeable<N> * b) {
    //assert(!conflicts(b));

    assert(_rc < 2 || closed);

    // update our local metadata
    AOMergeable<N>::_bitmap |= b->getBitmap();
    AOMergeable<N>::_pop += b->getPopulation();

    b->mergeInto(this);

    // When running with "New" VC, whenever we merge, it's always a
    // fresh page, so mark us "open for allocation!"
#if (USE_NEW_VC == 1)
    assert(AOMergeable<N>::_pop < N);
    closed = false;
#endif

    // if we cross the N/2 threshold, remove us from the FragManager
    if(AOMergeable<N>::_pop > N/2 && _fragListNode.list()) {
      _fragListNode.list()->removeBlock(&_fragListNode);
    }

    return this;
  }  

  void mergeInto(AOMergedBlock<N> * merged) {
#if (DEBUG_MERGE == 1)
    fprintf(stderr,"AOMB %p (rc %d) merging into %p (rc %d)\n",this,_rc,merged,merged->_rc);
    if(!(_rc > 0 && merged->_rc > 0)) {
      fprintf(stderr,"AOMB RC FUCKUP\n");
    }
#endif

    while(MyList::head) {
      AOBlock<N> * curr = static_cast<AOBlock<N>*>(MyList::head->data);
      // AOBlock<N>::mergeInto will remove the block
      //removeBlock(curr);
      curr->mergeInto(merged);
    }

    // Rc will drop to zero and we'll be reclaimed
  }

  // Double-dispatch stuff, called by AOBlock when it gets a merge request

  AOMergedBlock<N> * mergeWith(AOBlock<N> * rhs) {
    merge(rhs);
    return this;
  }

  void * operator new(size_t sz) {
    _mergedblocks++;
    return GlobalMetadataHeap::getInstance()->New(sz);
  }

  void operator delete(void * ptr) {
    //fprintf(stderr, "pid %d freeing %p\n",getpid(),ptr);

    _mergedblocks--;
    GlobalMetadataHeap::getInstance()->Delete(ptr);
  }

  virtual void registerBlock(Node * bl) {
    MyList::registerBlock(bl);

    _rc++;

    saved_pages++;
    
#if (DEBUG_MERGE == 1)
    sanityCheckSize();
#endif

    if(!(saved_pages % 100) && _fragListNode.list()) {
      fprintf(stderr,"now %d saved pages out of %d, list sz %d\n",
	      saved_pages,total_pages,_fragListNode.list()->getSize());
    }
  }

  virtual void removeBlock(Node * node) {
    MyList::removeBlock(node);

    AOMergeable<N> * bl = node->data;

    _rc--;

    sanityCheckSize();

#if (DEBUG_MERGE == 1)
    sanityCheckSize();
    fprintf(stderr,"removed %p from AOMB %p, now %d\n",bl,this,_rc);
#endif
      
    //assert(AOMergeable<N>::_bitmap.count() == AOMergeable<N>::_pop);

    if(_rc == 0) {
      if(_fragListNode.list()) {
	_fragListNode.list()->removeBlock(&_fragListNode);
      }
      delete this;
    } else {
      saved_pages--;
      assert(AOMergeable<N>::_bitmap.count() > 0);
    }
  } 

  inline void sanityCheckSize() {
#if (DEBUG_MERGE == 1)
    int ct = 0;
    for(Node * node = FragManager<N>::getHead(); node; node = node->next()) {
      ct++;
    }
    assert(ct == _rc);
#endif 
  }

  inline void sanityCheckClosed() {
#if (DEBUG_MERGE == 1)
    if(closed && _rc > 0 && AOMergeable<N>::_pop > 0 && AOMergeable<N>::_pop <= N/2) {
      assert(_fragListNode.list());
    }
    for(Node * node = FragManager<N>::getHead(); node; node = node->next()) {
      if(!node->data->isClosed()) return;
    }

    if(!closed) {
      for(Node * node = FragManager<N>::getHead(); node; node = node->next()) {
	fprintf(stderr,"%p\n",node->data);
      }
      abort();
    }
#endif
  }

  void checkForInsert() {
    // XXX THIS IS WEIRD, we might actually be freed at this point (but _rc == 0 in that case) if we merge 2 AOMBs
    if(closed && _rc > 0 && AOMergeable<N>::getPopulation() > 0 && AOMergeable<N>::getPopulation() <= N/2) {
      FragManagerType<N>::Type::getInstance()->registerBlock(&_fragListNode);
    }
  }

  void clearSlot(AOMergeable<N> * bl, unsigned int slot) {
    //fprintf(stderr,"AOMB clearing slot %d\n",slot);

    AOMergeable<N>::_bitmap.reset(slot);
    AOMergeable<N>::_pop--;

    // XXX no longer true until we add _pop++ on set slot from new VC policy
    assert(AOMergeable<N>::_bitmap.count() == AOMergeable<N>::getPopulation());

    if(_fragListNode.list())
      //static_cast<typename FragManagerType<N>::Type *>(_fragListNode.list())
      FragManagerType<N>::Type::getInstance()
	->clearSlot(this,slot);
    else 
      checkForInsert();

#if (DEBUG_MERGE == 1)
    sanityCheckClosed();
#endif
  }

  AOBlock<N> * getTarget() {
    if(MyList::head)
      return static_cast<AOBlock<N>*>(MyList::head->data);
    else return NULL;
  }

  void newAlloc() {
    AOMergeable<N>::_pop++;
  }

  virtual bool isClosed() const {
    return closed;
  }

  bool closed;

#if (DEBUG_MERGE == 0)
private:
#endif
  unsigned int _rc;
};

#endif
