#include <signal.h>

#include "../config.h"

#include "../ArchipelagoHeap.hpp"
#include "../traphandler.h"

#include "syscalls.h"

volatile int anyThreadCreated = 1;
uint64_t global_allocs = 0;
int total_pages = 0;

class TheCustomHeapType : public ANSIWrapper<ArchipelagoHeap<BlockType, AOLargeObject>> {};

inline static TheCustomHeapType *getCustomHeap(void) {
  static char thBuf[sizeof(TheCustomHeapType)];
  static TheCustomHeapType *th;
  static bool firsttime = true;

  // fprintf(stderr,"trying getCustomHeap\n");

  if (firsttime) {
    firsttime = false;
    // fprintf(stderr,"building new customheap at %p, ret %p\n",thBuf,__builtin_return_address(2));
    new (thBuf) TheCustomHeapType;
    th = (TheCustomHeapType *)thBuf;
    // fprintf(stderr,"plugheap instantiated\n");
    EnableSignalHandler();
    // XXX this borks omnetpp (SPEC06) for some reason)
    // atexit(exitfoo);
    fprintf(stderr, "PlugHeap initialized (pid %d).\n", getpid());
  }

  // fprintf(stderr,"getCustomHeap returning %p\n",th);
  return th;
}

extern "C" {
static bool inDlsym = false;

int __initialized_memtracer;

int mysigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
  typedef int (*sigaction_function_type)(int, const struct sigaction *, struct sigaction *);
  static sigaction_function_type real_sigaction = 0;

  if (!real_sigaction) {
    inDlsym = true;
    real_sigaction = (sigaction_function_type)dlsym(RTLD_NEXT, "sigaction");
    inDlsym = false;
  }

  return real_sigaction(signum, act, oldact);
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
  // fprintf(stderr,"sigaction wrapper");
  if (signum != SIGSEGV && signum != SIGUSR1) {
    return mysigaction(signum, act, oldact);
  } else {
    /// XXX: fix, return -1 and set errno
    return 0;
  }
}
}

#include "../pinstubs.cpp"

#include "wrapper.cpp"
