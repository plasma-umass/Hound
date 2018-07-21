#ifndef HOUND_AO_HEAP_H
#define HOUND_AO_HEAP_H

#include "platform.hpp"

#include <assert.h>
#include <stdio.h>
#include <bitset>

#include "AOCommon.hpp"
#include "HeapBase.hpp"
#include "Spinlock.hpp"
#include "gaussian.hpp"
#include "threshold_model.hpp"

#include "config.h"

#include "callsite.hpp"
#include "output.hpp"

#include "log2.h"

/**
 * HOW THIS HEAP IS ORGANIZED:
 *
 * This is a simple implementation of an age-ordered heap that supports
 * arbitrarily-sized small objects.  It is *NOT* a segregated fits allocator.
 * AOHeap is the entry point to the allocator an exports its public interface,
 * which is modelled after Rockall.  An AOHeap owns many AOBlocks.  An AOBlock
 * is the granularity at which lifetime and liveness is tracked.  That is,
 * age is stored for a group of objects rather than each object.
 *
 * The allocator is a simple bump pointer allocator.  Blocks are allocated on
 * demand in the allocator's slow path.  Blocks are greedily returned to the system
 * when their live object count reaches zero.
 *
 * There is a minimum allocation grain.  Chunk requests are automatically rounded up
 * and aligned to this grain.
 *
 * Size information is not kept directly.  To support _msize and reliable realloc(),
 * each AOBlock contains a bitmap for each slot (an interval of the grain size).
 * Bits are set for the starting slot of each live object.  To determine the size,
 * the method walks forward in the bitmap until it finds the next set bit.  This allows
 * the allocator to automatically support realloc()-in-place when a subsequent object is
 * freed.
 *
 * NB: There is some support (untested) for block sizes less than the OS-specified 64K
 * minimum virtual address mapping.  At the current time, the destroyBlock() operation
 * passes a single block to VirtualFree(), which may not be legal/good if the block size
 * is less than 64K.
 */

// CRT fputs() calls malloc(), yay.
void LTputs(const char *buf);

template <size_t OBJECT_SIZE>
class AOBlock;

// The public interface to an Age-ordered Heap
template <class BlockFactory>
class AOHeap : public HeapBase {
  using HeapBase::Node;

public:
  AOHeap(ULONG callsite_hash = 0ul) : _cs_hash(callsite_hash), _locked(0) {
  }

  ~AOHeap() {
  }

  PVOID New(size_t sz, int *Space, bool Zero) {
    SYNCHRONIZED(this);

    PVOID ret = 0;

    // round up to nearest multiple of the allocation grain
    sz = ((sz + MIN_ALLOC - 1) & ~(MIN_ALLOC - 1));

    if (Space)
      *Space = sz;

    if (sz == 0)
      sz = MIN_ALLOC;

    _alloc_ct++;

    if (sz > BPTR_MAX_ALLOC) {
      // fprintf(stderr,"Allocating new large object: %d",sz);
      return allocLargeObject(sz, Space, Zero);
    }

    int sc = log2(sz) - LOG_MIN_ALLOC;
    // For stats, track which size classes have been alloc'd from this heap
    _scmap.set(sc);

    if (_curr[sc] == NULL)
      allocNewBlock(sc);

    _ASSERTE(_curr[sc] != NULL);

    ret = _curr[sc]->New(sz, Space, Zero);

    if (!ret) {
      allocNewBlock(sc);
      _ASSERTE(_curr[sc] != NULL);
      ret = _curr[sc]->New(sz, Space, Zero);
    }

    assert(ret);

    // fprintf(stderr,"AOHeap alloc'd %p\n",ret);

    return ret;
  }

  BOOL Delete(PVOID ptr) {
    // Should always take direct path through block
    // I.e. plugheap gets a pointer to the block and calls free directly
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    assert(false);

    SYNCHRONIZED(this);

    BOOL ret = false;

    // Use address arithmetic to find the block header
    AOCommon *bl = AOCommon::fromPtr(ptr);

    // second part of conjunction should only be hit in testing where AppHeap is an AOHeap.
    if (bl && bl->_heap == this) {
      ret = bl->Delete(ptr);
    }

    return ret;
  }

  // This is never called (same reason as Delete)
  BOOL KnownArea(PVOID ptr) {
    SYNCHRONIZED(this);

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    assert(false);

    AOCommon *bl = AOCommon::fromPtr(ptr);
    return ((bl != NULL) && (bl->_heap == this));
  }

  // I think this is never called (same reason as Delete)
  BOOL Details(PVOID ptr, int *Space) {
    SYNCHRONIZED(this);

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    assert(false);

    if (!Space)
      return KnownArea(ptr);

    AOCommon *bl = AOCommon::fromPtr(ptr);

    if (!bl || bl->_heap != this)
      return false;

    return bl->Details(ptr, Space);
  }

  ULONG getAllocCount() const {
    return _alloc_ct;
  }

  ULONG getLiveCount() const {
    return _alloc_ct - _free_ct;
  }

  // Sets/resets page protection
  VOID updateProtection();

  BOOL triage() {
    static CALL_STACK cs;
    double score = 0.0;
    char buf[256];

    sprintf_s(buf, 256, "<allocsite hash=\"0x%x\">\n", _cs_hash);
    OutputDebugString(buf);
    OutputStats(buf);

    // No blocks
    if (head == NULL)
      return false;

    int blocks = 0;

    for (Node *blc = head; blc; blc = blc->next()) {
      AOCommon *bl = blc->data;
      blocks++;

      ULONG age = global_allocs - bl->_birthday;
      ULONG stale = bl->isProtected() ? (global_allocs - bl->_protTime) : 0;

      score += stale;

      sprintf_s(buf, 256, "<block addr=\"%p\">\n", bl->addr());
      OutputStats(buf);

      // sprintf_s(buf,256,"<population>%d</population>\n",bl->getPopulation());
      // OutputStats(buf);
      sprintf_s(buf, 256, "<stale>%d</stale>\n", stale);
      OutputStats(buf);
      sprintf_s(buf, 256, "<size>%d</size>\n", 1 << (bl->getSizeClass() + LOG_MIN_ALLOC));
      OutputStats(buf);
      sprintf_s(buf, 256, "<age>%d</age>\n", age);
      OutputStats(buf);

      if (!bl->isProtected()) {
        OutputStats("<protected>false</protected>\n");
      }

      // XXX: reenable (encapsulation BS);
      /*
      HASH_TYPE hash = StackHash(bl->_lastCallStack,STACK_DEPTH);
    
      // XXX: this is the hash for a all-null callstack
      if(hash != 0x7c5d0f85) {
        char buf[1024];
        char buf2[1024];
        sprintf(buf,"<touchsite hash=\"0x%x\">\n",hash);
        OutputStats(buf);
        cs.FormatCallStack( buf, bl->_lastCallStack, 1024, STACK_DEPTH );
        //xmlify(buf2,1024,buf);
        OutputStats(buf);
        OutputStats("</touchsite>\n");
      }
      */

      OutputStats("</block>\n");
    }

    sprintf_s(buf, 256, "\t<score>%f</score>\n", score);
    OutputStats(buf);
    return true;
  }

  void reportStats() {
    fprintf(stderr, "in reportStats\n");
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
      if (HeapBase::getHead() == NULL) {
        fprintf(stderr, "head is null\n");
        return;
      }

      // only an open block, so we're not leaking.
      /*
      if(false && !HeapBase::getHead()->data->isClosed()) {
        fprintf(stderr,"no closed blocks\n");
        return;
      }
      */

      bool foundopen = false;
      int blocks = 0;

      fprintf(stderr, "\n0x%x:\n", _cs_hash);
      // fprintf(stderr,"%d\n",_scmap.count());
      // fprintf(stderr,"\tsurvival: %f\n",(double)getLiveCount()/(double)getAllocCount());
      // fprintf(stderr,"\tfrag:");

      for (HeapBase::Node *node = HeapBase::getHead(); node; node = node->next()) {
        node->data->reportStats();
      }
    }
  }

  inline HASH_TYPE getCallSite() const {
    return _cs_hash;
  }

  // XXX: audit virtual
  virtual void removeBlock(Node *bl) {
    SYNCHRONIZED(this);

    int sc = bl->data->getSizeClass();
    if (_curr[sc] == bl->data)
      _curr[sc] = NULL;

    BlockListImpl<AOCommon>::removeBlock(bl);

    // NB: The bl ListNode<> struct is CONTAINED in bl->data,
    // so this removes and invalidates the ListNode here.
    delete bl->data;
  }

  SPINLOCK lock;

private:
  VOID insertBlock(int sc, AOCommon *bl) {
    BlockListImpl<AOCommon>::registerBlock(&bl->_heapListNode);
    bl->_birthday = global_allocs;
    bl->_protTime = global_allocs;
  }

  VOID allocNewBlock(int sc) {
    SYNCHRONIZED(this);

    AOCommon *bl = BlockFactory::allocBlock(sc);
    bl->_heap = this;
    _curr[sc] = bl;
    insertBlock(sc, bl);
  }

  PVOID allocLargeObject(size_t sz, int *Space, bool Zero) {
    SYNCHRONIZED(this);

    // fprintf(stderr,"Allocating new large object");

    LargeObjectType *lo = new LargeObjectType();
    lo->_heap = this;

    insertBlock(AOLO_SIZE_CLASS, lo);

    return lo->New(sz, Space, Zero);
  }

  // The block lists
  AOCommon *_curr[NUM_SIZE_CLASSES];

  // The callsite hash for debugging/info purposes
  ULONG _cs_hash;

  ULONG _alloc_ct;
  ULONG _free_ct;

#ifdef GATHER_STATS
  // Size class bitmap
  std::bitset<NUM_SIZE_CLASSES> _scmap;
#endif

  int _locked;
  DWORD _tid;

  template <size_t OBJECT_SIZE>
  friend class AOBlock;
};

#endif  // __AO_HEAP_H__
