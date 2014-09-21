#ifndef __AO_MERGEABLE_H__
#define __AO_MERGEABLE_H__

#include "config.h"

template <unsigned int>
class AOBlock;

template <unsigned int>
class AOMergedBlock;

#include <bitset>

#include "BitmapBase.hpp"

template <unsigned int N>
class AOMergeable : public BitmapBase<N> {
public:
  virtual void mergeInto(AOMergedBlock<N> * merged) = 0;
  virtual AOMergedBlock<N> * merge(AOMergeable<N> * rhs) = 0;
  virtual AOMergedBlock<N> * mergeWith(AOBlock<N> * rhs) = 0;

  virtual bool isClosed() const = 0;

  typename FragManagerType<N>::Type::Node _fragListNode;

protected:
  AOMergeable() : _fragListNode(this) {}
  //AOMergeable(const std::bitset<N> & bs) : _bitmap(bs) {}


};

#endif
