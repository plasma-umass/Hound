#include "pagetable.hpp"

#include "../pinstubs.hpp"

#include "../global_metadata_heap.hpp"

#include "../constants.h"
#include "../mmapwrapper_plug.h"

#include <assert.h>
#include <unistd.h>
#include <new>

#include "../AOCommon.hpp"

PageTable::PageTable() {
  // fprintf(stderr,"creating page table\n");
  memset(m, 0, sizeof(m));
  _initialized = 0xdeadbeef;

  fprintf(stderr, "done creating page table %p\n", this);

  plug_initialized();
}

PEntry::PEntry() {
  memset(m, 0, sizeof(m));
}

AOCommon *PageTable::get(void *ptr) {
  assert(_initialized == 0xdeadbeef);

  unsigned long idx = (unsigned long)ptr >> 12;

  unsigned long dx1 = idx >> 10;
  unsigned long dx2 = idx & ((1 << 10) - 1);

  // fprintf(stderr,"(0x%x) get 0x%x\n",this,ptr);

  if (m[dx1]) {
    AOCommon *bl = m[dx1]->m[dx2];
    if (!bl) {
      // fprintf(stderr,"(0x%x) failing (got null) on 0x%x\n",this,ptr);
      return 0;
    }
    assert(bl);
    // assert(bl->isValid());

    // fprintf(stderr, "got 0x%x\n",bl);
    return bl;
  } else
    return NULL;
}

void PageTable::set(void *ptr, AOCommon *entry, size_t sz) {
  assert(_initialized == 0xdeadbeef);

  // printf("(0x%x) set 0x%x: 0x%x, sz %d\n",this,ptr,entry,sz);

  size_t ct = sz >> 12;
  unsigned long base = ((unsigned long)ptr) >> 12;

  for (int i = 0; i < ct; i++) {
    setPrivate(base + i, entry);
  }

  assert(get(ptr) == entry);
}

void PageTable::clear(void *ptr, size_t sz) {
  // printf("clear 0x%x\n",ptr);

  size_t ct = sz >> 12;
  unsigned long base = ((unsigned long)ptr) >> 12;

  for (int i = 0; i < ct; i++) {
    clearPrivate(base + i);
  }
}

inline void PageTable::setPrivate(unsigned long idx, AOCommon *data) {
  unsigned long x1, x2;
  x1 = idx >> 10;
  x2 = idx & ((1 << 10) - 1);

  PEntry *entry;
  entry = m[x1];
  if (!entry) {
    void *buf = GlobalMetadataHeap::getInstance()->New(sizeof(PEntry));
    entry = new (buf) PEntry();
    m[x1] = entry;
  }

  assert(entry->m[x2] == NULL);
  entry->m[x2] = data;
}

inline void PageTable::clearPrivate(unsigned long idx) {
  unsigned long x1, x2;
  x1 = idx >> 10;
  x2 = idx & ((1 << 10) - 1);

  PEntry *entry;
  entry = m[x1];

  assert(entry);

  entry->m[x2] = NULL;
}
