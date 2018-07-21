// -*- C++ -*-

/**
 * @file   callsite.h
 * @brief  All the information needed to manage one partition (size class).
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @note   Copyright (C) 2005 by Emery Berger, University of Massachusetts Amherst.
 */

#ifndef HOUND_CAPTURE_CALLSITE_H
#define HOUND_CAPTURE_CALLSITE_H

#include <link.h>
#include <setjmp.h>
#include <stdio.h>
#include <cassert>

extern void *dl_base;
extern bool __computing_callsite;
extern sigjmp_buf __backtrace_jump;

static inline unsigned long normalize(unsigned long pc) {
  if (pc < (unsigned long)dl_base)
    return pc;
  else
    return pc - (unsigned long)dl_base;
}

unsigned short get_callsite(int skip, int STACK_DEPTH, void **BackTrace, unsigned long *BackTraceHash);

int dumpHeapDLCallback(struct dl_phdr_info *info, size_t size, void *data);
void getDLBase();

#endif  // _CALLSITE_H_
