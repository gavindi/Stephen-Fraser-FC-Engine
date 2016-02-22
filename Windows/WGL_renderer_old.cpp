/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
// --------------------------------------------------- Generic_OGL.h --------------------------------------------
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

#ifdef _WIN32
	#define WIN32
#endif

#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#include "../glext.h"
#define need_protos
#define glGetAddress wglGetProcAddress
#endif

#ifdef linux
#include <GL/gl.h>
#include "../glext.h"
#define need_protos
#define glGetAddress glXGetProcAddress
#endif

#include "../fc_h.h"

bool ogl_getfunc(char *fname, void **func);
void InitOGL(void *(*classfinder)(const char *), intf width, intf height, intf bpp);
void ShutDownOGL(void);
void ogl_updatecammatrix(void);


// ----------------------------------------------------------------------------------------------------------------------

#include "fc_win.h"

#define driver_Version	1000						// Video driver built against FC engine kernel version 1.000

void wgl_getfunc(char *fname, void **func);
void wgl_shutdown(void);
void (*wgl_msg)(const char *, const char *);

sWinVars	*wgl_WinVars;
//uintf		*wglscreenbpp;
HDC			wglhDC = NULL;
HGLRC		glRC = NULL;

DLL_EXPORT void *vd_init3D(void *_classfinder, intf width, intf height, intf bpp)
{	GLuint	PixelFormat;										// Holds The Results After Searching For A Match
	
	void *(*classfinder)(const char *) = (void *(*)(const char *))_classfinder;
	wgl_WinVars	= (sWinVars*)		classfinder("OEM");
	wgl_msg		= (void	(*)(const char *, const char *)) classfinder("msg");
	if (wgl_WinVars->minVersion > FC_BuildVer) 
		wgl_msg("Video Driver Obsolete","The video driver is too old for this application");

// ********************
//	wglscreenbpp	= (uintf *)ogl_graphics->getvarptr("screenbpp");

	bool fullscreen = (bpp>0);
	if (fullscreen)													// Attempt Fullscreen Mode?
	{
//###		showmouse(false);
		DEVMODE dmScreenSettings;									// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));		// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);			// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;					// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;					// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bpp;						// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
			fullscreen=FALSE;										// Windowed Mode Selected.  Fullscreen = FALSE
	}
	win32CreateWindow(width, height, bpp);
	
	if (fullscreen)
	{	SetWindowLong(wgl_WinVars->mainWin, GWL_STYLE, 0);
		SetWindowPos(wgl_WinVars->mainWin,HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_SHOWWINDOW);
	}
	static	PIXELFORMATDESCRIPTOR pfd=								// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),								// Size Of This Pixel Format Descriptor
		1,															// Version Number
		PFD_DRAW_TO_WINDOW |										// Format Must Support Window
		PFD_SUPPORT_OPENGL |										// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,											// Must Support Double Buffering
		PFD_TYPE_RGBA,												// Request An RGBA Format
		(byte)bpp,													// Select Our Color Depth
		0, 0, 0, 0, 0, 0,											// Color Bits Ignored
		0,															// No Alpha Buffer
		0,															// Shift Bit Ignored
		0,															// No Accumulation Buffer
		0, 0, 0, 0,													// Accumulation Bits Ignored
		24,															// 16Bit Z-Buffer (Depth Buffer)  
		8,															// No Stencil Buffer
		0,															// No Auxiliary Buffer
		PFD_MAIN_PLANE,												// Main Drawing Layer
		0,															// Reserved
		0, 0, 0														// Layer Masks Ignored
	};
	
	if (!(wglhDC=GetDC(wgl_WinVars->mainWin)))						// Did We Get A Device Context?
	{	wgl_shutdown();												// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;												// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(wglhDC,&pfd)))					// Did Windows Find A Matching Pixel Format?
	{	wgl_shutdown();												// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;												// Return FALSE
	}

	if(!SetPixelFormat(wglhDC,PixelFormat,&pfd))					// Are We Able To Set The Pixel Format?
	{	wgl_shutdown();												// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;												// Return FALSE
	}

	if (!(glRC=wglCreateContext(wglhDC)))							// Are We Able To Get A Rendering Context?
	{
		wgl_shutdown();												// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;												// Return FALSE
	}

	if(!wglMakeCurrent(wglhDC,glRC))								// Try To Activate The Rendering Context
	{	wgl_shutdown();												// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;												// Return FALSE
	}

	ShowWindow(wgl_WinVars->mainWin,SW_SHOW);						// Show The Window
	SetForegroundWindow(wgl_WinVars->mainWin);						// Slightly Higher Priority
	SetFocus(wgl_WinVars->mainWin);									// Sets Keyboard Focus To The Window

/*	// Aquire important functions
	if (ogl_ExtensionSupported("GL_ARB_multitexture"))
	{	glMultiTexCoord1fARB	= (PFNGLMULTITEXCOORD1FARBPROC)		wglGetProcAddress("glMultiTexCoord1fARB");
		glMultiTexCoord2fARB	= (PFNGLMULTITEXCOORD2FARBPROC)		wglGetProcAddress("glMultiTexCoord2fARB");
		glMultiTexCoord3fARB	= (PFNGLMULTITEXCOORD3FARBPROC)		wglGetProcAddress("glMultiTexCoord3fARB");
		glMultiTexCoord4fARB	= (PFNGLMULTITEXCOORD4FARBPROC)		wglGetProcAddress("glMultiTexCoord4fARB");
		glActiveTextureARB		= (PFNGLACTIVETEXTUREARBPROC)		wglGetProcAddress("glActiveTextureARB");
		glClientActiveTextureARB= (PFNGLCLIENTACTIVETEXTUREARBPROC)	wglGetProcAddress("glClientActiveTextureARB");		
		glCompressedTexImage2D  = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)	wglGetProcAddress("glCompressedTexImage2D");
	}
*/
	InitOGL(classfinder, width, height, bpp);
	return wgl_getfunc;
}

void wgl_shutdown(void)
{	ShutDownOGL();
	if (wglhDC) ReleaseDC(wgl_WinVars->mainWin,wglhDC);
}

void wgl_update(void)
{	SwapBuffers(wglhDC);
}

struct wgl_functype
{	const char name[25];
	void *fnptr;
} wgl_vidfuncs[]={	{"shutDownVideo",		wgl_shutdown},
//					{"positionwindow",		ogl_positionWindow},
					{"update",				wgl_update}
};

void wgl_getfunc(char *fname, void **func)
{	// Given a text description of a function, return a pointer to it
	if (ogl_getfunc(fname,func)) return;
	
	uintf numfuncs = sizeof(wgl_vidfuncs)/sizeof(wgl_functype);
	for (uintf i=0; i<numfuncs; i++)
	{	if (txticmp(wgl_vidfuncs[i].name,fname)==0) 
		{	*func = wgl_vidfuncs[i].fnptr;
			return;
		}
	}
	msg("OpenGL (WGL) Error",buildstr("Unknown Video Function: %s",fname));
}

// ### This func does not belong here
void StandardVSMatrixBuilder(float *result, float *InverseCameraProjectionMatrix, float *ObjectMatrix)
{	// Multiply matrix a * b and store in result, but transpose result along the way
	long element = 0;
	for( long i=0; i<4; i++ ) 
    {	long ii=i<<2;
		for( long j=0; j<4; j++ ) 
		{	result[element] =	InverseCameraProjectionMatrix[j   ] * ObjectMatrix[ii  ] +
								InverseCameraProjectionMatrix[j+ 4] * ObjectMatrix[ii+1] +
								InverseCameraProjectionMatrix[j+ 8] * ObjectMatrix[ii+2] +
								InverseCameraProjectionMatrix[j+12] * ObjectMatrix[ii+3];
			element+=4;
		}
		element -=15;
	}
}

