/* -*- C++ -*- */

/*
 * @file   liblea.cpp
 * @brief  This file replaces malloc etc. in your application.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Gene Novark
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include "constants.h"

#include "ansiwrapper.h"
#include "localmallocheap.h"
#include "profinj.hpp"

volatile int anyThreadCreated = 1;

using namespace HL;

class TheCustomHeapType :
    // public ANSIWrapper<ReentrantHeap<RockLayer<PlugHeap> > > {};
    public ANSIWrapper<ProfInj<LocalMallocHeap>> {};
// public ANSIWrapper<RockLayer<RockallAdaptor<PhkMallocHeap> > > {};

/*
static TheCustomHeapType _theCustomHeap;

static TheCustomHeapType * getCustomHeap (void) {
  return &_theCustomHeap;
}
*/

extern "C" void exitfoo();

extern "C" {
void SEGVHandler(int signum, siginfo_t *info, void *vtxt) {
  static bool in_handler = false;

  // handle stack overflow when computing callsite
  if (__computing_callsite) {
    // fprintf(stderr,"callsite stack crawl fucked itself\n");
    longjmp(__backtrace_jump, 1);
  }

  abort();
}
}

void EnableSignalHandler();

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

    atexit(exitfoo);
    EnableSignalHandler();
    fprintf(stderr, "Profiler/Injector initialized (pid %d).\n", getpid());
  }

  // fprintf(stderr,"getCustomHeap returning %p\n",th);
  return th;
}

extern "C" {
/*
sighandler_t signal(int signum, sighandler_t handler) {
  struct sigaction sa, osa;

  sa.sa_handler = handler;
  sigemptyset (&sa.sa_mask);

  sa.sa_flags = 0;
  if (sigaction (signum, &sa, &osa) < 0)
    return SIG_ERR;

  return osa.sa_handler;
}
*/

void exitfoo() {
  getCustomHeap()->reportStats();
}

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

void AOHUSR1Handler(int signum) {
  fprintf(stderr, "got USR1\n");
  printf("got USR1 (stdout)\n");
  // getCustomHeap()->reportStats();
}
}

void EnableSignalHandler() {
  struct sigaction info;

  sigemptyset(&info.sa_mask);
  info.sa_flags = SA_SIGINFO | SA_NODEFER;
  info.sa_sigaction = SEGVHandler;
  mysigaction(SIGSEGV, &info, 0);
}

#if defined(_WIN32)
#pragma warning(disable : 4273)
#endif

#include "../../heaplayers/wrapper.cpp"
