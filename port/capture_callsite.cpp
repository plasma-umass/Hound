#include "capture_callsite.h"

void * dl_base;
sigjmp_buf __backtrace_jump;
bool __computing_callsite;

int dumpHeapDLCallback(struct dl_phdr_info *info, size_t size, void * data) {
  Elf32_Addr *dl_addr = static_cast<Elf32_Addr *>(data);

  void * info_addr = (void*)info->dlpi_addr;

  if(info->dlpi_addr != 0) {
    if(info->dlpi_addr < *dl_addr) {
      *dl_addr = info->dlpi_addr;
    }
  }
  return 0;
}

/*
int dumpHeapEECallback(struct dl_phdr_info *info, size_t size, void * data) {
  Elf32_Addr *dl_addr = static_cast<Elf32_Addr *>(data);

  void * info_addr = (void*)info->dlpi_addr;

  if(info->dlpi_addr != 0) {
    if(info->dlpi_addr < *dl_addr && info->dlpi_addr > (Elf32_Addr)&__LIBEXTERMINATOR_END) {
      *dl_addr = info->dlpi_addr;
    }
  }
  return 0;
}
*/

void getDLBase() {
  dl_base = (void *)0xffffffff;
  dl_iterate_phdr(dumpHeapDLCallback,&dl_base);
}

static unsigned long ret;

// STATIC has skip 2
// ADAPTIVE has skip 3
unsigned short get_callsite(int skip,int STACK_DEPTH,void ** BackTrace,unsigned long *BackTraceHash) {  
  ret = 5381; 

#define RA(a) \
__builtin_frame_address(a) ? /*fprintf(stderr,"%d: 0x%x\n", a, __builtin_return_address(a)),*/ (unsigned long)__builtin_return_address(a) : 0UL;
  
  unsigned long tmp;
  int frames = 0;
  
  __computing_callsite = true;

  // setjmp should be OK here because SEGV can't be masked.
  // And sigsetjmp requires a kernel crossing (?) and is fscking expensive in any case.
  //if(sigsetjmp(__backtrace_jump,true) == 0) {
  if(setjmp(__backtrace_jump) == 0) {
    // XXX 1 for non-injected runs
    switch(skip+1) {
    case 0:
      tmp = RA(0);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 1:
      tmp = RA(1);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 2:
      tmp = RA(2);
      //fprintf(stderr,"tmp = 0x%x\n",tmp);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp; 
       BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 3:
      tmp = RA(3);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp; 
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 4:
      tmp = RA(4);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 5:
      tmp = RA(5);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 6:
      tmp = RA(6);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 7:
      tmp = RA(7);
      if(tmp == 0) break;
      tmp = normalize(tmp);
       ret = ((ret << 5) + ret) + tmp;
       BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 8:
      tmp = RA(8);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 9:
      tmp = RA(9);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 10:
      tmp = RA(10);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 11:
      tmp = RA(11);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 12:
      tmp = RA(12);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    case 13:
      tmp = RA(13);
      if(tmp == 0) break;
      tmp = normalize(tmp);
      ret = ((ret << 5) + ret) + tmp;
      BackTrace[frames] = (void*)tmp;
      frames++;
      if(frames == STACK_DEPTH) break;
    default:
      fprintf(stderr,"request for stack depth of %d\n",skip+1);
      assert(false);
    }
  }

  __computing_callsite = false;

  //fprintf(stderr,"%x\n",ret);

  if(BackTraceHash)
    *BackTraceHash = ret;

  return frames;
}
