#ifndef __EVICTION_MANAGER_H__
#define __EVICTION_MANAGER_H__

#include <sys/time.h>
#include <sys/resource.h>
#include <algorithm>
using std::min;
using std::max;

#include "IEvictionManager.hpp"

template <class HotList,
	  class ColdList,
	  unsigned int FAULT_COST,
	  // How long to wait between expensive check/resize
	  unsigned int SAMPLE_TIME = 125000,
	  unsigned int OVERHEAD_TARGET = 100>
class EvictionManager : public IEvictionManager {

public:
  typedef BlockListImpl<AOCommon>::Node Node;
  
  EvictionManager() : _cold(this),_fault_ct(0), _tsz(0) {}

  void registerBlock(Node * bl) {
    Guard<SPINLOCK> __lock(this->lock);

    _hot.registerBlock(bl);

    _added_ct++;
    
    checkRefill();

    //fprintf(stderr,"on aging queue: %p\n",bl);
  }

  void removeBlock(Node * bl) {
    Guard<SPINLOCK> __lock(this->lock);

    // we can never be the true parent
    assert(false);
    abort();
    //fprintf(stderr,"removing %p\n",bl);
  }

  void getSize() const {
    return _hot.getSize() + _cold.getSize();
  }

  // Called from _cold to move a block from _cold into _hot.
  // Block must already be removed from _cold.
  virtual void reheatBlock(Node * block) {
    Guard<SPINLOCK> __lock(this->lock);

    _fault_ct++;
    updateSizeTarget();

    // Don't register block with HOT list until after possibility of refill has past
    // o/w we might immediately reprotect this block.
    _hot.registerBlock(block);
  }

private:
 void checkRefill() {
    if(((int)_cold.getSize()) < _tsz) {
      minorRefill();
    }

    updateSizeTarget();
  }

  inline void updateSizeTarget() {
    struct rusage rbuf;
    getrusage(RUSAGE_SELF,&rbuf);
    
    unsigned int secs = rbuf.ru_utime.tv_sec;
    unsigned int usecs = rbuf.ru_utime.tv_usec;

    {
      // la la la i hate timevals

      secs += rbuf.ru_stime.tv_sec;
      usecs += rbuf.ru_stime.tv_usec;
    
      if(usecs > 1000000) {
	secs++;
	usecs -= 1000000;
      }
    }

    unsigned long diff = (secs - _last_sample.tv_sec)*1000000 + (usecs - _last_sample.tv_usec);
    // prevent div by zero
    if(diff == 0) diff = 1;

    if(_added_ct > 256
       || diff > SAMPLE_TIME 
       || _fault_ct*FAULT_COST > SAMPLE_TIME/10 ) {
      //fprintf(stderr,"diff is %d usec, faults %d added %d, ACT %d INACT %d TARG %d\n",diff,_fault_ct,_added_ct,_hot.getSize(),_cold.getSize(),_tsz);
      
      int overhead = _fault_ct * FAULT_COST * 10000 / diff;
      //fprintf(stderr,"overhead = %d\n",overhead);
      
      // XXX WTF is this fucking "refill" crap? what does the target
      // vs. actual size shit in the CRAMM paper mean?
   
      if(_fault_ct == 0) {
        // refill case
        _tsz += max(min(min(_cold.getSize(),_hot.getSize())/16u,256u),8u);
        majorRefill();
      } else if(overhead < OVERHEAD_TARGET/2) {
        // enlarge window
        _tsz += max(min(_cold.getSize(),_hot.getSize())/32,8u);
      } else if(overhead > 3*OVERHEAD_TARGET/2) {
        _tsz -= max(min(_cold.getSize(),_hot.getSize())/8,8u);
      }

      if(_tsz < 0) _tsz = 0;

      // reset window stats
      _last_sample.tv_sec = secs;
      _last_sample.tv_usec = usecs;
      _fault_ct = 0;
      _added_ct = 0;
    }
  }
  
  // NB: this may seem inefficient, but remember we have to walk the list anyway to find the 8th page back, etc.
  void minorRefill() {
    //fprintf(stderr, "doing minor refill, INACT %d, TARG %d\n",_cold.getSize(),_tsz);
    for(int i = 0; i < 8; i++) {
      _hot.evictOne(&_cold);
    }
  }

  void majorRefill() {
    //fprintf(stderr, "doing MAJOR refill, INACT %d, TARG %d\n",_cold.getSize(),_tsz);
    for(int i = 0; i < min(min((int)_hot.getSize(),(int)_tsz-(int)_cold.getSize()),256); i++) {
      _hot.evictOne(&_cold);
    }
  }

  HotList _hot;
  ColdList _cold;
  unsigned int _fault_ct;
  // Target cold list size
  int _tsz;

  unsigned int _added_ct;

  // Time of last check (in CPU time)
  struct timeval  _last_sample;
};

#endif // __EVICTION_MANAGER_H__
