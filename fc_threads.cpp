/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include "fc_threads.h"
#include "fc_h.h"

#define maxThreads 128

struct sThreadInfo
{	ThreadHandle	handle;
	char			name[32];
	void			(*entrypoint)(void *);
	void			*passeddata;
	bool			inuse;
} thread[maxThreads];

// Place a thread wrapper function here, one for each platform
#ifdef PLATFORM_win32
void _cdecl ThreadWrapper(void *data)
{	sThreadInfo *thisthread = (sThreadInfo *)data;
	thisthread->entrypoint(thisthread->passeddata);
	thisthread->inuse = false;
}
#endif

ThreadHandle newthread(char *name, void (*entrypoint)(void *), void *data)
{	ThreadHandle result;

	// Find an empty thread container
	intf usethread = -1;
	for (uintf i=0; i<maxThreads; i++)
		if (!thread[i].inuse)
		{	thread[i].inuse = true;
			usethread = i;
			break;
		}

	if (usethread<0) msg("Error Creating Thread","Cannot create any more threads");

	// Fill the threadinfo structure with details about this thread
	txtcpy(thread[usethread].name,sizeof(thread[usethread].name), name);
	thread[usethread].passeddata = data;
	thread[usethread].entrypoint = entrypoint;
	
#ifdef PLATFORM_win32
	result = _beginthread(ThreadWrapper, 0, &thread[usethread]);
	if (result < 0) msg("Failed to create thread",buildstr("Failed to create thread %s", name));
#else
	result = NULL;
	msg("Incomplete Code Error","Threads not yet supported on this platform");
#endif
	return result;
}

void threadsleep(uintf ms)	// A function to pause a thread for 'ms' milliseconds
{
#ifdef PLATFORM_win32
	Sleep((DWORD)ms);
#endif
}

