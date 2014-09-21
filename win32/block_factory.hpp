#include "AOHeap.hpp"
#include "AOChunk.hpp"

class BlockFactory {
public:
	static BlockFactory * getInstance();

	BlockFactory();
	PVOID New(size_t sz, AOCommon * bl);
	BOOL Delete(PVOID block);

	VOID RemoveChunkFromList(AOChunk * chunk);
	VOID AddChunkToList(AOChunk * chunk);

private:
	VOID refill(UINT);
	
	static const size_t NUM_CLASSES = StaticLog<VALLOC_SIZE>::VALUE-StaticLog<AOCHUNK_MIN_ALLOC>::VALUE;

	AOChunk * _head[NUM_CLASSES];
};
