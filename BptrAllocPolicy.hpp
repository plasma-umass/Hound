#ifndef HOUND_BPTR_ALLOC_POLICY
#define HOUND_BPTR_ALLOC_POLICY

template <class Super>
class BptrAllocPolicy : public Super {
public:
  BptrAllocPolicy() : _cursor(0) {
  }

  void *New(size_t sz, int *Space, bool Zero) {
    assert(_cursor <= Super::NUM_SLOTS);
    assert(sz <= Super::OBJECT_SIZE);

    AOMergedBlock<Super::NUM_SLOTS> *aomb = 0;

    if (Super::_aliased) {
#if (DEBUG_MERGE == 1)
      aomb = dynamic_cast<AOMergedBlock<Super::NUM_SLOTS> *>(Super::_fragListNode.list());
#else
      aomb = static_cast<AOMergedBlock<Super::NUM_SLOTS> *>(Super::_fragListNode.list());
#endif
    }

    if (_cursor == Super::NUM_SLOTS) {
      return NULL;
    }

    // Find a free slot, if there is one.
    while (_cursor < Super::NUM_SLOTS && Super::testSlot(_cursor)) {
      assert(Super::_aliased);
      // fprintf(stderr,"skipping slot %d on %p\n",_cursor,this);
      _cursor++;
    }

    if (_cursor == Super::NUM_SLOTS) {
      if (Super::_aliased) {
        // XXX FIXME THIS ONLY HAPPENS IF WE HIT THE END OF THE PAGE HERE, NOT WHEN WE ALLOC THE LAST SLOT ON A PAGE!!!
        // FIXME

        aomb->closed = true;

        if (Super::_pop == 0 && isClosed())
          Super::doCleared();

        aomb->checkForInsert();

        // fprintf(stderr,"%p closing %p\n",this,aomb);
      }
      return NULL;
    }

    assert(!isClosed());
    Super::setSlot(_cursor);

    // fprintf(stderr,"allocating slot %d on %p\n",_cursor,this);

    void *ret = (Super::_start + Super::OBJECT_SIZE * _cursor);

    // for(int i = 0; i < sz/4; i++) {
    //  assert(_cursor == 0 || static_cast<int *>(ret)[i] == 0);
    //}

    _cursor++;
    Super::_pop++;

    if (aomb && _cursor == Super::NUM_SLOTS) {
      aomb->closed = true;
      aomb->checkForInsert();
    }

#ifdef DEBUG_MERGE
    if (aomb) {
      aomb->sanityCheckClosed();
    }
#endif

    assert(ret < Super::_end);

    if (Zero) {
      memset(ret, 0, sz);
    }

    if (isClosed()) {
      // The block's birthday is the birthday of its YOUNGEST object
      Super::_birthday = global_allocs;
    }

    return ret;
  }

  size_t freeSlots() const {
    size_t ret = 0;
    for (int cur = 0; cur < Super::NUM_SLOTS; cur++) {
      if (!Super::testSlot(cur))
        ret++;
    }
    return ret;
  }

  virtual void breakpoint() const {
    if (Super::_aliased)
      abort();
  }

  BOOL Delete(PVOID ptr) {
    if (!Super::testSlot(Super::indexOf(ptr))) {
      printf("double free of 0x%x\n", ptr);
      // XXX Fail stop on double frees?
      _ASSERTE(false);
      return false;
    }

    // NB: MUST BE BEFORE doCleared() TO PREVENT CORRUPTION OF
    // BlockSourceHeap Freelist
    bzero(ptr, Super::OBJECT_SIZE);

    Super::_pop--;
    // XXX: Not used currently
    //_heap->_free_ct++;
    resetSlot(Super::indexOf(ptr));

    // fprintf(stderr,"freed slot %d on %p\n",Super::indexOf(ptr),this);

    // Reclaim this block if it's now empty
    if (Super::_pop == 0 && isClosed())
      Super::doCleared();

    assert(reinterpret_cast<unsigned long>(ptr) % Super::OBJECT_SIZE == 0);

    return true;
  }

  virtual bool isClosed() const {
    return _cursor == Super::NUM_SLOTS;
  }

protected:
  // Current slot cursor (slot to use for next alloc)
  unsigned int _cursor;
};

#endif
