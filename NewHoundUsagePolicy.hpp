#ifndef HOUND_NEW_HOUND_USAGE_POLICY
#define HOUND_NEW_HOUND_USAGE_POLICY

template<class Super>
class NewHoundUsagePolicy : public Super {
public:
  // Can I allocate into this slot?
  inline bool testSlot(unsigned int slot) const {
    if(Super::_aliased) {
      AOMergedBlock<Super::NUM_SLOTS> * aomb = getAOMB();
      return aomb->getBitmap().test(slot);
    }
    else
      return Super::_bitmap.test(slot);
  }

  size_t numSet() {
    if(Super::_aliased) {
      AOMergedBlock<Super::NUM_SLOTS> * aomb = getAOMB();
      return aomb->getBitmap().count();
    } else {
      return Super::_bitmap.count();
    }
  }

  inline void resetSlot(unsigned int slot) {
    Super::_bitmap.reset(slot);
    // AOMB Bitmap gets cleared in clearSlot, no need to do it here
/*
    if(Super::_aliased) {
      AOMergedBlock<Super::NUM_SLOTS> * aomb = dynamic_cast<AOMergedBlock<Super::NUM_SLOTS> *>(Super::_fragListNode.list());

      aomb->getBitmap().reset(slot);
    }
*/
  }

  inline void setSlot(unsigned int slot) {
    Super::_bitmap.set(slot);
    if(Super::_aliased) {
      AOMergedBlock<Super::NUM_SLOTS> * aomb = getAOMB();

      assert(!aomb->closed);
      assert(!aomb->_fragListNode.list());
      assert(!aomb->getBitmap().test(slot));
      assert(aomb->getBitmap().count() == aomb->getPopulation());

      aomb->getBitmap().set(slot);
      aomb->newAlloc();
      
      assert(aomb->getBitmap().count() == aomb->getPopulation());
    }
  }

  virtual void mergeInto(AOMergedBlock<Super::NUM_SLOTS> * merged) {
    Super::mergeInto(merged);
  }

private:
  inline AOMergedBlock<Super::NUM_SLOTS> * getAOMB() const {
#if (DEBUG_MERGE == 1) 
    return dynamic_cast<AOMergedBlock<Super::NUM_SLOTS> *>(Super::_fragListNode.list());
#else
    return static_cast<AOMergedBlock<Super::NUM_SLOTS> *>(Super::_fragListNode.list());
#endif
  }
};

#endif
