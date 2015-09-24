#ifndef HOUND_INACTIVE_LIST_HPP
#define HOUND_INACTIVE_LIST_HPP

#include "AOCommon.hpp"
#include "IEvictionManager.hpp"
#include "ProtectedBlockList.hpp"

class InactiveList : public ProtectedBlockList<AOCommon> {
  using BlockListImpl<AOCommon>::Node;

public:
  InactiveList(IEvictionManager * aq) : _mgr(aq) {
  }

  virtual void registerBlock(Node * bl) {
    BlockListImpl<AOCommon>::registerBlock(bl);
    // protection happens here
    bl->data->unpin();

    //fprintf(stderr,"(AOH) IAL registered block %p, now %d\n",
    //        static_cast<AOCommon *>(bl)->addr(),getSize());
  }

  virtual void removeBlock(Node * bl) {
    //fprintf(stderr,"removing %p from InactiveList\n",bl);

    BlockListImpl<AOCommon>::removeBlock(bl);
    //bl->unprotect();
    bl->data->pin();
  }

  virtual void barrierBlock(Node * bl) {
    removeBlock(bl);
    _mgr->reheatBlock(bl);
  }

private:
  //BlockListImpl<AOCommon> * _active;
  IEvictionManager * _mgr;
};


#endif // __INACTIVE_LIST_HPP
