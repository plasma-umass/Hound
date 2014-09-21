// Compute memory usage by the heap (roughly) by shimming
// mmap and sbrk
// XXX REQUIRES 4K PAGE SIZE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <new>

#include "myhashmap.h"
#include "bumpalloc.h"
#include "freelistheap.h"

//#define TRACE_MALLOC_INLINE
//#define TRACE_MALLOC_MAP

static int __mapped = 0;
static int __highwater = 0;
static bool __initializing = false;
static char _exec_filename[256] = "";

static int __alloced = 0;
static int __inspecial = 0;

// make these ints to prevent races
static int __aliased[1024*1024];
static int __tracked[1024*1024];

static size_t __pgsAliased = 0;

extern
int saved_pages;

extern int total_pages;

struct szinfo {
	size_t sz;
	size_t root_offset;
};

static void * _ptradd (void * p, int sz) {
	if (p == 0) return 0;
	return (void *)((char *)p + sz);
}

static int _szadd (int sz, int ad) {
	if (sz == 0) return 0;
	return sz + ad;
}

static void _setszinfo (void * p, size_t sz, size_t offset) {
	if (p == 0) return;
	((szinfo *)p)->sz = sz;
	((szinfo *)p)->root_offset = offset;
}

extern "C" {
  typedef void * (mmap_function_t)   (void *, size_t, int, int, int, off_t);
  typedef int    (munmap_function_t) (void *, size_t);
  typedef int    (brk_function_t)    (void *);
  typedef void * (sbrk_function_t)   (intptr_t);
  typedef void * (calloc_function_t) (size_t,size_t);
  typedef void * (malloc_function_t) (size_t);;
  typedef void * (realloc_function_t) (void *, size_t);
  typedef void	 (free_function_t)	 (void *);
  // NB: THIS IS THE 5-ARGUMENT "SECRET" mremap
  typedef void * (mremap_function_t) (void *, size_t, size_t, int, void *);

  typedef int	 (posix_memalign_function_t) (void ** memptr, size_t alignment, size_t size);

  typedef int	 (pthread_create_function_t) (pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
  typedef void	 (pthread_exit_function_t) (void *);


  mmap_function_t * real_mmap = 0;
  munmap_function_t * real_munmap = 0;
  brk_function_t * real_brk = 0;
  sbrk_function_t * real_sbrk = 0;
  calloc_function_t * real_calloc = 0;
  malloc_function_t * real_malloc = 0;
  realloc_function_t * real_realloc = 0;
  free_function_t * real_free = 0;
  mremap_function_t * real_mremap = 0;

  posix_memalign_function_t * real_posix_memalign = 0;

  pthread_create_function_t * real_pthread_create = 0;
  pthread_exit_function_t * real_pthread_exit = 0;
}

/**
 * Map for keeping track of object sizes -- for tracking malloc size
 * without needing to perturb the allocator we're testing.
 */

using namespace HL;

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20

class MmapAlloc {
public:
  static void * malloc (size_t sz) {
    void * ptr = real_mmap(0, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return ptr;
  }
  static void free (void *) {}
};

class SourceMapHeap : public HL::FreelistHeap<BumpAlloc<65536, MmapAlloc> > { };

// The map type, with all the pieces in place.
typedef MyHashMap<void *, size_t, SourceMapHeap> mapType;

class MallocMap {
public:

	static mapType* get () {
		static mapType _mm;
		return &_mm;
	}
};

/**
 * End of Map Setup
 */

extern "C" {

  void reportStats(void) {
    fprintf(stderr,"Total mapped memory: %d\n",__mapped);
  }

  static void check_highwater() {
    size_t realMem = __mapped - saved_pages*4096;
    if(realMem > __highwater) {
      __highwater = realMem;

      if(__highwater >= 2000000000) abort();
      fprintf(stderr,"(%s) NEW HIGH WATER: %dK (saved %d/%d)\n",_exec_filename,__highwater / 1024, saved_pages, total_pages);
#if defined(TRACE_MALLOC_INLINE) || defined(TRACE_MALLOC_MAP)
      fprintf(stderr,"(%s) CURRENT ALLOCED: %dK\n",_exec_filename,__alloced / 1024);
#endif
    }
  }

  static void init(void) {
    __initializing = true;
    real_calloc = (calloc_function_t *)dlsym(RTLD_NEXT,"calloc");
    real_malloc = (malloc_function_t *)dlsym(RTLD_NEXT,"malloc");
    real_realloc = (realloc_function_t *)dlsym(RTLD_NEXT,"realloc");
    real_free = (free_function_t *)dlsym(RTLD_NEXT,"free");
    real_sbrk = (sbrk_function_t *)dlsym(RTLD_NEXT,"sbrk");
    real_mmap = (mmap_function_t *)dlsym(RTLD_NEXT,"mmap"); 
    real_munmap = (munmap_function_t *)dlsym(RTLD_NEXT,"munmap");
    real_brk = (brk_function_t *)dlsym(RTLD_NEXT,"brk");
    real_mremap = (mremap_function_t *)dlsym(RTLD_NEXT,"mremap");

    real_posix_memalign = (posix_memalign_function_t *)dlsym(RTLD_NEXT,"posix_memalign");

    real_pthread_create = (pthread_create_function_t *)dlsym(RTLD_NEXT,"pthread_create");

    int count;
    count=readlink("/proc/self/exe", _exec_filename, 256);
    if (count <= 0 || count >= 256)
      {
        printf( "Failed to get current executable file name\n" );
        exit(1);
      }
    _exec_filename[count] = '\0';

    __initializing = false;
  }

  void *mmap(void *addr, size_t len, int prot, int flags,
              int fildes, off_t off)
  {
    if(real_mmap == 0) {
      real_mmap = (mmap_function_t *)dlsym(RTLD_NEXT,"mmap");
      if(real_mmap == 0) {
        fputs("Could not find mmap, which means you suck.\n",stderr);
        abort();
      }

      atexit(reportStats);
    }
    void * ret = real_mmap(addr,len,prot,flags,fildes,off);

    //if (fildes <= 0) {
    if (!__inspecial && fildes <= 0) {
      // Fix tracking problem with VC. Gn 1/19/10
      char * ptr = static_cast<char*>(ret);
      while(ptr < static_cast<char*>(ret)+len) {
    	__aliased[(unsigned long)ptr>>12] = 0;
    	__tracked[(unsigned long)ptr>>12] = 1;
	ptr += 4096;
      }

    	__mapped += (len+4095) & ~(4096-1);
    	check_highwater();

    	//volatile void * retaddr = __builtin_return_address(0);
    	//fprintf(stderr,"mapped from %p\n",retaddr);
    }

    //fprintf(stderr,"(mmap) Mapped %d\n",len);

    return ret;
  }

  int munmap(void *addr, size_t len) {
    if(real_munmap == 0) {
      init();
      if(real_munmap == 0) {
        fputs("Could not find munmap, blah blah blah, fail.\n",stderr);
        abort();
      }
    }
    
    //fprintf(stderr, "(munmap %p) tracked %d aliased %d inspecial %d\n", addr, __tracked[(unsigned long)addr>>12], __aliased[(unsigned long)addr>>12], __inspecial);

    if(__tracked[(unsigned long)addr>>12] && !__inspecial) {
      __mapped -= (len+4095) & ~(4096-1);
      __tracked[(unsigned long)addr>>12] = 0;
      //fprintf(stderr, "(munmap %p) saved %d vs %d\n", addr, __pgsAliased, saved_pages);
    }

	//fprintf(stderr,"(munmap) Unmapped %d\n",len);

    return real_munmap(addr,len);
  }

  // This is only a correct behavior for Hound's page compaction!
  void *mremap(void * targ, size_t osz, size_t nsz, int flags, void * old) {
    if(real_mremap == 0) {
      init();
    }

    // If we're doing the page alias goo
    if(osz == 0 && !__aliased[(unsigned long)old>>12] && __tracked[(unsigned long)old>>12]) {
      __aliased[(unsigned long)old>>12] = 1;
    }

    //fprintf(stderr, "(mremap) saved %d vs %d\n", __pgsAliased, saved_pages+1);

    return real_mremap(targ,osz,nsz,flags,old);
  }

  int brk(void *end) {
    void *orig;

    if(real_brk == 0) {
      init();
    }

    orig = sbrk(0);
    int len = ((char*)end-(char*)orig);
    __mapped += len;

    if (len > 4096) {
    	volatile int i = len;
    }

    //fprintf(stderr,"(brk) Mapped %d\n",len);

    check_highwater();

    return real_brk(end);
  }

  void *sbrk(intptr_t inc) {
    if(real_sbrk == 0) {
      init();
    }

    __mapped += inc;
    check_highwater();

    if (inc > 4096) {
		volatile int i = inc;
	}

    if(inc) {
      fprintf(stderr,"(sbrk) Mapped %d\n",inc);
    }

    return real_sbrk(inc);
  }

  void * __sbrk(intptr_t inc) {
    return sbrk(inc);
  }

  void * calloc(size_t ct, size_t sz) {
    if(__initializing) return NULL;

    if(real_calloc == 0) {
      init();
    }
    
#ifdef TRACE_MALLOC_INLINE
    //void * ptr = real_calloc(ct, sz);
    void * ptr = real_malloc(_szadd(sz * ct, sizeof(szinfo)));
    bzero(ptr, _szadd(sz * ct, sizeof(szinfo)));

    _setszinfo(ptr, ct * sz, sizeof(szinfo));
    __alloced += ct * sz;

    return _ptradd(ptr, sizeof(szinfo));
#elif defined(TRACE_MALLOC_MAP)
    void * ptr = real_calloc(ct, sz);
    if (ptr) {
    	MallocMap::get()->set(ptr, ct * sz);
    	__alloced += ct * sz;
      //fprintf(stderr,"aliased page %p\n",old);
    } else if(osz > 0) {
      __mapped -= osz;
      __mapped += nsz;
      check_highwater();
    }
    return ptr;
#else
    return real_calloc(ct, sz);
#endif
  }

  void * malloc(size_t sz) {
	  if(__initializing) return NULL;

	  if(real_malloc == 0) {
		init();
	  }

#ifdef TRACE_MALLOC_INLINE
	  void * ptr = real_malloc(_szadd(sz, sizeof(szinfo)));

	  _setszinfo(ptr, sz, sizeof(szinfo));
	  __alloced += sz;

	  //fprintf(stderr, "Malloc %08x (%d)\n", _ptradd(ptr, sizeof(szinfo)), sz);

	  return _ptradd(ptr, sizeof(szinfo));
#elif defined(TRACE_MALLOC_MAP)
	  void * ptr = real_malloc(sz);
	  if (ptr) {
		  MallocMap::get()->set(ptr, sz);
		  __alloced += sz;
	  }
	  return ptr;
#else
	  return real_malloc(sz);
#endif
  }

  void * realloc(void * optr, size_t sz) {
	  if (__initializing) return NULL;

	  if (real_realloc == 0) {
		  init();
	  }

#ifdef TRACE_MALLOC_INLINE
	  // Malloc
	  if (!optr) {
		  return malloc(sz);
	  }

	  // Free
	  if (!sz) {
		  free(optr);
		  return 0;
	  }

	  // Realloc
	  size_t old_size = ((szinfo *)_ptradd(optr, -sizeof(szinfo)))->sz;

	  void * ptr = malloc(sz);

	  memcpy(ptr, optr, (old_size < sz) ? old_size : sz);

	  free(optr);

	  return ptr;
#elif defined(TRACE_MALLOC_MAP)
	  if (!optr) {
		  return malloc(sz);
	  }
	  if (!sz) {
		  free(optr);
		  return 0;
	  }
	  size_t old_size = MallocMap::get()->get(optr);
	  //MallocMap::get()->erase(optr);

	  void * ptr = malloc(sz);
	  memcpy(ptr, optr, (old_size < sz) ? old_size : sz);

	  free(optr);

	  return ptr;
#else
	  return real_realloc(optr, sz);
#endif
  }

  void free(void* ptr) {
	  if (real_free == 0) {
		  init();
	  }

#ifdef TRACE_MALLOC_INLINE
	  if (ptr == 0) return;
	  szinfo* si = (szinfo *)_ptradd(ptr, -sizeof(szinfo));

	  __alloced -= si->sz;

	  //fprintf(stderr, "Free %08x\n", ptr);

	  real_free(_ptradd(ptr, -si->root_offset));
#elif defined(TRACE_MALLOC_MAP)
	  if (ptr == 0) return;
	  __alloced -= MallocMap::get()->get(ptr);
	  MallocMap::get()->erase(ptr);
	  real_free(ptr);
#else
	  real_free(ptr);
#endif
  }

  int posix_memalign(void **memptr, size_t alignment, size_t size) {
	  if (__initializing) return NULL;

	  if (real_posix_memalign == 0) {
		  init();
	  }

#ifdef TRACE_MALLOC_INLINE
	  int ret = real_posix_memalign(memptr, alignment, _szadd(size, alignment));
	  *memptr = _ptradd(*memptr, alignment);

	  //fprintf(stderr, "Posix_Memalign %08x (%d)\n", *memptr, size);

	  _setszinfo(_ptradd(*memptr, -sizeof(szinfo)), size, alignment);
	  __alloced += size;

	  return ret;
#elif defined(TRACE_MALLOC_MAP)
	  int ret = real_posix_memalign(memptr, alignment, _szadd(size, alignment));
	  if (*memptr) {
		  MallocMap::get()->set(*memptr, size);
		  __alloced += size;
	  }
	  return ret;
#else
	  return real_posix_memalign(memptr, alignment, size);
#endif
  }

  int pthread_create (pthread_t * thread, const pthread_attr_t * attr, void *(* sr)(void *), void * arg) {
	  if (real_pthread_create == 0) {
		  init();
	  }

	  __inspecial++;
	  int ret = real_pthread_create(thread, attr, sr, arg);
	  __inspecial--;

	  return ret;
  }

  /*void pthread_exit (void * value_ptr) __attribute((noreturn));
  void pthread_exit (void * value_ptr) {
	  if (real_pthread_exit == 0) {
		  init();
	  }

	  __inspecial++;
	  real_pthread_exit(value_ptr);
	  __inspecial--;
  }*/
}
