#ifndef __SITE_H__
#define __SITE_H__

#include "CallStack.hpp"
#include "constants.h"
#include "output.hpp"

#define SAMPLE_ALL_ALLOCS 0
// Number of live objects before AOH is enabled for a callsite
#define SAMPLE_LIVE_THRESHOLD  64
#define SAMPLE_BYTE_THRESHOLD  8192

typedef ULONG HASH_TYPE;

template<class PerCallsiteHeap>
class Site {
public:
  Site(HASH_TYPE h, VOID ** f) : 
    _cs_hash(h), 
    _allocs(0),
    _live(0),
    _allocatedBytes(0),
    _sample_counter(SAMPLE_INTERVAL),
    _N(1),
    _mean_allocs(1),
    _mean_live(1)
  {
    for(UINT i = 0; i < STACK_DEPTH; i++) {
      _cs_frames[i] = f[i];
    }
  }

  ~Site() {
    delete _heap;
  }
	
  // How we tell whether we're sitting on uninitialized data
  inline bool isValid() const {
    return _cs_hash;
  }

  void doAlloc(size_t bytes) {
    _allocs++;
    _allocatedBytes += bytes;
    _live++;
    //fprintf(stderr,"0x%x did alloc: ct now %d\n",_cs_hash,_live);
    
    // XXX: tracking disabled
    if(false && !--_sample_counter) {
      _sample_counter = SAMPLE_INTERVAL;
      _N++;
      double sweep = (_N-1.0)/_N;
      double d_allocs = getAllocCount() - _mean_allocs;
      double d_live   = getLiveCount()   - _mean_live;
      _sum_sq_allocs += d_allocs * d_allocs * sweep;
      _sum_sq_live   += d_live   * d_live   * sweep;
      _sum_coproduct += d_allocs * d_live   * sweep;
      _mean_allocs   += d_allocs / _N;
      _mean_live     += d_live   / _N;

      reportStats();
    }

    //if(_allocatedBytes > SAMPLE_BYTE_THRESHOLD)
    //	enableSampling();

    if(_live > SAMPLE_LIVE_THRESHOLD)
      enableSampling();

  }

  void doFree() {
    _live--;
    //printf("0x%x did free: ct now %d\n",_cs_hash,_live);
  }


  PerCallsiteHeap * getAllocator() {
#if (SAMPLE_ALL_ALLOCS == 1)
    if(!_heap) 
      enableSampling();
#endif
    return _heap;
  }

  inline HASH_TYPE getHashCode() const {
    return _cs_hash;
  }

  void reportStats() const {
    /*
      double sd_alloc, sd_live, cov, corr;

      sd_alloc = sqrt(_sum_sq_allocs / _N);
      sd_live  = sqrt(_sum_sq_live   / _N);
      cov = _sum_coproduct / _N;
      corr = cov/sd_alloc/sd_live;
	
      if(false && (corr > .25 && _allocs * corr > 25)) {
      printf("*** _sum_sq_allocs = %f, _sum_sq_live = %f\n",_sum_sq_allocs,_sum_sq_live);
      printf("_allocs = %ld, _live = %ld\n",_allocs,_live);
      printf("0x%lx hit sampling interval: r = %f, N = %d\n",_cs_hash,corr,_N);
      }
    */
    if(_heap) 
      _heap->reportStats();
  }

  void enableSampling() {
    if(_heap) {
      OutputDebugString("attempted to enable sampling for already-sampled site!\n");
      return;
    }

    _heap = new PerCallsiteHeap(_cs_hash);
    num_aoheaps++;

    if(!(num_aoheaps % 100)) {
      char buf[80];
      //printf("enabled callsite 0x%lx, with %d, now %d\n",_cs_hash,_live,num_aoheaps);
      //OutputDebugString(buf);
    }
  }

  void triage() 
  {
    static CALL_STACK cs;
    char buf[2048];

    if(_heap) {
      if(_heap->triage()) {
	sprintf(buf,"<site hash=\"0x%lx\">\n",_cs_hash);
	OutputStats(buf);
	cs.FormatCallStack( buf, _cs_frames, 2048, STACK_DEPTH );
	OutputStats(buf);
	OutputStats("</site>\n");
	OutputStats("</allocsite>\n");
      }
    }
  }

private:
  ULONG getAllocCount() const {
    return _allocs + (_heap ? _heap->getAllocCount() : 0);
  }
  
  ULONG getLiveCount() const {
    return _live + (_heap ? _heap->getLiveCount() : 0);
  }

  HASH_TYPE _cs_hash;
  VOID * _cs_frames[STACK_DEPTH];
  
  // CURRENT STATE
  
  // Total allocs since program start
  ULONG _allocs;
  // Current live
  ULONG _live;
  // Total live size
  ULONG _allocatedBytes;
  
  // STATISTICS
  
  static const int SAMPLE_INTERVAL = 23;
  int _sample_counter;
  
  // Number of samples
  int _N;
  
  // Sum of squares
  double _sum_sq_allocs;
  double _sum_sq_live;
  
  // Means
  double _mean_allocs;
  double _mean_live;
  
  // Sum of coproducts
  double _sum_coproduct;
  
  PerCallsiteHeap * _heap;

  static size_t num_aoheaps;
};

template<class PerCallsiteHeap>
size_t Site<PerCallsiteHeap>::num_aoheaps;

#endif
