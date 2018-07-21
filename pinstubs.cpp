#include "port/phkmallocheap.h"

extern "C" {

void plug_mmap(unsigned long) {}
void plug_munmap(unsigned long) {}
void plug_initialized() {}
void plug_object_allocated(void * ptr) {}
void plug_object_freed(void * ptr) {}
void * plug_findobject(void * ptr) {
  void * ret;
  //fprintf(stderr,"normalizing %p\n",ptr);
  //fprintf(stderr,"foo\n");
  
  AOCommon * bl = AOCommon::fromPtr(ptr);
  if(bl) ret = reinterpret_cast<void*>(bl->findObject(ptr));
  else ret = ((char*)phkmalloc_normalize(ptr))+4;
  return ret;
}

void plug_barrier(void * ptr) {
  fprintf(stderr,"in plug_barrier\n");
}

unsigned long plug_getsite(void * ptr) {
  // XXX
  return 0;

  /*
  AOCommon * bl = AOCommon::fromPtr(ptr);
  if(bl) return bl->getCallSite();
  
  PlugHeap::HEADER * hdr = (PlugHeap::HEADER *)phkmalloc_normalize(ptr);
  if(!hdr) return 0;
  else return hdr->_site;
  */
}

}

extern "C" size_t plug_getsize(void * ptr) {
  return getCustomHeap()->getSize(ptr);
}
