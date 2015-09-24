#ifndef HOUND_FRAG_MANAGER_FILTER_H
#define HOUND_FRAG_MANAGER_FILTER_H

template<unsigned int N>
class AOMergable;

template<unsigned int N>
class FragManager;

template<unsigned int N, template <unsigned int> class Manager, unsigned int DELAY>
class FragManagerFilter : public Manager<N> {
  typedef Manager<N> Super;
public:
  static FragManagerFilter<N,Manager,DELAY> * getInstance() {
    static FragManagerFilter<N,Manager,DELAY> _instance;
    return &_instance;
  }

  void registerBlock(typename FragManager<N>::Node * bl) {
    FragManager<N>::registerBlock(bl);
    while(FragManager<N>::getSize() > DELAY) {
      typename FragManager<N>::Node * rem = FragManager<N>::tail;
      removeBlock(FragManager<N>::tail);
      Super::registerBlock(rem);
    }
  }

  void removeBlock(typename FragManager<N>::Node * bl) {
    if(bl->list() == this) {
      FragManager<N>::removeBlock(bl);
    } else {
      Super::removeBlock(bl);
    }
  }

  void clearSlot(AOMergeable<N> * ob, unsigned int slot) {
    // skip
  }

private:
  Manager<N> _subManager;
};

#endif
