// -*- C++ -*-

/**
 * @file   mmapalloc.h
 * @brief  Obtains memory from Mmap but doesn't allow it to be freed.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @note   Copyright (C) 2005 by Emery Berger, University of Massachusetts Amherst.
 */

#ifndef HOUND_MMAPALLOC_H
#define HOUND_MMAPALLOC_H

#include "mmapwrapper.h"

/**
 * @class MmapAlloc
 * @brief Obtains memory from Mmap but doesn't allow it to be freed.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

class MmapAlloc {
public:
  static void *malloc(size_t sz) {
    void *ptr = HL::MmapWrapper::map(sz);
    return ptr;
  }
  static void free(void *) {
  }
};

#endif
