#include "mmapwrapper.h"

#include "AOChunk.hpp"
#include "block_factory.hpp"
#include "global_metadata_heap.hpp"

AOChunk *AOChunk::_liveChunks[1ul << (32 - LOG_VALLOC_SIZE)];

AOChunk *AOChunk::fromPtr(LPCVOID ptr) {
  AOChunk *bl = _liveChunks[(DWORD)ptr >> LOG_VALLOC_SIZE];

  return bl;
}

void *AOChunk::operator new(size_t sz) {
  return GlobalMetadataHeap::getInstance()->New(sz);
}

void AOChunk::operator delete(void *ptr) {
  GlobalMetadataHeap::getInstance()->Delete(ptr);
}

AOChunk::AOChunk(size_t sz, size_t grain) : _sz(sz), _grain(grain), _numelts(sz / grain), _pop(0), _ptr(0) {
  //_start = (DWORD)VirtualAlloc(NULL,
  //		  	 		  sz,
  //			   		  MEM_RESERVE | MEM_COMMIT,
  //					  PAGE_READWRITE);

  // this code isn't used right?
  abort();

  _start = (DWORD)HL::MmapWrapper::map(sz);
  fprintf(stderr, "new chusdfnk: 0x%x, size %d\n", _start, sz);

  assert(_numelts <= 32);

  _elts = (AOCommon **)GlobalMetadataHeap::getInstance()->New(sizeof(PVOID) * _numelts, NULL, true);

  size_t chunkSpan = ((sz + VALLOC_GRAIN - 1) / VALLOC_GRAIN);

  for (UINT i = 0; i < chunkSpan; i++) {
    _liveChunks[((DWORD)_start >> LOG_VALLOC_SIZE) + i] = this;
  }
}

AOChunk::~AOChunk() {
  GlobalMetadataHeap::getInstance()->Delete(_elts);

  assert(_liveChunks[(DWORD)_start >> LOG_VALLOC_SIZE]);

  size_t chunkSpan = ((_sz + VALLOC_GRAIN - 1) / VALLOC_GRAIN);

  for (UINT i = 0; i < chunkSpan; i++) {
    _liveChunks[((DWORD)_start >> LOG_VALLOC_SIZE) + i] = NULL;
  }

  HL::MmapWrapper::unmap((PVOID)_start, _sz);

  // VirtualFree((LPVOID)_start,0,MEM_RELEASE);
}

PVOID AOChunk::New(AOCommon *bl) {
  DWORD ret;

  assert(_pop < _numelts);

  while (_used.test(_ptr)) {
    _ptr++;
    _ptr %= _numelts;
  }

  _pop++;
  ret = _start + _grain * _ptr;
  _elts[_ptr] = bl;

  _used.set(_ptr);

  if (_pop == _numelts && _numelts > 1) {
    // full, so remove from free list in block factory
    BlockFactory::getInstance()->RemoveChunkFromList(this);
  }

  return (PVOID)ret;
}

size_t AOChunk::getIndex(LPCVOID ptr) {
  char *cptr = (char *)ptr;

  cptr -= _start;

  size_t idx = (size_t)cptr / _grain;
  assert(idx >= 0);
  assert(idx < _numelts);
  assert(_used.test(idx));

  return idx;
}

BOOL AOChunk::Delete(PVOID ptr) {
  size_t idx = getIndex(ptr);

  _used.reset(idx);
  _elts[idx] = NULL;

  if (_pop == _numelts && _numelts > 1) {
    BlockFactory::getInstance()->AddChunkToList(this);
  }

  _pop--;

  if (_pop == 0) {
    if (_numelts > 1)
      BlockFactory::getInstance()->RemoveChunkFromList(this);
    delete this;
  }

  return true;
}

AOCommon *AOChunk::ptrToBlock(LPCVOID ptr) {
  size_t idx = getIndex(ptr);

  return _elts[idx];
}
