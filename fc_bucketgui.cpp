// ****************************************************************************************
// ***								FC Engine GUI										***
// ***                              -------------										***
// ***																					***
// *** Bucket-Based GUI system for the FC engine.  Only renders windows when they have  ***
// *** changed,  otherwise it stores them as a bitmap and only draws the bitmap.		***
// ***																					***
// ****************************************************************************************
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <stdlib.h>
#include <fcio.h>
#include <bucketSystem.h>
#include <bucketgui.h>

#define guibordersize 4			// Width in pixels of the border for windows

extern fontsettings currentfont;

// Global Variables
guiWindow	*focuswin;			// Which window has focus
guiObject	*focusobj;			// Which object has focus (may be a window)
guiObject	*guimouseover;		// Which object is the mouse pointing at?
uintf		lastmousebtn;		// Used by the rungui function
guiObject	*lastclick;			// Used by the rungui function
guiSystem	*currentguiSystem;	// which is the current gui system being processed
guiObject	*usedguiObject, *freeguiObject;
void		(*DrawDragImage)(guiObject *me, intf x, intf y);

// ****************************************************************
// ***															***
// ***					Default Handlers Code					***
// ***															***
// ****************************************************************
char *getGuiTypeName(guiObjectType type)
{	switch(type)
	{	case guitype_System:	return (char *)"System";	break;
		case guitype_Window:	return (char *)"Window";	break;
		case guitype_Label:		return (char *)"Label";		break;
		case guitype_Button:	return (char *)"Button";	break;
		case guitype_Bitmap:	return (char *)"Bitmap";	break;
		case guitype_EditBox:	return (char *)"EditBox";	break;
		case guitype_Spinner:	return (char *)"Spinner";	break;
		case guitype_CheckBox:	return (char *)"CheckBox";	break;
		case guitype_RadioButton:return (char *)"RadioButton";	break;
		case guitype_BevelBox:	return (char *)"BevelBox";	break;
		case guitype_HotSpot:	return (char *)"HotSpot";	break;
		case guitype_DropBox:	return (char *)"DropBox";	break;
		case guitype_TabControl:return (char *)"TabControl";break;
		case guitype_Custom:	return (char *)"Custom";	break;
		default: return (char *)"Unknown";	break;
	}
}

guiObject *guiDefaultCoversPoint(guiObject *me, intf x, intf y)
{	return NULL;
}

guiObject *guiRectangularCoversPoint(guiObject *me, intf x, intf y)
{	x-=me->parent->canvasx;
	y-=me->parent->canvasy;
	if (x>=me->x && y>=me->y && x<=me->x+me->width-1 && y<=me->y+me->height-1)
		return me;
	else
		return NULL;
}

void guiDefaultDraw(guiObject *me)
{	msg("GUI Error", buildstr("Default Draw function called for an object of type %i (%s)",me->objecttype,getGuiTypeName(me->objecttype)));
}

void guiObjectDrawNothing(guiObject *me)
{	// This function is deliberately left empty
}

bool guiObjectRejectDrag(guiObject *receiver, guiObject *sender, bool testonly)
{	return false;
}

void guiDefaultKill(guiObject *me)
{	if (focusobj==me) focusobj = NULL;
	if (me->parent)
	{	guiWindow *parent = me->parent;
		guiObject *child = parent->children;
		if (!child) msg("GUI Corruption","A GUI object was deleted, but it's parent claims to have no children!");
		if (child==me) parent->children = me->siblings;
		else
		{	while (child->siblings!=me)
			{	child = child->siblings;
				if (!child) msg("GUI Corruption","A GUI object was deleted, but it's parent has no knowledge of it!");
			}
			child->siblings = me->siblings;
		}
	}
	deletebucket(guiObject, me);
}

guiObject *newguiObject(guiObjectType objecttype, intf x, intf y, intf width, intf height, guiWindow *parent)
{	guiObject *newobj;
	if (parent==NULL)
	{	// for a Parent to be NULL, various rules have to be met
		if ((objecttype!=guitype_System) && (objecttype!=guitype_TabPage) && (objecttype!=guitype_MenuItem))
			msg("Error building GUI","Only a GUI SYSTEM can have a NULL parent");
	}
	allocbucket(guiObject,newobj,flags,gui_memstruct,128,"GUI Buffers");
	newobj->objecttype = objecttype;
	newobj->x = x;
	newobj->y = y;
	newobj->width = width;
	newobj->height = height;
	newobj->parent = parent;
	newobj->dragdatatype = 0;
	newobj->flags &= gui_memstruct;
	newobj->CoversPoint = guiDefaultCoversPoint;
	newobj->MouseOver = NULL;
	newobj->draw = guiDefaultDraw;
	newobj->kill = guiDefaultKill;
	newobj->Drag = NULL;
	newobj->DrawDragImage = NULL;
	newobj->EndDrag = NULL;
	newobj->MouseDown = NULL;
	newobj->KeyPress = NULL;
	newobj->LoseFocus = NULL;
	newobj->facecolor = 0xffd4d0c8;
	newobj->siblings = NULL;
	newobj->AcceptDrag = guiObjectRejectDrag;
	if (parent)
	{	if (parent->children)
		{	guiObject *sibling = parent->children;
			while (sibling->siblings) sibling = sibling->siblings;
			sibling->siblings = newobj;
		}	else
		{	parent->children = newobj;
		}
		parent->setchanged(parent);
		if (parent->objecttype == guitype_Window)
		{	if (parent->maxx < x+width) parent->maxx = x+width;
			if (parent->maxy < y+height) parent->maxy = y+height;
		}
	}
	return newobj;
}

void drawbutton(intf x, intf y, intf width, intf height, bool pressed, uintf facecol)
{	hline(x,        y,         width-1, 0xfff0f0f0);
	hline(x,        y+height-1,width,   0xff4b4b4b);
	vline(x,        y+1,       height-2,0xfff0f0f0);
	vline(x+width-1,y,		   height-1,0xff4b4b4b);
	box(x+1,y+1,width-2,height-2,facecol);
}

// ****************************************************************
// ***															***
// ***							GUI Windows						***
// ***															***
// ****************************************************************
void guiWindow_Draw(guiObject *me)
{	sRenderTarget2D *backupRT;
	// uint32	titleLineCol = 0xffffffff;	// never used

	guiWindow *win = (guiWindow *)me;
	intf x = win->x;
	intf y = win->y;
	intf w = win->width;
	intf h = win->height;
	uintf flags = win->flags;

	if (flags & guiWindow_changed)
	{	backupRT = current2DRenderTarget;
		sRenderTarget2D canvasRT;
		create2DRenderTarget(&canvasRT, win->canvas);
		select2DRenderTarget(&canvasRT);
		cls2D(win->facecolor);
		guiObject *obj = win->children;
		while (obj)
		{	obj->draw(obj);
			obj = obj->siblings;
		}
		select2DRenderTarget(backupRT);
		win->flags &=~guiWindow_changed;
	}	else
	{
	}

	// Draw border
	if (flags & guiWindow_hasBorder)
	{	if (flags & guiWindow_thinBorder)
		{	// Draw thin border
			hline(x    , y    , w-1, 0xffffffff);	// top line
			vline(x    , y+1  , h-2, 0xffffffff);	// left line
			hline(x+1  , y+h-1, w-1, 0xffffffff);	// bottom line
			vline(x+w-1, y    , h-1, 0xffffffff);	// right line
			x++;
			y++;
			w-=2;
			h-=2;
		}	else
		{	// titleLineCol = 0xffD4D0C8;
			// Top Line
			hline(x    , y    , w  , 0xffD4D0C8);
			hline(x+1  , y+1  , w-2, 0xffffffff);
			hline(x+2  , y+2  , w-4, 0xffD4D0C8);
			hline(x+3  , y+3  , w-6, 0xffD4D0C8);

			// Left Line
			vline(x    , y+1  , h-2, 0xffD4D0C8);
			vline(x+1  , y+2  , h-4, 0xffffffff);
			vline(x+2  , y+3  , h-6, 0xffD4D0C8);
			vline(x+3  , y+4  , h-8, 0xffD4D0C8);

			// Bottom Line
			hline(x    , y+h-1, w  , 0xff202020);
			hline(x+1  , y+h-2, w-2, 0xff808080);
			hline(x+2  , y+h-3, w-4, 0xffD4D0C8);
			hline(x+3  , y+h-4, w-6, 0xffD4D0C8);

			// Right line
			vline(x+w-1, y    , h-1, 0xff202020);
			vline(x+w-2, y+1  , h-3, 0xff808080);
			vline(x+w-3, y+2  , h-5, 0xffD4D0C8);
			vline(x+w-4, y+3  , h-7, 0xffD4D0C8);

			x+=guibordersize;
			y+=guibordersize;
			w-=guibordersize<<1;
			h-=guibordersize<<1;
		}
	}

	// Draw window title
	if (flags & guiWindow_hasTitle)
	{	if (win->DrawTitle) win->DrawTitle(win);
		else
		{	if (win == focuswin)
			{	for (uintf i=0; i<18; i++)
				{	hline(x, y+i, w,0xff0000f0-i*12);
				}
				setfontcol(0xffffffff);
			}	else
			{	for (uintf i=0; i<18; i++)
				{	uintf ii = i*6;
					ii |= (ii<<8) | (ii<<16);
					hline(x, y+i, w,0xff808080-ii);
				}
				setfontcol(0xffc0c0c0);
			}
			textout(x + ((w - gettextwidth(win->name))>>1), y + 2, win->name);
			hline(x, y+18, w, 0xffD4D0C8);

			// Draw control buttons
			if (flags & guiWindow_hasClose)
			{	drawbutton(x+w-16,y+3,14,13,false,0xffc0c0c0);
				drawsprite(&guicross,x+w-13,y+6);
			}
		}
		y+=19;
		h-=19;
	}

	// Draw menu
	if (win->flags & guiWindow_hasMenu)
	{	win->menux = x;
		win->menuy = y;
		box(x,y,w,18,0xffd6d3ce);
		hline(x,y+18,w,0xff848484);
		setfontcolor(0xff000000);
		guiMenuItem *m = win->pulldownmenu;
		while (m)
		{	if (m->highlight)
			{	box(x + m->x, y, m->width, 18, 0xffffffff);
			}
			textout(x + m->x + 8, y + 2, m->name);
			m = m->link;
		}
		y+=19;
		h-=19;
	}
	drawbitmap(win->canvas, x, y);

	// Draw Scroll Bars
	w = win->canvas->width;
	h = win->canvas->height;
	if ((win->maxx > w) || (win->maxy > h))
	{	// Up button
		drawbutton(x+w-16,y,16,16,false,0xffd4d0c8);
		drawsprite(&guiuscroll,x+w-12,y+6);
		// Down button
		drawbutton(x+w-16,y+h-32,16,16,false,0xffd4d0c8);
		drawsprite(&guidscroll,x+w-12,y+h-26);
		// Draw U/D scroll space
		uintf scrollsize = h-48;
		uintf scrollbarsize = (scrollsize*scrollsize) / win->maxy;
		box(x+w-16, y+16, 16, scrollsize, 0xffe0e0e0);
		drawbutton(x+w-16,y+16,16,scrollbarsize,false,0xffd4d0c8);

		// Left Button
		drawbutton(x,y+h-16,16,16,false,0xffd4d0c8);
		drawsprite(&guilscroll,x+6,y+h-12);
		// Right Button
		drawbutton(x+w-32,y+h-16,16,16,false,0xffd4d0c8);
		drawsprite(&guirscroll,x+w-26,y+h-12);
		// Draw L/R scroll space
		scrollsize = w-48;
		scrollbarsize = (scrollsize*scrollsize) / win->maxx;
		box(x+16,y+h-16,scrollsize,16,0xffe0e0e0);
		drawbutton(x+16,y+h-16,scrollbarsize,16,false,0xffd4d0c8);
	}
}

void guiWindow_SetChanged(guiWindow *me)
{	if (me->flags & guiWindow_changed) return;
	me->flags |= guiWindow_changed;
	if (me->parent) me->parent->setchanged(me->parent);
}

void guiWindow_ApplyScroll(guiWindow *win, intf x, intf y)
{	intf difx = win->scrollx - x;
	intf dify = win->scrolly - y;
	win->scrollx = x;
	win->scrolly = y;
	guiObject *child = win->children;
	while (child)
	{	child->x += difx;
		child->y += dify;
		switch(child->objecttype)
		{	case guitype_TabControl:
			{	guiTabControl *tc = (guiTabControl *)child;
				for (intf i=0; i<tc->tabcount; i++)
				{	tc->window[i]->x += difx;
					tc->window[i]->y += dify;
				}
			}
			default:
			{}
		}
		child = child->siblings;
	}
	guiWindow_SetChanged(win);
}

guiObject *guiWindowCoversPoint(guiObject *me, intf x, intf y)
{	guiWindow *win = (guiWindow *)me;
	guiObject *result = NULL;

	if (x>win->screenx && x<win->screenx+win->width && y>win->screeny && y<win->screeny+win->height)
	{	result = me;

		// Test against pulldown menus
		if (win->flags & guiWindow_hasMenu)
		{	guiMenuItem *m = win->pulldownmenu;
			while (m)
			{	intf mx = win->menux + m->x;
				intf my = win->menuy + m->y;
				if ((x >= mx) &&
					(x <= (mx + m->width)) &&
					(y >= my) &&
					(y <= (my + m->height)))
					{	result = (guiObject *)m;
					}
				m = m->link;
			}
		}

		// Test against child objects
		guiObject *child = win->children;
		while (child)
		{	guiObject *cover = child->CoversPoint(child,x,y);
			if (cover) result = cover;
			child = child->siblings;
		}
	}

	return result;
}

void guiWindowMoved(guiWindow *win)
{	uintf flags = win->flags;
	guiWindow *parent = win->parent;

	win->canvasx = win->screenx = parent->canvasx + win->x;
	win->canvasy = win->screeny = parent->canvasy + win->y;

	if (flags & guiWindow_hasBorder)
	{ 	if (flags & guiWindow_thinBorder)
		{	win->canvasx++;
			win->canvasy++;
		}	else
		{	win->canvasx+=guibordersize;
			win->canvasy+=guibordersize;
		}
	}
	if (flags & guiWindow_hasTitle)
	{	win->canvasy+=19;
	}
	if (flags & guiWindow_hasMenu)
	{	win->canvasy+=19;
	}
	guiObject *child = win->children;
	while (child)
	{	if (child->objecttype==guitype_Window)
		{	guiWindowMoved((guiWindow*)child);
		}
		child = child->siblings;
	}
}

void guiWindowDrag(guiObject *me, intf x, intf y)
{	guiWindow *win = (guiWindow *)me;
	if (win->flags & guiWindow_dragtitle)
	{	win->x = win->dragofsx + x;
		win->y = win->dragofsy + y;
		guiWindowMoved(win);
		win->setchanged(win);
	}
}

void guiWindowEndDrag(guiObject *me, intf x, intf y)
{	me->flags &=~guiWindow_dragtitle;
}

void guiWindowBringToFront(guiWindow *win)
{	guiObject *child;
	guiWindow *parent = win->parent;
	if (!parent) return;				// Parent will be NULL for the guiSystem (which can have guiObjects placed on it too)
	if (win->siblings == NULL)
	{	// already at front
		if (parent->objecttype==guitype_Window)
			guiWindowBringToFront(parent);
		return;
	}
	// Remove this window from the parent's children list
	if (parent->children == (guiObject *)win)
	{	parent->children = win->siblings;
		win->siblings = NULL;
	}
	else
	{	child = parent->children;
		while (child)
		{	if (child->siblings == (guiObject *)win)
			{	child->siblings = win->siblings;
				win->siblings = NULL;
				break;
			}
			child = child->siblings;
		}
	}
	// Append this window onto the end of the list
	child = parent->children;
	while (child->siblings)	child = child->siblings;
	((guiWindow*)child)->siblings = (guiObject*)win;
	if (parent->objecttype==guitype_Window)
		guiWindowBringToFront(parent);
}

void guiSetWindowFocus(guiWindow *win)
{	if (win!=focuswin)
	{	if (focuswin)
		{	focuswin->setchanged(focuswin);
			if (focuswin->LoseFocusWin) focuswin->LoseFocusWin(focuswin);
		}
	}
	focuswin = win;
	if (win)
	{	guiWindowBringToFront(win);
		win->setchanged(win);
	}
}

void guiWindowMouseDown(guiObject *me, intf x, intf y)
{	guiWindow *win = (guiWindow *)me;
	guiSetWindowFocus(win);
	intf titleheight = 0;
	if (win->flags & guiWindow_hasBorder)
		titleheight+=guibordersize;
	if (win->flags & guiWindow_hasTitle)
		titleheight+=19;
	if ((y - win->screeny)<titleheight)
	{	win->flags |= guiWindow_dragtitle;
		win->dragofsx = win->x - x;
		win->dragofsy = win->y - y;
	}
	guiCreateMessage(currentguiSystem, guiCommand_Click, me);
}

void guiWindowKill(guiObject *me)
{	guiWindow *win = (guiWindow *)me;
	while (win->children)
	{	win->children->kill(win->children);
	}
	deleteBitmap(win->canvas);
	if (focuswin==win) focuswin = NULL;
	guiDefaultKill(me);
}

guiWindow *newguiWindow(const char *name, intf x, intf y, uintf width, uintf height, uintf flags, guiWindow *parent)
{	guiWindow *newwin;
	if (flags & guiWindow_TabPage)
	{	newwin = (guiWindow *)newguiObject(guitype_TabPage, x, y, width, height, NULL);
		newwin->parent = parent;
	}
	else
		newwin = (guiWindow *)newguiObject(guitype_Window, x, y, width, height, parent);
	txtcpy(newwin->name,wintitle_size,name);
	intf portalwidth = width;
	intf portalheight = height;
	newwin->flags |= flags | guiWindow_changed;
	newwin->draw = guiWindow_Draw;
	newwin->Drag = guiWindowDrag;
	newwin->EndDrag = guiWindowEndDrag;
	newwin->MouseDown = guiWindowMouseDown;
	newwin->setchanged = guiWindow_SetChanged;
	newwin->CoversPoint = guiWindowCoversPoint;
	newwin->kill = guiWindowKill;
	newwin->LoseFocusWin = NULL;
	newwin->DrawTitle = NULL;
	newwin->maxx = width;
	newwin->maxy = height;
	newwin->scrollx = 0;
	newwin->scrolly = 0;
	newwin->pulldownmenu = NULL;
	newwin->menux = 0;
	newwin->menuy = 0;
	if (flags & guiWindow_hasBorder)
	{	if (flags & guiWindow_thinBorder)
		{	newwin->width+=2;
			newwin->height+=2;
		}	else
		{	newwin->width+=guibordersize<<1;
			newwin->height+=guibordersize<<1;
		}
	}
	if (flags & guiWindow_hasTitle)
	{	newwin->height+=19;
	}
	if (flags & guiWindow_hasMenu)
	{	newwin->height+=19;
	}
	newwin->canvas = newbitmap(name, portalwidth, portalheight, bitmap_RGB_32bit | bitmap_Alpha | bitmap_RenderTarget);
	guiSetWindowFocus(newwin);
	guiWindowMoved(newwin);
	return newwin;
}

// ****************************************************************
// ***															***
// ***							GUI Menus						***
// ***															***
// ****************************************************************

void guiMenuMouseOver(guiObject *me, bool over)
{	me->parent->setchanged(me->parent);
	((guiMenuItem *)me)->highlight = over;
}

void guiMenuItemClick(guiHotSpot *hs)
{	guiMenuItem *m = (guiMenuItem *)hs->userptr[1];
	if (m->clickfunc) m->clickfunc(m);
	guiCreateMessage(currentguiSystem, guiCommand_Click, (guiObject *)m);
	guiWindow *win = hs->parent;
	win->kill((guiObject *)win);
}

void guiMenuItemEndDrag(guiObject *me, intf x, intf y)
{	fontsettings font;
	if (me==guimouseover)
	{	guiMenuItem *m = (guiMenuItem *)me;
		setfont((bitmap *)NULL);
		setfontcolor(0xff000000);
		getfont(&font);
		guiMenuItem *i = m->children;
		uintf height = 4;
		uintf width = me->width;
		while (i)
		{	height += 16;
			uintf w = gettextwidth(i->name) + 8;
			if (w>width) width = w;
			i = i->link;
		}
		newguiMenuWin(m->x, 0, width, height, guiMenuItemClick, m->children, &font, m);
	}
}

guiMenuItem *newguiMenuItem(char *name, guiMenuItem *parentItem, guiWindow *parentWin)
{	guiMenuItem *i;
	guiMenuItem *m = (guiMenuItem *)newguiObject(guitype_MenuItem, 0, 0, gettextwidth(name)+16, 18, NULL);

	if (parentItem==NULL)
	{	// This is a menu group (goes along the top of the window, eg: File, Edit, View, Help)
		if (parentWin->pulldownmenu)
		{	i = parentWin->pulldownmenu;
			while (i->link) i=i->link;
			i->link = m;
			m->x = i->x + i->width;
		}	else
		{	parentWin->pulldownmenu = m;
			m->x = 0;
		}
	}	else
	{	// This is an item within another group (eg: New, Open, Close, Save, Save As, Exit).
		if (parentItem->children)
		{	i = parentItem->children;
			while (i->link) i=i->link;
			i->link = m;
			m->y = i->y + 18;
		}	else
		{	parentItem->children = m;
			m->y = 2;
		}
		m->x = 2;
	}

	txtcpy(m->name, guiMenuItemNameLen, name);
	m->link = NULL;
	m->children = NULL;
	m->EndDrag = guiMenuItemEndDrag;
	m->parent = parentWin;
	m->MouseOver = guiMenuMouseOver;
	m->menuID = 0;
	return m;
}

void guiMenuWinCancel(guiWindow *win)
{	win->kill((guiObject *)win);
}

guiWindow *newguiMenuWin(intf x, intf y, uintf width, uintf height, void (*clickfunc)(guiHotSpot *), guiMenuItem *m, fontsettings *fontset, void *parent)
{	guiWindow *w = newguiWindow("",x,y,width,height,guiWindow_hasBorder|guiWindow_thinBorder,((guiObject *)parent)->parent);
	w->LoseFocusWin = guiMenuWinCancel;
	w->facecolor = 0xffdfdfdf;

	// Fill in the window with all the choices
	setfont(fontset);
	uintf itemcount = 0;
	while (m)
	{	guiHotSpot *tmp = newguiHotSpot(2, itemcount*16+2, width-4, 16, w);
		tmp->userptr[0] = parent;
		tmp->userptr[1] = m;
		tmp->clickfunc = clickfunc;
		newguiLabel(m->name, 4, itemcount*16+4, w);
		itemcount++;
		m = m->link;
	}
	return w;
}

// ****************************************************************
// ***															***
// ***							GUI Labels						***
// ***															***
// ****************************************************************
void guiLabelDraw(guiObject *me)
{	guiLabel *label = (guiLabel *)me;
	setfont(&label->fontset);
	if (label->flags & guiLabel_RightJustify)
		textout(label->x - label->textwidth, label->y, label->text);
	else
		textout(label->x,label->y,label->text);
}

void guiLabelChangeText(guiLabel *l, char *text)
{	txtcpy(l->text, guiLabelNameLen, text);
	l->textwidth = gettextwidth(text);
}

guiLabel *newguiLabel(const char *text,intf x, intf y, guiWindow *parent)
{	guiLabel *l = (guiLabel *)newguiObject(guitype_Label, x, y, gettextwidth(text), currentfont.bm->height, parent);
	getfont(&l->fontset);
	l->draw = guiLabelDraw;
	txtcpy(l->text,guiLabelNameLen, text);
	l->textwidth = gettextwidth(text);
	return l;
}

// ****************************************************************
// ***															***
// ***							GUI Buttons						***
// ***															***
// ****************************************************************
void guiButtonSetTextPos(guiButton *btn, guiTextPos textpos)
{	intf txtwidth, txtheight, glywidth, glyheight;
	btn->textlen = txtlen(btn->text);
	if (btn->textlen)
	{	txtwidth = gettextwidth(btn->text);
		txtheight= btn->fontset.bm[0].height;
	}	else
	{	txtwidth = 0;
		txtheight = 0;
	}

	if (btn->glyph)
	{	glywidth = btn->glyph->width;
		glyheight = btn->glyph->height;
	}	else
	{	glywidth = 0;
		glyheight = 0;
	}
	// Default text to center of button
	btn->textX = (btn->width - txtwidth)>>1;
	btn->textY = (btn->height- txtheight)>>1;

	// Default glyph to center of button
	btn->glyphX = (btn->width - glywidth)>>1;
	btn->glyphY = (btn->height - glyheight)>>1;

	switch (textpos)
	{	case TxtPos_Center:
			// Just use defaults
			break;

		case TxtPos_Below:
			if (btn->glyph)
			{	btn->glyphY = (btn->height - (glyheight + txtheight + (txtheight>>2)))>>1;
				btn->textY = btn->glyphY + glyheight + (txtheight>>2);
			}
			break;

		case TxtPos_Above:
			if (btn->glyph)
			{	btn->textY  = (btn->height - (glyheight + txtheight + (txtheight>>2)))>>1;
				btn->glyphY = btn->textY + txtheight + (txtheight>>2);
			}
			break;
		case TxtPos_Left:
			if (btn->glyph)
			{	btn->textX  = (btn->width - (glywidth + txtwidth + 5))>>1;
				btn->glyphX = btn->textX + txtwidth + 5;
			}
		case TxtPos_Right:
			if (btn->glyph)
			{	btn->glyphX  = (btn->width - (glywidth + txtwidth + 5))>>1;
				btn->textX = btn->glyphX + glywidth + 5;
			}
		default:
			{}
	}
}

void guiButtonDraw(guiObject *me)
{	intf x,y;
	guiButton *b = (guiButton*)me;
	if (!b->pressed)
	{	hline(b->x,            b->y,             b->width,    0xfff0f0f0);
		hline(b->x+1,          b->y+b->height-1, b->width-1,  0xff4b4b4b);
		vline(b->x,            b->y+1,           b->height-1, 0xfff0f0f0);
		vline(b->x+b->width-1, b->y,             b->height,   0xff4b4b4b);
	}	else
	{	hline(b->x,            b->y,             b->width,    0xff4b4b4b);
		hline(b->x+1,          b->y+b->height-1, b->width-1,  0xfff0f0f0);
		vline(b->x,            b->y+1,           b->height-1, 0xff4b4b4b);
		vline(b->x+b->width-1, b->y,             b->height,   0xfff0f0f0);
	}
	box(b->x+1, b->y+1, b->width-2, b->height-2, b->facecolor);
	if (b->glyph)
	{	x = b->x + b->glyphX;
		y = b->y + b->glyphY;
		if (b->pressed)
		{	x++;
			y++;
		}
		if ((b->glyph->flags & bitmap_DataInfoMask) == bitmap_RGB_8bit)
			drawsprite(b->glyph,x,y);
		else
			drawbitmap(b->glyph,x,y);
	}
	setfont(&b->fontset);
	x = b->x + b->textX;
	y = b->y + b->textY;
	if (b->pressed)
	{	x++;
		y++;
	}
	textout(x, y, b->text);
}

void guiButtonDrawDrag(guiObject *me, intf x, intf y)
{	// Check for dragging authorization
	if (guimouseover==me) return;
	if (!guimouseover)
	{	// Dragging is not compatible
		drawsprite(&guiredcross,x,y);
		return;
	}
	if (guimouseover->AcceptDrag(guimouseover, me, true))
	{	// Dragging is compatible
		box(x,y,16,16,0xff000000);
		box(x+1,y+1,14,14,0xffffffff);
	}	else
	{	// Dragging is not compatible
		drawsprite(&guiredcross,x,y);
	}
}

void guiButtonMouseDown(guiObject *me, intf x, intf y)
{	guiButton *b = (guiButton *)me;
	b->pressed = true;
	guiSetWindowFocus(b->parent);
}

void guiButtonDrag(guiObject *me, intf x, intf y)
{	guiButton *b = (guiButton *)me;
	b->parent->setchanged(b->parent);
	b->pressed = false;
	if (guimouseover==me)
	{	b->pressed = true;
	}
}

void guiButtonEndDrag(guiObject *me, intf x, intf y)
{	guiButton *b = (guiButton *)me;
	guiObject *dragdest = guimouseover;
	b->pressed = false;
	b->parent->setchanged(b->parent);
	if (dragdest==me)
	{	// Button has been clicked
		if (b->clickfunc) b->clickfunc(b);
		if (!(b->flags & guibtn_NoMessages))
		{	guiCreateMessage(currentguiSystem, guiCommand_Click, me);
		}
	}	else
	{	if (guimouseover) guimouseover->AcceptDrag(guimouseover, me, false);
	}
}

guiButton *newguiButtonBase(const char *text, bitmap *glyph, bool membitmap, intf x, intf y, uintf width, uintf height, guiWindow *parent)
{	guiButton *b = (guiButton *)newguiObject(guitype_Button, x, y, width, height, parent);
	b->glyph = glyph;
	memfill(b->text,0,sizeof(b->text));
	if (text)
		txtcpy(b->text,guiButtonTextLen,text);
	else
	{	b->text[0]=0;
		b->textX = 0;
		b->textY = 0;
	}
	b->textlen = txtlen(b->text);
	b->pressed = false;
	b->width = width;
	b->height = height;
	b->draw = guiButtonDraw;
	b->DrawDragImage = guiButtonDrawDrag;
	b->CoversPoint = guiRectangularCoversPoint;
	b->MouseDown = guiButtonMouseDown;
	b->Drag = guiButtonDrag;
	b->EndDrag = guiButtonEndDrag;
	b->clickfunc = NULL;
	b->userdata1 = NULL;
	b->userdata2 = 0;
	getfont(&b->fontset);
	guiButtonSetTextPos(b, TxtPos_Below);
	return b;
}

guiButton *newguiButton(const char *text, bitmap *glyph, intf x, intf y, uintf width, uintf height, guiWindow *parent)
{	return newguiButtonBase(text, glyph, false, x, y, width, height, parent);
}

guiButton *newguiButton(const char *text, const char *glyph, intf x, intf y, uintf width, uintf height, guiWindow *parent)
{	return newguiButtonBase(text, newbitmap(glyph), true, x, y, width, height, parent);
}

// ****************************************************************
// ***															***
// ***							GUI Bitmaps						***
// ***															***
// ****************************************************************
void guiBitmapDraw(guiObject *me)
{	guiBitmap *bm = (guiBitmap *)me;
	if (bm->flags & guiBitmap_sprite)
		drawsprite(bm->bm, bm->x, bm->y);
	else
		drawbitmap(bm->bm, bm->x, bm->y);
}

guiBitmap *newguiBitmapBase(bitmap *bm, bool sprite, bool membitmap, intf x, intf y, guiWindow *parent)
{	guiBitmap *b = (guiBitmap *)newguiObject(guitype_Bitmap, x, y, bm->width, bm->height, parent);
	b->bm = bm;
	b->draw = guiBitmapDraw;
	if (membitmap) b->flags += gui_membitmap;
	if (sprite) b->flags |= guiBitmap_sprite;
	return b;
}

guiBitmap *newguiBitmap(bitmap *bm, bool sprite, intf x, intf y, guiWindow *parent)
{	return newguiBitmapBase(bm, sprite, false, x, y, parent);
}

guiBitmap *newguiBitmap(char *flname, bool sprite, intf x, intf y, guiWindow *parent)
{	return newguiBitmapBase(newbitmap(flname), sprite, true, x, y, parent);
}

// ****************************************************************
// ***															***
// ***							GUI Editbox						***
// ***															***
// ****************************************************************
void guiEditBoxDraw(guiObject *me)
{	guiEditBox *eb = (guiEditBox*)me;
	hline(eb->x,             eb->y,              eb->width,    0xff4b4b4b);
	hline(eb->x+1,           eb->y+eb->height-1, eb->width-1,  0xfff0f0f0);
	vline(eb->x,             eb->y+1,            eb->height-1, 0xff4b4b4b);
	vline(eb->x+eb->width-1, eb->y,              eb->height,   0xfff0f0f0);
	box(eb->x+1, eb->y+1, eb->width-2, eb->height-2, eb->facecolor);
	setfont(&eb->fontset);
	intf xpos = eb->x+4;
	if (eb->flags & guiEditBox_RightJustify) xpos = eb->x + eb->width-4-gettextwidth(eb->text);
	if (eb->cursx>=0)
	{	textoutc(xpos, eb->y+4, eb->text,eb->cursx);
	}	else
	{	textout(xpos, eb->y+4, eb->text);
	}
}

void guiEditBoxMouseDown(guiObject *me, intf x, intf y)
{	guiEditBox *b = (guiEditBox *)me;
	if (b->flags & guiEditBox_ReadOnly) return;
	if (b->flags & guiEditBox_RightJustify)
		x-=b->parent->canvasx + b->x + b->width-4-gettextwidth(b->text);
	else
		x-=b->parent->canvasx + b->x + 4;
	setfont(&b->fontset);
	intf xpos = 0;
	b->cursx = 0;
	char tmp = b->text[b->cursx];
	while (tmp>0)
	{	if (x<=xpos) break;
		xpos += getcharwidth(tmp);
		b->cursx++;
		tmp = b->text[b->cursx];
	}
	guiSetWindowFocus(b->parent);
	txtcpy(b->undo,guiEditBoxTextLen, b->text);
	// Create a click message
	if (!(b->flags & guiEditBox_NoMessages))
		guiCreateMessage(currentguiSystem, guiCommand_Click, me);
	while (input->keyboard[0].keyReady) input->readKey();		// Clear keyboard queue
}

void guiEditBoxLoseFocus(guiObject *me)
{	guiEditBox *e = (guiEditBox *)me;
	e->cursx = -1;
	if (e->editfunc) e->editfunc(e,true);
	if (!(e->flags & guiEditBox_NoMessages))
		guiCreateMessage(currentguiSystem, guiCommand_EndEdit, me);
}

void guiEditBoxChangeText(guiEditBox *eb, const char *text)
{	txtcpy(eb->text,guiEditBoxTextLen, text);
	eb->parent->setchanged(eb->parent);
	if (focusobj==(guiObject*)eb)
	{	focusobj = NULL;
		eb->cursx = -1;
	}
	if (eb->editfunc) eb->editfunc(eb,true);
	if (!(eb->flags & guiEditBox_NoMessages))
		guiCreateMessage(currentguiSystem, guiCommand_EndEdit, (guiObject *)eb);
}

void guiEditBoxKeyPress(guiObject *me, unsigned char kg)
{	guiEditBox *e = (guiEditBox *)me;
	if (e->flags & guiEditBox_ReadOnly) return;
	e->parent->setchanged(e->parent);

	sEditText edit;
	edit.text = e->text;
	edit.maxLength = guiEditBoxTextLen;
	edit.flags = 0;
	edit.cursor = e->cursx;
	bool changed = editText(&edit, kg);
	e->cursx = edit.cursor;
	if (changed)
	{	if (e->editfunc) e->editfunc(e,false);
		if (!(e->flags & guiEditBox_NoMessages))
			guiCreateMessage(currentguiSystem, guiCommand_Edit, me);
	}

	if (kg==key_ENTER)								// ENTER
	{	focusobj = NULL;
		e->cursx = -1;
		if (e->editfunc) e->editfunc(e,true);
		if (!(e->flags & guiEditBox_NoMessages))
			guiCreateMessage(currentguiSystem, guiCommand_EndEdit, me);
	}
	if (kg==key_ESC)								// ESC
	{	focusobj = NULL;
		e->cursx = -1;
		txtcpy(e->text,guiEditBoxTextLen, e->undo);
		if (e->editfunc) e->editfunc(e,true);
		if (!(e->flags & guiEditBox_NoMessages))
			guiCreateMessage(currentguiSystem, guiCommand_EndEdit, me);
	}
}

guiEditBox *newguiEditBox(const char *text, intf x, intf y, intf width, guiWindow *parent)
{	guiEditBox *eb = (guiEditBox *)newguiObject(guitype_EditBox, x, y, width, currentfont.bm->height + 7, parent);
	getfont(&eb->fontset);
	eb->width = width;
	eb->height = eb->fontset.bm[0].height + 7;
	eb->cursx = -1;
	eb->editfunc = NULL;
	txtcpy(eb->text,guiEditBoxTextLen, text);
	eb->draw = guiEditBoxDraw;
	eb->MouseDown = guiEditBoxMouseDown;
	eb->KeyPress = guiEditBoxKeyPress;
	eb->LoseFocus = guiEditBoxLoseFocus;
	eb->CoversPoint = guiRectangularCoversPoint;//guiEditBoxCoversPoint;
	eb->facecolor = 0xffffffff;
	return eb;
}

// ****************************************************************
// ***															***
// ***							GUI Spinner						***
// ***															***
// ****************************************************************
void guiSpinnerUp(guiButton *upbtn)
{	guiSpinner *s = (guiSpinner *)upbtn->userdata1;
	if (s->value < s->maxvalue)
	{	s->value++;
		guiEditBoxChangeText(s->editbox,buildstr("%i",s->value));
	}
}

void guiSpinnerDn(guiButton *dnbtn)
{	guiSpinner *s = (guiSpinner *)dnbtn->userdata1;
	if (s->value > s->minvalue)
	{	s->value--;
		guiEditBoxChangeText(s->editbox,buildstr("%i",s->value));
	}
}

void guiSpinnerEdit(guiEditBox *b, bool finished)
{	guiSpinner *s = (guiSpinner *)b->userdata1;
	s->editbox->editfunc = NULL;
	intf value = atoi(b->text);
	if (value>=s->minvalue && value<=s->maxvalue)
	{	s->value = value;
	}	else
	{	if (finished)
		{	if (value < s->minvalue)
			{	s->value = s->minvalue;
				guiEditBoxChangeText(s->editbox, buildstr("%i",s->value));
			}
			if (value > s->maxvalue)
			{	s->value = s->maxvalue;
				guiEditBoxChangeText(s->editbox, buildstr("%i",s->value));
			}
		}
	}
	s->editbox->editfunc = guiSpinnerEdit;
}

guiSpinner *newguiSpinner(intf startvalue, intf minvalue, intf maxvalue, intf x, intf y, intf width, guiWindow *parent)
{	guiSpinner *s = (guiSpinner *)newguiObject(guitype_Spinner, x, y, 0,0, parent);
	s->draw = guiObjectDrawNothing;
	s->value = startvalue;
	s->minvalue = minvalue;
	s->maxvalue = maxvalue;

	s->editbox = newguiEditBox(buildstr("%i",startvalue),x,y,width-15,parent);
	s->editbox->flags |= guiEditBox_NoMessages;
	s->editbox->userdata1 = (void *)s;
	s->editbox->editfunc = guiSpinnerEdit;

	s->upbtn = newguiButton("",&guiuscroll,x+width-14,y,14,s->editbox->height>>1,parent);
	s->upbtn->flags |= guibtn_NoMessages;
	s->upbtn->userdata1 = (void *)s;
	s->upbtn->clickfunc = guiSpinnerUp;

	s->dnbtn = newguiButton("",&guidscroll,x+width-14,y+(s->editbox->height>>1),14,s->editbox->height>>1,parent);
	s->dnbtn->flags |= guibtn_NoMessages;
	s->dnbtn->userdata1 = (void *)s;
	s->dnbtn->clickfunc = guiSpinnerDn;
	return s;
}

// ****************************************************************
// ***															***
// ***							GUI CheckBox					***
// ***															***
// ****************************************************************
void guiCheckBoxDraw(guiObject *me)
{	guiCheckBox *c = (guiCheckBox *)me;
	intf width = c->width;
	intf height= c->height;
	hline(c->x,         c->y,          width,    0xff4b4b4b);
	hline(c->x+1,       c->y+height-1, width-1,  0xfff0f0f0);
	vline(c->x,         c->y+1,        height-1, 0xff4b4b4b);
	vline(c->x+width-1, c->y,          height,   0xfff0f0f0);
	box(c->x+1, c->y+1, width-2,  height-2, c->facecolor);
	if (c->checked)
	{	drawsprite(&guitick,c->x+2,c->y+2);
	}
}

guiObject *guiCheckBoxCoversPoint(guiObject *me, intf x, intf y)
{	intf width = 13;
	intf height= 12;

	guiCheckBox *c = (guiCheckBox *)me;
	x-=c->parent->canvasx;
	y-=c->parent->canvasy;
	if (x>=c->x && y>=c->y && x<=c->x+width && y<=c->y+height)
		return me;
	else
		return NULL;
}

void guiCheckBoxMouseDown(guiObject *me, intf x, intf y)
{	guiCheckBox *c = (guiCheckBox *)me;
	c->checked = !c->checked;
	c->parent->setchanged(c->parent);
	if (c->checkfunc) c->checkfunc(c);
	guiCreateMessage(currentguiSystem, guiCommand_Click, me);
}

guiCheckBox *newguiCheckBox(intf x, intf y, guiWindow *parent)
{	guiCheckBox *c = (guiCheckBox*)newguiObject(guitype_CheckBox, x, y, 13, 12, parent);
	c->CoversPoint = guiCheckBoxCoversPoint;
	c->draw = guiCheckBoxDraw;
	c->MouseDown = guiCheckBoxMouseDown;
	c->checked = false;
	c->checkfunc = NULL;
	c->facecolor = 0xffffffff;
	return c;
}

// ****************************************************************
// ***															***
// ***							GUI RadioButton					***
// ***															***
// ****************************************************************
void guiRadioButtonDraw(guiObject *me)
{	guiRadioButton *r = (guiRadioButton *)me;
	if (r->checked)
		drawsprite(&guiradio_c, r->x, r->y);
	else
		drawsprite(&guiradio_nc,r->x, r->y);
}

void guiRadioButtonMouseDown(guiObject *me, intf x, intf y)
{	guiRadioButton *r = (guiRadioButton *)me;
	guiObject *o = r->parent->children;
	while (o)
	{	if (o->objecttype == guitype_RadioButton)
		{	guiRadioButton *rb = (guiRadioButton *)o;
			if (rb->groupID==r->groupID) rb->checked = false;
		}
		o = o->siblings;
	}
	r->checked = true;
	r->parent->setchanged(r->parent);
	if (r->checkfunc) r->checkfunc(r);
	guiCreateMessage(currentguiSystem, guiCommand_Click, me);
}

guiObject *guiRadioButtonCoversPoint(guiObject *me, intf x, intf y)
{	intf width = guiradio_c.width;
	intf height= guiradio_c.height;

	guiRadioButton *c = (guiRadioButton *)me;
	x-=c->parent->canvasx;
	y-=c->parent->canvasy;
	if (x>=c->x && y>=c->y && x<=c->x+width && y<=c->y+height)
		return me;
	else
		return NULL;
}

guiRadioButton *newguiRadioButton(intf groupID, intf x, intf y, guiWindow *parent)
{	guiRadioButton *r = (guiRadioButton *)newguiObject(guitype_RadioButton,x,y,guiradio_c.width,guiradio_c.height, parent);
	r->groupID = groupID;
	r->checkfunc = NULL;
	r->draw = guiRadioButtonDraw;
	r->CoversPoint = guiRadioButtonCoversPoint;
	r->MouseDown = guiRadioButtonMouseDown;

	// Scan the parent window to see if this is the first use of this GroupID, and if so, check it
	r->checked = true;
	guiObject *o = parent->children;
	while (o)
	{	if ((o->objecttype == guitype_RadioButton) && ((guiRadioButton *)o != r))
		{	guiRadioButton *rb = (guiRadioButton *)o;
			if (rb->groupID==groupID) r->checked = false;
		}
		o = o->siblings;
	}
	return r;
}

void guiRadioButtonCheck(guiRadioButton *rb);

// ****************************************************************
// ***															***
// ***							GUI BevelBox					***
// ***															***
// ****************************************************************
void guiBevelBoxDraw(guiObject *me)
{	guiBevelBox *b = (guiBevelBox *)me;

	// Top Line
	hline(b->x,            b->y,             8,           0xff808080);
	hline(b->x+1,          b->y+1,           7,           0xffffffff);
	hline(b->x+14+b->namewidth, b->y,		 b->width-b->namewidth-15, 0xff808080);
	hline(b->x+14+b->namewidth, b->y+1,		 b->width-b->namewidth-16, 0xffffffff);

	// Right Line
	vline(b->x+b->width-2, b->y+1,           b->height-2, 0xff808080);
	vline(b->x+b->width-1, b->y+1,           b->height-1, 0xffffffff);

	// Bottom Line
	hline(b->x,            b->y+b->height-2, b->width-2,  0xff808080);
	hline(b->x+1,          b->y+b->height-1, b->width-2,  0xffffffff);

	// Left Line
	vline(b->x,            b->y+1,           b->height-2, 0xff808080);
	vline(b->x+1,          b->y+2,           b->height-4, 0xffffffff);

	setfont(&b->fontset);
	textout(b->x+11, b->y-(b->fontset.bm[0].height>>1),b->name);
}

guiBevelBox *newguiBevelBox(char *name, intf x, intf y, intf width, intf height, guiWindow *parent)
{	guiBevelBox *b = (guiBevelBox *)newguiObject(guitype_BevelBox, x, y, width, height, parent);
	b->draw = guiBevelBoxDraw;
	b->width = width;
	b->height = height;
	getfont(&b->fontset);
	b->namewidth = gettextwidth(name);
	txtcpy(b->name,guiBevelBoxNameLen, name);
	return b;
}

// ****************************************************************
// ***															***
// ***							GUI HotSpot						***
// ***															***
// ****************************************************************
void guiHotSpotDraw(guiObject *me)
{	guiHotSpot *h = (guiHotSpot*)me;
	if (guimouseover==me)
	{	box(h->x, h->y, h->width, h->height, h->facecolor);
	}
	else
	{	box(h->x, h->y, h->width, h->height, h->parent->facecolor);
	}
}

void guiHotSpotMouseDown(guiObject *me, intf x, intf y)
{	guiSetWindowFocus(me->parent);
}

void guiHotSpotDrag(guiObject *me, intf x, intf y)
{//	### TODO: Finish Function
//	guiHotSpot *h = (guiHotSpot *)me;
//	guiObject *dragdest = guimouseover;
//	{	// Check for dragging authorization
//	}
}

void guiHotSpotEndDrag(guiObject *me, intf x, intf y)
{	guiHotSpot *h = (guiHotSpot *)me;
	guiObject *dragdest = guimouseover;
	if (dragdest==me)
	{	// Button has been clicked
		if (h->clickfunc) h->clickfunc(h);
		if (!(h->flags & guiHotSpot_NoMessages))
		{	guiCreateMessage(currentguiSystem, guiCommand_Click, me);
		}
	}	else
	{	// Attempt to perform a drag-drop
	}
}

guiObject *guiHotSpotCoversPoint(guiObject *me, intf x, intf y)
{	guiHotSpot *b = (guiHotSpot *)me;
	x-=b->parent->canvasx;
	y-=b->parent->canvasy;
	if (x>=b->x && y>=b->y && x<=b->x+b->width-1 && y<=b->y+b->height-1)
		return me;
	else
		return NULL;
}

void guiHotSpotMouseOver(guiObject *me, bool over)
{	me->parent->setchanged(me->parent);
}

guiHotSpot *newguiHotSpot(intf x, intf y, intf width, intf height, guiWindow *parent)
{	guiHotSpot *h = (guiHotSpot *)newguiObject(guitype_HotSpot, x, y, width, height, parent);
	h->width = width;
	h->height = height;
	h->pressed = false;
	h->MouseDown = guiHotSpotMouseDown;
	h->Drag = guiHotSpotDrag;
	h->EndDrag = guiHotSpotEndDrag;
	h->clickfunc = NULL;
	for (uintf i=0; i<guiHotSpotData; i++)
	{	h->userptr[i] = NULL;
		h->useruintf[i] = 0;
	}
	h->draw = guiHotSpotDraw;
	h->MouseOver = guiHotSpotMouseOver;
	h->CoversPoint = guiHotSpotCoversPoint;
	h->facecolor = 0xff8080ff;
	return h;
}

// ****************************************************************
// ***															***
// ***							GUI DropBox						***
// ***															***
// ****************************************************************
void guiDropBoxChangeSelection(guiDropBox *d, guiMenuItem *selection)
{	if (!selection)
	{	d->selection = NULL;
		guiEditBoxChangeText(d->editbox, "");
		return;
	}
	d->selection = selection;
	guiEditBoxChangeText(d->editbox, selection->name);
}

void guiDropBoxItemClick(guiHotSpot *h)
{	guiDropBox *d = (guiDropBox *)h->userptr[0];
	guiMenuItem *selection = (guiMenuItem *)h->userptr[1];
	guiDropBoxChangeSelection(d, selection);
	h->parent->kill((guiObject *)h->parent);
	if (selection->clickfunc) selection->clickfunc(selection);
	if (d->changefunc) d->changefunc(d);
}

void guiDropBoxDrop(guiButton *b)
{	guiDropBox *d = (guiDropBox *)b->userdata1;
	intf height = 16*d->numChoices+8;
	d->dropwin = newguiMenuWin(d->x, d->y+20, d->editbox->width+16, height, guiDropBoxItemClick, d->choices, &d->editbox->fontset, d);
}

guiDropBox *newguiDropBox(intf x, intf y,intf width, guiWindow *parent)
{	guiDropBox *d = (guiDropBox *)newguiObject(guitype_DropBox, x, y, width+16, 20, parent);	// ### TODO: work out a propper height
	d->draw = guiObjectDrawNothing;
	d->editbox = newguiEditBox("* Uninitialized *",x,y,width,parent);
	d->editbox->flags |= guiEditBox_NoMessages | guiEditBox_ReadOnly;
	d->dropbutton = newguiButton("",&guidscroll, x+width,y,16,20,parent);
	d->dropbutton->userdata1 = d;
	d->dropbutton->clickfunc = guiDropBoxDrop;
	d->dropbutton->flags |= guibtn_NoMessages;
	d->selection = NULL;
	d->choices = NULL;
	d->numChoices = 0;
	d->changefunc = NULL;
	return d;
}

guiMenuItem *newguiDropBoxItem(const char *name, void (*clickfunc)(guiMenuItem *me), guiDropBox *parent)
{	guiMenuItem *m = (guiMenuItem *)newguiObject(guitype_MenuItem, 0, 0, gettextwidth(name)+16, 18, NULL);

	intf y = 0;

	if (parent->choices)
	{	guiMenuItem *i = parent->choices;
		while (i->link)
		{	i=i->link;
			y+=18;
		}
		i->link = m;
		m->x = 0;
		m->y = y;
	}	else
	{	parent->choices = m;
		m->x = 0;
		m->y = y;
		guiEditBoxChangeText(parent->editbox, name);
	}

	txtcpy(m->name, guiMenuItemNameLen, name);
	m->children = NULL;
	m->clickfunc = NULL;
	m->parent = NULL;
	m->MouseOver = NULL;
	parent->numChoices++;
	return m;
}

// ****************************************************************
// ***															***
// ***							GUI TabControl					***
// ***															***
// ****************************************************************

void guiTabControlDraw(guiObject *me)
{	guiTabControl *t = (guiTabControl *)me;
	setfont(&t->fontset);
	hline(t->x,t->y+21,t->width,0xffffffff);
	vline(t->x,t->y+22,t->height-23,0xffffffff);

	hline(t->x+1,t->y+t->height-2,t->width-2,0xff808080);
	hline(t->x  ,t->y+t->height-1,t->width,0xff404040);

	vline(t->x+t->width-2,t->y+22,t->height-22,0xff808080);
	vline(t->x+t->width-1,t->y+21,t->height-21,0xff404040);
	for (intf i=0; i<t->tabcount; i++)
	{	if (i==t->tabindex)
		{	// Draw active Tab header
			intf x = t->x + t->tabx[i] - 2;
			intf y = t->y;
			vline(x, y+3, 18, 0xffffffff);
			vline(x+1,y+3,18, t->parent->facecolor);
			pset(x+1,y+2, 0xffffffff);
			hline(x+2, y+1, t->tabwidth[i], 0xffffffff);
			pset(x+t->tabwidth[i]+2,y+2,0xff404040);
			vline(x+t->tabwidth[i]+3,y+3,18,0xff404040);
			vline(x+t->tabwidth[i]+2,y+3,18,0xff808080);
			textout(x+9, y+5, t->window[i]->name);
			hline(x+1,y+21,t->tabwidth[i], t->parent->facecolor);
		}	else
		{	// Draw non-active Tab header
			intf x = t->x + t->tabx[i];
			intf y = t->y;
			if ((i-1)!=t->tabindex)
			{	vline(x, y+5, 16, 0xffffffff);
				pset(x+1,y+4, 0xffffffff);
			}
			hline(x+2, y+3, t->tabwidth[i]-4, 0xffffffff);
			pset(x+t->tabwidth[i]-2,y+4,0xff404040);
			vline(x+t->tabwidth[i]-1,y+5,16,0xff404040);
			vline(x+t->tabwidth[i]-2,y+5,16,0xff808080);
			textout(x+6, y+7, t->window[i]->name);
		}
	}
	if (t->tabindex<t->tabcount)
	{	t->window[t->tabindex]->draw((guiObject*)t->window[t->tabindex]);
	}
}

bool guiTabControlChangeIndex(guiTabControl *tab, intf index)
{	if (index<0 || index>=tab->tabcount) return false;
	tab->tabindex = index;
	tab->parent->setchanged(tab->parent);
	return true;
}

void guiTabControl_MouseDown(guiObject *me, intf x, intf y)
{	guiTabControl *t = (guiTabControl*)me;
	x -= t->x + t->parent->canvasx;
	y -= t->y + t->parent->canvasy;
	for (intf i=0; i<t->tabcount; i++)
	{	if (x>=t->tabx[i] && x<=t->tabwidth[i]+t->tabx[i])
			guiTabControlChangeIndex(t,i);
	}
}

guiObject *guiTabControlCoversPoint(guiObject *me, intf x, intf y)
{	guiTabControl *t = (guiTabControl *)me;
	guiObject *result = NULL;
	intf lx = x-t->parent->canvasx;
	intf ly = y-t->parent->canvasy;
	if (lx>t->x && lx<t->x+t->width-1 && ly>t->y && ly<t->y+t->height-1)
	{	result = me;
		if (t->tabindex<t->tabcount)
		{	guiObject *tmp = t->window[t->tabindex]->CoversPoint((guiObject*)t->window[t->tabindex],x,y);
			if (tmp) result = tmp;
		}
	}
	return result;
}

guiWindow *guiTabAddPage(guiTabControl *tab, char *name)
{	if (tab->tabcount+1>=guiMaxTabPages) return NULL;
	tab->window[tab->tabcount] = newguiWindow(name,tab->x+1, tab->y+22, tab->width-3, tab->height-24, guiWindow_TabPage, tab->parent);
	tab->tabwidth[tab->tabcount] = gettextwidth(name) + 12;
	if (tab->tabcount==0)
		tab->tabx[tab->tabcount] = 2;
	else
		tab->tabx[tab->tabcount] = tab->tabx[tab->tabcount-1] + tab->tabwidth[tab->tabcount-1];
	tab->tabcount++;
	return tab->window[tab->tabcount-1];
}

guiTabControl *newguiTabControl(intf x, intf y, intf width, intf height, guiWindow *parent)
{	guiTabControl *t = (guiTabControl *)newguiObject(guitype_TabControl, x,y, width, height, parent);
	t->draw = guiTabControlDraw;
	t->width = width;
	t->height = height;
	t->tabcount = 0;
	t->tabindex = 0;
	t->CoversPoint = guiTabControlCoversPoint;
	t->MouseDown = guiTabControl_MouseDown;
	t->x = x;
	t->y = y;
	getfont(&t->fontset);
	return t;
}

// ****************************************************************
// ***															***
// ***							GUI System						***
// ***															***
// ****************************************************************
void guiSystem_SetChanged(guiSystem *me)
{	me->flags |= guiWindow_changed;
}

void guiSystemKill(guiObject *me)
{	guiSystem *sys = (guiSystem *)me;
	while (sys->children)
	{	sys->children->kill(sys->children);
	}
	guiDefaultKill(me);
}

guiSystem *newguiSystem(void)
{	guiSystem *s = (guiSystem *)newguiObject(guitype_System, 0, 0, 0,0, NULL);
	s->children = NULL;
	s->setchanged = guiSystem_SetChanged;
	s->canvasx = 0;
	s->canvasy = 0;
	s->oldestmessage = 0;
	s->newestmessage = 0;
	s->scrollx = 0;
	s->scrolly = 0;
	s->kill = guiSystemKill;
	focuswin = NULL;
	focusobj = NULL;
	guimouseover = NULL;
	lastmousebtn = 0;
	lastclick = NULL;
	return s;
}

void drawgui(guiSystem *gui)
{	guiWindow *win = (guiWindow *)gui->children;
	while (win)
	{	win->draw((guiObject *)win);
		win = (guiWindow *)win->siblings;
	}
	gui->flags &=~guiWindow_changed;
}

uintf rungui(guiSystem *gui, intf mousex, intf mousey, uintf mousebut, unsigned char key)
{	uintf result = guiResult_KeysValid;

	currentguiSystem = gui;
	DrawDragImage = NULL;
	guiObject *lastmouseover = guimouseover;
	guimouseover = guiCoversPoint(gui, mousex, mousey);
	if (lastmouseover!=guimouseover)
	{	if (lastmouseover)
			if (lastmouseover->MouseOver) lastmouseover->MouseOver(lastmouseover, false);
		if (guimouseover)
			if (guimouseover->MouseOver) guimouseover->MouseOver(guimouseover, true);
	}

	if (mousebut & 1)
	{	if ((lastmousebtn&1)==0)
		{	// Left Click has occured
			if (guimouseover!=focusobj)
			{	if (focusobj)
				{	if (focusobj->LoseFocus) focusobj->LoseFocus(focusobj);
				}
				focusobj = guimouseover;
			}

			if (guimouseover)
			{	if (guimouseover->MouseDown) guimouseover->MouseDown(guimouseover, mousex, mousey);
			}	else
			{	guiSetWindowFocus(NULL);
			}
			lastclick = guimouseover;
		}	else
		{	// We're dragging
			if (lastclick)
			{	if (lastclick->Drag) lastclick->Drag(lastclick, mousex, mousey);
				DrawDragImage = lastclick->DrawDragImage;
			}
		}
	}	else if (lastclick)
	{	// We've finished dragging an object
		if (lastclick->EndDrag) lastclick->EndDrag(lastclick, mousex, mousey);
		lastclick = NULL;
	}

	if (focusobj)
	{	if (focusobj->KeyPress)
		{	result &=~guiResult_KeysValid;
			if (key) focusobj->KeyPress(focusobj,key);
		}
	}
	currentguiSystem = NULL;
	lastmousebtn = mousebut;
	return result;
}

guiObject *guiCoversPoint(guiSystem *gui, intf x, intf y)
{	guiWindow *win = (guiWindow *)gui->children;
	guiObject *clickobj = NULL;
	while (win)
	{	guiObject *lastobj = win->CoversPoint((guiObject *)win, x, y);
		if (lastobj) clickobj = lastobj;
		win = (guiWindow *)win->siblings;
	}
	return clickobj;
}

void guiValidateTest(const char *objectname, uintf objectsize)
{	if (objectsize > sizeof(guiObject)) msg("Gui Validation Error",buildstr("%s too large (%i bytes, maximum = %i bytes)",objectname,objectsize,sizeof(guiObject)));
}

void guiValidate(void)
{	// This function performs a series of tests on the GUI to ensure there's size-related bugs
	guiValidateTest("guiSystem",sizeof(guiSystem));
	guiValidateTest("guiWindow",sizeof(guiWindow));
	guiValidateTest("guiMenuItem",sizeof(guiMenuItem));
	guiValidateTest("guiLabel", sizeof(guiLabel));
	guiValidateTest("guiButton",sizeof(guiButton));
	guiValidateTest("guiBitmap",sizeof(guiBitmap));
	guiValidateTest("guiEditBox",sizeof(guiEditBox));
	guiValidateTest("guiSpinner",sizeof(guiSpinner));
	guiValidateTest("guiCheckBox",sizeof(guiCheckBox));
	guiValidateTest("guiHotSpot",sizeof(guiHotSpot));
	guiValidateTest("guiDropBox",sizeof(guiDropBox));
	guiValidateTest("guiTabControl",sizeof(guiTabControl));
}

void guiCreateMessage(guiSystem *gui, guiCommand cmd, guiObject *obj)
{	guiMessage *m = &gui->messages[gui->newestmessage];
	m->cmd = cmd;
	m->obj = obj;

	// Perform circular array access
	gui->newestmessage++;
	if (gui->newestmessage>=guiMaxMessages) gui->newestmessage=0;
	if (gui->newestmessage==gui->oldestmessage)
	{	gui->oldestmessage++;
		if (gui->oldestmessage>=guiMaxMessages) gui->oldestmessage = 0;
	}
}

guiMessage *guiRetrieveMessage(guiSystem *gui)
{	// We throw in a checkmsg call here in case the programmer is brain-dead and created a loop waiting for a message which will never come
	checkmsg();

	if (gui->oldestmessage == gui->newestmessage) return NULL;  // No new messages
	guiMessage *result = &gui->messages[gui->oldestmessage];
	gui->oldestmessage++;
	if (gui->oldestmessage>=guiMaxMessages) gui->oldestmessage = 0;
	return result;
}

void drawguimouse(guiSystem *gui, intf mousex, intf mousey)
{	if (DrawDragImage) DrawDragImage(lastclick, mousex+8, mousey+15);
	//drawsprite(&mousebitmap, mousex, mousey);
}

bool guiStandardDrawDrag(guiObject *me, intf x, intf y)
{	// Check for dragging authorization
	if (guimouseover==me) return false;
	if (!guimouseover)
	{	// Dragging is not compatible
		drawsprite(&guiredcross,x,y);
		return false;
	}
	if (guimouseover->AcceptDrag(guimouseover, me, true))
	{	return true;
	}	else
	{	drawsprite(&guiredcross,x,y);
		return false;
	}
}

void initBucketGui(void)
{	usedguiObject = NULL;
	freeguiObject = NULL;
	guiValidate();
}

void killBucketGui(void)
{	killbucket(guiObject, flags, gui_memstruct);
}
