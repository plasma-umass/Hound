#include "output.hpp"
#include <stdio.h>
#include "platform.hpp"

#ifdef _WIN32

static HANDLE _XMLfile;

void OpenStatFile() {
  char buf[80];
  sprintf_s(buf, 80, "%d.xml", GetCurrentProcessId());
  _XMLfile = CreateFile(buf, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (INVALID_HANDLE_VALUE == _XMLfile) {
    OutputDebugString("Failed to open output XML file!");
  }
}

void CloseStatFile() {
  CloseHandle(_XMLfile);
}

void OutputStats(std::string buf) {
  DWORD foo;
  WriteFile(_XMLfile, buf.c_str(), strlen(buf.c_str()), &foo, NULL);
}

#else

#include <unistd.h>

static FILE *_XMLfile;

void OpenStatFile() {
  char buf[80];
  snprintf(buf, 80, "%d.xml", getpid());
  _XMLfile = fopen(buf, "w");
}

void CloseStatFile() {
  fclose(_XMLfile);
}

void OutputStats(std::string buf) {
  fprintf(_XMLfile, buf.c_str());
}

#endif
