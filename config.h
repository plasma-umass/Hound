#ifndef HOUND_CONFIG_H__
#define HOUND_CONFIG_H__

#include <sys/types.h>

#undef USE_PIN

#define GATHER_STATS
#undef PROFILE_FREES
#undef GATHER_FRAG_STATS

#ifdef USE_PIN
  #define ENABLE_BLOCK_MERGE 0
#else
  #define ENABLE_BLOCK_MERGE 1
#endif 

#define USE_NEW_VC 1

// Print a bunch of debug info for Virtual Compaction
#define DEBUG_MERGE 0

template <unsigned int N>
class VCFragManager;

template <unsigned int N>
class PageReuseFragManager;

template<size_t N, template<unsigned int> class, size_t DELAY>
  class FragManagerFilter;

template<size_t N> class AOBlock;

template <unsigned int N>
class FragManagerType {
 public:
#if (USE_NEW_VC == 0)
  typedef VCFragManager<N> Type;
#else
  typedef PageReuseFragManager<N> Type;
  //typedef FragManagerFilter<N, PageReuseFragManager, 256> Type;
#endif
};

class AOLargeObject;

#include "AgingBlock.hpp"
#include "AOLargeObject.hpp"
#include "FragListBlock.hpp"
#include "BptrAllocPolicy.hpp"
#include "SingletonAllocPolicy.hpp"
#include "HoundUsagePolicy.hpp"
#include "NewHoundUsagePolicy.hpp"

#include "oneheap.h"
#include "freelistheap.h"
#include "chunkheap.h"
#include "pageheap.h"

// Source for fresh virtual memory (page blocks & large objects)
typedef HL::OneHeap<HL::FreelistHeap<HL::ChunkHeap<256*1024, PageHeap<256*1024> > > > BlockSourceHeap;
//typedef PageHeap<4*1024> BlockSourceHeap;

#include "AgingQueue.hpp"

#if defined(HOUND)

#include "FreshBlockFactory.hpp"
#include "RecyclingBlockFactory.hpp"

#if (USE_NEW_VC == 1) 
  #define UsagePolicy NewHoundUsagePolicy;
#else
  #define UsagePolicy HoundUsagePolicy;
#endif

#define _BlockType FragListBlock<N,AgingBlock<AgingQueue,BptrAllocPolicy<NewHoundUsagePolicy<AOBlock<N> > > > >
//#define _BlockType BptrAllocPolicy<HoundUsagePolicy<AOBlock<N> > >

template <size_t N>
class BlockType : public _BlockType {
 public:
 BlockType() : _BlockType() {}
};

#include "rocklayer.hpp"

#if (USE_NEW_VC == 1) 
  typedef RecyclingBlockFactory<BlockType> BlockFactory;
#else
  typedef FreshBlockFactory<BlockType> BlockFactory;
#endif

typedef AgingBlock<AgingQueue,AOLargeObject> LargeObjectType;
//typedef AOLargeObject LargeObjectType;

template<class BlockFactory>
class AOHeap;

typedef AOHeap<BlockFactory> PerCallsiteHeapType;

#elif defined(ARCHIPELAGO)

#include "PageBlock.hpp"

#include "ArchipelagoAllocPolicy.hpp"
#include "ArchipelagoCompaction.hpp"
#include "ArchipelagoHeap.hpp"

typedef 
AgingBlock<AgingQueue,ArchipelagoCompaction<ArchipelagoAllocPolicy<PageBlock> > >
//ArchipelagoAllocPolicy<PageBlock>
BlockType;

#endif

#endif // __CONFIG_H__
