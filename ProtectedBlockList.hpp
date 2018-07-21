#ifndef HOUND_PROTECTED_LIST
#define HOUND_PROTECTED_LIST

#include "BlockList.hpp"

template <typename F>
class ProtectedBlockList : public BlockListImpl<F> {
public:
  virtual void barrierBlock(typename BlockListImpl<F>::Node *bl) = 0;
};
#endif
