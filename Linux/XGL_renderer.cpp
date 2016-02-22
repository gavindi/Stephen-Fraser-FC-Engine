/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <GL/gl.h>					// Main OpenGL header
#include <GL/glu.h>					// OpenGL Utilities
#include <GL/glx.h>					// GLX additions

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include <stdio.h>                  // Standard IO (sprintf)
#include <math.h>
#include "../fc_h.h"

#include "fc_linux.h"

extern void	(*FileDropCallback)(const char *filename, intf x, intf y);	// Callback used when a file is dropped on the window.  X and Y indicate the point where the files were dropped.  If X<0, then the drop's position is unknown (maybe it was faked via another application)

void OSGL_deactivate(void);
void ogl_changeSize(uintf w, uintf h);

// ------------------------------------------------ Error and Warning text ----------------------------------------------------------
/*
#define wglTextDef \
txtCode(wgltxt_regClass1,"Failed to register temporary Window Class")		\
txtCode(wgltxt_noWindow1,"Failed to create temporary Window")				\
txtCode(wgltxt_noWindow2,"Failed to create OpenGL Window")

#define txtCode(name, text) name,
enum {
wglTextDef
};
#undef txtCode

#define txtCode(name, text) text,
const char *wglText[]={
wglTextDef
};
#undef txtCode
*/

// ------------------------------------------------ OS-Specific Variables -------------------------------------------------------------
unsigned int X11eventMask =
KeyPressMask			|
KeyReleaseMask			|
ButtonPressMask			|
ButtonReleaseMask		|
EnterWindowMask			|
LeaveWindowMask			|
PointerMotionMask		|
Button1MotionMask		|
Button2MotionMask		|
Button3MotionMask		|
Button4MotionMask		|
Button5MotionMask		|
ButtonMotionMask		|
KeymapStateMask			|
ExposureMask			|
VisibilityChangeMask	|
StructureNotifyMask		|		// Needed for ClientEvent (Timers)
ResizeRedirectMask		|
SubstructureNotifyMask	|
SubstructureRedirectMask|
FocusChangeMask			|
PropertyChangeMask		|
ColormapChangeMask		|
OwnerGrabButtonMask		;

GLXContext	glxContext = NULL;						// OpenGL Context

bool		OSGL_initialized = false;
bool		OSGL_isFullScreen;
bool		OSGL_FSAA;
bool		OSGL_VSYNC = true;

void OSGL_preInit(uintf flags)
{	// OpenGL initialisation is mostly handled in the main fc_openGL.cpp file, however it starts off by calling this function to initialise any OS-specific stuff

	// Full screen or windowed?
	if ((flags & 1)==0)
		OSGL_isFullScreen = false;
	else
		OSGL_isFullScreen = true;

	// Full Screen Anti-Aliasing
	if ((flags&2) == 0)
		OSGL_FSAA = true;
	else
		OSGL_FSAA = false;

//	OSGL_useVerticalSync = TRUE;	// Vertical Sync.	if (gltIsExtSupported("WGL_EXT_swap_control"))

//	winVars.deactivate = OSGL_deactivate;
//	winVars.changeSize = ogl_changeSize;
}

void OSGL_postInit(uintf flags)
{   // Set up VSYNC if possible

	// Get swap interval function pointer if it exists
//	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
//    if(wglSwapIntervalEXT != NULL && OSGL_VSYNC == TRUE)
//        wglSwapIntervalEXT(1);
}

extern Texture*	lastBindedTexture[];

void OSGL_update(void)
{	 glXSwapBuffers(linuxVars.dpy, linuxVars.mainWin);
	// Clear all renderring cache
	lastBindedTexture[0]=NULL;
}

// ****************************************** Window Handling Functions ********************************************
/*
LRESULT CALLBACK wgl_tmpWndProc(HWND hWnd,	UINT message, WPARAM wParam, LPARAM lParam)
{	switch (message)
	{	// Window creation, setup for OpenGL
		case WM_CREATE:
			break;

		// The painting function.  This message sent by Windows
		// whenever the screen needs updating.
		case WM_PAINT:
		{	PAINTSTRUCT ps;
			HDC bp = BeginPaint(hWnd,&ps);
			EndPaint(hWnd,&ps);
		}
		break;

        default:   // Passes it on if unproccessed
			return (DefWindowProc(hWnd, message, wParam, lParam));
	}

	return (0L);
}
*/

/*
int gltIsWGLExtSupported(HDC hDC, const char *szExtension)
{	GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	where = (GLubyte *) strchr(szExtension, ' ');
	if (where || *szExtension == '\0')
		return 0;

	extensions = glXQueryExtensionsString(linuxVars.dpy, screen);

	start = extensions;
	for (;;)
	{	where = (GLubyte *) strstr((const char *) start, szExtension);
		if (!where)
			break;

		terminator = where + strlen(szExtension);

		if (where == start || *(where - 1) == ' ')
			{
			if (*terminator == ' ' || *terminator == '\0')
				return 1;
			}
		start = terminator;
		}
	return 0;
	}
*/
/*
void wgl_FindBestPF(HDC hDC, int *nRegularFormat, int *nMSFormat)
{	*nRegularFormat = 0;
	*nMSFormat = 0;

	// easy check, just look for the entrypoint
	if(gltIsWGLExtSupported(hDC, "WGL_ARB_pixel_format"))
		if(wglGetPixelFormatAttribivARB == NULL)
			wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");

    // First try to use new extended wgl way
    if(wglGetPixelFormatAttribivARB != NULL)
        {
        // Only care about these attributes
        int nBestMS = 0;
        int i;
        int iResults[9];
        int iAttributes [9] = {	WGL_SUPPORT_OPENGL_ARB, // 0
								WGL_ACCELERATION_ARB,   // 1
								WGL_DRAW_TO_WINDOW_ARB, // 2
								WGL_DOUBLE_BUFFER_ARB,  // 3
								WGL_PIXEL_TYPE_ARB,     // 4
								WGL_DEPTH_BITS_ARB,     // 5
								WGL_STENCIL_BITS_ARB,   // 6
								WGL_SAMPLE_BUFFERS_ARB, // 7
								WGL_SAMPLES_ARB}; 	    // 8

        // How many pixelformats are there?
        int nFormatCount[] = { 0 };
        int attrib[] = { WGL_NUMBER_PIXEL_FORMATS_ARB };
        wglGetPixelFormatAttribivARB(hDC, 1, 0, 1, attrib, nFormatCount);

        // Loop through all the formats and look at each one
        for(i = 0; i < nFormatCount[0]; i++)
            {
            // Query pixel format
            wglGetPixelFormatAttribivARB(hDC, i+1, 0, 10, iAttributes, iResults);

            // Match? Must support OpenGL AND be Accelerated AND draw to Window
            if(iResults[0] == 1 && iResults[1] == WGL_FULL_ACCELERATION_ARB && iResults[2] == 1)
            if(iResults[3] == 1)                    // Double buffered
            if(iResults[4] == WGL_TYPE_RGBA_ARB)    // Full Color
            if(iResults[5] >= 16)                   // Any Depth greater than 16
            if(iResults[6] > 0)                     // Any Stencil depth (not zero)
                {
                // We have a candidate, look for most samples if multisampled
                if(iResults[7] == 1)	            // Multisampled
                    {
                    if(iResults[8] > nBestMS)       // Look for most samples
                        {
                        *nMSFormat = i;			// Multisamples
                        nBestMS = iResults[8];	// Looking for the best
                        }
                    }
                else // Not multisampled
                    {
                    // Good enough for "regular". This will fall through
                    *nRegularFormat = i;
                    }
                }
            }
        }
    else
        {
        // Old fashioned way...
        // or multisample
        PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,      // Full color
        32,                 // Color depth
        0,0,0,0,0,0,0,      // Ignored
        0,0,0,0,            // Accumulation buffer
        24,                 // Depth bits
        8,                  // Stencil bits
        0,0,0,0,0,0 };      // Some used, some not

        *nRegularFormat = ChoosePixelFormat(hDC, &pfd);
	}
}
*/

/*
void wgl_createMyWindow(BOOL fullScreen, HWND *hWnd)
{	int x,y;
	UINT uiStyle,uiStyleX;
	if (*hWnd)
	{	if (!fullScreen)
			ChangeDisplaySettings(NULL,0);
	}
	if (fullScreen)
	{	wgl_devMode.dmPelsWidth = wgl_PrefFSWidth;			// Retrieve preferred screen dimensions (theres were previously enumerated, so they are known to be safe)
		wgl_devMode.dmPelsHeight = wgl_PrefFSHeight;

		ChangeDisplaySettings(&wgl_devMode, CDS_FULLSCREEN);
		uiStyle = WS_POPUP;
		uiStyleX = WS_EX_TOPMOST;
		x = 0;
		y = 0;
	}	else
	{	uiStyle = WS_OVERLAPPEDWINDOW;
		uiStyleX = 0;
		RECT windowedRECT;
		SystemParametersInfo(SPI_GETWORKAREA,0,(PVOID*)&windowedRECT,0);
		uintf w = windowedRECT.right - windowedRECT.left;
		uintf h = windowedRECT.bottom - windowedRECT.top;
		if (wgl_devMode.dmPelsWidth  > w) wgl_devMode.dmPelsWidth  = w;
		if (wgl_devMode.dmPelsHeight > h) wgl_devMode.dmPelsHeight = h;
		x = (w - wgl_devMode.dmPelsWidth) / 2;
		y = (h - wgl_devMode.dmPelsHeight) / 2;
	}
	if (*hWnd)
	{	SetWindowLongPtr(*hWnd, GWL_STYLE, uiStyle);
		SetWindowLongPtr(*hWnd, GWL_EXSTYLE, uiStyleX);
		MoveWindow(*hWnd, x,y, wgl_devMode.dmPelsWidth, wgl_devMode.dmPelsHeight, TRUE);
	}	else
	{	*hWnd = CreateWindowEx(uiStyleX, winVars.wc.lpszClassName, programName, uiStyle,
      				x, y, wgl_devMode.dmPelsWidth, wgl_devMode.dmPelsHeight, NULL, NULL, winVars.hinst, NULL);
	}
	// Make sure window manager stays hidden
	//wgl_onWinCreate(*hWnd);
	ShowWindow(*hWnd,SW_SHOW);
	UpdateWindow(*hWnd);
	OSGL_initialized = true;
	if (FileDropCallback)
		DragAcceptFiles(*hWnd, true);
}
*/

int attrListIdeal[] =
{	GLX_RGBA, GLX_DOUBLEBUFFER,
	GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_DEPTH_SIZE, 24,
    GLX_STENCIL_SIZE, 8,
    None
};

int attrListWorkable[] =
{	GLX_RGBA, GLX_DOUBLEBUFFER,
	GLX_RED_SIZE, 4,
    GLX_GREEN_SIZE, 4,
    GLX_BLUE_SIZE, 4,
    GLX_DEPTH_SIZE, 16,
    None
};

extern XEvent timerMsg;

void OSGL_createOpenGLWindow(uintf width, uintf height)
{	// Supposed to be parameters
	bool fullscreenflag = false;
	const char *title = "GLX Test Application";

	// Component Version numbers
    int vidModeMajorVersion, vidModeMinorVersion;	// XF86 Video Mode version
    int glxMajorVersion, glxMinorVersion;			// GLX version (this is NOT the OpenGL version)

    XF86VidModeModeInfo **modes;
    int modeNum;

	// Local variables
    XVisualInfo *vi;
    int dpyWidth, dpyHeight;
    int i;
    int bestMode;
    Atom wmDelete;
    Window winDummy;
    unsigned int borderDummy;

    linuxVars.isFullScreen = fullscreenflag;
    // set best mode to current
    bestMode = 0;
    // get a connection
    linuxVars.screen = DefaultScreen(linuxVars.dpy);
    XF86VidModeQueryVersion(linuxVars.dpy, &vidModeMajorVersion, &vidModeMinorVersion);
    XF86VidModeGetAllModeLines(linuxVars.dpy, linuxVars.screen, &modeNum, &modes);
    // save desktop-resolution before switching modes
    linuxVars.desktopMode = *modes[0];
    // look for mode with requested resolution
    for (i = 0; i < modeNum; i++)
    {	if ((modes[i]->hdisplay == width) && (modes[i]->vdisplay == height))
        {
            bestMode = i;
        }
    }
    /* get an appropriate visual */
    vi = glXChooseVisual(linuxVars.dpy, linuxVars.screen, attrListIdeal);
    if (vi == NULL)
    {   vi = glXChooseVisual(linuxVars.dpy, linuxVars.screen, attrListWorkable);
    }
    if (vi == NULL)
    {   msg("Error Initializing OpenGL","Sorry, it appears your computer is unable to run this program.\nI failed to obtain a 'Visual' with at least 16 bit color and a depth buffer of 16 bits.\nYou could try upgrading your video driver and/or video card.");
    }
    glXQueryVersion(linuxVars.dpy, &glxMajorVersion, &glxMinorVersion);
    // create a GLX context
	glxContext = glXCreateContext(linuxVars.dpy, vi, 0, GL_TRUE);
    // create a color map
    linuxVars.attr.colormap = XCreateColormap(linuxVars.dpy, RootWindow(linuxVars.dpy, vi->screen), vi->visual, AllocNone);
    linuxVars.attr.border_pixel = 0;

    if (linuxVars.isFullScreen)
    {	XF86VidModeSwitchToMode(linuxVars.dpy, linuxVars.screen, modes[bestMode]);
        XF86VidModeSetViewPort(linuxVars.dpy, linuxVars.screen, 0, 0);
        dpyWidth = modes[bestMode]->hdisplay;
        dpyHeight = modes[bestMode]->vdisplay;
        //printf("Resolution %dx%d\n", dpyWidth, dpyHeight);
        XFree(modes);

        // create a fullscreen window
        linuxVars.attr.override_redirect = True;
        linuxVars.attr.event_mask = X11eventMask;// ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | StructureNotifyMask;
        linuxVars.mainWin = XCreateWindow(linuxVars.dpy, RootWindow(linuxVars.dpy, vi->screen),
            0, 0, dpyWidth, dpyHeight, 0, vi->depth, InputOutput, vi->visual,
            CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect, &linuxVars.attr);
        XWarpPointer(linuxVars.dpy, None, linuxVars.mainWin, 0, 0, 0, 0, 0, 0);
		XMapRaised(linuxVars.dpy, linuxVars.mainWin);
        XGrabKeyboard(linuxVars.dpy, linuxVars.mainWin, True, GrabModeAsync, GrabModeAsync, CurrentTime);
        XGrabPointer(linuxVars.dpy, linuxVars.mainWin, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, linuxVars.mainWin, None, CurrentTime);
    }
    else
    {	// create a window in window mode
        linuxVars.attr.event_mask = X11eventMask;/* ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | StructureNotifyMask; */
        linuxVars.mainWin = XCreateWindow(linuxVars.dpy, RootWindow(linuxVars.dpy, vi->screen),
            0, 0, width, height, 0, vi->depth, InputOutput, vi->visual,
            CWBorderPixel | CWColormap | CWEventMask, &linuxVars.attr);
        /* only set window title and handle wm_delete_events if in windowed mode */
        wmDelete = XInternAtom(linuxVars.dpy, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(linuxVars.dpy, linuxVars.mainWin, &wmDelete, 1);
        XSetStandardProperties(linuxVars.dpy, linuxVars.mainWin, title, title, None, NULL, 0, NULL);
        XMapRaised(linuxVars.dpy, linuxVars.mainWin);
    }

	timerMsg.xclient.window = linuxVars.mainWin;			// Set up the timer's target window

    // connect the glx-context to the window
    glXMakeCurrent(linuxVars.dpy, linuxVars.mainWin, glxContext);
    XGetGeometry(linuxVars.dpy, linuxVars.mainWin, &winDummy, &linuxVars.x, &linuxVars.y, &linuxVars.width, &linuxVars.height, &borderDummy, &linuxVars.depth);
    //printf("Depth %d\n", linuxVars.depth);
//    if (glXIsDirect(linuxVars.dpy, glxContext))
//        printf("Congrats, you have Direct Rendering!\n");
//    else
//        printf("Sorry, no Direct Rendering possible!\n");

	// This block was previously inside a function called ... OSGL_onWinCreate(winVars.mainWin)
	//wgl_hDC = GetDC(hWnd);												// Store the device context
	//wgl_FindBestPF(wgl_hDC, &wgl_PixelFormatSTD, &wgl_PixelFormatFSAA);	// The screen and desktop may have changed, so do this again

	// Set pixelformat
	//if(OSGL_FSAA == TRUE && (wgl_PixelFormatFSAA != 0))
	//	SetPixelFormat(wgl_hDC, wgl_PixelFormatFSAA, NULL);
	//else
	//	SetPixelFormat(wgl_hDC, wgl_PixelFormatSTD, NULL);

	// Create the rendering context and make it current
	//wgl_hRC = wglCreateContext(wgl_hDC);
	//wglMakeCurrent(wgl_hDC, wgl_hRC);

	OSGL_initialized = true;
	applicationState = AppState_running;
}

/*
void OSGL_createOpenGLWindow(uintf width, uintf height)
{	// Create a temporary window so we can query some values from the video driver
	int nPF;
	HDC				tmphDC;				// Temporary device context
	HGLRC			tmphRC;				// Temporary Renderring Context
	WNDCLASS		tmpWc;				// Temporary Windows class structure
	HWND			tmphWnd;			// Temporary Window Handle
	uintf			iMode;

	// Register Window style
    tmpWc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    tmpWc.lpfnWndProc	= (WNDPROC) wgl_tmpWndProc;
    tmpWc.cbClsExtra	= 0;
    tmpWc.cbWndExtra	= 0;
    tmpWc.hInstance 	= winVars.hinst;
    tmpWc.hIcon			= NULL;
    tmpWc.hCursor		= LoadCursor(NULL, IDC_ARROW);
    tmpWc.hbrBackground	= NULL;										// No need for background brush for OpenGL window
    tmpWc.lpszMenuName	= NULL;
    tmpWc.lpszClassName	= "Windows GL Temp Window";
    if(RegisterClass(&tmpWc) == 0)
		wglError(wgltxt_regClass1);
	tmphWnd = CreateWindowEx(WS_EX_TOPMOST, tmpWc.lpszClassName, tmpWc.lpszClassName, WS_POPUP, 0, 0, 400, 400, NULL, NULL, winVars.hinst, NULL);
	if(tmphWnd == NULL)
		wglError(wgltxt_noWindow1);
	ShowWindow(tmphWnd,SW_SHOW);
	UpdateWindow(tmphWnd);

	PIXELFORMATDESCRIPTOR pfd =		// Not going to be too picky
	{   sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,		// Full color
		32,					// Color depth
		0,0,0,0,0,0,0,		// Ignored
		0,0,0,0,		    // Accumulation buffer
		24,					// Depth bits
		8,					// Stencil bits
		0,0,0,0,0,0 };		// Some used, some not

	// Create a "temporary" OpenGL rendering context
	tmphDC = GetDC(tmphWnd);

	// Set pixel format one time....
	nPF = ChoosePixelFormat(tmphDC, &pfd);
	SetPixelFormat(tmphDC, nPF, &pfd);
	DescribePixelFormat(tmphDC, nPF, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	// Create the GL context
	tmphRC = wglCreateContext(tmphDC);
	wglMakeCurrent(tmphDC, tmphRC);

	// Find a multisampled and non-multisampled pixel format
	wgl_FindBestPF(tmphDC, &wgl_PixelFormatSTD, &wgl_PixelFormatFSAA);

	iMode = 0;
	int bestMode = 0;
	int bestScore = 0x7FFFFFFF;
	while (EnumDisplaySettings(NULL, iMode, &wgl_devMode))
	{	int score = 1;
		if (wgl_devMode.dmBitsPerPel!=32) score *= 256;
		int xDif = abs((int)(wgl_devMode.dmPelsWidth - width))+1;
		int yDif = abs((int)(wgl_devMode.dmPelsHeight - height))+1;
		score *= xDif * yDif;
		if (score<bestScore)
		{	bestScore = score;
			bestMode = iMode;
		}
		iMode++;
	}
	EnumDisplaySettings(NULL, bestMode, &wgl_devMode);
	wgl_PrefFSWidth = wgl_devMode.dmPelsWidth;			// Preferred fullscreen width, height
	wgl_PrefFSHeight = wgl_devMode.dmPelsHeight;

	// Done with GL context
	wglMakeCurrent(tmphDC, NULL);
	wglDeleteContext(tmphRC);
	DestroyWindow(tmphWnd);

	// ***************** Now create the real OGL window
	wgl_createMyWindow(OSGL_isFullScreen, &winVars.mainWin);
	// If window was not created, quit
	if(winVars.mainWin == NULL)
		wglError(wgltxt_noWindow2);

	MSG			msg;				// Windows message structure
	while (applicationState!=AppState_running)
	{	GetMessage(&msg, NULL, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	OSGL_onWinCreate(winVars.mainWin);
}

void OSGL_deactivate(void)
{	// Deactivate the OpenGL window (we've alt-tabbed out of it)
	OSGL_isFullScreen = FALSE;
	wgl_createMyWindow(OSGL_isFullScreen, &winVars.mainWin);
}
*/

void ogl_shutdown(void);

void OSGL_shutDownVideo(void)
{	if (!OSGL_initialized) return;
	ogl_shutdown();

	if (glxContext)
	{	glXMakeCurrent(linuxVars.dpy, None, NULL);
        glXDestroyContext(linuxVars.dpy, glxContext);
        glxContext = NULL;
    }
    /* switch back to original desktop resolution if we were in fs */
    if (linuxVars.isFullScreen)
    {	XF86VidModeSwitchToMode(linuxVars.dpy, linuxVars.screen, &linuxVars.desktopMode);
		XF86VidModeSetViewPort(linuxVars.dpy, linuxVars.screen, 0, 0);
    }
    XCloseDisplay(linuxVars.dpy);
	linuxVars.mainWin = 0;
	// ### TODO : Free all resources
	//DestroyWindow(winVars.mainWin);
	//winVars.mainWin=NULL;
	// Restore Display Settings
}

