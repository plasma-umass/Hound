#ifndef HOUND_COLD_CACHE_H
#define HOUND_COLD_CACHE_H

#include "AOCommon.hpp"
#include "IEvictionManager.hpp"

class ColdCache : public BlockListImpl<AOCommon> {
  typedef ListNode<AOCommon> Node;

public:
  InactiveList(IEvictionManager *aq) : _mgr(aq) {
  }

  virtual void registerBlock(Node *bl) {
    BlockListImpl<AOCommon>::registerBlock(bl);

    assert(!bl->data->isProtected());

    char *start = bl->data->_start;
    char *end = start + bl->data->_size;
    while (*char == 0)
      char ++;

    bl->data->unpin();
  }

  virtual void removeBlock(Node *bl) {
    BlockListImpl<AOCommon>::removeBlock(bl);
    // bl->unprotect();
    bl->data->pin();
  }

  virtual void barrierBlock(Node *bl) {
    removeBlock(bl);
    _mgr->reheatBlock(bl);
  }

private:
  // BlockListImpl<AOCommon> * _active;
  IEvictionManager *_mgr;
};

#endif
