#ifndef HOUND_AOCOMMON_H
#define HOUND_AOCOMMON_H

#include <mutex>

#include "platform.hpp"
#include <assert.h>

#include "constants.h"

#include "pagetable.hpp"
#include "BlockList.hpp"
#include "HeapBase.hpp"

#include "sys/syscall.h"

#include "port/callsite.hpp"

extern uint64_t global_allocs;

// T must be duck-typed to include _mutex, _lock, _tid
/*
template <class T>
class Synchronized {
public:
	inline Synchronized(T * h) : _crit(h) {
		h->_mutex.ClaimLock();
		_crit->_tid = syscall(SYS_gettid);
		h->_locked++;
	}

	inline ~Synchronized() {
		_crit->_locked--;
		assert(_crit->_locked >= 0);
		assert	(_crit->_tid == syscall(SYS_gettid));
		_crit->_mutex.ReleaseLock();
	}

private:
	T * _crit;
};
*/

//#define SYNCHRONIZED(s) Synchronized<AOHeap> __STRUCTURED_LOCK_##s(static_cast<AOHeap*>(s));
#define SYNCHRONIZED(s) std::lock_guard<SPINLOCK>(s->lock);

class AOCommon {
public:
	inline static AOCommon * fromPtr(LPCVOID ptr) {
#if defined(_WIN32)
    AOChunk * chunk = AOChunk::fromPtr(ptr);
    if(chunk == NULL)
      return NULL;
    else {
      AOCommon * ret = chunk->ptrToBlock(ptr);
      
      return ret;
    }
#else
    return PageTable::getInstance()->get(ptr);
#endif
  }

  static BOOL faultHandler(DWORD VA);

  virtual ~AOCommon();

  virtual void reportStats() {} 

  void pin();
  void unpin();

  void * operator new(size_t sz);
  void operator delete(void * ptr);

  //virtual bool isClosed() const = 0;

  virtual PVOID New(size_t sz, int * Space = NULL, bool Zero = false) = 0;
  virtual BOOL Delete(PVOID p) = 0;
  virtual BOOL Details(PVOID p, int * space) = 0;

  VOID doCleared();

  virtual size_t getAllocs() const = 0;
  
  size_t getSize() const { return (_end - _start); }

  virtual unsigned int getSizeClass() const = 0;

  //HASH_TYPE getCallSite();
  
  inline bool isValid() {
    return (_pinned != -42) && (_heapListNode.list());
  }

  LONG isPinned() const { return _pinned; }
  BOOL isProtected() const { return _protected; }

  virtual ULONG findObject(PVOID) const  = 0;

  virtual unsigned long getStaleness() {
    if(_protected)
      return global_allocs - _protTime;
    else 
      return 0;
  }

  virtual void breakpoint() const {
    //abort();
  }

  // This is called whenever the runtime system notices a reference to the
  // page.  Currently (10/1/2009 and earlier) this occurs from two places:
  // 1) Segfault handler
  // 2) System call shims (in port/syscalls.h)

  // NB: VIRTUAL call, some mixins (e.g. AgingBlock) override and do their own
  // thing (and call this superclass member function)
  virtual VOID barrier(DWORD addr) {
    UNREFERENCED_PARAMETER(addr);

    SYNCHRONIZED(_heap);
    // We can take a barrier here if we are sent into a syscall (the
    // MemTracer code will attempt to unprotect us by calling barrier()
    if(!_protected) {
      return;
    }

    assert(!_pinned);
  
    // NB: Let the inactive list/eviction manager handle its own invariant.
    // I.e., if it's on the IAL, it's protected, on the AL, unprotected, etc.
    //unprotect();

    // We just do two things: track where the access occurred...

    // XXX: does this make sense still?  Do we still skip the same number
    // of frames on syscalls and on segvs?

    // Fill the last-touched callstack
    for(int i = 0; i < STACK_DEPTH; i++) {
      _lastCallStack[i] = 0;
    }
    // XXX reenable, it's breaking stuff right now when multiple dudes are in a segv handler? Gn 11/5/2009
    //CaptureStackBackTrace(4,STACK_DEPTH,_lastCallStack,NULL);
	
    if(false) {
      static CALL_STACK cs;
      char buf[256];
      cs.FormatCallStack(buf,_lastCallStack,256,STACK_DEPTH);
      puts(buf);
    }
  }

  inline char * addr() const { return _start; }

 // The time at which this block was activated
  ULONG _birthday;
  // Time at which page was last protected
  ULONG _protTime;

  HeapBase::Node _heapListNode;

  HeapBase * _heap;

protected:
  AOCommon() : _heap(NULL), _heapListNode(this), _magic(MAGIC_NUMBER),_pinned(1), _protected(false) {
    for(int i = 0; i < STACK_DEPTH; i++) {
      _lastCallStack[i] = 0;
    }
  }
  virtual void protect();
  virtual void unprotect();

  // first address of this block
  char * _start;
  // one past the end of this block
  char * _end;

  LONG _pinned;

  static const ULONG MAGIC_NUMBER = 0xd00f1234;
  volatile ULONG _magic;

  PVOID _lastCallStack[STACK_DEPTH];

private:
  BOOL _protected;
};

#endif
