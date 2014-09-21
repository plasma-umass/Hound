#ifndef __HOUND_USAGE_POLICY__
#define __HOUND_USAGE_POLICY__

template<class Super>
class HoundUsagePolicy : public Super {
public:
  bool testSlot(unsigned int slot) const {
    return Super::_bitmap.test(slot);
  }

  void resetSlot(unsigned int slot) {
    Super::_bitmap.reset(slot);
  }

  void setSlot(unsigned int slot) {
    Super::_bitmap.set(slot);
  }
};

#endif
