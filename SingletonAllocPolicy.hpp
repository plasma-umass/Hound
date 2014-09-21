#ifndef __SINGLETON_ALLOC_POLICY_H__
#define __SINGLETON_ALLOC_POLICY_H__

#include <cstdlib>

template<class Super>
class SingletonAllocPolicy : public Super {
public:
  SingletonAllocPolicy() : _cursor(random() % Super::NUM_SLOTS) {}

  void * New(size_t sz, int * Space, bool Zero) {		    
    if(_cursor == Super::NUM_SLOTS) return NULL;

    Super::setSlot(_cursor);

    void * ret = (Super::_start + Super::OBJECT_SIZE*_cursor);
    _cursor = Super::NUM_SLOTS;
    
    if(Zero) {
      memset(ret,0,sz);
    }
      
    Super::_birthday = global_allocs;

    return ret;
  }

  bool Delete(void * ptr) {
    Super::doCleared();

    return true;
  }

  // Page table stuff will ensure that the ptr is actually on this page.
  bool Details(void * ptr, int * Space) {
    if(Space)
      *Space = (int)(Super::_end - ptr);

    return true;
  }

  virtual bool isClosed() const {
    return true;
  }

  virtual int getAllocs() const {
    return 1;
  }

  virtual int getSizeClass() const {
    return Super::LOG_OBJECT_SIZE - LOG_MIN_ALLOC;
  }

  virtual unsigned long findObject(void * ptr) const {
    return reinterpret_cast<unsigned long>(Super::_start);
  }

private:
  int _cursor;
};

#endif
