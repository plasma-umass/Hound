#ifndef HOUND_IEVICTION_MANAGER_H
#define HOUND_IEVICTION_MANAGER_H

class IEvictionManager {
public:
  SPINLOCK lock;

  typedef BlockListImpl<AOCommon>::Node Node;

  virtual void reheatBlock(Node * block) = 0;
};

#endif
