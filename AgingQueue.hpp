#ifndef __AGING_QUEUE_H__
#define __AGING_QUEUE_H__

#include "AOMergedBlock.hpp"
#include "AOCommon.hpp"
#include "constants.h"
#include "staticlog.h"
#include "BlockList.hpp" 
#include "EvictionManager.hpp"
#include "ActiveList.hpp"
#include "InactiveList.hpp"

#include <algorithm>
using namespace std;

#ifdef USE_PIN
  #define PIN_MULTIPLIER 50
#else
  #define PIN_MULTIPLIER 1
#endif

// estimate of minor fault cost in usec
#define FAULT_COST (500*PIN_MULTIPLIER)
// 1/8 second
#define SAMPLE_TIME (125000)

class AgingQueue : public EvictionManager<ActiveList,InactiveList,FAULT_COST,SAMPLE_TIME> {

public:
  static AgingQueue * getInstance() {
    static AgingQueue _instance;
    return &_instance;
  }

  AgingQueue() {}
};

#endif
