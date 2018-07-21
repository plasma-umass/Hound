#ifndef HOUND_PAGE_BLOCK_H
#define HOUND_PAGE_BLOCK_H

#include "AOCommon.hpp"

extern int total_pages;

class PageBlock : public AOCommon {
public:
  PageBlock() {
    total_pages++;

#if defined(_WIN32)
    _start = (char *)BlockFactory::getInstance()->New(AO_BLOCK_SIZE, this);
#else
    _start = (char *)BlockSourceHeap().malloc(AO_BLOCK_SIZE);
    PageTable::getInstance()->set(_start, this, AO_BLOCK_SIZE);
#endif

    // fprintf(stderr,"new AOBlock %p, %d, %d\n",this,AO_BLOCK_SIZE/N,blockCount);

    _end = _start + AO_BLOCK_SIZE;
  }

protected:
  ~PageBlock() {
    total_pages--;
  }
};

#endif
