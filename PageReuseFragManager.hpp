#ifndef HOUND_PAGE_REUSE_FRAG_MANAGER_H
#define HOUND_PAGE_REUSE_FRAG_MANAGER_H

#include "FragManager.hpp"

// Stores lists of objects segregated by population into a number of bins.
//

template <unsigned int N>
class BinType : public FragManager<N> {
public:
  void clearSlot(AOMergeable<N> *bl, unsigned int slot) {
    PageReuseFragManager<N>::getInstance()->clearSlot(bl, slot);
  }
};

template <unsigned int N>
class PageReuseFragManager : public FragManager<N> {
  // Only store pages with population <= N/POP_CUTOFF
  static const unsigned int POP_CUTOFF = 2;
  // Don't need more bins than possible discrete values
  static const unsigned int NUM_BINS = N / POP_CUTOFF < 4 ? N / POP_CUTOFF : 4;

  // Gets the "bin number" corresponding to this population.
  // NB: pop 0 needs no bin (page is reclaimed).  Also, max pop is
  // N/POP_CUTOFF which is a multiple of NUM_BINS, it doesn't need its
  // own bin... thus the (pop-1)
  static unsigned int bin_number(unsigned int pop) {
    return (pop - 1) * NUM_BINS / N / POP_CUTOFF;
  }

public:
  static PageReuseFragManager<N> *getInstance() {
    static PageReuseFragManager<N> _instance;
    return &_instance;
  }

  void registerBlock(typename FragManager<N>::Node *bl) {
    assert(bl->data->getPopulation() <= N / POP_CUTOFF);

    volatile int which = bin_number(bl->data->getPopulation());
    assert(which < NUM_BINS);
    _bins[which].registerBlock(bl);
    /*
    _pqueue.push_back(bl);
    push_heap(_pqueue.begin(),_pqueue.end());
    */

    // if(getSize() > 0 && N == 16)
    // fprintf(stderr,"registerBlock (%p) size class %d has %d (%d)\n",this,N,getSize(),_bins[0].getSize());
  }

  size_t getSize() const {
    size_t ret = 0;
    for (int i = 0; i < NUM_BINS; i++) {
      ret += _bins[i].getSize();
    }

    return ret;
  }

  void removeBlock(typename FragManager<N>::Node *bl) {
    assert(bl->data->getPopulation() == 0);

    assert(bl->list() == &_bins[0]);
    _bins[0].removeBlock(bl);
    /*
    pqueue_t::iterator pos = std::find(_pqueue.begin(),_pqueue.end(),bl);
    std::swap(_pqueue[0],*pos);

    pop_heap(_pqueue.begin(),_pqueue.end());
    _pqueue.pop_back();
    */
  }

  void clearSlot(AOMergeable<N> *bl, unsigned int slot) {
    unsigned int pop = bl->getPopulation();
    unsigned int old_bin = bin_number(pop + 1);
    unsigned int new_bin = bin_number(pop);

    // NB: this should be > not >= for N==2 case, general sanity.
    if (pop > N / POP_CUTOFF)
      return;
    if (pop > 0 && new_bin != old_bin) {
      _bins[old_bin].removeBlock(&bl->_fragListNode);
      _bins[new_bin].registerBlock(&bl->_fragListNode);
    }
  }

  AOMergeable<N> *popSparseBlock() {
    AOMergeable<N> *ret;

    for (unsigned int i = 0; i < NUM_BINS; i++) {
      typename BinType<N>::Node *node = _bins[i].getHead();
      assert(node || !_bins[i].getSize());
      if (node) {
        ret = node->data;
        _bins[i].removeBlock(node);
        assert((~ret->getBitmap()).any());
        return ret;
      }
    }

    return 0;
  }

private:
  BinType<N> _bins[NUM_BINS];
};

#endif
