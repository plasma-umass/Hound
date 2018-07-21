#ifndef HOUND_VC_FRAG_MANAGER_H
#define HOUND_VC_FRAG_MANAGER_H

#include "FragManager.hpp"

template <unsigned int N>
class VCFragManager : public FragManager<N> {
  typedef FragManager<N> SuperList;
  using SuperList::head;

public:
  static VCFragManager<N> *getInstance() {
    static VCFragManager<N> _instance;
    return &_instance;
  }

  virtual void registerBlock(typename SuperList::Node *bl) {
    SuperList::registerBlock(bl);

    clearSlot(bl->data, 0);

    // if(SuperList::getSize() > 0 && !(SuperList::getSize() % 50))
    // fprintf(stderr,"%d is size %d\n",N,SuperList::getSize());
  }

  virtual void clearSlot(AOMergeable<N> *bl, unsigned int slot) {
    if (bl->getPopulation() == 0)
      return;

#if (ENABLE_BLOCK_MERGE == 1)
    for (typename SuperList::Node *i = head; i != NULL; i = i->next()) {
      AOMergeable<N> *m = i->data;
      if (!m->conflicts(bl)) {
#if (DEBUG_MERGE == 1)
        fprintf(stderr, "merging %p and %p\n", i, bl);
#endif
        m->merge(bl);
        size_t sz = FragManager<N>::getSize();

        // if(sz > 100)
        // fprintf(stderr,"frag manager %d has %d pages\n",N,sz);
        return;
      }
    }
#endif
  }
};

#endif
