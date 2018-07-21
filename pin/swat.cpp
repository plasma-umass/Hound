/*BEGIN_LEGAL
Intel Open Source License

Copyright (c) 2002-2007 Intel Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/* ===================================================================== */
/*
  @ORIGINAL_AUTHOR: Robert Cohn
*/

/* ===================================================================== */
/*! @file
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <signal.h>
#include <sys/mman.h>
#include <ext/hash_map>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include "pin.H"
/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

std::ofstream TraceFile;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "pinatrace.out", "specify trace file name");
KNOB<BOOL> KnobValues(KNOB_MODE_WRITEONCE, "pintool", "values", "1", "Output memory values reads and written");

/* ===================================================================== */

VOID *pf_normalize;
VOID *pf_getsite;
VOID *pf_getsize;
VOID *plug_signal;

/* SWAT State */
bool samplingEnabled = false;
std::map<ADDRINT, short> ncheck0;
std::map<ADDRINT, short> ncheck;
short ninstr = 0;
uint64_t currTime = 0;
bool heapReady = false;
bool inPin = false;

uint64_t ticks = 0;
uint64_t sampledTicks = 0;

const short MIN_RATE = 999;   // in multipls of 0.1
const short DEC_RATE = 10;    // div by this amount
const short NUM_INSTR = 100;  // burst length in number of checks

struct StaleData {
  uint64_t sampleTime;
  uint64_t realTime;
};

map<ADDRINT, StaleData> staleMap;
map<ADDRINT, uint64_t> pageMap;
map<ADDRINT, uint64_t> realPageMap;

uint64_t samples = 0;

PIN_LOCK mutex;

static VOID sample(ADDRINT site) {
  GetLock(&mutex, PIN_ThreadId() + 1);

  ticks++;
  if (!samplingEnabled) {
    short &ct = ncheck[site];
    // is the counter for this site now zero?
    if (ct == 0) {
      short &rate = ncheck0[site];
      if (rate < MIN_RATE) {
        rate = 10 * (rate + 1) - 1;
      }

      // fprintf(stderr,"Enabling sampling for site %x (rate %d)\n",site,rate);

      ct = (rate)*NUM_INSTR;

      samplingEnabled = true;
      ninstr = NUM_INSTR;
    } else {
      ct--;
    }
  } else {
    sampledTicks++;
    if (--ninstr == 0) {
      samplingEnabled = false;
    }
  }

  ReleaseLock(&mutex);
}

static INT32 Usage() {
  cerr << "This tool produces a memory address trace.\n"
          "For each (dynamic) instruction reading or writing to memory the the ip and ea are recorded\n"
          "\n";

  cerr << KNOB_BASE::StringKnobSummary();

  cerr << endl;

  return -1;
}

static VOID TracePageUnmap(ADDRINT ptr);

static VOID RecordMem(CONTEXT *cxt, VOID *ip, CHAR r, VOID *addr, INT32 size, BOOL isPrefetch, CHAR *rtnname = "") {
  ADDRINT data = 0;

  if (!inPin && heapReady && pf_normalize && addr) {
    // fprintf(stderr,"ptr is %p\n",addr);

    inPin = true;

    // GetLock(&mutex,PIN_GetTid()+1);

    // fprintf(stderr,"calling normalize function from %s\n",rtnname);

    /*PIN_CallApplicationFunction(cxt, PIN_ThreadId(),
                                                            CALLINGSTD_DEFAULT, (AFUNPTR)pf_normalize,
                                                            PIN_PARG(ADDRINT), &data,
                                                            PIN_PARG(VOID *), addr,
                                                            PIN_PARG_END() );*/

    data = reinterpret_cast<ADDRINT>(((void *(*)(void *))pf_normalize)(addr));
    // fprintf(stderr,"got %p\n",data);

    inPin = false;

    if (data && staleMap.find(data) != staleMap.end()) {
      // GetLock(&mutex,PIN_ThreadId()+1);

      // fprintf(stderr,"data is %p, addr %p\n",data,addr);
      if (true && random() < 200000) {
        fprintf(stderr, "map size is %d\n", staleMap.size());
      }

      // Oracle
      StaleData &st = staleMap[data];
      st.realTime = currTime;

      ADDRINT page = (data >> 12) << 12;

      // SWAT
      if (samplingEnabled || st.sampleTime == 0) {
        // fputs("sample\n",stderr);
        st.sampleTime = currTime;
        samples++;
      }

      // Plug
      if (pageMap.find(page) != pageMap.end()) {
        siginfo_t sig;
        sig.si_addr = (VOID *)page;

        if (pageMap[page] < realPageMap[page]) {
          fprintf(stderr, "fucked up page %p\n", page);
        }

        /*
        PIN_CallApplicationFunction(cxt, PIN_ThreadId(),
                                    CALLINGSTD_DEFAULT, (AFUNPTR)plug_signal,
                                    PIN_PARG(INT), 11,
                                    PIN_PARG(VOID *), &sig,
                                    PIN_PARG(VOID *), &sig,
                                    PIN_PARG_END() );
*/
        fprintf(stderr, "(PIN) sigsegving on page %x\n", page);
        ((void (*)(int, void *, void *))plug_signal)(11, &sig, NULL);
        TracePageUnmap(page);
        fprintf(stderr, "(PIN) done with callbackon %x\n", page);
      }

      // per-page oracle
      realPageMap[page] = currTime;

      // ReleaseLock(&mutex);

      currTime++;
    }
  }
}

static VOID *WriteAddr;
static INT32 WriteSize;

static VOID RecordWriteAddrSize(VOID *addr, INT32 size) {
  WriteAddr = addr;
  WriteSize = size;
}

static VOID RecordMemWrite(CONTEXT *cxt, VOID *ip) {
  RecordMem(cxt, ip, 'W', WriteAddr, WriteSize, false);
}

static VOID TraceFree(ADDRINT ptr) {
  // fprintf(stderr,"freeing %p\n",ptr);
  if (false && pf_getsize) {
    uint32_t size = ((unsigned long (*)(ADDRINT))pf_getsize)(ptr);
    fprintf(stderr, "freeing %p: %d\n", ptr, size);
  }
  staleMap.erase(ptr);
}

static VOID TraceMalloc(ADDRINT ptr) {
  staleMap[ptr].sampleTime = currTime;
  staleMap[ptr].realTime = currTime;
  // fprintf(stderr,"malloc %p\n",ptr);
}

static VOID TracePageMap(ADDRINT ptr) {
  ptr = (ptr >> 12) << 12;
  // GetLock(&mutex,PIN_GetTid()+1);
  pageMap[ptr] = currTime;
  // fprintf(stderr,"(PIN) mapped page %p, now %d\n",ptr,pageMap.size());
  // ReleaseLock(&mutex);
  mprotect((VOID *)ptr, 4096, PROT_READ | PROT_WRITE);
}

static VOID TracePageUnmap(ADDRINT ptr) {
  // GetLock(&mutex,PIN_GetTid()+1);
  pageMap.erase(ptr);
  // fprintf(stderr,"(PIN) unmapping %p, now %d\n",ptr,pageMap.size());
  // ReleaseLock(&mutex);
}

static VOID PlugInitialized() {
  fprintf(stderr, "heap ready!\n");
  heapReady = true;
}

static VOID printData() {
  TraceFile.open(KnobOutputFile.Value().c_str());
  // TraceFile.write(trace_header.c_str(),trace_header.size());
  TraceFile.setf(ios::showbase);
  for (map<ADDRINT, StaleData>::iterator i = staleMap.begin(); i != staleMap.end(); i++) {
    ADDRINT ob = i->first;
    ADDRINT page = (ob >> 12) << 12;
    // fprintf(stderr,"%p\t%llu\t%llu\t%llu\n",ob,i->second.realTime,i->second.sampleTime,pageMap[page]);

    int64_t plugTime = (pageMap[page] == 0 ? 0 : (int64_t)(currTime - pageMap[page]));
    unsigned long site = ((unsigned long (*)(ADDRINT))pf_getsite)(ob);
    uint32_t size = ((unsigned long (*)(ADDRINT))pf_getsize)(ob);

    fprintf(stderr, "%p size is %d\n", ob, size);

    TraceFile << std::hex << ob << '\t' << (currTime - i->second.realTime) << '\t' << (currTime - i->second.sampleTime)
              << '\t' << plugTime << '\t' << (currTime - realPageMap[(ob >> 12) << 12]) << '\t' << site << '\t' << size
              << endl;
  }

  fprintf(stderr, "samples %llu out of %llu\n", samples, currTime);
  fprintf(stderr, "ticks %llu out of %llu\n", sampledTicks, ticks);

  TraceFile.close();
}

VOID Instruction(INS ins, VOID *v) {
  // instruments loads using a predicated call, i.e.
  // the call happens iff the load will be actually executed

  if (RTN_Name(INS_Rtn(ins)) == "phkmalloc_normalize")
    return;
  if (RTN_Name(INS_Rtn(ins)) == "phkmalloc")
    return;
  if (RTN_Name(INS_Rtn(ins)) == "phkmalloc_usable_size")
    return;
  if (RTN_Name(INS_Rtn(ins)) == "plug_findobject")
    return;
  if (RTN_Name(INS_Rtn(ins)) == ".plt")
    return;

  if (RTN_Name(INS_Rtn(ins)) == "__cxa_guard_acquire")
    return;
  if (RTN_Name(INS_Rtn(ins)) == "__cxa_guard_release")
    return;
  if (RTN_Name(INS_Rtn(ins)) == "__cxa_current_exception_type")
    return;
  if (RTN_Name(INS_Rtn(ins)) == "__pthread_mutex_lock")
    return;
  if (RTN_Name(INS_Rtn(ins)) == "fixup")
    return;

  if (IMG_Name(SEC_Img(RTN_Sec(INS_Rtn(ins)))) == "/nfs/sting/users1/gnovark/scratch/svn/projects/plug/libplug.so")
    return;

  if (INS_IsMemoryRead(ins) && !INS_IsStackRead(ins) && !INS_IsIpRelRead(ins)) {
    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_CONTEXT, IARG_INST_PTR, IARG_UINT32, 'R',
                             IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_UINT32, INS_IsPrefetch(ins), IARG_PTR,
                             RTN_Name(INS_Rtn(ins)).c_str(), IARG_END);
  }

  if (INS_HasMemoryRead2(ins)) {
    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_CONTEXT, IARG_INST_PTR, IARG_UINT32, 'R',
                             IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_UINT32, INS_IsPrefetch(ins), IARG_PTR,
                             RTN_Name(INS_Rtn(ins)).c_str(), IARG_END);
  }

  // instruments stores using a predicated call, i.e.
  // the call happens iff the store will be actually executed
  if (INS_IsMemoryWrite(ins) && !INS_IsStackWrite(ins) && !INS_IsIpRelWrite(ins)) {
    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordWriteAddrSize, IARG_MEMORYWRITE_EA,
                             IARG_MEMORYWRITE_SIZE, IARG_END);

    if (INS_HasFallThrough(ins)) {
      INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)RecordMemWrite, IARG_CONTEXT, IARG_INST_PTR, IARG_END);
    }
    if (INS_IsBranchOrCall(ins)) {
      INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)RecordMemWrite, IARG_CONTEXT, IARG_INST_PTR, IARG_END);
    }
  }

  // Find loop backedges and function calls to enable/disable sample bursts
  if (INS_IsDirectBranchOrCall(ins)) {
    // static int skipped = 0;
    // static int instr = 0;
    // ETYPE type = INS_IsCall(ins) ? ETYPE_CALL : ETYPE_BRANCH;

    // static targets can map here once
    // COUNTER *pedg = Lookup( EDGE(INS_Address(ins),  INS_DirectBranchOrCallTargetAddress(ins),
    // INS_NextAddress(ins), type) );
    if (!INS_IsCall(ins)) {
      if (INS_Address(ins) < INS_DirectBranchOrCallTargetAddress(ins)) {
        //		skipped++;
        return;
      } else {
        //		instr++;
        //		fprintf(stderr,"%d %d instrumenting backward branch at %x (to
        //%x)\n",instr,skipped,INS_Address(ins),INS_DirectBranchOrCallTargetAddress(ins));
      }
    } else {
      // fprintf(stderr,"instrumenting call at %x (to %x)\n",INS_Address(ins),INS_DirectBranchOrCallTargetAddress(ins));
    }
    INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)sample, IARG_INST_PTR, IARG_END);
  }
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v) {
  fprintf(stderr, "Ending PINTOOL ***********\n");
  printData();
}

BOOL SigUSR2Handler(THREADID id, INT32 sig, CONTEXT *ctxt, BOOL hasHndlr, VOID *v) {
  Fini(0, 0);
  return false;
}

/* ===================================================================== */

VOID ImageLoad(IMG img, VOID *v) {
  cout << IMG_Name(img) << endl;

  RTN rtn1 = RTN_FindByName(img, "plug_findobject");
  if (RTN_Valid(rtn1)) {
    pf_normalize = reinterpret_cast<VOID *>(RTN_Address(rtn1));
    fprintf(stderr, "found plug_findobject: %p\n", pf_normalize);
  }

  rtn1 = RTN_FindByName(img, "plug_getsite");
  if (RTN_Valid(rtn1)) {
    pf_getsite = reinterpret_cast<VOID *>(RTN_Address(rtn1));
    fprintf(stderr, "found plug_getsite: %p\n", pf_getsite);
  }

  rtn1 = RTN_FindByName(img, "plug_getsize");
  if (RTN_Valid(rtn1)) {
    pf_getsize = reinterpret_cast<VOID *>(RTN_Address(rtn1));
    fprintf(stderr, "found plug_getsize: %p\n", pf_getsize);
  }

  rtn1 = RTN_FindByName(img, "AOHSEGVHandler");
  if (RTN_Valid(rtn1)) {
    plug_signal = reinterpret_cast<VOID *>(RTN_Address(rtn1));
    fprintf(stderr, "found AOHSEGVHandler: %p\n", plug_signal);
  }

  rtn1 = RTN_FindByName(img, "plug_mmap");
  if (RTN_Valid(rtn1)) {
    // pf_plug_mmap = reinterpret_cast<VOID *>(RTN_Address(rtn1));
    fprintf(stderr, "found plug_mmap\n");
    RTN_Open(rtn1);
    RTN_InsertCall(rtn1, IPOINT_BEFORE, (AFUNPTR)TracePageMap, IARG_G_ARG0_CALLEE, IARG_END);
    RTN_Close(rtn1);
  }

  rtn1 = RTN_FindByName(img, "plug_barrier");
  if (RTN_Valid(rtn1)) {
    // pf_plug_mmap = reinterpret_cast<VOID *>(RTN_Address(rtn1));
    fprintf(stderr, "found plug_barrier\n");
    RTN_Open(rtn1);
    RTN_InsertCall(rtn1, IPOINT_BEFORE, (AFUNPTR)TracePageUnmap, IARG_G_ARG0_CALLEE, IARG_END);
    RTN_Close(rtn1);
  }

  rtn1 = RTN_FindByName(img, "plug_object_allocated");
  if (RTN_Valid(rtn1)) {
    fprintf(stderr, "found plug_object_allocated\n");
    RTN_Open(rtn1);
    RTN_InsertCall(rtn1, IPOINT_BEFORE, (AFUNPTR)TraceMalloc, IARG_G_ARG0_CALLEE, IARG_END);
    RTN_Close(rtn1);
  }

  rtn1 = RTN_FindByName(img, "plug_munmap");
  if (RTN_Valid(rtn1)) {
    // pf_plug_munmap = reinterpret_cast<VOID *>(RTN_Address(rtn1));
    fprintf(stderr, "found plug_munmap\n");
    RTN_Open(rtn1);
    RTN_InsertCall(rtn1, IPOINT_BEFORE, (AFUNPTR)TracePageUnmap, IARG_G_ARG0_CALLEE, IARG_END);
    RTN_Close(rtn1);
  }

  rtn1 = RTN_FindByName(img, "plug_initialized");
  if (RTN_Valid(rtn1)) {
    // pf_plug_munmap = reinterpret_cast<VOID *>(RTN_Address(rtn1));
    fprintf(stderr, "found plug_initialized\n");
    RTN_Open(rtn1);
    RTN_InsertCall(rtn1, IPOINT_BEFORE, (AFUNPTR)PlugInitialized, IARG_G_ARG0_CALLEE, IARG_END);
    RTN_Close(rtn1);
  }

  RTN freeRtn = RTN_FindByName(img, "plug_object_freed");
  if (RTN_Valid(freeRtn)) {
    fprintf(stderr, "found plug_object_freed\n");
    RTN_Open(freeRtn);
    RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)TraceFree, IARG_G_ARG0_CALLEE, IARG_END);
    RTN_Close(freeRtn);
  }

  /*
  // Forward pass over all sections in an image
  for( SEC sec= IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec) ) {
          for( RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn) ) {
                  fprintf(stderr,"%p: %s\n",RTN_Address(rtn),RTN_Name(rtn).c_str());
          }
  }
  */
}

/* ===================================================================== */

int main(int argc, char *argv[]) {
  string trace_header = string(
      "#\n"
      "# Memory Access Trace Generated By Pin\n"
      "#\n");

  PIN_InitSymbols();

  if (PIN_Init(argc, argv)) {
    return Usage();
  }

  InitLock(&mutex);

  IMG_AddInstrumentFunction(ImageLoad, 0);
  INS_AddInstrumentFunction(Instruction, 0);
  PIN_AddFiniFunction(Fini, 0);
  PIN_AddSignalInterceptFunction(SIGUSR2, SigUSR2Handler, 0);

  samplingEnabled = false;

  // Never returns

  PIN_StartProgram();

  RecordMemWrite(NULL, 0);
  RecordWriteAddrSize(0, 0);

  return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
