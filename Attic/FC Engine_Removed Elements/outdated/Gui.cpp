// ****************************************************************************
// ***						GUI bonus utility for FC engine					***
// ***----------------------------------------------------------------------***
// *** Author .......... Stephen Fraser										***
// *** Created .........  1 AUG 2001										***
// *** Last Modified ... 30 Dec 2001										***
// *** Requirements .... none												***
// ***----------------------------------------------------------------------***
// *** See GUI manual for details on using the GUI							***
// ***----------------------------------------------------------------------***
// *** Known Problems - Does not gracefully handle multiple windows			***
// ***					Switching windows, does not work properly			***
// ***					Attempting to drag a window while others are on		***
// ***						screen does not work properly					***
// ****************************************************************************

#include <string.h>
#include "..\fcio.h"
#include "..\gui.h"

// Global Variables for GUI
char *tooltip;
char tooltipholder[80];
dword mouseappendage;
dword mouseicon;

// Misc functions
void settooltip(char *msg)
{	strcpy(tooltipholder,msg);
	tooltip = tooltipholder;
}

bool guidrag = false;
guiWindow *focaschanged;
guiWindow *modalwindow = NULL;
guiWindow *guidesktop = NULL;

bool rungui(void)
{	mouseappendage = 0;
	if (!guidesktop) return false;
	focaschanged = NULL;
	// Check for the end of a mouse drag on a GUI item
	if (guidrag && !(mousebut & 1))
	{	guidesktop->dragend();
		guidrag = false;
		if (!guidesktop) return false;	// Desktop window may have been deleted, so check for it here to prevent a crash
	}

	if (guidrag && (mousebut & 1))
	{	// Handle mouse dragging event on the GUI
		guidesktop->mousedrag();
		guidesktop->run(false);
		if (focaschanged) focaschanged->bringforeward();
		return true;
	}	else
	{	// Handle regular GUI interface
		mouseicon = 0;
		if (guidesktop->run(true) && mousebut & 1) 
		{	guidrag = true;
			if (focaschanged) focaschanged->bringforeward();
			return true;
		}
		if (focaschanged) focaschanged->bringforeward();
		if (modalwindow) return true;
		return false;
	}
}

void drawgui(void)
{	if (guidesktop) guidesktop->draw();
}

void drawguimouse(void)
{	switch (mouseappendage)
	{	case 0:
				break;
		case appendage_button:
				box(mousex+5,mousey+10,20,15,0);
				box(mousex+6,mousey+11,18,13,0xffffff);
				break;
		case appendage_cross:
				box(mousex+5,mousey+10,20,15,0);
				box(mousex+6,mousey+11,18,13,0xff0000);
				break;
	}	
	switch(mouseicon)
	{	case 0 : drawsprite(&mousebitmap,mousex,mousey); break;
		case 1 : drawsprite(&hotmousebitmap,mousex-6,mousey); break;
		case 2 : drawsprite(&mouseovertext,mousex-5,mousey-10); break;
		default: drawsprite(&mousebitmap,mousex,mousey); break;
	}
	drawtooltip();
}

void drawtooltip(void)
{	if (tooltip)
	{	setfontcolor(0);
		long ttwidth = gettextwidth(tooltip)+6;
		long ttx = mousex-3;
		long tty = mousey+20;
		if (ttx<0) ttx=0;
		if (ttx+ttwidth>screenwidth) ttx=(screenwidth-ttwidth)-1;
		if (tty+19>screenheight) tty = mousey-20;
		box(ttx,tty,ttwidth,18,0);
		box(ttx+1,tty+1,ttwidth-2,16,0xe3e3bf);
		underlining = false;
		textoutu(ttx+3,tty+3,tooltip);
	}
}

// ****************************************************************************************
// ***							guiLabel : An editable text label						***
// ****************************************************************************************
guiLabel::guiLabel(long _x,long _y,char *_text,dword _flags,guiWindow *_parent) 
{	flags = _flags;
	parent = _parent; 
	x=_x+parent->canvasx; 
	y=_y+parent->canvasy; 
	changelabel(_text);
	parent->addelement(this);
	color = 0;
	otherThis = this;
}

void guiLabel::setotherThis(void *other)
{	if (other)	otherThis = other;
		else	otherThis = this;
}

void guiLabel::draw(void) 
{	dword justify = flags & label_justifyMask;
	setfontcolor(color);
	long tx;
	switch (justify)
	{	case label_justifyLeft:		tx=x;					break;
		case label_justifyRight:	tx=x-textwidth;			break;
		case label_justifyCenter:	tx=x-(textwidth>>1);	break;
	}
	if (!(flags & label_editing))
		textout(tx,y,text);
	else
		textoutc(tx,y,text,editpos);
	if (flags & label_underline) hline(x,y+11,textwidth,0);
}

void guiLabel::changelabel(char *_text)
{	strcpy(text,_text); 
	textwidth = gettextwidth(text);
}
	
void guiLabel::changelabel(void)
{	textwidth = gettextwidth(text);
}
		
bool guiLabel::run(bool react)
{	if ((flags & label_editing) && (parent->flags & winFocus) && (keyready))
	{	unsigned char kg = readkey();
		long tl = (long)strlen(text);
		long i;
		if (kg>31 && kg<128)
		{	// Insert character at this position
			if (tl>0)
			{	for (i=tl; i>=editpos && i>=0; i--)
					text[i+1]=text[i];
				text[editpos++]=kg;
			}	else
			{	text[0]=kg;
				text[1]=0;
				editpos=1;
			}
			if (editHandler) editHandler(otherThis,label_changed,editpos);
		}	else
		if (kg==203 && editpos>0) editpos--;	// Left
		if (kg==205 && editpos<tl) editpos++;	// Right
		if (kg==199) editpos=0;					// Home
		if (kg==207) editpos=tl;				// End
		if (kg==211 && editpos<tl)
		{	// Delete Key
			for (i=editpos; i<=tl; i++)
				text[i]=text[i+1];
			if (editHandler) editHandler(otherThis,label_changed,editpos);
		}
		if (kg==8 && editpos>0)
		{	// Backspace Key
			editpos--;
			for (i=editpos; i<=tl; i++)
				text[i]=text[i+1];
			if (editHandler) editHandler(otherThis,label_changed,editpos);
		}
		if (kg==13) // ENTER
		{	flags &= ~label_editing;
			if (editHandler) editHandler(otherThis,label_enter,editpos);
		}
		if (kg==27) // ESC
		{	flags &= ~label_editing;
			strcpy(text,editbuffer);
			if (editHandler) editHandler(otherThis,label_cancel,editpos);
		}
	}
	return false;
}

void guiLabel::edit(long pos,void *_editHandler)
{	flags |= label_editing;
	strcpy(editbuffer,text);
	if (pos<0 || pos>strlen(text)) pos = strlen(text);
	editpos = pos;
	editHandler = (void (*)(void *,dword,dword))_editHandler;
	while(keyready) readkey();	// Clear out the keyboard cache (if any)
}

// ****************************************************************************************
// ***							guiCloseBox : Close Box for Windows						***
// ****************************************************************************************
guiCloseBox::guiCloseBox(long _x,long _y,dword _flags,guiWindow *parent)
{	init(_x,_y,15,15,NULL,"Close",parent);
	flags = _flags;
}

void guiCloseBox::customdraw(bool mouseover,bool pressed)
{	long xx = x;
	long yy = y;
	if (mouseover && pressed) 
	{	xx++;
		yy++;
	}
	box		(xx+1 ,yy+1,14,14,0xc0c0c0);
	if (yy<0 || yy+15>screenheight) return;	// ### (drawline function does not yet clip)
	drawsprite(&guicross,xx+4,yy+4);
}

void guiCloseBox::click(void)
{	if (parent)
	{	if (parent->parent)
			parent->parent->removeelement(parent);
		delete parent;		// We shouldn't need to call this but somehow it's destructor is not called from removeelement!
	}
}

// ****************************************************************************************
// ***						guiBevelHLine : Horizontal Bevel Line							***
// ****************************************************************************************
guiBevelHLine::guiBevelHLine(long _x,long _y,long _width,dword _flags,guiWindow *_parent)
{	width = _width;
	flags = _flags;
	parent = _parent;
	x=_x+parent->canvasx;
	y=_y+parent->canvasy;
	parent->addelement(this);
}

void guiBevelHLine::draw(void)
{	if (flags==bevelIn)
	{	hline(x,y,width,0x4b4b4b);
		hline(x,y+1,width,0xf0f0f0);
	}	else
	{	hline(x,y,width,0xf0f0f0);
		hline(x,y+1,width,0x4b4b4b);
	}	
}

// ****************************************************************************************
// ***							guiBevelBox : A simple bevel box							***
// ****************************************************************************************
guiBevelBox::guiBevelBox(long _x,long _y,long _width,long _height,dword _flags,guiWindow *_parent)
{	parent = _parent;
	width = _width;
	height= _height;
	flags = _flags;
	x=_x+parent->canvasx;
	y=_y+parent->canvasy;
	parent->addelement(this);
}

void guiBevelBox::draw(void)
{	if (flags&bevelIn)
	{	hline(x,y,width,0x4b4b4b);
		hline(x+1,y+height,width,0xf0f0f0);
		vline(x,y,height,0x4b4b4b);
		vline(x+width,y+1,height-1,0xf0f0f0);
	}	else
	{	hline(x,y,width,0xf0f0f0);
		hline(x+1,y+height,width,0x4b4b4b);
		vline(x,y,height,0xf0f0f0);
		vline(x+width,y+1,height-1,0x4b4b4b);
	}	
	if (flags&bevelFilled)
	{	box(x+2,y+2,width-3,height-3,0xffffff);
	}
}

// ****************************************************************************************
// ***							guiBitmap : A basic bitmap image							***
// ****************************************************************************************
guiBitmap::guiBitmap(long _x,long _y,bitmap *_image,guiWindow *_parent)
{	parent = _parent;	
	x=parent->canvasx+_x; 
	y=parent->canvasy+_y; 
	image=_image; 
	parent->addelement(this);
}

void guiBitmap::draw(void)
{	drawbitmap(image,x,y);
}

// ****************************************************************************************
// ***							guiSprite : A basic sprite image							***
// ****************************************************************************************
guiSprite::guiSprite(long _x,long _y,bitmap *_bitmap,guiWindow *_parent) 
{	parent = _parent;	
	x=parent->canvasx+_x; 
	y=parent->canvasy+_y; 
	bm=_bitmap; 
	parent->addelement(this);
}

void guiSprite::draw(void)
{	drawsprite(bm,x,y);
}

// ****************************************************************************************
// ***							guiSelectBox : A selection box							***
// ****************************************************************************************
guiSelectBox::guiSelectBox(long _x,long _y, long _width, long _height,dword col,void (*_dblclick)(guiSelectBox *),guiWindow *_parent)
{	parent = _parent;
	x = parent->canvasx+_x; 
	y = parent->canvasy+_y;
	width = _width;
	height = _height;
	selectcolor = col;
	selected = false;
	dblclick = _dblclick;
	parent->addelement(this);
}

void guiSelectBox::draw(void)
{	if (selected)
	{	if (parent->flags & winFocus)
			box(x,y,width,height,selectcolor);
		else
		{	hline(x,y,width,selectcolor);
			hline(x,y+height-1,width,selectcolor);
			vline(x,y,height,selectcolor);
			vline(x+width,y,height,selectcolor);
		}
	}
}

bool guiSelectBox::run(bool react)
{	if (!(mousebut & 1)) return false;
	if (react && mousex>=x && mousey>=y && mousex<=x+width && mousey<=y+height)
	{	// This object has just been selected ... check for a double click
		if (selected && globalTimer<(selecttime+currentTimerHz) && dblclick)
		{	// Object has been double clicked
			parent->deselect();
			while (mousebut & 1) 
			{	getmouse();
				checkmsg();
			}
			dblclick(this);
			return true;
		}  else selecttime = globalTimer;
		if (!keydown[fckey_LEFTCTRL] && !keydown[fckey_RIGHTCTRL])  parent->deselect();
		selected = true;
		return true;
	}
	return false;
}

void guiSelectBox::deselect(void)
{	selected = false;
}

// ****************************************************************************************
// ***				guiBasicButton : The building blocks for most buttons				***
// ****************************************************************************************
void guiBasicButton::init(long _x,long _y,long _width,long _height,void (*_handler)(guiBasicButton *button),char *_tooltiptext,guiWindow *_parent)
{	width = _width;
	height= _height;
	x=_x+_parent->canvasx;
	y=_y+_parent->canvasy;
	dragdata = 0;
	pressed = false;
	handler = _handler;
	parent = _parent;
	accept = NULL;
	copy = NULL;
	facecolor = 0xc0c0c0;
	mouseover_fn = NULL;
	if (_tooltiptext)
		strcpy(tooltiptext,_tooltiptext);
	else
		tooltiptext[0]=0;
	if (parent) parent->addelement(this);
	overlaytype = overlay_button;
}

void guiBasicButton::draw(void)
{	bool mouseover = (mousex>x) && (mousex<x+width) && (mousey>y) && (mousey<y+height);
	if (pressed && mouseover)
	{	hline(x,y,width,0x4b4b4b);
		hline(x+1,y+height,width,0xf0f0f0);
		vline(x,y,height,0x4b4b4b);
		vline(x+width,y+1,height-1,0xf0f0f0);
	}	else
	{	hline(x,y,width,0xf0f0f0);
		hline(x+1,y+height,width,0x4b4b4b);
		vline(x,y,height,0xf0f0f0);
		vline(x+width,y+1,height-1,0x4b4b4b);
	}
	if (parent->flags & winFilled)
	{	box(x+1,y+1,width-1,height-1,facecolor); 
	}
	customdraw(mouseover,pressed);
}

bool guiBasicButton::run(bool react)
{	if (!react) return false;
	bool mouseover = mousex>x && mousex<x+width && mousey>y && mousey<y+height;
	if (mouseover) mouseicon = 1;
	if (mouseover && mouseover_fn) mouseover_fn(this);
	if (tooltiptext[0] && mouseover && !(mousebut&1))
	{	tooltip = tooltiptext;
	}
	if (!(mousebut & 1)) return false;
	if (mouseover)
	{	pressed = true;
		return true;
	}
	return false;
}

void guiBasicButton::mousedrag(void)
{	long i;
	lastdragdest = NULL;
	if (accept)
	{	mouseappendage = appendage_cross;
		// Iterate through all buttons within the parent and check for an accept command
		for (i=0; i<parent->element.size(); i++)
		{	guiOverlay *tmp = parent->element[i];
			if (tmp->overlaytype&overlay_button)
			{	// The current overlay is a button
				guiBasicButton *but = (guiBasicButton *)tmp;
				if (!but->accept) continue;	// button does not accept drag & drop events
				if ((mousex>but->x) && (mousex<but->x+but->width) && (mousey>but->y) && (mousey<but->y+but->height))
				{	// Mouse is actually over this button, check it for compatibility
					if (but==this) 
					{	// Mouse is still over the originating button ... don't display appendage
						mouseappendage = 0;
						continue;
					}
					if (but->accept(but,this))
					{	lastdragdest = but;
						mouseappendage = appendage_button;
						break;
					}
				}	// if mouse over button
			}	// if overlay type is a button
		}	// for each overlay element within the current window
	}	// if this button accepts drag, drop events
}

void guiBasicButton::dragend(void)
{	pressed = false;
	if (mousex>x && mousex<x+width && mousey>y && mousey<y+height)
		click();
	if (lastdragdest && copy)
		copy(lastdragdest,this);
}

void guiBasicButton::enabledragdrop(dword _dragdata,bool (*accept_func)(guiBasicButton *receiver, guiBasicButton *sender),void (*copy_func)(guiBasicButton *receiver, guiBasicButton *sender))
{	dragdata = _dragdata;
	accept   = accept_func;
	copy	 = copy_func;
}


// guiColorButton - plain colored button
guiColorButton::guiColorButton(long _x,long _y,long _width,long _height,dword _color, void (*_handler)(guiBasicButton *button),guiWindow *_parent)
{	init(_x,_y,_width,_height,_handler,NULL,_parent);
	color = _color;
}

void guiColorButton::customdraw(bool mouseover,bool pressed) 
{	box(x+1,y+1,width-1,height-1,color);
}

// cSpriteButton - Button containing a registered sprite
guiBitmapButton::guiBitmapButton(long _x,long _y,long _width,long _height,bitmap *_bitmap, void (*_handler)(guiBasicButton *button),guiWindow *_parent)
{	init(_x,_y,_width,_height,_handler,NULL,_parent);
	bm = _bitmap;
}

void guiBitmapButton::customdraw(bool mouseover,bool pressed) 
{	if (bm)
		if ((bm->flags & bitmap_bppmask) == bitmap_8bit)
			drawsprite(bm,x+((width-bm->width)>>1)+1,y+((height-bm->height)>>1)+1);
		else
			drawbitmap(bm,x+((width-bm->width)>>1)+1,y+((height-bm->height)>>1)+1);
}

// guiTextButton - a button containing only text
guiTextButton::guiTextButton(long _x,long _y, long _width, long _height, char *_label, void (*_handler)(guiBasicButton *button), guiWindow *_parent)
{	init (_x,_y,_width,_height,_handler,NULL,_parent);
	strcpy(label,_label);
}

void guiTextButton::customdraw(bool mouseover,bool pressed)
{	setfontcolor(0);
	long xx = x+((width-gettextwidth(label))>>1)+1;
	long yy = y+4;
	if (mouseover && pressed) {xx++; yy++;}
	textout(xx,yy,label);
}

void guiTextButton::click(void)
{	if (handler) handler(this);
}

// guiSpinnerUp - a small up button used for spinner controls
guiSpinnerUp::guiSpinnerUp(long _x,long _y, guiSpinner *_parent, guiWindow *_winparent)
{	spinparent = _parent;
	init(_x,_y,13,7,NULL,NULL,_winparent);
}

void guiSpinnerUp::customdraw(bool mouseover, bool pressed)
{	hline(x+6,y+2,2,0);
	hline(x+5,y+3,4,0);
	hline(x+4,y+4,6,0);
	hline(x+3,y+5,8,0);
}

void guiSpinnerUp::click(void)
{	spinparent->change(1);
}

// guiSpinnerUp - a small up button used for spinner controls
guiSpinnerDn::guiSpinnerDn(long _x,long _y, guiSpinner *_parent, guiWindow *_winparent)
{	spinparent = _parent;
	init(_x,_y,13,7,NULL,NULL,_winparent);
}

void guiSpinnerDn::customdraw(bool mouseover, bool pressed)
{	hline(x+3,y+2,8,0);
	hline(x+4,y+3,6,0);
	hline(x+5,y+4,4,0);
	hline(x+6,y+5,2,0);
}

void guiSpinnerDn::click(void)
{	spinparent->change(-1);
}

// ****************************************************************************************
// ***							guiSpinner : Name [ 1234]#								***
// ****************************************************************************************
void spinnerchange(void *spinner, dword exitcode, dword posx)
{	guiSpinner *s = (guiSpinner *)spinner;
	s->textedited(exitcode,posx);
}

guiSpinner::guiSpinner(long _x,long _y,long _width,long _min, long _max, long _value, char *_label, void (*_handler)(guiSpinner *spinner), guiWindow *_parent)
{	x = _x+_parent->canvasx;
	y = _y+_parent->canvasy;
	width = _width;
	min = _min;
	max = _max;
	value = _value;
	parent = _parent;
	customdraw = NULL;
	editbox = new guiEditBox(_x,_y,width,_label,"*",spinnerchange,parent);
	editbox->setotherThis(this);
	sprintf(editbox->data->text,"%i",value);
	editbox->data->changelabel();
	up = new guiSpinnerUp(_x+width+1,_y+1,this,parent);
	down = new guiSpinnerDn(_x+width+1,_y+10,this,parent);
	handler = _handler;
	parent->addelement(this);
}

void guiSpinner::change(long difference)
{	value += difference;
	if (value<min) value = min;
	if (value>max) value = max;
	if (handler) 
		handler(this);
	if (customdraw)
		customdraw(this,editbox->data->text,value);
	else
		sprintf(editbox->data->text,"%i",value);
	editbox->data->changelabel();
	editbox->selectbox->selected = false;
	editbox->facecolor = 0xffffffff;
	editbox->selectbox->selectcolor = 0xffffffff;
}

bool strtoint(char *txt, long *value)
{	long ofs = 0;
	long result = 0;
	bool gotonechar = false;
	bool negative = false;
	
	while (txt[ofs]==' ') ofs++;	// Skip spaces
	
	// Check for a negative number
	if (txt[ofs]=='-')
	{	negative = true;
		ofs++;
	}

	while (txt[ofs]!=0)
	{	if (txt[ofs]>='0' && txt[ofs]<='9')
		{	gotonechar = true;
			result = (result*10) + (long)(txt[ofs]-'0');
		}	else
			return false;
		ofs++;
	}
	if (negative) result = -result;
	*value = result;
	return gotonechar;
}

void guiSpinner::textedited(dword exitcode, dword posx)
{	bool valid = strtoint(editbox->data->text,&value);
	if (value>max) { value = max; valid = false; }
	if (value<min) { value = min; valid = false; }
	if (valid)
	{	editbox->facecolor = 0xffffffff;
		editbox->selectbox->selectcolor = 0xffffffff;
		if (handler) 
			handler(this);
//		if (customdraw)
//			customdraw(this,editbox->data->text,value);
//		else
//			sprintf(editbox->data->text,"%i",value);
//		editbox->data->changelabel();
	}	else
	{	editbox->facecolor = 0xfff0a0a0;
		editbox->selectbox->selectcolor = 0xfff0a0a0;
	}
}

// ****************************************************************************************
// ***							guiEditBox : A standard edit box						***
// ****************************************************************************************
void guieditboxedit(void *editbox,dword exitcondition,dword exitX) // Callback function - redirects to class
{	guiEditBox *eb = (guiEditBox *)editbox;
	eb->textchanged(exitcondition, exitX);
}

guiEditBox::guiEditBox(long _x,long _y, long _width, char *_label, char *_data, void (*_handler)(void *editbox,dword exitcode,dword posX), guiWindow *_parent)
{	parent = _parent;
	handler = (void (*)(void *,dword,dword))_handler;
	x = _x+parent->canvasx;
	y = _y+parent->canvasy;
	facecolor = 0xffffffff;
	width = _width;
	bevelbox = new guiBevelBox(_x,_y,width,17,bevelIn,parent);
	if (_label)
	{	label = new guiLabel(_x-3,_y+3,_label,label_justifyRight,parent);
	}	else
		label = NULL;
	parent->addelement(this);
	selectbox = new guiSelectBox(_x+1,_y+1,width-1,16,0xffffff,NULL,parent);
	data  = new guiLabel(_x+5,_y+3,_data,label_justifyLeft,parent);
	data->setotherThis((void *)this);
	otherThis = (void *)this;
}

void guiEditBox::draw(void)
{	box(x+1,y+1,width-1,16,facecolor); 
}

bool guiEditBox::run(bool react)
{	if (data->flags & label_editing)
	{	if (!selectbox->selected)
		{	data->flags &= ~label_editing;
		}
	}	else
	{	if (selectbox->selected)
		{	// Just clicked inside edit box
			char *msg = data->text;
			long x = data->x;
			long pos = 0;
			while (mousex>x && msg[pos]!=0)
			{	x+=getcharwidth(msg[pos++]);
			}
			data->edit(pos,guieditboxedit);
		}
	}
	return false;
}	

void guiEditBox::textchanged(dword exitcondition, dword exitX)
{	if (exitcondition!=label_changed)
	{	selectbox->deselect();
	}
	if (handler) 
		handler((void *)otherThis,exitcondition,exitX);
}

void guiEditBox::setotherThis(void *other)
{	if (other)
		otherThis = other;
	else
		otherThis = (void *)this;
}

void guiEditBox::edit(long cursorX)
{	selectbox->selected = true;
	data->edit(cursorX,guieditboxedit);
}

// ****************************************************************************************
// ***							 	guiSlider : A sliderbar								***
// ****************************************************************************************
guiSlider::guiSlider(long _x,long _y,long _width,float _min,float _max, float _value, char *_label, void (*_handler)(guiSlider *slider),guiWindow *_parent)
{	parent = _parent;
	x = _x + parent->canvasx;
	y = _y + parent->canvasy;
	width = _width;
	min = _min;
	max = _max;
	scale = (float)width / (max-min);
	value = _value;
	handler = _handler;
	label=NULL;
	if (_label)
	{	if (strlen(_label)>0)
			label = new guiLabel((_x - 6),_y+1,_label,label_justifyRight,parent);
	}
	parent->addelement(this);
	calcposition();
}

void guiSlider::draw(void)
{	hline(x,y+6,width,0);
	vline(x,y+4,5,0);
	vline(x+width,y+4,5,0);
	
	// Draw the slider box
	vline(x+position-4,y,   14,0xf0f0f0);
	hline(x+position-3,y,    7,0xf0f0f0);
	hline(x+position-3,y+14, 8,0x4b4b4b);
	vline(x+position+4,y+1, 13,0x4b4b4b);
	//if (parent->flags & winFilled)
		box(x+position-3,y+1,7,12,0xc0c0c0);
}

bool guiSlider::run(bool react)
{	bool retval = false;
	if (mousey>y && mousey<y+12 && mousex>=x && mousex<=x+width) 
	{	retval = true;
		if (react)
			settooltip(buildstr("%.2f",value));
	}

	if (!react || (mousebut & 1)==0) return false;
	// Return true if mouse event was handled
	return retval;
}

void guiSlider::mousedrag(void)
{	position = mousex-x;
	if (position<0) position=0;
	if (position>width) position=width;
	calcvalue();
	settooltip(buildstr("%.2f",value));
	if (handler) handler(this);
}


// ****************************************************************************************
// ***					 	guiProgressBar : A progress indicator						***
// ****************************************************************************************
guiProgressBar::guiProgressBar(long _x,long _y,long _width,float _min,float _max, float _value, 
							   char *_label, void (*_handler)(guiSlider *slider),
							   guiWindow *_parent) : 
							   guiSlider(_x,_y,_width,_min,_max,_value,_label,_handler,_parent)
{	rangestart = _min;
	rangeend = _max;
}

void guiProgressBar::draw(void)
{	float range = max-min;
	dword left = (dword)((width*(rangestart-min))/range);
	dword right= (dword)((width*(rangeend-min))/range);
	hline(x,y,width,0x4b4b4b);
	hline(x+1,y+14,width,0xf0f0f0);
	vline(x,y,14,0x4b4b4b);
	vline(x+width,y+1,13,0xf0f0f0);
	box(x+left,y+1,position-left,12,0xffffffff);
	if (left>0)
		box(x+1,y+1,left,12,0xff707070);
	if (right<width)
		box(x+1+right,y+1,width-right,12,0xff707070);
	drawsprite(&guislideptr,x+position-4,y);
}

// ****************************************************************************************
// ***							guiWindow : Main Window class							***
// ****************************************************************************************
#define windrag_none		0
#define windrag_title		1
#define windrag_udscroll	2
#define windrag_lrscroll	3

guiWindow::guiWindow(char *_title,long _x, long _y, long _width, long _height, dword _flags,guiWindow *_parent)
{	//consoleadd("Window %x Created",this);
	if (_parent == NULL)
	{	guidesktop = this;
	}
	parent = _parent;
	closefunc = NULL;
	renderfunc = NULL;
	if (parent)
	{	_x+=parent->canvasx;
		_y+=parent->canvasy;
	}
	closebox = NULL;
	width = _width;
	height= _height;
	portalwidth = width-2;
	portalheight = height-2;
	flags = _flags;
	visible = true;
	canvasx = (x =_x)+1;
	canvasy = (y =_y)+1;
	scrollx = 0;
	scrolly = 0;
	if (flags & winHasTitle)
	{	rename(_title);
		if (flags & winHasClose)
		{	closebox = new guiCloseBox(portalwidth-17,1,0,this);
			removeelement(closebox);
		}
		canvasy+=19;
		portalheight-=19;
	}
	
	if (flags & winHasMenu)
	{	canvasy+=19;
		portalheight-=19;
	}
	menu = NULL;
	activemenu = NULL;
	canvaswidth = portalwidth;
	canvasheight = portalheight;
	clickeditem = NULL;
	if (parent)
	{	parent->addchild(this);
	}
	overlaytype = overlay_dontcare;
	if ((flags & winNeverFocus)==0) guidesktop->setfocus(this);
	if (flags & winModal) 
		modalwindow = this;
	else
		modalwindow = NULL;
	facecolor = 0xc0c0c0;
}

void guiWindow::rename(char *_title)
{	strcpy(title,_title);
	titlex = x + ((width-gettextwidth(title)) >> 1);
}

void guiWindow::show(void)
{	visible = true;
}

void guiWindow::hide(void)
{	//visible = false;
}

void guiWindow::move(long dx, long dy)
{	x += dx;
	y += dy;
	titlex += dx;
	canvasx += dx;
	canvasy += dy;
	for (long i=0; i<element.size(); i++)
		element[i]->move(dx,dy);
	for (i=0; i<children.size(); i++)
		children[i]->move(dx,dy);
	if (flags & winHasClose)
		closebox->move(dx,dy);
}

void guiWindow::draw(void)
{	// Draw the outline of the Window
	if (!visible) return;
	long i;
	
	dword frameheight = 0;
	if (flags & winHasFrame)
		frameheight=height-1;
	else
	if (flags & winHasTitle)
		frameheight=19;

	if (frameheight)
	{	hline(x, y,width,0xf0f0f0);					// Top line
		vline(x, y+1,height-2,0xf0f0f0);			// Left Line
		vline(x+width-1, y+1,height-2,0xf0f0f0);	// Right Line
		if (!(flags & winHasTitle))
			hline(x, y+height-1,width,0xf0f0f0);	// Bottom Line
	}
	
	if (flags & winHasTitle)
	{	// Draw Title Bar
		if (flags & winFocus)
		{	for (dword i=1; i<19; i++) 
			{	hline(x+1,y+i,width-2,255-i*12);
			}
		}	else
		for (dword i=1; i<19; i++) 
		{	dword col=128-i*6;
			hline(x+1,y+i,width-2,(col << 16) + (col << 8) + col);
		}
		hline(x,y+19,width,0xf0f0f0);				// Seperator between title bar and window canvas
		setfontcolor(0xffffff);
		textout(titlex,y+4,title);
	}
	if (flags & winHasClose)
		closebox->draw();
	
	// This code used to be below the pulldown menu
	if (flags & winFilled)
		box(canvasx,canvasy,portalwidth,portalheight,facecolor);

	// Handle the pulldown menu system
	if (flags & winHasMenu)
	{	box(x+1,y+20,width-2,18,0xd6d3ce);
		hline(x+1,y+38,width-2,0x848484);
		setfontcolor(0);
		guiMenuItem *pdmenu = menu;
		while (pdmenu)
		{	if (mousex>x+pdmenu->x-3 && mousex<x+pdmenu->x+pdmenu->width+3 && mousey>y+21 && mousey<y+35)
			{	if (activemenu==NULL)
				{	hline(x+pdmenu->x-5,y+21,pdmenu->width+9,0xffffff);
					hline(x+pdmenu->x-4,y+35,pdmenu->width+9,0x808080);
					vline(x+pdmenu->x-4,y+22,14,0xffffff);
					vline(x+pdmenu->x+pdmenu->width+5,y+22,14,0x808080);
				}
				else
				{	activemenu = pdmenu;
					hline(x+pdmenu->x-5,y+21,pdmenu->width+9,0x808080);
					hline(x+pdmenu->x-4,y+35,pdmenu->width+9,0xffffff);
					vline(x+pdmenu->x-4,y+22,14,0x808080);
					vline(x+pdmenu->x+pdmenu->width+5,y+22,14,0xffffff);					
				}
			}
			textoutu(x+pdmenu->x,y+22,pdmenu->name);
			pdmenu = pdmenu->next;
		}		
	}
	
//	This code got moved up
//	if (flags & winFilled)
//		box(canvasx,canvasy,portalwidth,portalheight,facecolor);

	long sbwidth = 0,sbheight = 0;
	if (canvaswidth>portalwidth)
	{	sbheight=13;
		if (canvasheight+13>portalheight)
		{	sbwidth = 13;
		}
	}	else
	if (canvasheight>portalheight)
	{	sbwidth=13;
		if (canvaswidth+13>portalwidth)
		{	sbheight=13;
		}
	}
	
	portalwidth-=sbwidth;
	portalheight-=sbheight;

	if (canvaswidth>portalwidth)
	{	// Left / Right scrollbars
		long btx = canvasx+1;
		long bty = canvasy+portalheight+sbheight-13;
		long btw = 12;
		long bth = 12;
		hline(btx    ,bty    ,btw  ,0xf0f0f0);
		hline(btx+1  ,bty+bth,btw  ,0x4b4b4b);
		vline(btx    ,bty    ,bth  ,0xf0f0f0);
		vline(btx+btw,bty+1  ,bth-1,0x4b4b4b);
		drawsprite(&guilscroll,btx+3,bty+3);

		long scrwidth = portalwidth+sbwidth-39;
		dword ratio = (scrwidth<<8) / canvaswidth;	
		dword boxstart = ((scrollx            ) * ratio)>>8;
		dword boxstop  = ((scrollx+portalwidth) * ratio)>>8;
		box(btx+btw+1,bty,scrwidth,bth,0xe0e0e0);
		dword boxwidth = boxstop-boxstart;
		boxstart+=btx+btw+1;
		box(boxstart+1,bty+1,boxwidth-2,bth-2,0xc0c0c0);
		hline(boxstart    ,bty    ,boxwidth  ,0xf0f0f0);
		hline(boxstart+1  ,bty+bth,boxwidth  ,0x4b4b4b);
		vline(boxstart    ,bty    ,bth  ,0xf0f0f0);
		vline(boxstart+boxwidth,bty+1  ,bth-1,0x4b4b4b);
		
		btx = canvasx+portalwidth+sbwidth-26;
		hline(btx    ,bty    ,btw  ,0xf0f0f0);
		hline(btx+1  ,bty+bth,btw  ,0x4b4b4b);
		vline(btx    ,bty    ,bth  ,0xf0f0f0);
		vline(btx+btw,bty+1  ,bth-1,0x4b4b4b);
		drawsprite(&guirscroll,btx+5,bty+3);
	}

	if (canvasheight>portalheight)
	{	// Up / Down scrollbars
		long btx = canvasx+portalwidth+sbwidth-13;
		long bty = canvasy+1;
		long btw = 12;
		long bth = 12;
		hline(btx    ,bty    ,btw  ,0xf0f0f0);
		hline(btx+1  ,bty+bth,btw  ,0x4b4b4b);
		vline(btx    ,bty    ,bth  ,0xf0f0f0);
		vline(btx+btw,bty+1  ,bth-1,0x4b4b4b);
		drawsprite(&guiuscroll,btx+3,bty+4);

		long scrheight = portalheight+sbheight-39;
		dword ratio = (scrheight<<8) / canvasheight;	
		dword boxstart = ((scrolly             ) * ratio)>>8;
		dword boxstop  = ((scrolly+portalheight) * ratio)>>8;
		box(btx,bty+bth+1,btw,scrheight,0xe0e0e0);
		dword boxheight = boxstop-boxstart;
		boxstart+=bty+bth+1;
		box  (btx+1  ,boxstart+1        ,btw-2      ,boxheight-2,0xc0c0c0);
		hline(btx    ,boxstart          ,btw        ,0xf0f0f0);
		hline(btx+1  ,boxstart+boxheight,btw        ,0x4b4b4b);
		vline(btx    ,boxstart+1        ,boxheight-1,0xf0f0f0);
		vline(btx+btw,boxstart+1        ,boxheight-1,0x4b4b4b);

		bty = canvasy+portalheight+sbheight-26;
		hline(btx    ,bty    ,btw  ,0xf0f0f0);
		hline(btx+1  ,bty+bth,btw  ,0x4b4b4b);
		vline(btx    ,bty    ,bth  ,0xf0f0f0);
		vline(btx+btw,bty+1  ,bth-1,0x4b4b4b);
		drawsprite(&guidscroll,btx+3,bty+5);
	}

	long cx1 = clipx1,cx2 = clipx2,cy1 = clipy1,cy2 = clipy2;
	setclipper(canvasx,canvasy,canvasx+portalwidth,canvasy+portalheight);
	if (renderfunc) renderfunc(this);

	for (i=0; i<element.size(); i++)
		element[i]->draw();
	for (i=0; i<children.size(); i++)
		children[i]->draw();

	resetclipper();
	setclipper(cx1,cy1,cx2,cy2);

	portalwidth+=sbwidth;
	portalheight+=sbheight;

	if (flags & winHasMenu && activemenu)
	{	long mw = 120;	// Width of menu
		long mh =   0;	// Height of drop-down menu
		guiMenuItem *pdmenu = activemenu->children;
		while (pdmenu)
		{	mh+=18; 
			pdmenu=pdmenu->next;
		}
		if (mh>0)
		{	long x1 = x+activemenu->x-5;
			long y1 = y+39;
			hline(x1     ,y1-1 ,mw+1 ,0xffffff);
			hline(x1     ,y1+mh,mw+1 ,0x808080);
			vline(x1     ,y1   ,mh   ,0xffffff);
			vline(x1+mw+1,y1   ,mh+1 ,0x808080);
			box  (x1+1   ,y1   ,mw,mh,0xd6d3ce);
		}
		pdmenu = activemenu->children;
		mh = y+42;
		while (pdmenu)
		{	if (mousex>x+activemenu->x && mousex<x+activemenu->x+mw && mousey>mh && mousey<mh+18)
			{	box(x+activemenu->x-4,mh-2,mw,17,0x08246b);
				setfontcolor(0xffffff);	
			}	
			else setfontcolor(0);
			textoutu(x+activemenu->x,mh,pdmenu->name);
			mh+=18;
			pdmenu=pdmenu->next;
		}
	}
}

bool guiWindow::run(bool react)
{	bool result = false;
	bool childactivated = false;
	bool reactonstart = react;
	long i;
	
	if (!visible) return false;

	if (activemenu) react=false;
	guiMenuItem *lastmenu = activemenu;
	
	if (!(flags & winFocus))
		activemenu = false;

	long scrollingx = 0;
	long scrollingy = 0;
	long sbwidth = 0;
	long sbheight = 0;
	if (flags &winFocus && canvasheight>portalheight)
	{	scrollingy -= mousewheel*40;
		mousewheel = 0;
	}
	
	if (flags&winFocus && mousebut&1 && ((!modalwindow) || modalwindow==this))
	{	// Check for clicks on the scrollbars
		if (canvaswidth>portalwidth)
		{	sbheight=13;
			if (canvasheight+13>portalheight)
			{	sbwidth = 13;
			}
		}	else
		if (canvasheight>portalheight)
		{	sbwidth=13;
			if (canvaswidth+13>portalwidth)
			{	sbheight=13;
			}
		}
	
		portalwidth-=sbwidth;
		portalheight-=sbheight;
		
		if (canvaswidth>portalwidth)
		{	// Left / Right scrollbars
			long bty = canvasy+portalheight+sbheight-13;
			if (mousey>bty && mousey<bty+12)
			{	if (mousex>canvasx && mousex<canvasx+13 && scrollx>0)
				{	scrollingx-=portalwidth>>5;
				}
				long btx = canvasx+portalwidth+sbwidth-26;
				if (mousex>btx && mousex<btx+13 && scrollx+portalwidth<canvaswidth)
				{	scrollingx+=portalwidth>>5;
				}
				if (mousex>canvasx+13 && mousex<btx)
				{	//dragging = windrag_lrscroll;
					long scrwidth = portalwidth+sbwidth-39;
					dword ratio = (scrwidth<<8) / canvaswidth;	
					long boxwidth = (portalwidth * ratio)>>8;
					long pos = (mousex-(canvasx+13));
					long newboxstart = pos-(boxwidth>>1);
					long newboxstop = newboxstart+boxwidth;
					if (newboxstart<0) 
					{	newboxstop=boxwidth;
						newboxstart = 0;
					}
					if (newboxstop>scrwidth)
					{	newboxstart=scrwidth-boxwidth;
						newboxstop=scrwidth;
					}
					scrollingx = (newboxstart*canvaswidth)/scrwidth-scrollx;
				}
			}
		}

		if (canvasheight>portalheight)
		{	// Up / Down scrollbars
			long btx = canvasx+portalwidth+sbwidth-13;
			if (mousex>btx && mousex<btx+12)
			{	if (mousey>canvasy && mousey<canvasy+13 && scrolly>0)
				{	scrollingy-=portalheight>>5;
				}
				long bty = canvasy+portalheight+sbheight-26;
				if (mousey>bty && mousey<bty+13 && scrolly+portalheight<canvasheight)
				{	scrollingy+=portalheight>>5;
				}
				if (mousey>canvasy+13 && mousey<bty)
				{	long scrheight = portalheight+sbheight-39;
					dword ratio = (scrheight<<8) / canvasheight;	
					long boxheight = (portalheight * ratio)>>8;
					long pos = (mousey-(canvasy+13));
					long newboxstart = pos-(boxheight>>1);
					long newboxstop = newboxstart+boxheight;
					if (newboxstart<0) 
					{	newboxstop=boxheight;
						newboxstart = 0;
					}
					if (newboxstop>scrheight)
					{	newboxstart=scrheight-boxheight;
						newboxstop=scrheight;
					}
					scrollingy = (newboxstart*canvasheight)/scrheight-scrolly;
				}
			}
		}
	}

	if (scrollingx!=0 || scrollingy!=0)
	{	for (long i=0; i<element.size(); i++)
			element[i]->move(scrollx,scrolly);
		for (i=0; i<children.size(); i++)
			children[i]->move(scrollx,scrolly);

		if (scrolly+portalheight>canvasheight) scrolly = canvasheight-portalheight;
		if (scrolly<0) scrolly = 0;

		scrollx+=scrollingx;
		scrolly+=scrollingy;
		
		for (i=0; i<element.size(); i++)
			element[i]->move(-scrollx,-scrolly);
		for (i=0; i<children.size(); i++)
			children[i]->move(-scrollx,-scrolly);
		result = true;
		react = false;
	}
	portalwidth+=sbwidth;
	portalheight+=sbheight;

	bool clickedwindow = false;
	bool inwin = (mousey>=y && mousey<=y+height && mousex>=x && mousex<=x+width);
	
	// Run all the children (regardless)
	for (i=children.size()-1; i>=0; i--)
		if (children[i]->run(react))
		{	result = true;
			if (mousebut & 1 && react) 
			{	clickeditem = children[i];
				react = false;
				childactivated = true;
			}
		}

	if ((!modalwindow) || modalwindow==this)
	{	if (react && (mousebut & 1) && (!dragging) && inwin && (!childactivated))
		{	clickedwindow = true;
			// We've clicked somewhere in this window, so bring it forewards
			guidesktop->setfocus(this); 
			result = true;
		}
		// Run all the elements
		for (i=element.size()-1; i>=0; i--)
			if (element[i]->run(react))
			{	if (mousebut & 1 && react) 
				{	clickeditem = element[i];
					react = false;
					clickedwindow = false;
				}
			}

		if (activemenu) react = true;
		if ((mousebut & 1) && react)
		{
			if (inwin && (flags&winHasTitle) && mousey<=y+20)
			{	lastmousex = mousex;
				lastmousey = mousey;
				dragging = windrag_title;
				clickedwindow = false;
				result=true;
			}
			if (flags & winHasClose && react)
				if (closebox->run(react)) 
				{	clickeditem = closebox;
					clickedwindow = false;
				}
			if (inwin && flags & winHasMenu)
			{	if (activemenu)
				{	guiMenuItem *pdmenu = activemenu->children;
					long x1 = x+activemenu->x-5;
					activemenu = NULL;
					long mw = 120;	// Width of menu
					long mh =   0;	// Height of drop-down menu
					if (mousex>x1 && mousex<x1+mw && mousey>39)
					{	while (pdmenu)
						{	long y1 = y+mh+39;
							if (mousey>y1 && mousey<y1+18)
							{	if (pdmenu->callback)
									pdmenu->callback(pdmenu,this);
							}
							mh+=18; 
							pdmenu=pdmenu->next;
						}
						result = true;
						clickedwindow = false;
					}
				}
				
				if (mousey>y+21 && mousey<y+38)
				{	// We're inside the main menu, activate it
					activemenu = menu;
					setfocus(this);
					result = true;
					clickedwindow = false;
				}
			}
		}
	}
	return result;
}

void guiWindow::addelement(guiOverlay *newelement)
{	newelement->x -= scrollx;
	newelement->y -= scrolly;
	newelement->overlaytype = overlay_dontcare;
	element.push_back(newelement);
}

void guiWindow::addchild(guiWindow *child)
{	children.push_back(child);
}

void guiWindow::bringforeward(void)
{	long i;

	if (!parent) return;
	if (*parent->children.end()-1==this) return;
	
	for (i=parent->children.size()-2; i>=0; i--)
	{	if (parent->children[i]==this)
		{	parent->children[i]=parent->children[i+1];
			parent->children[i+1]=this;
		}
		parent->bringforeward();
	}
}

void guiWindow::removeelement(guiOverlay *oldelement)
{	for (guiOverlay **i=element.begin(); i<element.end(); i++)
	{	if (*i==oldelement)
		{	//consoleadd("Overlay %x removed from window %x",oldelement,this);
			element.erase(i);
		}
	}
	for (guiWindow **j=children.begin(); j<children.end(); j++)
	{	if (*j==oldelement)
		{	//consoleadd("Child Window %x removed from window %x",oldelement,this);
			children.erase(j);
			//delete oldelement;
		}
	}
}

void guiWindow::mousedrag(void)
{	if (clickeditem) 
		clickeditem->mousedrag();
	else
	{	if (flags & winMovable && dragging==windrag_title)
		{	move(mousex-lastmousex,mousey-lastmousey);
			lastmousex = mousex;
			lastmousey = mousey;
		}
	}
}

void guiWindow::dragend(void)
{	if (clickeditem) 
	{	clickeditem->dragend(); 
		clickeditem=NULL;
	}
	dragging = windrag_none;
}

void guiWindow::setfocus(guiWindow *focaswin)
{	focaschanged = focaswin;
	if (this==focaswin) 
	{	flags |= winFocus; 
	}	else 
		flags &=~winFocus;

	// Iterate through all the windows, setting focus
	for (long i=0; i<children.size(); i++)
		children[i]->setfocus(focaswin);
	
	if (canvasheight>portalheight)
		mousewheel = 0;
}

void guiWindow::deselect(void)
{	for (long i=0; i<element.size(); i++)
		element[i]->deselect();
}

void guiWindow::resizecanvas(long _width, long _height)
{	//if (_width<portalwidth) _width = portalwidth;
	//if (_height<portalheight) _height = portalheight;
	canvaswidth = _width;
	canvasheight = _height;
}

guiWindow::~guiWindow(void)
{	//consoleadd("Window %x Closed",this);
	long i;
	if (closefunc) closefunc(this);
	if (guidesktop==this) guidesktop = NULL;
	for (i=0; i<children.size(); i++)
		delete children[i];
	for (i=0; i<element.size(); i++)
	{	delete element[i];
	}
	if (closebox) delete closebox;
	modalwindow = NULL;
}

guiMenu *newguiMenu(dword size)
{	byte *buf = fcalloc(sizeof(guiMenu)+size*sizeof(guiMenuItem),"GUI Menu");
	guiMenu *menu = (guiMenu *)buf;		buf +=sizeof(guiMenu);
	guiMenuItem *menuitem = (guiMenuItem *)buf;
	for (long i=0; i<size-1; i++)
		menuitem[i].next = &menuitem[i+1];
	menuitem[size-1].next = NULL;
	menu->freeitems = menuitem;
	menu->headNode = NULL;
	menu->numberused = 0;
	return menu;
}

guiMenuItem *guiMenuAdd(char *text,void	(*callback)(guiMenuItem *,guiWindow *),guiMenuItem *parentitem,guiMenu *owner)
{	guiMenuItem *insert;
	guiMenuItem *m = owner->freeitems;
	if (m)
	{	owner->freeitems = m->next;
	}	else
	{	msg("GUI Menu Overflow","This program has placed too many menu items into a menu");
	}
	
	owner->numberused++;

	m->name = text;
	m->callback = callback;
	m->next = NULL;
	m->children = NULL;

	if (!parentitem)
	{	if (!owner->headNode)
		{	owner->headNode = m;
			return m;
		}
		insert = owner->headNode;
		while (insert->next) insert=insert->next;
		insert->next = m;
		return m;
	}
	
	// This is a child node
	if (!parentitem->children)
	{	parentitem->children = m;
		return m;
	}
	insert = parentitem->children;
	while (insert->next) insert=insert->next;
	insert->next = m;
	return m;
}

void guiFixMenu(guiMenu *menu, dword flags)
{	if (flags == gui_pulldown)
	{	long x=10;
		guiMenuItem *item = menu->headNode;
		while (item)
		{	item->x = x;
			item->width = gettextwidthu(item->name);
			x+=15+item->width;
			item = item->next;
		}
	}
}

