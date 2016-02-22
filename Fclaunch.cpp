/******************************************************************************/
/*  						FC Launcher Source File.                          */
/*	    					------------------------                          */
/*							(c) by Stephen Fraser		                      */
/*	 					(c) 1998-2002 by Stephen Fraser						  */
/*                                                                            */
/* Contains the FC Initialization Routines and standard system services.	  */
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#include <stdarg.h>		// Needed for va_start
#include <stdlib.h>
#include <time.h>		// Needed for random numbers
#include "fc_h.h"		// Standard FC internals

#define maxautoshutdown 256

void initArchives(void);
void initcompressors(void);
void initimageplugins(void);
void initimagesaveplugins(void);
void init3dplugins(void);

struct sEngineWarningInfo
{	EngineWarningParamType	pType;
	const char				*errormsg;
}	EngineWarningArray[] = {
	{	EngineWarningParam_NoParam,	"No Warning to Report" },									// NoWarning
	{	EngineWarningParam_String,	"A string (%s) is larger than allowed" }					// StringTooLarge
};

// ************************** Private System Variables ****************************
// Wrapper Class instances
mInput		*input;
fc_bitmap	*_fcbitmap;
fc_texture	*_fctexture;
fc_text		*_fctext;
fc_lighting	*_fclighting;
fc_plugin 	*_fcplugin;
fc_printer	*_fcprinter;
void		*_OEMpointer;

byte	*engineStaticMemory = NULL;
uintf	engineStaticMemorySize;

logfile	*errorslog;
void	(*OSclose_callback)(void);	// Pointer to the function to call when the user tries to close the app via the OS
void 	(*timertask)(void) = NULL;	// The task called whenever the timer ticks
char	errorOnExitTxt[512];		// Error-On-Exit Text, Errors that must be reported, but can wait until execution finishes, are stored here.
intf	fcapplaunched;
void	(*autoshutdown[maxautoshutdown])(void);
intf	nextautoshutdown;

// ************************** Public System Variables ****************************
char	*cmdline;					// Pointer to the command line arguments
char	runpath[256];				// The directory the program was run from
char	programflname[256];			// The path and filename of the program's EXE file

char	programName[128];			// Name of the current program (read-only, use the 'setProgramName' function to change it)
uintf	numCPUs;					// Number of CPU cores available.
uintf	OS_close;					// Number of times the user has tried to close the application via the OS
uintf	currentTimerHz;				// The current speed in Hz of the timer
uintf	globalTimer;				// An incrementing timer which the app should NEVER change
intf 	timer;						// Internal timer variable
intf	randomseed;					// Random seed as it was when the engine started up
bool	audioworks;					// Is the audio system working?
EngineWarning	LastEngineWarning;	// Last Engine warning
void	*EngineWarningParam;		// Parameter for the last warning
eAppState applicationState;			// Current state of the application


// ************************ Private System Functions ***********************************
void OSCloseEvent(void)
{	// It appears this function is never called
	msg("Weird stuff","The function 'OSCloseEvent', which is tagged as never called, was just called!");
	//OS_close++;
	//keyhit(15);
	//if (OSclose_callback)
	//	OSclose_callback();
}

// ************************ Public System Functions ***********************************
void *classfinder(const char *classname)
{	if (txticmp(classname,"input")==0) return input;
	if (txticmp(classname,"bitmap")==0) return _fcbitmap;
	if (txticmp(classname,"texture")==0) return _fctexture;
	if (txticmp(classname,"text")==0) return _fctext;
	if (txticmp(classname,"lighting")==0) return _fclighting;
	if (txticmp(classname,"plugins")==0) return _fcplugin;
	if (txticmp(classname,"printer")==0) return _fcprinter;
	if (txticmp(classname,"OEM")==0) return _OEMpointer;
	if (txticmp(classname,"msg")==0) return (void *)msg;
//	msg("Unknown Class",buildstr("Cannot find class %s",classname));
	return NULL;
}

void resourceError(const char *res)
{	char buf[256];
	tprintf(buf,sizeof(buf),"The system has run out of a resource called '%s'\r\nThis is due to a program fault, and is probably\r\nnot something you can buy to upgrade your computer",res);
	msg("Out of Resources",buf);
}

extern uintf frametick;

void fcTimerTick(void)
{	frametick++;
	timer--;
	globalTimer++;
	if (timertask) timertask();
}

void set_OSclose_callback(void (*cb)(void))
{	OSclose_callback = cb;
}

void addautoshutdown(void (*shutdownfn)(void))
{	if (nextautoshutdown>=maxautoshutdown)
	{	memorylog->log("Auto Shutdown buffers are full.");
		return;
	}
	autoshutdown[nextautoshutdown++]=shutdownfn;
}

float frand(float maxvalue)
{	return ((float)rand() / RAND_MAX)*maxvalue;
}

/*
void _cdecl systemMessage(char *txt,...)
{	if (!errorOnExitTxt[0])
	{	constructstring((char *)sysmsgtxt,txt,sizeof(sysmsgtxt));
	}
}
*/

/*
class OEMsystem : public fc_system
{	public:
	void set_OSclose_callback(void (*cb)(void))		{		::set_OSclose_callback(cb);};
	void addautoshutdown(void (*shutdownfn)(void))	{		::addautoshutdown(shutdownfn);};
	float frand(float maxvalue)						{return ::frand(maxvalue);};
	void checkmsg(void)								{		::checkmsg();};						// in platform startup file
	void setTimerTask(void taskadr(void),intf ticks){		::setTimerTask(taskadr, ticks);};	// in platform startup file
	void stamptime(vlong *dest)						{		::stamptime(dest);};				// in platform startup file
	void msg( const char *title, const char *thismessage ){	::msg(title,thismessage);};			// in platform startup file
	void exitfc(void)								{		::exitfc();};						// in platform startup file
	void *getvarptr(const char *name);
};

void *OEMsystem::getvarptr(const char *name)
{	if (txticmp(name,"OS_close")==0) return &OS_close;
	if (txticmp(name,"CurrentTimerHz")==0) return &currentTimerHz;
	if (txticmp(name,"GlobalTimer")==0) return &globalTimer;
	if (txticmp(name,"timer")==0) return &timer;
	if (txticmp(name,"randomseed")==0) return &randomseed;
	if (txticmp(name,"audioworks")==0) return &audioworks;
	if (txticmp(name,"LastEngineWarning")==0) return &LastEngineWarning;
	if (txticmp(name,"EngineWarningParam")==0) return &EngineWarningParam;
	if (txticmp(name,"applicationState")==0) return &applicationState;
	return NULL;
}
*/

// ----------------  FCLAUNCH : FC Initializes at this point -------------------
#define modList				\
	defMod(mod_bitmap)		\
	defMod(mod_2DRender)	\
	defMod(mod_plugins)		\

// Create prototypes for each module ... ie:  'mod_bitmap' becomes 'extern sEngineModule mod_bitmap;'
#define defMod(mod) extern sEngineModule mod;
modList
#undef defMod

// Create an array of engine modules ... ie:  sEngineModule engModule[] = { mod_bitmap, mod_2DRender, mod_plugins, ... };
#define defMod(mod) mod,
sEngineModule engModule[] =
{	modList
};
const uintf numEngModules = (sizeof(engModule) / sizeof(sEngineModule));
#undef defMod

void testDataSize(const char *nameStr, int size, int expecting)
{	if (size!=expecting)
	{	printf("Data Size Mismatch: %s is %i bytes in size, expecting %i\n",nameStr,size,expecting);
		printf("Press ENTER to continue\n");
		exit(0);
	}
}

void fclaunch(void)
{	// Initialize non-OS specific variables
	uintf i;

	// Perform sanity check on data sizes - Currently only supports 32 bit
	testDataSize("float",sizeof(float),4);
	testDataSize("int32",sizeof(int32),4);
	testDataSize("int16",sizeof(int16),2);
	testDataSize("int8" ,sizeof(int8 ),1);
//	testDataSize("void *",sizeof(void *),4);

	videoDriverPlugin = NULL;

	errorOnExitTxt[0] = 0;
	input = NULL;
	_fcbitmap = NULL;
	_fctexture = NULL;
	_fctext = NULL;
	_fclighting = NULL;
	_fcplugin = NULL;
	_fcprinter = NULL;

	errorslog	= NULL;

	applicationState=AppState_Initializing;
	fcapplaunched = 0;
	OS_close = 0;
	OSclose_callback = NULL;
	audioworks = false;		// We default to a non-working audio system
	nextautoshutdown = 0;
	LastEngineWarning = EngineWarning_NoWarning;
	EngineWarningParam = NULL;

	initLUTs();

	// Initialize random number generator
	randomseed=(unsigned)time( NULL );
	srand( randomseed );

	// Initialize Timer
	timertask=NULL;
	currentTimerHz = 100;
	globalTimer = 0;

	// Obtain Engine's static memory requirements
	engineStaticMemorySize = 0;
	for (i=0; i<numEngModules; i++)
	{	engModule[i].staticMemoryNeeded = (engModule[i].staticMemoryNeeded + 15) & ~15;
		engineStaticMemorySize += engModule[i].staticMemoryNeeded;
	}
	engineStaticMemory = (byte *)malloc(engineStaticMemorySize);

	// Initialize all modules
				  initdisk();
	errorslog	= newlogfile("logs/errors.log");
				  initmemoryservices();
				  initutils();
	_fctext		= inittext();
	input		= initinput(0x00000000);

	// Initialize newly-formatted modules (ones that provide a sEngineModule struct
	byte *staticBuf = engineStaticMemory;
	for (i=0; i<numEngModules; i++)
	{	engModule[i].initialize(staticBuf);
		staticBuf += engModule[i].staticMemoryNeeded;
	}

	initgeometry();
	initmatrix();
	initgraphicssystem();
	initBucketGui();
	initArchives();
	initcompressors();
	initimageplugins();
	initimagesaveplugins();
	init3dplugins();

	// Optional Modules
//	_fcprinter	= initprinter();

}

void errorOnExit(const char *errorText,...)
{	char errortxt[512];
	constructstring((char *)errortxt,errorText,sizeof(errortxt));
	errorslog->log(errortxt);
	if (errorOnExitTxt[0]==0)
	{	txtcpy((char *)errorOnExitTxt, sizeof(errorOnExitTxt), errortxt);
	}
}

// ----------------- _EXITFC : FC shuts down but doesn't exit ------------------
void _exitfc(void)
{	intf i;
	for (i=0; i<nextautoshutdown; i++)
		if (autoshutdown[i])
		{	autoshutdown[i]();
			autoshutdown[i] = NULL;
		}

	fcapplaunched = 0;
	timertask = NULL;
//	killaudio();

//	if (_fcprinter)		killprinter();		_fcprinter	= NULL;

	// Terminate newly-formatted modules (ones that provide a sEngineModule struct
	for (i=numEngModules-1; i>=0; i--)
	{	if (engModule[i].shutdown)
			engModule[i].shutdown();
		engModule[i].shutdown = NULL;
	}
	if (engineStaticMemory)
		free(engineStaticMemory);
	engineStaticMemory = NULL;

	killBucketGui();
						killgraphics();
	if (_fctext)		killtext();			_fctext		= NULL;
						killmatrix();
						killgeometry();
//	if (_fcbitmap)		killbitmap();		_fcbitmap	= NULL;
	if (input)			input->shutdown(0x00000000);	input	= NULL;
						killutils();
						killmemory();
	if (errorslog)		delete errorslog;	errorslog	= NULL;
/*	if (_fcdisk)	*/	killdisk();//			_fcdisk		= NULL;
//	if (_fcsystem)		delete _fcsystem;	_fcsystem	= NULL;
}
