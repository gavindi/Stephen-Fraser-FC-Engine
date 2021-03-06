/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
struct sLinuxVars
{	uintf		minVersion;				// Minimum version number that code can be compiled against to use this data

	Window		mainWin;				// Window to draw to
	Window		parentWin;				// Handle of the application's parent window
	Display		*dpy;					// Display Handle
	int			screen;					// Screen ID
	int			appActive;				// current active state of the window  (0 = Inactive, 1 = active, 2 = restoring)
	bool		isFullScreen;			// True if engine is running in window mode, false for fullscreen
//	intf		winPosX,winPosY;		// Co-Ordinates of the top left corner of the client area of the app's window
	XF86VidModeModeInfo desktopMode;	// Mode of the desktop prior to launching this program
	XSetWindowAttributes attr;			// Attributes of our window

    int x, y;
    unsigned int width, height;
    unsigned int depth;

	// Callback functions
	void (*deactivate)(void);				// Called when the window is deactivated (ALT-TAB out of the app)
	void (*changeSize)(uintf w, uintf h);	// Called when the window is resized
};
extern sLinuxVars	linuxVars;

void linuxCreateWindow(uintf width, uintf height, uintf depth);
