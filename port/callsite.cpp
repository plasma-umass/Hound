#if defined(_WIN32)
void InitCaptureStackBackTrace() {
  // RtlCaptureStackBackTrace is not in any header files, so we have to get a ref to it another way
  HMODULE ntdll = GetModuleHandle("ntdll.dll");
  if (!ntdll) {
    printf("GetModuleHandle returned error %d\n", GetLastError());
  }
  RtlCaptureStackBackTrace =
      (USHORT(NTAPI *)(ULONG, ULONG, PVOID *, PULONG))GetProcAddress(ntdll, "RtlCaptureStackBackTrace");
}

#else

void InitCaptureStackBackTrace() {
  getDLBase();
}

#endif

inline ULONG StackHash(VOID *Frames[], int MaxFrames) {
  ULONG ret = 5381;

  for (int i = 0; i < MaxFrames; i++) {
    ret = ((ret << 5) + ret) + (unsigned long)Frames[i];
  }

  return ret;
}
