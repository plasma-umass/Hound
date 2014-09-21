#ifndef __ARCHIPELAGO_ALLOC_POLICY_H__
#define __ARCHIPELAGO_ALLOC_POLICY_H__

#include <cstdlib>

template<class Super>
class ArchipelagoAllocPolicy : public Super {
public:
  ArchipelagoAllocPolicy() : _ob(0) {}

  void * New(size_t sz, int * Space = 0, bool Zero = false) {		    
    if(_ob) return 0;

    size_t slots = (4096-sz)/8;
    _ob = Super::_start + (random() % slots)*8;
    assert(_ob + sz <= Super::_end);

    if(Zero) {
      memset(_ob,0,sz);
    }
      
    Super::_birthday = global_allocs;

    assert(_ob != 0);

    return _ob;
  }

  bool Delete(void * ptr) {
    Super::doCleared();

    return true;
  }

  // Page table stuff will ensure that the ptr is actually on this page.
  bool Details(void * ptr, int * Space) {
    if(ptr < _ob) return false;

    if(Space)
      *Space = (int)(Super::_end - (char*)ptr);

    return true;
  }

  virtual bool isClosed() const {
    return true;
  }

  virtual size_t getAllocs() const {
    return 1;
  }

  // XXX HACK FOR AOHeap
  virtual unsigned int getSizeClass() const {
    return AOLO_SIZE_CLASS;
  }

  virtual unsigned long findObject(void * ptr) const {
    return reinterpret_cast<unsigned long>(Super::_start);
  }

private:
  char * _ob;
};

#endif
