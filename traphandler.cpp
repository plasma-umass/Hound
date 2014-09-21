#include "traphandler.h"
#include "capture_callsite.h"
#include "AOCommon.hpp"

#if defined(_WIN32)

// Entry point for the access violation exception handler
LONG NTAPI AOHVectorHandler(PEXCEPTION_POINTERS info) {
	static bool inHandler = false;

	// Prevent infinite recursion if we have a bug in the exception handling code
	if(inHandler) return EXCEPTION_CONTINUE_SEARCH;

	inHandler = true;

	LONG exceptionCode = info->ExceptionRecord->ExceptionCode;

	//printf("fault handler 0x%x\n",exceptionCode);

	LONG ret = EXCEPTION_CONTINUE_SEARCH;

	if ( exceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    PEXCEPTION_RECORD rec = info->ExceptionRecord;
    
    if(rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    
      DWORD VA = (ULONG)rec->ExceptionInformation[1];
      if(AOCommon::faultHandler(VA)) {
        ret = EXCEPTION_CONTINUE_EXECUTION;
      } 
    }
  }
    
	inHandler = false;

	return ret;
}

VOID EnableSignalHandler() {
	AddVectoredExceptionHandler(1,AOHVectorHandler);
}

VOID DisableSignalHandler() {
  RemoveVectoredExceptionHandler(AOHVectorHandler);
}

#elif defined(__GNUC__)

#include <signal.h>
#include <sys/types.h>

static void * altstack = 0;

extern "C" void AOHSEGVHandler(int signum, siginfo_t * info, void * vtxt) {
  ucontext_t * ctxt = (ucontext_t *)vtxt;

  static bool in_handler = false;

  if(in_handler) return;

  // handle stack overflow when computing callsite
  if(__computing_callsite) {
    //fprintf(stderr,"callsite stack crawl fucked itself\n");
    longjmp(__backtrace_jump,1);
  }

  in_handler = true;

  if(!info) {
    fprintf(stderr,"GOT NULL INFO STRUCT\n");
    abort();
  }

  DWORD VA = (DWORD)info->si_addr;

  if(VA == 0) {
    fputs("Null pointer exception\n",stderr);
    abort();
  }

  //fprintf(stderr,"faulting on VA 0x%x, info 0x%x, due to 0x%x\n",VA,info,info->si_code);

  if(AOCommon::faultHandler(VA)) {
    in_handler = false;
    return;
  } else {
    fprintf(stderr,"Can't handle fault on VA %p\n",VA);
    abort();
  }
}

extern "C" void AOHUSR1Handler(int signum);

extern "C" {
  extern int mysigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
}

void EnableSignalHandler() {
  struct sigaction info;

  if(altstack) abort();

  altstack = mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

  assert(altstack);

  stack_t st;

  st.ss_sp = altstack;
  st.ss_size = 4096;
  st.ss_flags = 0;
  if(sigaltstack(&st,0) < 0) {
    perror("sigaltstack");
  }

  sigemptyset(&info.sa_mask);
  info.sa_flags = SA_SIGINFO | SA_NODEFER;
  info.sa_sigaction = AOHSEGVHandler;
  mysigaction(SIGSEGV,&info,0);

#ifdef HOUND
  info.sa_handler = AOHUSR1Handler;
  info.sa_flags = 0;
  mysigaction(SIGUSR1,&info,0);
#endif
}

void DisableSignalHandler() {
  struct sigaction info;
  
  sigemptyset(&info.sa_mask);
  info.sa_flags = 0;
  info.sa_handler = SIG_DFL;
  mysigaction(SIGSEGV,&info,0);
}

#endif
