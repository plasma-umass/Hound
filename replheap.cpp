//////////////////////////////////////////////////////////////////////////////
//
//  Module:     replheap.dll
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Microsoft Research Detours Package, Version 2.1 (Build_207)
//
//

#define WIN32_LEAN_AND_MEAN
#ifndef COMPILING_ROCKALL_LIBRARY
#define COMPILING_ROCKALL_LIBRARY
#endif

#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include "detours.h"
#include "verify.cpp"
#include "plugheap.hpp"

//static PlugHeap AppHeap;
static FAST_HEAP AppHeap;

#define DETOURED_CRT "msvcrt.dll"

// CRT fputs() calls malloc(), yay.
VOID LTputs(const char * buf);

static VOID * (CDECL * TrueMalloc)(size_t sz) = NULL;
static VOID (CDECL * TrueFree)(VOID * ptr) = NULL;
static VOID * (CDECL * TrueRealloc)(VOID *, size_t sz) = NULL;
static VOID * (CDECL * TrueCalloc)(size_t, size_t sz) = NULL;
static size_t (CDECL * TrueMsize)(VOID *) = NULL;
static int (WINAPI * TrueWinMain)(HINSTANCE hInstance,
                                  HINSTANCE hPrevInstance,
                                  LPSTR lpCmdLine,
                                  int nCmdShow) = NULL;
static int (WINAPI * RawWinMain)(HINSTANCE hInstance,
                                 HINSTANCE hPrevInstance,
                                 LPSTR lpCmdLine,
                                 int nCmdShow) = NULL;

static BOOL (WINAPI * TrueReadFile)(HANDLE,
									LPVOID,
									DWORD,
									LPDWORD,
									LPOVERLAPPED) = NULL;

static BOOL (WINAPI * TrueWriteFile)(HANDLE,
									 LPCVOID,
									 DWORD,
									 LPDWORD,
									 LPOVERLAPPED) = NULL;

VOID * CDECL LTMalloc(size_t sz)
{
	PVOID ptr = AppHeap.New(sz);

	if(!ptr) {
		printf("malloc returning null\n");
	}

	//printf("malloc returning 0x%x\n",ptr);

	return ptr;
}

VOID CDECL LTFree(VOID * ptr)
{
	if(!ptr) return;

	// Sanity check for objects not alloc'd from AppHeap
	if( !AppHeap.Delete(ptr) ) {
		printf("AppHeap didn't find pointer, calling CRT free on 0x%x\n",ptr);
		int old_errno = errno;
		TrueFree(ptr);
		if(old_errno != errno) {
			printf("got error: %d\n",errno);
			fflush(stdout);
		}
	}
}

// XXX: optimize this
VOID * CDECL LTRealloc(VOID * ptr, size_t sz) {
	if(!ptr) return LTMalloc(sz);
	if(!sz) {
		LTFree(ptr);
		return NULL;
	}

	VOID * nptr = AppHeap.Resize(ptr,sz);
	
	if(!nptr) {
		printf("reallocing CRT: 0x%x\n",ptr);
		// Migrate object from old heap
		size_t o_sz = _msize(ptr);
		nptr = LTMalloc(sz);

		if(nptr)
			memcpy(nptr,ptr,(o_sz < sz ? o_sz : sz));

		LTFree(ptr);
	}

	//printf("realloc returning 0x%x\n",nptr);
	
	return nptr;
}

size_t CDECL LTMsize(VOID * ptr) {
	if ( AppHeap.KnownArea( ptr ) )
		{
		AUTO int Space;

		if ( AppHeap.Details( ptr,& Space ) )
			{ return Space; }
		else
			{ return 0; }
		}
	else
		{ return TrueMsize( ptr ); }
}

VOID * CDECL LTCalloc( size_t Number, size_t Size ) {
	int * c = (int *)AppHeap.New(Number*Size,NULL,true);
	return (PVOID)c;
}

static BOOL WINAPI LTReadFile(HANDLE a1, 
									 LPVOID a2,
									 DWORD a3,
									 LPDWORD a4,
									 LPOVERLAPPED a5) 
{
	volatile char c;
	c = *((char *)a2);

	return TrueReadFile(a1,a2,a3,a4,a5); 
}

static BOOL WINAPI LTWriteFile(HANDLE a1, 
									 LPCVOID a2,
									 DWORD a3,
									 LPDWORD a4,
									 LPOVERLAPPED a5) 
{
	char c;
	c = *((char *)a2);

	return TrueWriteFile(a1,a2,a3,a4,a5); 
}

int WINAPI LTWinMain(HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine,
                        int nCmdShow)
{
    // We couldn't call LoadLibrary in DllMain,
    // so we detour malloc/free/realloc/calloc here...
    LONG error;

	//freopen("log.txt","w",stdout);

    TrueMalloc = (VOID * (CDECL * )(size_t sz))
        DetourFindFunction(DETOURED_CRT, "malloc");

	printf("got malloc at 0x%x\n",TrueMalloc);

	TrueFree = (VOID (CDECL * )(VOID *))
        DetourFindFunction(DETOURED_CRT, "free");
	TrueRealloc = (VOID * (CDECL * )(VOID *, size_t sz))
        DetourFindFunction(DETOURED_CRT, "realloc");
	TrueCalloc = (VOID * (CDECL * )(size_t, size_t sz))
        DetourFindFunction(DETOURED_CRT, "calloc");
	TrueMsize = (size_t (CDECL *)(VOID *))
		DetourFindFunction(DETOURED_CRT, "_msize");
	TrueReadFile = (BOOL (WINAPI *)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED))
		DetourFindFunction("kernel32.dll","ReadFile");
	TrueWriteFile = (BOOL (WINAPI *)(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED))
		DetourFindFunction("kernel32.dll","WriteFile");
		

	printf("ntdll!ReadFile = 0x%x\n",TrueReadFile);
	printf("ntdll!WriteFile = 0x%x\n",TrueWriteFile);

	void * foo = TrueMalloc(32);
	TrueFree(foo);
	printf("sampled CRT heap object: 0x%x\n",foo);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID&)TrueMalloc,  LTMalloc);
	DetourAttach(&(PVOID&)TrueFree,    LTFree);
	DetourAttach(&(PVOID&)TrueRealloc, LTRealloc);
	DetourAttach(&(PVOID&)TrueCalloc,  LTCalloc);
	DetourAttach(&(PVOID&)TrueMsize,   LTMsize);
	DetourAttach(&(PVOID&)TrueReadFile,LTReadFile);
	DetourAttach(&(PVOID&)TrueWriteFile,LTWriteFile);

    error = DetourTransactionCommit();

    if (error == NO_ERROR) {
        printf("replheap.dll: Detoured malloc functions.\n");
    }
    else {
        printf("replheap.dll: Error detouring malloc functions: %d\n", error);
    }

    Verify("malloc", (PVOID)malloc);
    printf("\n");
    fflush(stdout);

    printf("replheap.dll: Calling WinMain\n");
    fflush(stdout);

    return TrueWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH) {
        printf("replheap.dll: Starting.\n");
		Verify("malloc",malloc);
        fflush(stdout);

        DetourRestoreAfterWith();

        // NB: DllMain can't call LoadLibrary, so we hook the app entry point.
        TrueWinMain =
            (int (WINAPI *)(HINSTANCE, HINSTANCE, LPSTR, int))
            DetourGetEntryPoint(NULL);
        RawWinMain = TrueWinMain;

        //Verify("WinMain", RawWinMain);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueWinMain, LTWinMain);
        error = DetourTransactionCommit();

        //Verify("WinMain after attach", RawWinMain);
        //Verify("WinMain trampoline", TrueWinMain);

        if (error == NO_ERROR) {
            printf("simple.dll: Detoured main.\n");
        }
        else {
            printf("simple.dll: Error detouring main: %d\n", error);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueMalloc, LTMalloc);
		DetourDetach(&(PVOID&)TrueFree, LTFree);
		DetourDetach(&(PVOID&)TrueRealloc, LTRealloc);
		DetourDetach(&(PVOID&)TrueCalloc, LTCalloc);
		DetourDetach(&(PVOID&)TrueMsize, LTMsize);
        error = DetourTransactionCommit();

        printf("simple.dll: Removed malloc() (result=%d)s.\n",
               error);
        fflush(stdout);
    }
    return TRUE;
}

//
///////////////////////////////////////////////////////////////// End of File.
