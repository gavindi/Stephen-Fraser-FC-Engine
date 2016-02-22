/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
// GDI Renderer

// Driver Details
// --------------
// Type    : Video
// Driver  : Windows GDI software renderer
// Author  : Stephen Fraser
// Version : 4.0
// Last Mod: 06 / 05 / 2014  (SHF)
//
// Special Notes - This is a reference 2D driver

#include <windows.h>
#include "../fc_h.h"
#include "fc_win.h"

// Video driver information
const char *GDIdriverName="Windows GDI Renderer";
const char *GDIdriverNameShort="GDI";
const char *GDIdriverAuthor="Stephen Fraser";
const char *GDIDriverDescription="A small, fast 2D renderer";

// Video driver's own internal variables
BITMAPINFO	*GDIbitmapinfo=NULL;	// GDI backbuffer
bitmap      gdi_screenBitmap;       // FC Engine Backbuffer (### Redundant)

int         gdi_desktopBPP;         // Bit depth of the desktop
HDC			GDIhdc;				    // handle to the device context
void		*GDIbackbufferlfb;		// pointers to bitmap's Linear Frame Buffer
HBITMAP		GDIbackbufferhandle;   // handle of DIB section

// Prototypes for functions that must be accessed before they can be defined
void gdi_getfunc(char *fname, void **func);

void gdi_shutdown(void)
{   showMouse(true);
    if (GDIbitmapinfo) fcfree(GDIbitmapinfo);
}

DLL_EXPORT void *vd_init2D(void *_classfinder,intf width, intf height, intf bpp)
{	// Make any necessary changes to winVars.wc and then register the class prior to creating our first window
	if (winVars.mainWin==NULL)
	{	if(RegisterClass(&winVars.wc) == 0)
			return NULL;			// Some failure has occured, we need to handle it somehow
	}

    // Cleanup anything from a previous call
    gdi_shutdown();

	win32CreateWindow(width, height, bpp);

    // Create a bitmap to use as the backbuffer
	bool fullscreen = (bpp>0);
	screenbpp = 32;
    screenWidth = width;
    screenHeight = height;

	if (fullscreen)													// Attempt Fullscreen Mode?
	{
		showMouse(false);
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

	// If we failed to change to fullscreen, the fullscreen flag will have been set to false
	if (fullscreen)
	{	SetWindowLong(winVars.mainWin, GWL_STYLE, 0);
		SetWindowPos(winVars.mainWin,HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_SHOWWINDOW);
	}	else
	{	RECT workarea;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0);
		int win32width  = width + (GetSystemMetrics(SM_CXBORDER)+GetSystemMetrics(SM_CXFIXEDFRAME))*2;
		int win32height = height + (GetSystemMetrics(SM_CXBORDER)+GetSystemMetrics(SM_CXFIXEDFRAME))*2+GetSystemMetrics(SM_CYCAPTION);
		SetWindowPos(winVars.mainWin,HWND_TOP,
				((workarea.right-workarea.left) - win32width) / 2,
				((workarea.bottom-workarea.top) - win32height) / 2,
				win32width, win32height, SWP_SHOWWINDOW);
	}

	//SetWindowPos(*GDIWin,HWND_TOP,0,0,width,height,SWP_NOMOVE|SWP_SHOWWINDOW);
	HDC hdc = GetDC(winVars.mainWin);
	GDIbitmapinfo = (BITMAPINFO *)fcalloc(sizeof(BITMAPV4HEADER),"Renderring Backbuffer");
	if (GDIbitmapinfo == NULL)
		msg("GDI Driver Failure","Failed to create the backbuffer");

	BITMAPV4HEADER *h = (BITMAPV4HEADER*) &GDIbitmapinfo->bmiHeader;

	h->bV4Size			= sizeof(BITMAPV4HEADER);
	h->bV4Width			= width;
	h->bV4Height		= -height;
	h->bV4Planes		= 1;
	h->bV4BitCount		= 32;
	h->bV4V4Compression	= BI_RGB;
	h->bV4SizeImage		= 0;
	h->bV4XPelsPerMeter = 0;
	h->bV4YPelsPerMeter = 0;
	h->bV4ClrUsed		= 0;
	h->bV4ClrImportant	= 0;
	h->bV4CSType		= LCS_CALIBRATED_RGB;

	GDIbackbufferhandle = CreateDIBSection (hdc, GDIbitmapinfo, DIB_RGB_COLORS, (void **)&GDIbackbufferlfb, NULL, 0);
	if (!GDIbackbufferhandle)
	{   free(GDIbitmapinfo);
        GDIbitmapinfo = NULL;
		msg("GDI Driver Failure","Failed to create the backbuffer's linear frame buffer");
	}

	ReleaseDC(winVars.mainWin, hdc);

    // Build the Y lookup table
    gdi_screenBitmap.width = width;
    gdi_screenBitmap.height = height;
    gdi_screenBitmap.palette = NULL;
    gdi_screenBitmap.pixel = (byte *)GDIbackbufferlfb;
    gdi_screenBitmap.flags = bitmap_RenderTarget | bitmap_x8r8g8b8;
	gdi_screenBitmap.renderinfo = fcalloc(ptrsize * height,"Backbuffer Y lookup table");
    uintf *ofs = (uintf *)(gdi_screenBitmap.renderinfo);
	for (intf i=0; i<height; i++)
        ofs[i] = i*width*4;

	return (void *)gdi_getfunc;
}

void gdi_getvideoinfo(void *data)
{	videoinfostruct vi;
	txtcpy(vi.DriverName,       sizeof(vi.DriverName),      GDIdriverName);
	txtcpy(vi.DriverNameShort,  sizeof(vi.DriverNameShort), GDIdriverNameShort);
	txtcpy(vi.AuthorName,       sizeof(vi.AuthorName),      GDIdriverAuthor);
	txtcpy(vi.DeviceDescription,sizeof(vi.DeviceDescription),GDIDriverDescription);
#ifdef STATICDRIVERS
	txtcpy(vi.LoadingMethod     ,sizeof(vi.LoadingMethod),  "Static");
#else
	txtcpy(vi.LoadingMethod,    sizeof(vi.LoadingMethod),   "Dynamic");
#endif

	vi.MaxTextureWidth	= 4096;
	vi.MaxTextureHeight	= 4096;
	vi.NumTextureUnits	= 1;
	vi.TextureMemory	= 0;

	videoinfostruct *dest = (videoinfostruct *)data;
	dword size = dest->structsize;
	if (size>sizeof(videoinfostruct)) size = sizeof(videoinfostruct);
	memcpy(dest,&vi,size);
	dest->structsize = size;
}

void gdi_update(void)
{	// Copy the backbuffer to the window
	HDC			hdcDIBSection;
	HBITMAP		holdbitmap;

	HDC hdc = GetDC(winVars.mainWin);

	hdcDIBSection = CreateCompatibleDC(hdc);
	holdbitmap = (HBITMAP)SelectObject(hdcDIBSection, (HGDIOBJ)GDIbackbufferhandle);

	BitBlt(hdc, 0, 0, screenWidth, screenHeight, hdcDIBSection,
          0, 0, SRCCOPY);

	ReleaseDC(winVars.mainWin, hdc);
	SelectObject(hdcDIBSection, holdbitmap);
	DeleteDC(hdcDIBSection);
}

bitmap *gdi_getScreenBitmap(void)
{   return &gdi_screenBitmap;
}

void gdi_WinMove(intf x1, intf y1, intf x2, intf y2)
{   // ###
}

void gdi_apprestore(void)
{	// ### fcapppaused = 0;
}

struct functype
{	char name[25];
	void *fnptr;
} gdi_vidfuncs[]={
                {"shutDownVideo",		(void *)gdi_shutdown},
				{"update",				(void *)gdi_update},
				{"getScreenBitmap",		(void *)gdi_getScreenBitmap},
				{"apprestore",			(void *)gdi_apprestore},
				{"winmove",				(void *)gdi_WinMove},
				{"getvideoinfo",		(void *)gdi_getvideoinfo}
};

void gdi_getfunc(char *fname, void **func)
{	// Given a text description of a function, return a pointer to it
	long numfuncs = sizeof(gdi_vidfuncs)/sizeof(functype);
	for (long i=0; i<numfuncs; i++)
	{	if (stricmp(gdi_vidfuncs[i].name,fname)==0)
		{	*func = gdi_vidfuncs[i].fnptr;
			return;
		}
	}
	msg("Unknown Video Driver function",fname);
}


BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{	// We don't usually have to do anything here - FC doesn't expect anything done.
	// Initialization for FC is handled through vd_init(...)
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
