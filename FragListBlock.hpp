#ifndef __FRAG_LIST_BLOCK_H__
#define __FRAG_LIST_BLOCK_H__

#include "AOMergedBlock.hpp"

template<size_t N, class Super>
class FragListBlock : public Super {
  // In AOMergeable
  using Super::_fragListNode;
  public:
  FragListBlock() {} 
    
    virtual ~FragListBlock() {
      if(_fragListNode.list()) {
        _fragListNode.list()->removeBlock(&_fragListNode);
      }
    }

    void * New(size_t sz, int * Space = NULL, bool Zero = false) {
      void * ret = Super::New(sz,Space,Zero);
      
      checkAndInsert();
      
      return ret;
    }

    bool Delete(void * ptr) {
      
      // NB: WHAT IS GOING ON HERE?
      // Deleting the last object frmo this block will remove this block from any AOMergedBlock we're on.
      // We need to correctly clear the slot on the AOMB.  However, it's also possible that we merge two AOMBs
      // and thus our fragList changes after the Super::Delete operation.  So we store the current list
      // in case we're removed from the AOMBs totally, but only use it if the fragList after Super::Delete is null.
      // Whew.

      FragManager<N> * oldList = getFragManager();

      bool ret = Super::Delete(ptr);

      if(ret) {
	if(_fragListNode.list()) {
	  getFragManager()->clearSlot(this,Super::indexOf(ptr));
	} else if(oldList) {
	  // NB: oldList MAY BE A DANGLING POINTER if we merge our AOMB into another one.  Make sure clearSlot() won't shit itself if this is the case.
	  oldList->clearSlot(this,Super::indexOf(ptr));
	}
	
	checkAndInsert();
      }

      return ret;
    }

    virtual void mergeInto(AOMergedBlock<N> * merged) {
      Super::mergeInto(merged);

      if(_fragListNode.list()) {
	assert(_fragListNode.list() != merged);
	_fragListNode.list()->removeBlock(&_fragListNode);
      }
      merged->registerBlock(&_fragListNode);

#if (DEBUG_MERGE == 1)
      fprintf(stderr,"block %p merged into %p, now %d, total saved %d\n",this,merged,merged->_rc,saved_pages);
#endif

      assert(Super::_aliased && _fragListNode.list());
    }
    
  private:

    inline FragManager<N> * getFragManager() {
#if (DEBUG_MERGE == 1)
	return dynamic_cast<FragManager<N> *>(_fragListNode.list());
#else
	return static_cast<FragManager<N> *>(_fragListNode.list());
#endif
    }
    
    inline void checkAndInsert() {
      unsigned int pop = Super::getPopulation();
      
      // Conditional is ordered in likelihood of failing test (I think)
      // Because I'm feeling pedantic today or something.
      if(pop <= N/2 && !_fragListNode.list() && Super::isClosed() && pop > 0) {
	FragManagerType<N>::Type::getInstance()->registerBlock(&_fragListNode);
      }
    }
};

#endif // __FRAG_LIST_BLOCK_H__
