// -*- C++ -*-

/**
 * @file   callsite.h
 * @brief  All the information needed to manage one partition (size class).
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @note   Copyright (C) 2005 by Emery Berger, University of Massachusetts Amherst.
 */

#ifndef _CALLSITE_H_
#define _CALLSITE_H_

#include <link.h>
#include <stdio.h>
#include <cassert>
#include <setjmp.h>

extern void * dl_base;
extern bool __computing_callsite;
extern sigjmp_buf __backtrace_jump;

static inline unsigned long normalize(unsigned long pc) {
  if(pc < (unsigned long)dl_base) return pc;
  else return pc - (unsigned long)dl_base;
}

unsigned short get_callsite(int skip,int STACK_DEPTH,void ** BackTrace,unsigned long *BackTraceHash);

int dumpHeapDLCallback(struct dl_phdr_info *info, size_t size, void * data);
void getDLBase();

#endif // _CALLSITE_H_
