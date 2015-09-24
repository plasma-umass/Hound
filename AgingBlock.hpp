#ifndef HOUND_AGING_BLOCK_H
#define HOUND_AGING_BLOCK_H

#include <unistd.h>
#include <cstdio>

#include "platform.hpp"

#include "BlockList.hpp"
#include "ProtectedBlockList.hpp"

template<class Queue, class Super>
class AgingBlock : public Super {
public:
  AgingBlock() : _agingQueueNode(this) {}
  
  virtual ~AgingBlock() {
    //fprintf(stderr,"~AgingBlock(%p)\n",this);
    // NB: We can be destroyed without being put on the AgingQueue if we notice that we can't allocate in the last slot after freeing the last alloc'd object on a page (under "New" VC)
    //assert(_agingQueueNode.list() != NULL);
    if(_agingQueueNode.list())
      _agingQueueNode.list()->removeBlock(&_agingQueueNode);
  }

  void * New(size_t sz, int * Space = NULL, bool Zero = false) {
    void * ret = Super::New(sz,Space,Zero);

    // We're now "closed", so no more allocs are possible.
    // Aging queue will let us become stale and then mprotect us.
    if(Super::isClosed() && Super::getPopulation() > 0) {
      if(!_agingQueueNode.list()) {
	Queue::getInstance()->registerBlock(&_agingQueueNode);
      }
    }

    return ret;
  }
    
  virtual void barrier(DWORD va) {
    Super::barrier(va);

    if(_agingQueueNode.list()) {
      static_cast<ProtectedBlockList<AOCommon> *>(_agingQueueNode.list())->barrierBlock(&_agingQueueNode);
    }
  }

private:
  typename Queue::Node _agingQueueNode;
};

#endif
