/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include "../fc_h.h"

#define DriverNameshort	"XWindows 2D"					// Short Driver Name
#define DriverAuthor	"Stephen Fraser"				// Name of the device driver's author
#define DriverNameTech	"XWindows2D"					// Technical Driver Name
#define DriverNameUser	"XWindows 2D Display Driver"	// User Friendly Driver Name

//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
void	xw_getfunc(char *fname, void **func);
//HRESULT WinInit( HINSTANCE hInst, int nCmdShow );
//void	CreateDisplay( HWND hWnd, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP );
//VOID    xw_shutDown();
//void	DisplayFrame();




//-----------------------------------------------------------------------------
// Name: dx_ShutDown()
// Desc:
//-----------------------------------------------------------------------------
void xw_shutDown()
{
}


void xw_WinMove(intf x1, intf y1, intf x2, intf y2)
{
}


//-----------------------------------------------------------------------------
// Name: dx_getvideoinfo
// Desc:
//-----------------------------------------------------------------------------
void xw_getvideoinfo(void *vidinfo)
{	videoinfostruct vi;
	txtcpy(vi.DriverName,sizeof(vi.DriverName),DriverNameUser);
	txtcpy(vi.AuthorName,sizeof(vi.AuthorName),DriverAuthor);
	txtcpy(vi.DriverNameShort,sizeof(vi.DriverNameShort),DriverNameshort);
	txtcpy(vi.LoadingMethod,sizeof(vi.LoadingMethod),"Static");
	txtcpy(vi.DeviceDescription,sizeof(vi.DeviceDescription),"XWindows");
	vi.MaxTextureWidth	= 0;
	vi.MaxTextureHeight	= 0;
	vi.NumTextureUnits	= 0;
	vi.TextureMemory	= 0;
	vi.MaxAnisotropy	= 0;
	vi.Features			= 0;

	videoinfostruct *dest = (videoinfostruct *)vidinfo;
	dword size = dest->structsize;
	if (size>sizeof(videoinfostruct)) size = sizeof(videoinfostruct);
	memcopy(dest,&vi,size);
	dest->structsize = size;
}

// *** Init Driver - Must set the driver into specified screen mode, capable of immediate
//					 renderring, and return a pointer to the Function Retreival routine
DLL_EXPORT void *vd_init2D(void *_classfinder, intf width, intf height, intf bpp)
{	return (void *)xw_getfunc;
}

bitmap xw_screenBitmapClone;

bitmap *xw_getScreenBitmap(void)
{	// Return a CLONE of the screen bitmap to ensure the calling application can't free it accidently (Engine will attempt to automatically)
//	xw_screenBitmapClone.pixel = xw_screenBitmap->pixel;
//	xw_screenBitmapClone.palette = xw_screenBitmap->palette;
//	xw_screenBitmapClone.width = xw_screenBitmap->width;
//	xw_screenBitmapClone.height = xw_screenBitmap->height;
//	xw_screenBitmapClone.renderinfo = xw_screenBitmap->renderinfo;
//	xw_screenBitmapClone.flags = xw_screenBitmap->flags & (~bitmap_memmask);
	return NULL;//&dx_screenBitmapClone;
}

//-----------------------------------------------------------------------------
// Name: dx_update()
// Desc:
//-----------------------------------------------------------------------------
void xw_update()
{
}


void xw_apprestore(void)
{	applicationState = AppState_running;
}


struct functype
{	char name[25];
	void *fnptr;
} xwvidfuncs[]={{"shutDownVideo",		(void *)xw_shutDown},
				{"update",				(void *)xw_update},
				{"getScreenBitmap",		(void *)xw_getScreenBitmap},
				{"apprestore",			(void *)xw_apprestore},
				{"winmove",				(void *)xw_WinMove},
				{"getvideoinfo",		(void *)xw_getvideoinfo}
};

void xw_getfunc(char *fname, void **func)
{	// Given a text description of a function, return a pointer to it
	long numfuncs = sizeof(xwvidfuncs)/sizeof(functype);
	for (long i=0; i<numfuncs; i++)
	{	if (txticmp(xwvidfuncs[i].name,fname)==0)
		{	*func = xwvidfuncs[i].fnptr;
			return;
		}
	}
	func=NULL;
	//dx_winerror(buildstr("Unknown Video Function: %s",fname));
}

