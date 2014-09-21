#ifndef __ACTIVE_LIST_H__
#define __ACTIVE_LIST_H__

class ActiveList : public BlockListImpl<AOCommon> {
  using BlockListImpl<AOCommon>::Node;
public:
  bool evictOne(BlockListImpl<AOCommon> * target) {
    if(!BlockListImpl<AOCommon>::tail) 
      return false;
    else {
      Node * bl = BlockListImpl<AOCommon>::tail;
      removeBlock(bl);
      target->registerBlock(bl);
      return true;
    }
  }

#ifdef GATHER_FRAG_STATS
  virtual void registerBlock(Node * bl) {
    fprintf(stderr,"adding %p to ActiveList\n",bl);
    BlockListImpl<AOCommon>::registerBlock(bl);
    //_live_ct += bl->size();
  }

  virtual void removeBlock(Node * bl) {
    fprintf(stderr,"removing %p from ActiveList\n",bl);
    BlockListImpl<AOCommon>::removeBlock(bl);
    //_live_ct -= bl->size();
  }

  //size_t live() const {
    //return _live_ct;
  //}

private:
  //size_t _live_ct;
#endif
};

#endif //__ACTIVE_LIST_H__
