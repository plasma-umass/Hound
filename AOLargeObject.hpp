#ifndef __AOLARGE_OBJECT__
#define __AOLARGE_OBJECT__

#include "AOCommon.hpp"
#include "mmapwrapper.h"

class AOLargeObject : public AOCommon {
public:
  AOLargeObject() {};

  ~AOLargeObject() {
    HL::MmapWrapper::unmap(_start,getSize());
  }

  PVOID New(size_t sz, int * Space = 0, bool Zero = true) {
    
#if defined(_WIN32)
    // round up to nearest 64K 
    // XXX should this be somewhere else?
    if(sz > VALLOC_SIZE) {
      sz = ((sz+VALLOC_GRAIN-1)/VALLOC_GRAIN)*VALLOC_GRAIN;
    }
    _start = (PCHAR)BlockFactory::getInstance()->New(sz,this);
#else
    sz = ((sz+PROT_GRAIN-1)/PROT_GRAIN)*PROT_GRAIN;
    _start = (char*)HL::MmapWrapper::map(sz);
    PageTable::getInstance()->set(_start,this,sz);
#endif

    //fprintf(stderr,"new AOLO @ %p\n",_start);

    _end = _start+sz;
    _sz = sz;

    if(Space) *Space = sz;

    if(Zero)
      memset(_start,0,sz);

    return _start;
  }

  virtual BOOL Delete(PVOID ptr) {
    assert(ptr == _start);    
    doCleared();
    return true;
  }

  PVOID Resize(PVOID p, size_t sz);
  BOOL Details(PVOID ptr, int * Space) {
    assert(ptr == _start);
    
    if(Space) *Space = _sz;
    return true;
  }
  
  virtual bool isClosed() const {
    return true;
  }

  inline size_t getPopulation() const {
    return 1;
  }

  ULONG findObject(PVOID ptr) const {
    if(ptr < _start || ptr >= _end) return 0;
    else return reinterpret_cast<ULONG>(_start);
  }

  virtual size_t getAllocs() const { return 1; }

  virtual unsigned int getSizeClass() const {
    return AOLO_SIZE_CLASS;
  }

private:
  size_t _sz;
};

#endif
