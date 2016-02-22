// ************************************************************************************
// ***																				***
// ***							Thread Handling										***
// ***																				***
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
// ***------------------------------------------------------------------------------***
// ***					MUST be #included BEFORE FCIO.H								***
// ************************************************************************************

#ifdef _WIN32		// This is needed for newer MSVC versions
#ifndef WIN32
	#define WIN32
#endif
#endif

#ifdef WIN32
	#include <windows.h>
	#include <process.h>
	#define ThreadHandle unsigned long
	#define _thread_uintf unsigned long
#endif

#ifdef __APPLE__
	#error "Find the #include File for Mac Threading!"
	#define ThreadHandle ThreadID
	#define _thread_uintf unsigned long
#endif

#ifdef __linux__
	#include <pthread.h>
	#define ThreadHandle pthread_t
	#define _thread_uintf unsigned long
#endif

#ifdef __ANDROID__
	#include <pthread.h>
	#define ThreadHandle pthread_t
	#define _thread_uintf unsigned long
#endif

ThreadHandle newthread(char *name, void (*entrypoint)(void *), void *data);
void threadsleep(_thread_uintf ms);

