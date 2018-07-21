#ifndef HOUND_RECYCLING_BLOCK_FACTORY
#define HOUND_RECYCLING_BLOCK_FACTORY

template <template <size_t N> class BlockType>
class RecyclingBlockFactory {
private:
  template <size_t NUM_SLOTS>
  static AOCommon *allocBlock() {
    AOMergeable<NUM_SLOTS> *target = FragManagerType<NUM_SLOTS>::Type::getInstance()->popSparseBlock();
    if (target) {
      BlockType<NUM_SLOTS> *bl = new BlockType<NUM_SLOTS>();
      // fprintf(stderr,"bl has %d, target has %d\n",bl->getBitmap().count(),target->getBitmap().count());
      bl->merge(target);
      // fprintf(stderr,"allocated recycled block: %p target %p\n",static_cast<AOMergeable<NUM_SLOTS> *>(bl), target);
      return bl;
      // return 0;
    } else {
      return 0;
    }
  }

#define NUM_SLOTS(N) (AO_BLOCK_SIZE / (1 << (N + LOG_MIN_ALLOC)))

#define RE_ALLOC_CASE(N)                  \
  case N:                                 \
    bl = allocBlock<NUM_SLOTS(N)>();      \
    if (!bl)                              \
      bl = new BlockType<NUM_SLOTS(N)>(); \
    return bl;

public:
  static AOCommon *allocBlock(int sc) {
    AOCommon *bl = NULL;

    switch (sc) {
      RE_ALLOC_CASE(0);
      RE_ALLOC_CASE(1);
      RE_ALLOC_CASE(2);
      RE_ALLOC_CASE(3);
      RE_ALLOC_CASE(4);
      RE_ALLOC_CASE(5);
      RE_ALLOC_CASE(6);
      RE_ALLOC_CASE(7);
    default:
      break;
    }
  }
};

#endif
