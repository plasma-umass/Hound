#ifndef __IEVICTION_MANAGER_H__
#define __IEVICTION_MANAGER_H__

class IEvictionManager {
public:
  SPINLOCK lock;

  typedef BlockListImpl<AOCommon>::Node Node;

  virtual void reheatBlock(Node * block) = 0;
};

#endif
