#ifndef HOUND_PORT_SPINLOCK_H
#define HOUND_PORT_SPINLOCK_H

#include "recursivelock.h"
#include "spinlock.h"

/*
class SPINLOCK : private HL::RecursiveLockType<HL::SpinLockType> {
public:
  void ClaimLock() {
    lock();
  }

  void ReleaseLock() {
    unlock();
  }
};
*/

typedef HL::RecursiveLockType<HL::SpinLockType> SPINLOCK;

#endif // __PORT_SPINLOCK_H__
