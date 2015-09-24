#ifndef HOUND_PORT_SPINLOCK_H__
#define HOUND_PORT_SPINLOCK_H__

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
