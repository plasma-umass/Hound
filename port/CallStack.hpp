#ifndef HOUND_CALLSTACK_H
#define HOUND_CALLSTACK_H

#include "../platform.hpp"
#include "capture_callsite.h"

class CALL_STACK {
public:
  void FormatCallStack(char *buf, PVOID *functions, size_t buf_sz, int depth);
};

#endif  // __CALLSTACK_H__
