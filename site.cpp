#include <cmath>
#include "site.hpp"
#include "output.hpp"

/*
bool Site::operator==(const Site & rhs) {
	if(this._cs_hash != rhs._cs_hash) {
		return false;
	}

#ifdef VERIFY_HASH_FUNCTION
	for(int i = 0; i < NUM_STACK_FRAMES; i++) {
		if(_cs_frames[i] != rhs._cs_frames[i]) {
			fprintf(stderr,"GOT TWO COLLIDING CALLSITES\n");
			return false;
			// XXX: add code to print conflicting stacks here
		}
	}
#endif
	
	// if we're not verifying, then trust the hashes to disambiguate
	return true;
}
*/
