#include "platform.hpp"

#include <new>
#include "AOHeap.hpp"
#include "AOBlock.hpp"
#include "AOLargeObject.hpp"
#include "AgingBlock.hpp"
#include "FreshBlockFactory.hpp"
#include "CallStack.hpp"
#include "callsite.hpp"
#include "output.hpp"
#include "log2.h"

static void xmlify(char * dst, size_t sz, char * src) {
	// number of chars written
	size_t wr = 0;

	while(wr < sz) {
		if(0 == *src)
			break;
		else if('<' == *src) {
			if(wr+4 > sz) break;
			else {
				dst[wr++] = '&';
				dst[wr++] = 'l';
				dst[wr++] = 't';
				dst[wr++] = ';';
			}
		}
		else if('>' == *src) {
			if(wr+4 > sz) break;
			else {
				dst[wr++] = '&';
				dst[wr++] = 'g';
				dst[wr++] = 't';
				dst[wr++] = ';';
			}
		}
		else dst[wr++] = *src;

		src++;
	}

	dst[wr++] = 0;
}
