#ifndef HOUND_FRAG_MANAGER_H
#define HOUND_FRAG_MANAGER_H

#include "AOMergedBlock.hpp"
#include "AOCommon.hpp"
#include "constants.h"
#include "staticlog.h"

#include "platform.hpp"

template <unsigned int N>
class AOBlock;

template <unsigned int N>
class FragManager : public BlockListImpl<AOMergeable<N> > {
protected:
  typedef BlockListImpl<AOMergeable<N> > SuperList;

public:

  /*
  class Node : public BlockListImpl<AOMergeable<N> >::Node {
  public:
    Node(AOMergeable<N> * data) : BlockListImpl<AOMergeable<N> >::Node(data) {}

    FragManager<N> * list() { 
      return static_cast<FragManager<N> *>(BlockListImpl<AOMergeable<N> >::Node::_list);
    }
  };
  */

  virtual void clearSlot(AOMergeable<N> * bl, unsigned int slot) = 0;
};

#endif
