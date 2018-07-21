#include "platform.hpp"

#ifdef _WIN32
#include <psapi.h>
#endif

#include "constants.h"
#include "output.hpp"
#include "plugheap.hpp"

#include "traphandler.h"

#include <assert.h>

#define ADD_HEADER 1

// for bookkeeping from AOMergedBlock
int saved_pages = 0;
int _mergedblocks = 0;

// XXX: This heap may misbehave if multiple instances of it are created (not tested)

static CALL_STACK cs;

DECLSPEC void Pin(LPCVOID ptr) {
  AOCommon *bl = AOCommon::fromPtr(ptr);
  if (!bl)
    return;

  bl->pin();
}

DECLSPEC void Unpin(LPCVOID ptr) {
  AOCommon *bl = AOCommon::fromPtr(ptr);
  if (!bl)
    return;

  bl->unpin();
}
