#include "block_factory.hpp"
#include "log2.h"

BlockFactory * BlockFactory::getInstance() {
	static BlockFactory factory;
	return &factory;
}

BlockFactory::BlockFactory() {
	for(int i = 0; i < NUM_CLASSES; i++) {
		_head[i] = NULL;
	}
}

VOID BlockFactory::refill(UINT idx) {
	size_t sz = (1ul << (idx+LOG_AOCHUNK_MIN_ALLOC));

	AOChunk * chunk = new AOChunk(VALLOC_SIZE,sz);

	AddChunkToList(chunk);
}

PVOID BlockFactory::New(size_t sz, AOCommon * bl) {
	UINT idx = log2(sz)-StaticLog<AOCHUNK_MIN_ALLOC>::VALUE;

	if(idx >= NUM_CLASSES) {
		// (Very) Large Object request
		AOChunk * chunk = new AOChunk(sz,sz);
		return chunk->New(bl);
	}

	if(idx < 0) idx = 0;

	if(_head[idx] == NULL) {
		refill(idx);
	}

	return _head[idx]->New(bl);
}

BOOL BlockFactory::Delete(PVOID bl) {
	return AOChunk::fromPtr(bl)->Delete(bl);	
}

VOID BlockFactory::RemoveChunkFromList(AOChunk * chunk) {
	// NB: This only removes the chunk from the list of available chunks.
	// It specifically does not free any memory associated with the chunk metadata
	// or the backing pages themselves.
	UINT idx = log2(chunk->getGrain())-StaticLog<AOCHUNK_MIN_ALLOC>::VALUE;

	/*char buf[256];
	sprintf(buf,"removing chunk 0x%x, head 0x%x\n",chunk,_head[idx]);
	OutputDebugString(buf);
*/
	if(_head[idx] == chunk) {
		_head[idx] = _head[idx]->_next;
		if(_head[idx])
			_head[idx]->_prev = NULL;
	} else {
		chunk->_prev->_next = chunk->_next;
		if(chunk->_next)
			chunk->_next->_prev = chunk->_prev;
	}
}

inline VOID BlockFactory::AddChunkToList(AOChunk * chunk) {
	UINT idx = log2(chunk->getGrain())-StaticLog<AOCHUNK_MIN_ALLOC>::VALUE;
/*
	char buf[256];
	sprintf(buf,"adding chunk 0x%x, head 0x%x\n",chunk,_head[idx]);
	OutputDebugString(buf);
*/
	chunk->_prev = NULL;
	chunk->_next = _head[idx];
	if(_head[idx])
		_head[idx]->_prev = chunk;

	_head[idx] = chunk;
}
