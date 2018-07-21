#ifndef HOUND_MPROTECT_WRAPPER_H
#define HOUND_MPROTECT_WRAPPER_H

#if defined(_WIN32)
void protect_range(void *start, size_t len) {
  DWORD old;

  VirtualProtect(_start, len, PAGE_NOACCESS, &old);
}

bool unprotect_range(void *start, size_t len) {
  DWORD old;

  return (0 != VirtualProtect(_start, len, PAGE_READWRITE, &old));
}
#else

#include <sys/mman.h>

void protect_range(void *start, size_t len) {
  // PROTECTION CONFUSES PIN, THE POOR THING
#ifndef USE_PIN
  mprotect(start, len, PROT_NONE);
#endif
}

bool unprotect_range(void *start, size_t len) {
  return (0 == mprotect(start, len, PROT_READ | PROT_WRITE));
}
#endif

#endif  // __MPROTECT_WRAPPER_H__
