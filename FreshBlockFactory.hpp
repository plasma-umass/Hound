#ifndef __FRESH_BLOCK_FACTORY_H__
#define __FRESH_BLOCK_FACTORY_H__

#include "AOBlock.hpp"

#include "AgingBlock.hpp"

// Allocates fresh AOBlocks (i.e. fresh, unaliased page)

template<template<size_t N> class BlockType>
class FreshBlockFactory {
#define ALLOC_CASE(N) case N : bl = new BlockType< AO_BLOCK_SIZE / (1<<(N+LOG_MIN_ALLOC) ) >(); return bl;

public:
  static AOCommon * allocBlock(int sc) {
    AOCommon *bl = NULL;

    switch(sc) {
      ALLOC_CASE(0);
      ALLOC_CASE(1);
      ALLOC_CASE(2);
      ALLOC_CASE(3);
      ALLOC_CASE(4);
      ALLOC_CASE(5);
      ALLOC_CASE(6);
      ALLOC_CASE(7);
    default:
      break;
    } 
  }
};

#endif // __FRESH_BLOCK_FACTORY_H__
