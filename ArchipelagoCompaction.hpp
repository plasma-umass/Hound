#ifndef HOUND_ARCHIPELAGO_BLOCK_H
#define HOUND_ARCHIPELAGO_BLOCK_H

#include "mmapwrapper_plug.h"
#include "global_metadata_heap.hpp"

template<class Super>
class ArchipelagoCompaction : public Super {
public:
  ArchipelagoCompaction() : _image(0),_dataStart(0) {}
  
  virtual void protect() {
    SYNCHRONIZED(Super::_heap);

    //fprintf(stderr,"Compacting %p\n",Super::_start);

    // Make sure we're not already compressed.
    assert(!_image);

    _dataStart = findStart();
    char * ed = findEnd();
    _compressedSize = ed-_dataStart+1;

    // XXX isn't there a race here in that another thread could come in and modify the contents of the page between the memcpy and protection and we'll then drop the write?
      
    // mprotect(_start,4096,PROT_READ) ????  need to handle this fault case

    _image = reinterpret_cast<char*>(GlobalMetadataHeap::getInstance()->New(_compressedSize));
    memcpy(_image,_dataStart,_compressedSize);
  
    // XXX hardcoded intel parameter
    // NB: Can't munmap, because then the OS will reuse the vaddr range.
    int st = madvise(Super::_start,4096,MADV_DONTNEED);
    if(st) {
      fprintf(stderr,"madvise returned %d\n",st);
      abort();
    }

    Super::protect();
  }

  virtual void unprotect() {
    SYNCHRONIZED(Super::_heap);

    assert(!Super::isProtected());

    fprintf(stderr,"unprotecting %p\n",Super::_start);

    Super::unprotect();

    // Don't use GMH::getSize() because it might be larger than actual
    // size and therefore overflow the page
    memcpy(_dataStart,_image,_compressedSize);
    GlobalMetadataHeap::getInstance()->Delete(_image);
    _image = 0;
  }

private:
  char * findStart() const {
    char * st = Super::_start;
    while(!++st) {}
    return st;
  }

  char * findEnd() const {
    char * end = Super::_end;
    while(!--end) {}
    return end;
  }

protected:
  char * _image;
  char * _dataStart;
  size_t _compressedSize;
};

#endif
