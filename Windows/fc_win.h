/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
struct sWinVars
{	uintf		minVersion;				// Minimum version number that code can be compiled against to use this data
	
	HINSTANCE	hinst;					// Instance of this program
	WNDCLASS	wc;						// Window Class
	HICON		appicon;				// Application Icon
	HWND		mainWin;				// Window to draw to
	HWND		parentWin;				// Handle of the application's parent window
	WORD		factive;				// current active state of the window
	WNDPROC		winProc;				// Window Handling Procedure
	int			cmdShow;				// Specifies how the window is to be shown
	bool		isWindowMode;			// True if engine is running in window mode, false for fullscreen
	intf		winPosX,winPosY;		// Co-Ordinates of the top left corner of the client area of the app's window

	// Callback functions
	void (*deactivate)(void);				// Called when the window is deactivated (ALT-TAB out of the app)
	void (*changeSize)(uintf w, uintf h);	// Called when the window is resized
};
extern sWinVars	winVars;

void win32CreateWindow(uintf width, uintf height, uintf depth);
