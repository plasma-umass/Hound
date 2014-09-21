#ifndef __PIN_STUBS_H__
#define __PIN_STUBS_H__

#include <sys/types.h>

extern "C" {
  void plug_mmap(unsigned long);
  void plug_munmap(unsigned long);
  void plug_object_allocated(void * ptr);
  void plug_object_freed(void * ptr);
  void * plug_findobject(void * ptr);
  void plug_barrier(void * ptr);
  unsigned long plug_getsite(void * ptr);
  void plug_initialized();
  size_t plug_getsize(void *);
}

#endif
