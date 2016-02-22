// ****************************************************************************************
// ***								Printer Tools (win32)								***
// ***                              -------------										***
// ***																					***
// *** Initialization Dependancies: None												***
// ***																					***
// ****************************************************************************************
// ***								Bitmap Tools										***
// ***                              ------------										***
// ***																					***
// *** Initialization Dependancies: None												***
// ***																					***
// *** External Functions declared here:												***
// ***																					***
// ****************************************************************************************

#include <windows.h>
#include <commdlg.h>
#include "fc_h.h"
#include "windows/fc_win.h"

char *commdlgerror(DWORD errnum)
{	char *result = "Unknown COM Error";
	switch(errnum)
	{	case	CDERR_DIALOGFAILURE:	result = "The dialog box could not be created. The common dialog box function's call to the DialogBox function failed. For example, this error occurs if the common dialog box call specifies an invalid window handle."; break;
		case	CDERR_FINDRESFAILURE:	result = "The common dialog box function failed to find a specified resource."; break;
		case	CDERR_INITIALIZATION:	result = "The common dialog box function failed during initialization. This error often occurs when sufficient memory is not available."; break;
		case	CDERR_LOADRESFAILURE:	result = "The common dialog box function failed to load a specified resource."; break;
		case	CDERR_LOADSTRFAILURE:	result = "The common dialog box function failed to load a specified string."; break;
		case	CDERR_LOCKRESFAILURE:	result = "The common dialog box function failed to lock a specified resource."; break;
		case	CDERR_MEMALLOCFAILURE:	result = "The common dialog box function was unable to allocate memory for internal structures."; break;
		case	CDERR_MEMLOCKFAILURE:	result = "The common dialog box function was unable to lock the memory associated with a handle."; break;
		case	CDERR_NOHINSTANCE:		result = "The ENABLETEMPLATE flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a corresponding instance handle."; break;
		case	CDERR_NOHOOK:			result = "The ENABLEHOOK flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a pointer to a corresponding hook procedure."; break;
		case	CDERR_NOTEMPLATE:		result = "The ENABLETEMPLATE flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a corresponding template."; break;
		case	CDERR_REGISTERMSGFAIL:	result = "The RegisterWindowMessage function returned an error code when it was called by the common dialog box function."; break;
		case	CDERR_STRUCTSIZE:		result = "The lStructSize member of the initialization structure for the corresponding common dialog box is invalid."; break;
		case	PDERR_CREATEICFAILURE:	result = "The PrintDlg function failed when it attempted to create an information context."; break;
		case	PDERR_DEFAULTDIFFERENT:	result = "You called the PrintDlg function with the DN_DEFAULTPRN flag specified in the wDefault member of the DEVNAMES structure, but the printer described by the other structure members did not match the current default printer. (This error occurs when you store the DEVNAMES structure and the user changes the default printer by using the Control Panel.)To use the printer described by the DEVNAMES structure, clear the DN_DEFAULTPRN flag and call PrintDlg again. To use the default printer, replace the DEVNAMES structure (and the structure, if one exists) with NULL; and call PrintDlg again."; break;
		case	PDERR_DNDMMISMATCH:		result = "The data in the DEVMODE and DEVNAMES structures describes two different printers."; break;
		case	PDERR_GETDEVMODEFAIL:	result = "The printer driver failed to initialize a DEVMODE structure."; break;
		case	PDERR_INITFAILURE:		result = "The PrintDlg function failed during initialization, and there is no more specific extended error code to describe the failure. This is the generic default error code for the function."; break;
		case	PDERR_LOADDRVFAILURE:	result = "The PrintDlg function failed to load the device driver for the specified printer."; break;
		case	PDERR_NODEFAULTPRN:		result = "A default printer does not exist."; break;
		case	PDERR_NODEVICES:		result = "No printer drivers were found."; break;
		case	PDERR_PARSEFAILURE:		result = "The PrintDlg function failed to parse the strings in the [devices] section of the WIN.INI file."; break;
		case	PDERR_PRINTERNOTFOUND:	result = "The [devices] section of the WIN.INI file did not contain an entry for the requested printer."; break;
		case	PDERR_RETDEFFAILURE:	result = "The PD_RETURNDEFAULT flag was specified in the Flags member of the PRINTDLG structure, but the hDevMode or hDevNames member was not NULL."; break;
		case	PDERR_SETUPFAILURE:		result = "The PrintDlg function failed to load the required resources."; break;
		case	CFERR_MAXLESSTHANMIN:	result = "The size specified in the nSizeMax member of the CHOOSEFONT structure is less than the size specified in the nSizeMin member."; break;
		case	CFERR_NOFONTS:			result = "No fonts exist."; break;
		case	FNERR_BUFFERTOOSMALL:	result = "The buffer pointed to by the lpstrFile member of the OPENFILENAME structure is too small for the file name specified by the user. The first two bytes of the lpstrFile buffer contain an integer value specifying the size, in TCHARs, required to receive the full name."; break;
		case	FNERR_INVALIDFILENAME:	result = "A file name is invalid."; break;
		case	FNERR_SUBCLASSFAILURE:	result = "An attempt to subclass a list box failed because sufficient memory was not available."; break;
		case	FRERR_BUFFERLENGTHZERO:	result = "A member of the FINDREPLACE structure points to an invalid buffer."; break;
	}
	return result;
}

class fcprinter : public fc_printer
{	PrinterDocument *newPrinterDocument (void)	{return ::newPrinterDocument ();};
	void addPrinterPage (PrinterDocument *doc)	{		::addPrinterPage(doc);};
	dword PrintDocument (PrinterDocument *doc)	{return	::PrintDocument(doc);};
} *OEMprinter;

fc_printer *initprinter(void)
{	OEMprinter = new fcprinter;
	return OEMprinter;
}

void killprinter(void)
{	delete OEMprinter;
}

PrinterDocument *newPrinterDocument (void)
{	PrinterDocument *doc = (PrinterDocument *)fcalloc(sizeof(PrinterDocument), "Printer Document");
	doc->numpages = 1;
	doc->page[0] = newbitmap("Printer Page",1024,2048,bitmap_RGB_32bit | bitmap_RenderTarget);
	return doc;
}

void addPrinterPage (PrinterDocument *doc)
{	doc->page[doc->numpages++] = newbitmap("Printer Page",1024,2048,bitmap_RGB_32bit | bitmap_RenderTarget);
}

dword PrintDocument(PrinterDocument *doc)
{	DWORD		errnum;
	PRINTDLG	printersettings;
	DOCINFO		di;
	
	printersettings.lStructSize = sizeof (printersettings);
	printersettings.hwndOwner = winVars.mainWin;
	printersettings.hDevMode = NULL;							// ### should specify a value here
	printersettings.hDevNames = NULL;							// ### should specify a value here
	printersettings.hDC = NULL;
	printersettings.Flags = PD_RETURNDC | PD_PAGENUMS | PD_NOSELECTION;
	printersettings.nFromPage = 1;
	printersettings.nToPage = (WORD)doc->numpages;
	printersettings.nMinPage = 1;
	printersettings.nMaxPage = (WORD)doc->numpages;
	printersettings.nCopies = 1;
	printersettings.hInstance = winVars.hinst;
	printersettings.lCustData = NULL;
    printersettings.lpfnPrintHook = NULL;
    printersettings.lpfnSetupHook = NULL;
    printersettings.lpPrintTemplateName = NULL;
    printersettings.lpSetupTemplateName = NULL;
    printersettings.hPrintTemplate = NULL;
    printersettings.hSetupTemplate = NULL;

	if (!PrintDlg(&printersettings))
	{	errnum = CommDlgExtendedError();
		if (errnum==0) return PrintJob_Cancelled;		// User cancelled the print job
		msg("Printing Error",commdlgerror(errnum));
		return PrintJob_Failed;
	}
	
    memset( &di, 0, sizeof(DOCINFO) );
    di.cbSize = sizeof(DOCINFO); 
    di.lpszDocName = "FC Engine Printing Tool"; 
    di.lpszOutput = (LPTSTR) NULL; 
    di.lpszDatatype = (LPTSTR) NULL; 
    di.fwType = 0; 
    errnum = StartDoc(printersettings.hDC, &di); 
    if (errnum == SP_ERROR) return PrintJob_Failed; 
 
	for (dword i=0; i<doc->numpages; i++)
	{	StartPage(printersettings.hDC);

/*
    // Retrieve the number of pixels-per-logical-inch in the 
    // horizontal and vertical directions for the display upon which 
    // the bitmap was created. These are likely the same as for 
    // the present display, so we use those values here. 
 
    hWinDC = GetDC(hWnd);
    fLogPelsX1 = (float) GetDeviceCaps(hWinDC, LOGPIXELSX); 
    fLogPelsY1 = (float) GetDeviceCaps(hWindDC, LOGPIXELSY); 
 
    // Retrieve the number of pixels-per-logical-inch in the 
    // horizontal and vertical directions for the printer upon which 
    // the bitmap will be printed. 
 
    fLogPelsX2 = (float) GetDeviceCaps(pd.hDC, LOGPIXELSX); 
    fLogPelsY2 = (float) GetDeviceCaps(pd.hDC, LOGPIXELSY); 
 
    // Determine the scaling factors required to print the bitmap and 
    // retain its original proportions. 
 
    if (fLogPelsX1 > fLogPelsX2) 
        fScaleX = (fLogPelsX1 / fLogPelsX2); 
    else fScaleX = (fLogPelsX2 / fLogPelsX1); 
 
    if (fLogPelsY1 > fLogPelsY2) 
        fScaleY = (fLogPelsY1 / fLogPelsY2); 
    else fScaleY = (fLogPelsY2 / fLogPelsY1); 
 
    // Compute the coordinates of the upper left corner of the 
    // centered bitmap. 
 
    cWidthPels = GetDeviceCaps(pd.hDC, HORZRES); 
    xLeft = ((cWidthPels / 2) - ((int) (((float) bmih.biWidth) 
            * fScaleX)) / 2); 
    cHeightPels = GetDeviceCaps(pd.hDC, VERTRES); 
    yTop = ((cHeightPels / 2) - ((int) (((float) bmih.biHeight) 
            * fScaleY)) / 2); 
 
    // Use StretchDIBits to scale the bitmap and maintain 
    // its original proportions (that is, if the bitmap was square 
    // when it appeared in the application's client area, it should 
    // also appear square on the page). 
 
    if (StretchDIBits(pd.hDC, xLeft, yTop, (int) ((float) bmih.biWidth 
        * fScaleX), (int) ((float) bmih.biHeight * fScaleY), 0, 0, 
        bmih.biWidth, bmih.biHeight, lpBits, lpBitsInfo, iUsage, 
        SRCCOPY) == GDI_ERROR) 
    {
        errhandler("StretchDIBits Failed", hwnd); 
    }
 
 
    // Retrieve the width of the string that specifies the full path 
    // and filename for the file that contains the bitmap. 
 
    GetTextExtentPoint32(pd.hDC, ofn.lpstrFile, 
        ofn.nFileExtension + 3, &szMetric); 
 
    // Compute the starting point for the text-output operation. The 
    // string will be centered horizontally and positioned three lines 
    // down from the top of the page. 
 
    xLeft = ((cWidthPels / 2) - (szMetric.cx / 2)); 
    yTop = (szMetric.cy * 3); 
 
    // Print the path and filename for the bitmap, centered at the top 
    // of the page. 
 
    TextOut(pd.hDC, xLeft, yTop, ofn.lpstrFile, 
        ofn.nFileExtension + 3); 
 
*/	
		EndPage(printersettings.hDC);
	}
	EndDoc(printersettings.hDC);
	DeleteDC(printersettings.hDC);

	return PrintJob_Success;
}
