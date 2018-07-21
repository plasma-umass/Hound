#include "platform.hpp"

#include "config.h"

#include "AOMergedBlock.hpp"

#include <new>
#include "AOBlock.hpp"
#include "AOHeap.hpp"
#include "AOLargeObject.hpp"
#include "AgingBlock.hpp"
#include "CallStack.hpp"
#include "FreshBlockFactory.hpp"
#include "callsite.hpp"
#include "log2.h"
#include "output.hpp"

static void xmlify(char *dst, size_t sz, char *src) {
  // number of chars written
  size_t wr = 0;

  while (wr < sz) {
    if (0 == *src)
      break;
    else if ('<' == *src) {
      if (wr + 4 > sz)
        break;
      else {
        dst[wr++] = '&';
        dst[wr++] = 'l';
        dst[wr++] = 't';
        dst[wr++] = ';';
      }
    } else if ('>' == *src) {
      if (wr + 4 > sz)
        break;
      else {
        dst[wr++] = '&';
        dst[wr++] = 'g';
        dst[wr++] = 't';
        dst[wr++] = ';';
      }
    } else
      dst[wr++] = *src;

    src++;
  }

  dst[wr++] = 0;
}
