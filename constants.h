#ifndef HOUND_AOH_CONSTANTS_H__
#define HOUND_AOH_CONSTANTS_H__

#include <sys/types.h>

#include "staticlog.h"

#define PIN_TRACE_EVENTS 1

typedef unsigned long HASH_TYPE;

static const size_t PAGE_SIZE = 4096;

// Size of each AOBlock
static const size_t AO_BLOCK_SIZE = PAGE_SIZE;
static const size_t LOG_AO_BLOCK_SIZE = StaticLog<AO_BLOCK_SIZE>::VALUE;

// Protection granularity (page size)
static const size_t PROT_GRAIN = PAGE_SIZE;
static const size_t LOG_PROT_GRAIN = StaticLog<PROT_GRAIN>::VALUE;

// Minimum allocation size in AOBlock
static const size_t MIN_ALLOC = 16;
static const size_t LOG_MIN_ALLOC = StaticLog<MIN_ALLOC>::VALUE;

// Maximum allocation size in AOBlock
static const size_t BPTR_MAX_ALLOC = AO_BLOCK_SIZE / 2;
static const size_t LOG_BPTR_MAX_ALLOC = StaticLog<BPTR_MAX_ALLOC>::VALUE;

// Granularity to allocate memory from the system
// Will allocate more for large objects
static const size_t VALLOC_SIZE = 65536;
static const size_t LOG_VALLOC_SIZE = StaticLog<VALLOC_SIZE>::VALUE;

// Granularity of memory returned from VirtualAlloc
static const size_t VALLOC_GRAIN = 65536;
static const size_t LOG_VALLOC_GRAIN = StaticLog<VALLOC_GRAIN>::VALUE;

// Smallest allocation satisfiable from an AOChunk (min AOBlock size)
static const size_t AOCHUNK_MIN_ALLOC = 4096;
static const size_t LOG_AOCHUNK_MIN_ALLOC = StaticLog<AOCHUNK_MIN_ALLOC>::VALUE;

// Number of stack frames to capture
static const size_t STACK_DEPTH = 4;

// Size classes for segfits allocation
static const size_t NUM_SIZE_CLASSES = LOG_BPTR_MAX_ALLOC - LOG_MIN_ALLOC + 1 + 1;  // last +1 for large objects
static const size_t AOLO_SIZE_CLASS = NUM_SIZE_CLASSES - 1;

#endif
