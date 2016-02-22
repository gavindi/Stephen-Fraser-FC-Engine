/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <Carbon/Carbon.h>
#include <DrawSprocket/DrawSprocket.h>
#include <AGL/agl.h>
#include <AGL/aglRenderers.h>
#include "fc_genericOGL.h"

uintf buffernumber;		// This toggles between 0 and 1 each frame to indicate if we need to do another swap upon exit

void agl_getfunc(char *fname, void **func);

enum My_ErrorCode
{	ERR_No_Error,

	// FC Video Driver Error Codes
	ERR_IncompatibleDriver,
	ERR_WindowNotSupported,

	// Standard Error Codes
	ERR_OGLNotInstalled,
	ERR_OGLPixelFormat,
	ERR_OGLCreateContext,
	ERR_OGL_SetDrawable,
	ERR_OGL_SetFullScreen,
	ERR_OGL_SetCurrentContext,
};
	
char *GetErrorText (My_ErrorCode Error)
{	char *result;

	switch (Error)
	{	case ERR_IncompatibleDriver: result = "The video driver is incompatible with the client application"; break;
		case ERR_WindowNotSupported: result = "This video driver does not support Windowed mode operation"; break;

		case ERR_OGLNotInstalled:	result = "OpenGL is not installed on this system"; break;
		case ERR_OGLPixelFormat:	result = "Failed to choose an OpenGL pixel format"; break;
		case ERR_OGLCreateContext:	result = "Failed to create an OpenGL rendering context"; break;
		case ERR_OGL_SetDrawable:   result = "OpenGL 'Set Drawable' failed"; break; 
		case ERR_OGL_SetFullScreen:	result = "OpenGL failed to enter full screen mode"; break;
		case ERR_OGL_SetCurrentContext:	result = "OpenGL failed to set the current context"; break;
		default:					result = "Unknown Error"; break;
	}
	return result;
}
 
void FatalError (My_ErrorCode Error, char *ExtraInfo)
{	if (!ExtraInfo)
		printf ("%s\n", GetErrorText (Error));
	else
		printf ("%s\n--> %s", GetErrorText (Error), ExtraInfo);
	ExitToShell();
}

bool isOSX(void)
{	UInt32 response;
    
	if ((Gestalt(gestaltSystemVersion, (SInt32 *) &response) == noErr) && (response >= 0x01000))
		return true;
	return false;
}

//-----------------------------------------------------------------------------------------------------------------------

GDHandle hTargetDevice = NULL;		// The device to be drawn to
Rect gRectPort = {0, 0, 0, 0};		// Rectangle to render into

// ****************************************************************************
// ***																		***
// ***							DrawSprocket Code							***
// ***																		***
// ****************************************************************************
bool DSactive = false;
int DSversion;					// Version number of DrawSprockets
AGLDrawable gpDSpPort = NULL; 	// will be NULL for full screen under X
DSpContextAttributes gContextAttributes;
DSpContextReference gContext = 0;
const RGBColor rgbBlack = { 0x0000, 0x0000, 0x0000 };

void DSerror(char *fnname, OSStatus error)
{	if (error==noErr) return;
	char *result;
	switch (error)
	{	case kDSpNotInitializedErr:			result = "DSp Error: Not initialized"; break;
		case kDSpSystemSWTooOldErr: 		result = "DSp Error: system Software too old"; break;
		case kDSpInvalidContextErr:			result = "DSp Error: Invalid context"; break;
		case kDSpInvalidAttributesErr:		result = "DSp Error: Invalid attributes"; break;
		case kDSpContextAlreadyReservedErr:	result = "DSp Error: Context already reserved"; break;
		case kDSpContextNotReservedErr:		result = "DSp Error: Context not reserved"; break;
		case kDSpContextNotFoundErr:		result = "DSp Error: Context not found"; break;
		case kDSpFrameRateNotReadyErr:		result = "DSp Error: Frame rate not ready"; break;
		case kDSpConfirmSwitchWarning:		return; //result = "Dsp Error: Must confirm switch"; break;
		case kDSpInternalErr:				result = "DSp Error: Internal error"; break;
		case kDSpStereoContextErr:			result = "DSp Error: Stereo context"; break;
		default:							result = "Unknown DrawSprocket Error"; break;
	}
	printf("DrawSprocket Error %s",result);
	ExitToShell();
}

// --------------------------------------------------------------------------

void initDrawSprocket(void)	
{	NumVersion gVersionDSp;

	DSversion = 0;
	if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) DSpStartup) 
		return;
	DSerror("DSpStartup",DSpStartup());
	DSactive = true;

	if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) DSpGetVersion) 
		return;

	gVersionDSp = DSpGetVersion ();
	DSversion = (gVersionDSp.majorRev<<8)+(gVersionDSp.minorAndBugRev);
}	

//-----------------------------------------------------------------------------------------------------------------------

// Set up DSp screen on graphics device requested
// side effect: sets both gpDSpWindow and gpPort

CGrafPtr SetupDSpFullScreen (GDHandle hGD,dword width,dword height,dword bpp)
{
	DSpContextAttributes foundAttributes;
	DisplayIDType displayID = NULL;
	
	if (DSversion<0x0199) //if ((gVersionDSp.majorRev == 0x01) && (gVersionDSp.minorAndBugRev < 0x99))
	{	// this version of DrawSprocket is not completely functional on Mac OS X
		if (isOSX())
		{	printf ("DrawSprocket 1.99 or greater required on Mac OS X, please update to at least Mac OS X 10.1.");
			return NULL;
		}
	}
			
	// Note: DSp < 1.7.3 REQUIRES the back buffer attributes even if only one buffer is required
	dword depthmask =	kDSpDepthMask_1	|	kDSpDepthMask_2	|	kDSpDepthMask_4	|
						kDSpDepthMask_8	|	kDSpDepthMask_16|   kDSpDepthMask_32;
						
	memset(&gContextAttributes, 0, sizeof (DSpContextAttributes));
	gContextAttributes.displayWidth			= width;
	gContextAttributes.displayHeight		= height;
	gContextAttributes.colorNeeds			= kDSpColorNeeds_Require;
	gContextAttributes.displayBestDepth		= bpp;
	gContextAttributes.backBufferBestDepth	= bpp;
	gContextAttributes.displayDepthMask		= depthmask;
	gContextAttributes.backBufferDepthMask	= depthmask;
	gContextAttributes.pageCount			= 1;					
	
	DMGetDisplayIDByGDevice (hGD, &displayID, true);
	
	DSerror("DSpFindBestContext",DSpFindBestContextOnDisplayID (&gContextAttributes, &gContext, displayID));
	DSerror("DSpContect_GetAttributes",DSpContext_GetAttributes (gContext, &foundAttributes)); // see what we actually found

	// reset width and height to full screen and handle our own centering
	// HWA will not correctly center less than full screen size contexts
	gContextAttributes.displayWidth 	= foundAttributes.displayWidth;
	gContextAttributes.displayHeight 	= foundAttributes.displayHeight;
	gContextAttributes.pageCount		= 1;									// only the front buffer is needed
	gContextAttributes.contextOptions	= 0 | kDSpContextOption_DontSyncVBL;	// no page flipping and no VBL sync needed

	DSpSetBlankingColor(&rgbBlack);

	DSerror("DSpContext_Reserve",DSpContext_Reserve ( gContext, &gContextAttributes)); // reserve our context
 	DSerror("DSpContext_SetState",DSpContext_SetState (gContext, kDSpContextState_Active)); // activate our context

	if ((isOSX ()) && (DSversion<0x0199)) // DSp should be supported in version after 1.98
	{	printf ("Mac OS X with DSp < 1.99 does not support DrawSprocket for OpenGL full screen");
		return NULL;
	}
	else if (isOSX ()) // DSp should be supported in versions 1.99 and later
	{
		CGrafPtr pPort;
		// use DSp's front buffer on Mac OS X
		DSerror("DSpContext_GetFrontBuffer",DSpContext_GetFrontBuffer (gContext, &pPort));

		// there is a problem in Mac OS X 10.0 GM CoreGraphics that may not size the port pixmap correctly
		// this will check the vertical sizes and offset if required to fix the problem
		// this will not center ports that are smaller then a particular resolution
		{
			intf deltaV, deltaH;
			Rect portBounds;
			PixMapHandle hPix = GetPortPixMap (pPort);
			Rect pixBounds = (**hPix).bounds;
			GetPortBounds (pPort, &portBounds);
			deltaV = (portBounds.bottom - portBounds.top) - (pixBounds.bottom - pixBounds.top) +
			         (portBounds.bottom - portBounds.top - height) / 2;
			deltaH = -(portBounds.right - portBounds.left - width) / 2;
			if (deltaV || deltaH)
			{
				GrafPtr pPortSave;
				GetPort (&pPortSave);
				SetPort ((GrafPtr)pPort);
				// set origin to account for CG offset and if requested drawable smaller than screen rez
				SetOrigin (deltaH, deltaV);
				SetPort (pPortSave);
			}
		}
		return pPort;
	}
	else // Mac OS 9 or less
	{
		WindowPtr pWindow;
		Rect rectWin;
		RGBColor rgbSave;
		GrafPtr pGrafSave;
		// create a new window in our context 
		// note: OpenGL is expecting a window so it can enumerate the devices it spans, 
		// center window in our context's gdevice
		rectWin.top  = (short) ((**hGD).gdRect.top + ((**hGD).gdRect.bottom - (**hGD).gdRect.top) / 2);	  	// h center
		rectWin.top  -= (short) (height / 2);
		rectWin.left  = (short) ((**hGD).gdRect.left + ((**hGD).gdRect.right - (**hGD).gdRect.left) / 2);	// v center
		rectWin.left  -= (short) (width / 2);
		rectWin.right = (short) (rectWin.left + width);
		rectWin.bottom = (short) (rectWin.top + height);
		
		pWindow = NewCWindow (NULL, &rectWin, (unsigned char *)"p", 0, plainDBox, (WindowPtr)-1, 0, 0);	// quotes were \p, but that is an unknown ESCAPE-SEQUENCE

		// paint back ground black before fade in to avoid white background flash
		ShowWindow(pWindow);
		GetPort (&pGrafSave);
		SetPortWindowPort (pWindow);
		GetForeColor (&rgbSave);
		RGBForeColor (&rgbBlack);
		{
			Rect paintRect;
			GetWindowPortBounds (pWindow, &paintRect);
			PaintRect (&paintRect);
		}
		RGBForeColor (&rgbSave);		// ensure color is reset for proper blitting
		SetPort (pGrafSave);
		return (GetWindowPort (pWindow));
	}
}

void videodriver_event (EventRecord *theEvent, Boolean *fProcessed)
{	if (DSactive)
		DSpProcessEvent(theEvent,fProcessed);
}

// clean up DSp
void ShutdownDSp (CGrafPtr pDSpPort)
{
	if ((NULL != pDSpPort) && !isOSX ())
		DisposeWindow (GetWindowFromPort (pDSpPort));
	DSpContext_SetState( gContext, kDSpContextState_Inactive);
	DSpContext_Release (gContext);
}

// ****************************************************************************
// ***																		***
// ***								AGL Code								***
// ***																		***
// ****************************************************************************

AGLContext gaglContext = 0;

// OpenGL Cleanup
void CleanupAGL (void)
{	if (buffernumber) aglSwapBuffers(gaglContext);	// Make sure we're on the desktop buffer to avoid screen distortion
	aglSetDrawable (gaglContext, NULL);
	aglSetCurrentContext (NULL);
	aglDestroyContext (gaglContext);
}

void agl_shutdown(void)
{	ogl_shutdown();
	CleanupAGL (); // Cleanup the OpenGL context
	gaglContext = 0;
	if (DSactive)
	{	if (gpDSpPort)
		{	ShutdownDSp (gpDSpPort); // DSp shutdown
			gpDSpPort = NULL;
		}
		DSpShutdown ();
	}
}

//-----------------------------------------------------------------------------------------------------------------------

void SetupAGL(short width,short height,short depth)
{	AGLPixelFormat 	fmt;
	GLint			attrib[64];
	
	buffernumber = 0;
	screenbpp = depth;
	
// different possible pixel format choices for different renderers 
// basics requirements are RGBA and double buffer
// OpenGL will select acclerated context if available

	short i = 0;
	attrib [i++] = AGL_RGBA; // red green blue and alpha
	attrib [i++] = AGL_DOUBLEBUFFER; // double buffered
	attrib [i++] = AGL_ACCELERATED; // HWA pixel format only
	if (!DSactive)
		attrib [i++] = AGL_FULLSCREEN;
	attrib [i++] = AGL_PIXEL_SIZE;
	attrib [i++] = depth;
	attrib [i++] = AGL_DEPTH_SIZE;
	attrib [i++] = 24;
	attrib [i++] = AGL_STENCIL_SIZE;
	attrib [i++] = 8;
	
	if (depth==16)
	{	attrib[i++] = AGL_RED_SIZE;
		attrib[i++] = 5;
		attrib[i++] = AGL_GREEN_SIZE;
		attrib[i++] = 6;
		attrib[i++] = AGL_BLUE_SIZE;
		attrib[i++] = 5;
	}	else
	if (depth==32)
	{	attrib[i++] = AGL_ALPHA_SIZE;
		attrib[i++] = 8;
		attrib[i++] = AGL_RED_SIZE;
		attrib[i++] = 8;
		attrib[i++] = AGL_GREEN_SIZE;
		attrib[i++] = 8;
		attrib[i++] = AGL_BLUE_SIZE;
		attrib[i++] = 8;
	}
	attrib [i++] = AGL_NONE;

	if ((Ptr) kUnresolvedCFragSymbolAddress == (Ptr) aglChoosePixelFormat) // check for existance of OpenGL
	{	FatalError (ERR_OGLNotInstalled, NULL);
	}	

	if (hTargetDevice)
		fmt = aglChoosePixelFormat(&hTargetDevice, 1, attrib); // this may fail if looking for acclerated across multiple monitors
	else
		fmt = aglChoosePixelFormat(NULL, 0, attrib); // this may fail if looking for acclerated across multiple monitors
	if (NULL == fmt) 
	{	FatalError (ERR_OGLPixelFormat, (char *)aglErrorString(aglGetError()));
	}

	gaglContext = aglCreateContext (fmt, NULL); // Create an AGL context
	if (NULL == gaglContext)
	{	FatalError (ERR_OGLCreateContext, (char *)aglErrorString(aglGetError()));
	}

	if (DSactive)
	{	if (!aglSetDrawable (gaglContext, gpDSpPort)) // attach the window to the context
		{	FatalError (ERR_OGL_SetDrawable, (char *)aglErrorString(aglGetError()));
		}
	}	else
	{	if (!aglSetFullScreen (gaglContext, width, height, 60, 0))
		{	FatalError (ERR_OGL_SetFullScreen, (char *)aglErrorString(aglGetError()));
		}
	}

	if (!aglSetCurrentContext (gaglContext)) // make the context the current context
	{	GLenum AGLError = aglGetError();
		CleanupAGL ();
		FatalError (ERR_OGL_SetCurrentContext, (char *)aglErrorString(AGLError));
	}

	aglDestroyPixelFormat(fmt); // pixel format is no longer needed
}

// --------------------------------------------------------------------------

void agl_update(void)
{	buffernumber = 1 - buffernumber;
	aglSwapBuffers(gaglContext); // send swap command
}

/*
extern	uintf	winmousex,winmousey;	
extern	intf	winmousebut;				
extern	intf	winmousewheel;

static pascal OSStatus myWindowEvtHndlr (EventHandlerCallRef myHandler, EventRef event, void* userData)
{	uint16 button;
	uint32 eventkind = GetEventKind(event);
    switch (eventkind)
	{	case kEventMouseUp :
			GetEventParameter (event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(button), NULL, button);
			switch (button)
			{	case 1:	winmousebut |= 1; break;
				case 2:	winmousebut |= 2; break;
				case 3:	winmousebut |= 4; break;
			}
			break;
	}
	return noErr;
}
*/

DLL_EXPORT void *vd_init3D(void *_classfinder, intf width, intf height, intf bpp)
{	if (bpp==0) 
	{	FatalError(ERR_WindowNotSupported, NULL);
	}
	
	GDHandle hGD;
	short numDevices = 0;
	short whichDevice = 0; // number of device to try (0 = 1st device)

	gaglContext = 0;
	
	hGD = DMGetFirstScreenDevice (true); // check number of screens
	hTargetDevice = hGD; // default to first device							
	do
	{	if (numDevices == whichDevice)
			hTargetDevice = hGD; // if device number matches						
		numDevices++;
		hGD = DMGetNextScreenDevice (hGD, true);
	}
	while (hGD);

	short sdepth = bpp, swidth = width, sheight = height; 

	// Try and initialize the screen with just AGL (faster)
	if (isOSX ())
	{	SetupAGL (swidth, sheight,sdepth); // Setup the OpenGL context
		SetRect (&gRectPort, 0, 0, swidth, sheight); // l, t, r, b
		//printf ("AGL Full Screen: %d x %d x %d\n", swidth, sheight, sdepth);
	}

	// If AGL fails, try DrawSprocket
	if (!gaglContext)
	{	initDrawSprocket();
		//printf("DrawSprocket Version %i.%i Detected\n",DSversion>>8,DSversion&0xff);
		if (NULL != (gpDSpPort = SetupDSpFullScreen (hTargetDevice,swidth,sheight,sdepth))) // Setup DSp for OpenGL sets hTargetDeviceto device actually used 
		{	GetPortBounds (gpDSpPort, &gRectPort);
			SetupAGL (swidth,sheight,sdepth);
			//printf ("DrawSprocket Full Screen: %d x %d x %d\n", gRectPort.right - gRectPort.left, gRectPort.bottom - gRectPort.top, (**(**hTargetDevice).gdPMap).pixelSize);
		}
	}

	if (!gaglContext)
	{	printf ("Failed to initialize graphics system\nYou could try upgrading to OSX 10.1 or higher\nor alternatively, upgrade DrawSprocket to Version 1.99 or better\n");
		ExitToShell();
	}
	
	InitOGLExtensions();
//	Rect *pRectPort = &gRectPort;
//	GLint vwidth = pRectPort->right - pRectPort->left;
//	GLint vheight = pRectPort->bottom - pRectPort->top;
	InitOGL(_classfinder, width, height, bpp);
//	glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE);		// Although this works, it's horribly slow compared to my alpha-roll code
/*
	EventHandlerRef  ref;
	EventHandlerUPP gWinEvtHandler;      // window event handler
	EventTypeSpec  list[] = { 	//{ kEventClassWindow, 	kEventWindowShowing },
								//{ kEventClassWindow, 	kEventWindowClose },
								//{ kEventClassWindow, 	kEventWindowDrawContent },
								{ kEventClassMouse, 	kEventMouseUp },
								{ kEventClassMouse, 	kEventMouseDown } };
								//{ kEventClassMouse,		kEventMouseDragged },
								//{ kEventClassWindow, 	kEventWindowResizeCompleted },
								//{ kEventClassWindow, 	kEventWindowDragCompleted },
								//{ kEventClassWindow, 	kEventWindowZoomed} };
	WindowRef window = FrontWindow ();
	gWinEvtHandler = NewEventHandlerUPP(myWindowEvtHndlr);
	InstallWindowEventHandler (window, gWinEvtHandler, GetEventTypeCount (list), list, (void*) window, &ref);
*/
	return (void *)agl_getfunc;
}

void agl_notimplemented(void)
{
}

void agl_NULL(void)
{
}

struct functype
{	char name[25];
	void *fnptr;
} vidfuncs[]={	{"shutdown3d",			(void *)agl_shutdown},
				{"update",				(void *)agl_update},
				{"get3dpixel",			(void *)agl_NULL},//wgl_get3dpixel},
				{"apprestore",			(void *)ogl_apprestore},
				{"lock3d",				(void *)ogl_fakelock3d},
				{"unlock3d",			(void *)ogl_fakeunlock3d},
				{"enableZbuffer",		(void *)ogl_enableZbuffer},
				{"disableZbuffer",		(void *)ogl_disableZbuffer},
				{"enableZwrites",		(void *)ogl_enableZwrites},
				{"disableZwrites",		(void *)ogl_disableZwrites},
				{"enablespecular",		(void *)agl_NULL},//wgl_enablespecular},
				{"disablespecular",		(void *)agl_NULL},//wgl_disablespecular},
				{"enableAA",			(void *)ogl_enableAA},
				{"disableAA",			(void *)ogl_disableAA},
				{"setshademode",		(void *)ogl_setshademode},
				{"setfillmode",			(void *)ogl_setfillmode},
				{"sethorizon",			(void *)ogl_sethorizon},
				{"setambientlight",		(void *)ogl_setambientlight},
				{"setspecularcolor",	(void *)agl_NULL},//wgl_setspecularcolor},
				{"setfog",				(void *)agl_NULL},//wgl_setfog},
				{"enablefog",			(void *)agl_NULL},//wgl_enablefog},
				{"disablefog",			(void *)agl_NULL},//wgl_disablefog},
				{"enabledithering",		(void *)ogl_enabledithering},
				{"disabledithering",	(void *)ogl_disabledithering},
				{"setzoom",				(void *)agl_NULL},//wgl_setzoom},
				{"getstatesize",		(void *)ogl_getstatesize},
				{"savestate",			(void *)agl_NULL},//wgl_savestate},
				{"restorestate",		(void *)ogl_restorestate},
				{"rendermode",			(void *)agl_NULL},//wgl_rendermode},
				{"setreflection",		(void *)agl_NULL},//wgl_setreflection},
				{"setbumpscale",		(void *)agl_NULL},//wgl_setbumpscale},
				{"newtextures",			(void *)ogl_newtextures},
				{"downloadbitmaptex",	(void *)ogl_downloadbmtex},
				{"downloadcubemapface",	(void *)agl_NULL},//wgl_downloadcubemapface},
				{"cleartexture",		(void *)ogl_cleartexture},
				{"updatecammatrix",		(void *)ogl_updatecammatrix},
				{"specmodecolor",		(void *)agl_NULL},//wgl_specmodecolor},
				{"disablelights",		(void *)ogl_disablelights},
				{"changelight",			(void *)ogl_changelight},
				{"test_fov",			(void *)ogl_test_fov},
				{"testobjvis",			(void *)ogl_testobjvis},
				{"changeviewport",		(void *)ogl_changeviewport},
				{"zcls",				(void *)ogl_zcls},
				{"cls",					(void *)ogl_cls},
				{"beginscene",			(void *)agl_NULL},//wgl_beginscene},
				{"endscene",			(void *)agl_NULL},//wgl_endscene},
				{"renderobj3d",			(void *)ogl_renderobj3d},
				{"bucketobject",		(void *)ogl_bucketobject},
				{"flushZsort",			(void *)ogl_flushZsort},
				{"objconvert",			(void *)ogl_convertobj3d},
				{"removepreconvertbuf",	(void *)agl_NULL},//wgl_removepreconvertbuf},
				{"getvideoinfo",		(void *)ogl_getvideoinfo},
				{"world2screen",		(void *)ogl_world2screen},
				//{"screen2vec",		(void *)wgl_NULL},//wgl_screen2vec},
};

void agl_getfunc(char *fname, void **func)
{	// Given a text description of a function, return a pointer to it
	intf numfuncs = sizeof(vidfuncs)/sizeof(functype);
	for (intf i=0; i<numfuncs; i++)
	{	if (stricmp(vidfuncs[i].name,fname)==0) 
		{	*func = vidfuncs[i].fnptr;
			return;
		}
	}
	*func = (void *)agl_notimplemented;
}

