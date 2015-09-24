#ifndef HOUND_BITMAP_BASE_H
#define HOUND_BITMAP_BASE_H

template<size_t N>
class BitmapBase {
public:
  BitmapBase() : _pop(0) {}
  
  virtual bool conflicts(BitmapBase<N> * rhs) {
    assert(!_bitmap.none() && !rhs->_bitmap.none());
    return (bool)((_bitmap & rhs->_bitmap).any());
  }

  inline size_t getPopulation() const { return _pop; }

  // NB: changed this to be non-const so that NewHoundUsagePolicy can
  // update the bitmap of an AOMergeable when we set the bit on the
  // AOBlock
  inline std::bitset<N> & getBitmap() {
    return _bitmap;
  }

protected:
  std::bitset<N> _bitmap;
  // Number of bits set in _bitmap
  unsigned long _pop;
};

#endif
