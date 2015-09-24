/* -*- C++ -*- */

/*

  Heap Layers: An Extensible Memory Allocation Infrastructure
  
  Copyright (C) 2000-2004 by Emery Berger
  http://www.cs.umass.edu/~emery
  emery@cs.umass.edu
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef HOUND_PAGEHEAP_H_
#define HOUND_PAGEHEAP_H_

#if defined(_WIN32)
#include <windows.h>
#else
// UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <map>
#endif

#include "sassert.h"
#include "mmapwrapper_plug.h"

#include <new>

template<size_t ChunkSize>
class PageHeap {
public:

  /// All memory from here is zeroed.
  enum { ZeroMemory = 1 };

  // Linux and most other operating systems align memory to a 4K boundary.
  enum { Alignment = 4 * 1024 };

  inline void * malloc (size_t sz) {
    assert(sz == ChunkSize);
    return MmapWrapperPlug::map(sz);
  }
    
  inline void free (void * ptr) {
    MmapWrapperPlug::unmap (reinterpret_cast<char *>(ptr), ChunkSize);
  }

  inline size_t getSize (void * ptr) {
    return ChunkSize;
  }
};

#endif
