#ifndef AO_CHUNK_H__
#define AO_CHUNK_H__

#include "platform.hpp"
#include <assert.h>
#include <bitset>

#include "AOCommon.hpp"

/**
 * An AOChunk is the ``grain'' of allocation from the OS.
 * An AOChunk may be either an AOBlock (bump pointer space),
 * an AOLOChunk (carved into large objects), or
 * an AOBigChunk (a single large object spanning several AOChunk grains)
 */

class AOChunk {
public:
	AOChunk(size_t sz, size_t grain);
	~AOChunk();

	void * operator new(size_t sz);
	void operator delete(void * ptr);

	static AOChunk * fromPtr(LPCVOID);

	PVOID New(AOCommon * bl);
	BOOL Delete(PVOID);

	AOCommon * ptrToBlock(LPCVOID);

	size_t getGrain() const { return _grain; }
	size_t getSize() const { return _sz; }

private:
	size_t getIndex(LPCVOID);

	size_t _sz;
	DWORD _start;
	size_t _grain;
	size_t _numelts;
	AOCommon ** _elts;

	size_t _pop;
	size_t _ptr;

	AOChunk * _next;
	AOChunk * _prev;

	std::bitset<32> _used;

	static AOChunk * _liveChunks[1ul<<(32-LOG_VALLOC_SIZE)];

	friend class BlockFactory;
};

#endif
