#ifndef __PROTECTED_LIST__
#define __PROTECTED_LIST__

#include "BlockList.hpp"

template <typename F>
class ProtectedBlockList : public BlockListImpl<F> {
public:
  virtual void barrierBlock(typename BlockListImpl<F>::Node * bl) = 0;
};
#endif
