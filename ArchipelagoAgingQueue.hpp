#ifndef __ARCHIPELAGO_AGING_QUEUE_H__
#define __ARCHIPELAGO_AGING_QUEUE_H__

#include "EvictionManager.hpp"
#include "ActiveList.hpp"
#include "ColdCache.hpp"

#ifdef USE_PIN
  #define PIN_MULTIPLIER 50
#else
  #define PIN_MULTIPLIER 1
#endif

// estimate of minor fault cost in usec
#define FAULT_COST (1000*PIN_MULTIPLIER)
// 1/8 second
#define SAMPLE_TIME (125000)
#define STALENESS_THRESHOLD 25000

class ArchipelagoAgingQueue : public EvictionManager<ActiveList,ColdCache,500,125000,10000> {}

#endif
