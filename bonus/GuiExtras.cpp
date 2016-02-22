//
//	*******************************************************************
//  ***						FC Engine GUI Extras					***
//	***						--------------------					***
//	***																***
//	***					  (C) 2005 Stephen Fraser					***
//	***																***
//	***  This file contains a series of GUI controls that are		***
//  *** considdered to be extras, and are not part of the standard  ***
//  *** engine gui.  Licensed FC Engine users are free to use and   ***
//  *** modify this code in source code format, but may only		***
//  *** distribute the code in binary format.						***
//	*******************************************************************
//
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <stdlib.h>
#include <fcio.h>
#include <bucketgui.h>
#include <bonus/GuiExtras.h>

bitmap *colorpicker_HS, *colorpicker_L;
void guiColorPickerEB_Edit(guiEditBox* EB,bool finished);
guiWindow *guiColorPickerWin;
guiButton *guiColorPickerResultColor, *guiColorPicker_OKBtn, *guiColorPicker_CancelBtn;
guiBitmap *guiColorPicker_HS, *guiColorPicker_HS_Select;
guiBitmap *guiColorPicker_L, *guiColorPicker_L_SelectL, *guiColorPicker_L_SelectR;
guiEditBox *guiColorPickerEB[6];	// 0..2 = HSL, 3..5 = RGB
void (*guiColorPickerCallback)(uint32 RGB, void *userdata, bool changed);
void *guiColorPickerUserData;

void initGuiExtras(void)
{	colorpicker_HS = buildHSLgrid(1.0f, 255, 255, NULL);
	colorpicker_L = buildHSL_Lcolumn(0.5f, 0.5f, 10, 255, NULL);
}

void killGuiExtras(void)
{	deletebitmap(colorpicker_HS);
	deletebitmap(colorpicker_L);
}

void guiColorPickerUpdateColor(void)
{	uintf i;
	for (i=0; i<6; i++) guiColorPickerEB[i]->editfunc = NULL;
	guiEditBoxChangeText(guiColorPickerEB[0],buildstr("%i",     guiColorPicker_HS_Select->x-1 ));
	guiEditBoxChangeText(guiColorPickerEB[1],buildstr("%i",255-(guiColorPicker_HS_Select->y-2)));
	guiEditBoxChangeText(guiColorPickerEB[2],buildstr("%i",255-(guiColorPicker_L_SelectR->y-2)));
	guiColorPickerResultColor->facecolor = HSLtoRGB(
		(float)(    (guiColorPicker_HS_Select->x-1)) / 255.0f, 
		(float)(255-(guiColorPicker_HS_Select->y-2)) / 255.0f,
		(float)(255-(guiColorPicker_L_SelectL->y-2)) / 255.0f);
	guiEditBoxChangeText(guiColorPickerEB[3],buildstr("%i",(guiColorPickerResultColor->facecolor&0xff0000)>>16));
	guiEditBoxChangeText(guiColorPickerEB[4],buildstr("%i",(guiColorPickerResultColor->facecolor&0x00ff00)>> 8));
	guiEditBoxChangeText(guiColorPickerEB[5],buildstr("%i",(guiColorPickerResultColor->facecolor&0x0000ff)    ));
	for (i=0; i<6; i++) guiColorPickerEB[i]->editfunc = guiColorPickerEB_Edit;
}

void mapgui(char*filename,guiSystem *mygui)
{	logfile *guilog = newlogfile(filename);
	guiWindow *win = (guiWindow *)mygui->children;
	while (win)
	{	guilog->log("+ %08X : Window (%s)",win, win->name);
		guiObject *child = win->children;
		while (child)
		{	char *guitypename = getGuiTypeName(child->objecttype);
			guilog->log("  |-- %08X : [%i] %s",child, child->objecttype, guitypename);
			child = child->siblings;
		}
		win = (guiWindow *)win->siblings;
	}
	delete guilog;
}

void guiColorHSPickerClick(guiObject *me, intf x, intf y)
{	x -= me->parent->canvasx+5;
	y -= me->parent->canvasy+5;
	if (x<0) x=0;
	if (x>255) x=255;
	if (y<0) y=0;
	if (y>255) y=255;
	guiColorPicker_HS_Select->x = x+1;
	guiColorPicker_HS_Select->y = y+2;
	buildHSL_Lcolumn((float)x/255.0f, 1.0f - (float)y/255.0f, 10, 255, colorpicker_L);
	guiColorPickerUpdateColor();
	me->parent->setchanged(me->parent);
}

void guiColorLPickerClick(guiObject *me, intf x, intf y)
{	y -= me->parent->canvasy+5;
	if (y<0) y=0;
	if (y>255) y=255;
	guiColorPicker_L_SelectL->y = y+2;
	guiColorPicker_L_SelectR->y = y+2;
	guiColorPickerUpdateColor();
	me->parent->setchanged(me->parent);
}

void guiColorPickerEB_Edit(guiEditBox* EB,bool finished)
{	uintf i, ebnum;
	int tmp;
	float3 HSL;
	uint32 RGB;

	ebnum = 99;
	for (i=0; i<6; i++) guiColorPickerEB[i]->editfunc = NULL;
	for (i=0; i<6; i++)
	{	if (EB==guiColorPickerEB[i]) ebnum = i;
	}
	if (ebnum>=6) return;		// This should never happen
	if (ebnum<3)
	{	// Edited HSL
		tmp = atoi(guiColorPickerEB[0]->text);
		if (tmp<0) tmp = 0;
		if (tmp>255) tmp = 255;
		HSL.x = (float)tmp / 255.0f;
		if (finished) guiEditBoxChangeText(guiColorPickerEB[0],buildstr("%i",tmp));
		tmp = atoi(guiColorPickerEB[1]->text);
		if (tmp<0) tmp = 0;
		if (tmp>255) tmp = 255;
		HSL.y = (float)tmp / 255.0f;
		if (finished) guiEditBoxChangeText(guiColorPickerEB[1],buildstr("%i",tmp));
		tmp = atoi(guiColorPickerEB[2]->text);
		if (tmp<0) tmp = 0;
		if (tmp>255) tmp = 255;
		HSL.z = (float)tmp / 255.0f;
		if (finished) guiEditBoxChangeText(guiColorPickerEB[2],buildstr("%i",tmp));
		RGB = HSLtoRGB(HSL.x, HSL.y, HSL.z);
		guiEditBoxChangeText(guiColorPickerEB[3],buildstr("%i",(guiColorPickerResultColor->facecolor&0xff0000)>>16));
		guiEditBoxChangeText(guiColorPickerEB[4],buildstr("%i",(guiColorPickerResultColor->facecolor&0x00ff00)>> 8));
		guiEditBoxChangeText(guiColorPickerEB[5],buildstr("%i",(guiColorPickerResultColor->facecolor&0x0000ff)    ));
	}	else
	{	// Edited RGB
		tmp = atoi(guiColorPickerEB[3]->text);
		if (tmp<0) tmp = 0;
		if (tmp>255) tmp = 255;
		RGB = tmp << 16;
		if (finished) guiEditBoxChangeText(guiColorPickerEB[3],buildstr("%i",tmp));
		tmp = atoi(guiColorPickerEB[4]->text);
		if (tmp<0) tmp = 0;
		if (tmp>255) tmp = 255;
		RGB |= tmp << 8;
		if (finished) guiEditBoxChangeText(guiColorPickerEB[4],buildstr("%i",tmp));
		tmp = atoi(guiColorPickerEB[5]->text);
		if (tmp<0) tmp = 0;
		if (tmp>255) tmp = 255;
		RGB |= tmp;
		if (finished) guiEditBoxChangeText(guiColorPickerEB[5],buildstr("%i",tmp));
		RGBtoHSL(RGB, &HSL);
		guiEditBoxChangeText(guiColorPickerEB[0],buildstr("%i",(intf)(HSL.x*255.0f)));
		guiEditBoxChangeText(guiColorPickerEB[1],buildstr("%i",(intf)(HSL.y*255.0f)));
		guiEditBoxChangeText(guiColorPickerEB[2],buildstr("%i",(intf)(HSL.z*255.0f)));
	}

	buildHSL_Lcolumn(HSL.x, HSL.y, 10, 255, colorpicker_L);
	guiColorPicker_HS_Select->x = (uintf)(HSL.x*255.0f)+1;
	guiColorPicker_HS_Select->y = (uintf)(255-(HSL.y*255.0f))+2;
	guiColorPicker_L_SelectL->y = (uintf)(255-(HSL.z*255.0f))+2;
	guiColorPicker_L_SelectR->y = (uintf)(255-(HSL.z*255.0f))+2;
	EB->parent->setchanged(EB->parent);
	guiColorPickerResultColor->facecolor = 0xff000000 | RGB;
	for (i=0; i<6; i++) guiColorPickerEB[i]->editfunc = guiColorPickerEB_Edit;
}

void guiColorPicker_Close(guiWindow *)
{	// Window has lost focus or has been closed
	if (guiColorPickerCallback)
		guiColorPickerCallback(guiColorPickerResultColor->facecolor, guiColorPickerUserData, false);
	guiColorPickerWin->kill((guiObject *)guiColorPickerWin);	
}

void guiColorPicker_Cancel(guiButton *)
{	// User has hit the cancel button
	if (guiColorPickerCallback)
		guiColorPickerCallback(guiColorPickerResultColor->facecolor, guiColorPickerUserData, false);
	guiColorPickerWin->kill((guiObject *)guiColorPickerWin);
}

void guiColorPicker_OK(guiButton *)
{	// User has hit the OK button
	if (guiColorPickerCallback)
		guiColorPickerCallback(guiColorPickerResultColor->facecolor, guiColorPickerUserData, true);
	guiColorPickerWin->kill((guiObject *)guiColorPickerWin);
}

extern guiSystem *gui;
extern bitmap *guiRT;

void guiDebug(void)
{/*	byte rt[1024];
	saverendertarget((void *)&rt[0]);

	setrendertarget2d(guiRT);
	cls2D();
	drawgui(gui);
	drawguimouse(gui,mousex,mousey);
	setrendertarget2d(NULL);
	_lock3d();
	drawbitmap(guiRT,0,0);
	_unlock3d();
	update();
	readkey();
	
	loadrendertarget((void *)&rt[0]);
*/
}

void newGuiColorPicker(intf posx, intf posy, uint32 startRGB,void (*callback)(uint32 RGB, void *userdata, bool changed), void *userdata, guiWindow *parent)
{	float3 HSL;
	RGBtoHSL(startRGB, &HSL);
	buildHSL_Lcolumn(HSL.x, HSL.y, 10, 255, colorpicker_L);

	guiColorPickerCallback = callback;
	guiColorPickerUserData = userdata;

guiDebug();
	guiColorPickerWin = newguiWindow("Color Picker",posx,posy,300,380,guiWindow_normal, parent);
	guiColorPickerWin->LoseFocusWin = guiColorPicker_Close;
	guiColorPicker_HS = newguiBitmap(colorpicker_HS, false, 5,5,guiColorPickerWin);
	guiColorPicker_HS->CoversPoint = guiRectangularCoversPoint;
	guiColorPicker_HS->MouseDown = guiColorHSPickerClick;
	guiColorPicker_HS->Drag = guiColorHSPickerClick;
	guiColorPicker_HS_Select = newguiBitmap(&guipicker, true, 1+(uintf)(HSL.x*255.0f), 2+(uintf)((1.0f-HSL.y)*255.0f), guiColorPickerWin);
guiDebug();
	guiColorPicker_L = newguiBitmap(colorpicker_L,  false, 275,5,guiColorPickerWin);
	guiColorPicker_L->CoversPoint = guiRectangularCoversPoint;
	guiColorPicker_L->MouseDown = guiColorLPickerClick;
	guiColorPicker_L->Drag = guiColorLPickerClick;
guiDebug();
	uintf y = 2+(uintf)((1.0f-HSL.z)*255.0f);
	guiColorPicker_L_SelectL = newguiBitmap(&guirscroll, true, 270, y,guiColorPickerWin);
guiDebug();
	guiColorPicker_L_SelectR = newguiBitmap(&guilscroll, true, 286, y,guiColorPickerWin);
guiDebug();
	
	guiColorPickerResultColor = newguiButton("",(bitmap *)NULL,160,265,130,70,guiColorPickerWin);
guiDebug();
	setfontcol(0xff000000);
	newguiLabel("H",5,269,guiColorPickerWin);
guiDebug();
	newguiLabel("S",5,294,guiColorPickerWin);
guiDebug();
	newguiLabel("L",5,319,guiColorPickerWin);
guiDebug();
	guiColorPickerEB[0] = newguiEditBox("0",18,265,50,guiColorPickerWin);
guiDebug();
	guiColorPickerEB[1] = newguiEditBox("0",18,290,50,guiColorPickerWin);
guiDebug();
	guiColorPickerEB[2] = newguiEditBox("0",18,315,50,guiColorPickerWin);
guiDebug();
	newguiLabel("R",85,269,guiColorPickerWin);
guiDebug();
	newguiLabel("G",85,294,guiColorPickerWin);
guiDebug();
	newguiLabel("B",85,319,guiColorPickerWin);
guiDebug();
	guiColorPickerEB[3] = newguiEditBox("0",98,265,50,guiColorPickerWin);
guiDebug();
	guiColorPickerEB[4] = newguiEditBox("0",98,290,50,guiColorPickerWin);
guiDebug();
	guiColorPickerEB[5] = newguiEditBox("0",98,315,50,guiColorPickerWin);
guiDebug();
	guiColorPickerUpdateColor();
guiDebug();
	for (uintf i=0; i<6; i++)
	{	guiColorPickerEB[i]->flags |= guiEditBox_RightJustify;
	}
	guiColorPicker_OKBtn = newguiButton("Ok",(bitmap *)NULL,5,345,130,25,guiColorPickerWin);
	guiColorPicker_OKBtn->clickfunc = guiColorPicker_OK;
guiDebug();
	
	guiColorPicker_CancelBtn = newguiButton("Cancel",(bitmap *)NULL,160,345,130,25,guiColorPickerWin);
	guiColorPicker_CancelBtn->clickfunc = guiColorPicker_Cancel;
guiDebug();
}

