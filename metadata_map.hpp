#ifndef HOUND_METADATA_MAP
#define HOUND_METADATA_MAP

#include "platform.hpp"
#include "site.hpp"

#include "global_metadata_heap.hpp"

template <class PerCallsiteHeap>
class MetadataMap {
  typedef unsigned long KEY_TYPE;
  typedef Site<PerCallsiteHeap> VALUE_TYPE;

public:
  MetadataMap() : _entries((VALUE_TYPE *)__init_buf), _map_size(INIT_SIZE), _mask(INIT_SIZE - 1) {
  }

  ~MetadataMap() {
    for (UINT i = 0; i < _map_size; i++) {
      if (_entries[i].isValid())
        _entries[i].~Site<PerCallsiteHeap>();
    }
  }

  void doFree(KEY_TYPE key) {
    // XXX: must be in map, but need to error check NULL
    VALUE_TYPE *s = findOrInsertSite(key, NULL);
    if (s)
      s->doFree();
  }

  VALUE_TYPE *getSite(KEY_TYPE key, PVOID *frames) {
    VALUE_TYPE *s = findOrInsertSite(key, frames);
    return s;
  }

  void triage() {
    for (UINT i = 0; i < _map_size; i++) {
      if (_entries[i].isValid())
        _entries[i].triage();
    }
  }

  void reportStats() {
    fprintf(stderr, "map atexit\n");
    for (UINT i = 0; i < _map_size; i++) {
      if (_entries[i].isValid()) {
        _entries[i].reportStats();
      }
    }
  }

private:
  void grow() {
    // OutputDebugString("GROWING HASHTABLE***********************\n");

    // fprintf(stderr,"old map size: %d\n",_num_elts);

    VALUE_TYPE *old_entries = _entries;
    size_t old_map_size = _map_size;

    _entries = (VALUE_TYPE *)GlobalMetadataHeap::getInstance()->New(_map_size * 2 * sizeof(VALUE_TYPE), NULL, true);
    _map_size *= 2;
    _mask = _map_size - 1;

    // rehash

    UINT ct = 0;

    for (UINT i = 0; i < old_map_size; i++) {
      if (old_entries[i].isValid()) {
        ct++;
        insertNewSite(old_entries[i]);
      }
    }

    // fprintf(stderr,"new map size: %d\n",_num_elts);

    assert(ct == _num_elts);

    if ((void *)old_entries != (void *)__init_buf)
      GlobalMetadataHeap::getInstance()->Delete(old_entries);
  }

  /** Inserts the given site into the map.
   *  Precondition: no site with a matching hash value is in the map.
   */
  // XXX: change to bitwise operations for speed
  void insertNewSite(const VALUE_TYPE &s) {
    int begin = s.getHashCode() & _mask;
    int lim = (begin - 1 + _map_size) & _mask;

    // NB: we don't check slot lim, but we're actually guaranteed never to get
    // there since the load factor can't be 1.0
    for (int i = begin; i != lim; i = (i + 1) & _mask) {
      if (_entries[i].isValid()) {
        //
        continue;
      } else {
        // invoke copy constructor via placement new
        new (&_entries[i]) VALUE_TYPE(s);
        return;
      }
    }

    assert(false);
  }

  VALUE_TYPE *findOrInsertSite(KEY_TYPE key, VOID **frames) {
    int begin = key & _mask;
    int lim = (begin - 1 + _map_size) & _mask;

    int probes = 0;

    for (int i = begin; i != lim; i = (i + 1) & _mask) {
      probes++;
      if (probes == 10) {
        // XXX: fix hash function to lower clustering?
        // printf("probed a lot of times\n");
      }
      if (_entries[i].isValid()) {
        if (_entries[i].getHashCode() == key) {
          return &_entries[i];
        } else {
          continue;
        }
      } else {
        if (frames == NULL) {
          // freeing a weird object (we didn't alloc it)
          // just abort this op
          return NULL;
        }
        if (_num_elts + 1 > 0.5 * _map_size) {
          grow();
          return findOrInsertSite(key, frames);
        } else {
          _num_elts++;
          if (!(_num_elts % 100)) {
            char buf[80];
            sprintf(buf, "now %d active callsites\n", _num_elts);
            // OutputDebugString(buf);
          }
          return new (&_entries[i]) VALUE_TYPE(key, frames);
        }
      }
    }

    // path cannot be reached---must find empty bin since load factor < 1.0
    printf("failing to find key 0x%x\n", key);

    abort();
    return NULL;
  }

  static const int INIT_SIZE = 64;
  VALUE_TYPE *_entries;
  size_t _map_size;
  size_t _mask;
  size_t _num_elts;

  char __init_buf[INIT_SIZE * sizeof(VALUE_TYPE)];
};

#endif  // __METADATA_MAP__
