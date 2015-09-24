#ifndef HOUND_PAGE_TABLE_H__
#define HOUND_PAGE_TABLE_H__

// for size_t
#include <sys/mman.h>

class AOCommon;

class PEntry {
public:
  PEntry();
  AOCommon * m[1 << 10];
};

class PageTable {
public:
  AOCommon * get(void * p);
  void set(void *p, AOCommon * entry, size_t);
  void clear(void * p, size_t);

  static PageTable * getInstance() {
    //static PageTable * ptbl = new PageTable();
    static PageTable ptbl;
    //fprintf(stderr,"instance: %p\n",ptbl);
    return &ptbl;
  }
private:
  PageTable();

  void setPrivate(unsigned long idx, AOCommon *);
  void clearPrivate(unsigned long idx);

  // XXX hardcoded for 32 bit VADDR, 4K page

  PEntry * m[1 << 10];

  unsigned long _initialized;
};

#endif
