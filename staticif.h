// -*- C++ -*-

/**
 * @file   staticif.h
 * @brief  Statically returns a value based on a conditional.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @note   Copyright (C) 2005 by Emery Berger, University of Massachusetts Amherst.
 */

#ifndef HOUND_STATICIF_H
#define HOUND_STATICIF_H

template <bool b, int a, int c>
class StaticIf;

template <int a, int b>
class StaticIf<true, a, b> {
 public:
  enum { VALUE = a };
};

template <int a, int b>
class StaticIf<false, a, b> {
 public:
  enum { VALUE = b };
};


#endif
