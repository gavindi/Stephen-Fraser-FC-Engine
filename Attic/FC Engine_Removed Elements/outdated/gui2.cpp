#include "..\fcio.h"
#include "..\gui2.h"

gWindow *desktopwin = NULL;
gWindow *focuswin	= NULL;
gOverlay *dragging	= NULL;
dword dragparam		= 0;			// Paramater - like for a window, defined Title bar / scroll bar / white space
dword dragdatatype	= 0;			// A user-defined ID number for the type of data being copied.  0 always means drag and drop is not accepted.  As an example, 1 may mean that the data is a color, and 2 may mean a filename
void *dragdataptr	= NULL;			// A pointer to the actual data being dragged
gOverlay *guiKBfocus= NULL;			// A Pointer to the current oberlay with keyboard focus (or NULL for none)
dword guiParameter	= 0;			// Some GUI callback commands require an additional parameter.  The meaning of the parameter differs from command to command.
bool deletedesktop = false;
gOverlay *mouseover;				// Which overlay is the mouse currently over?

extern dword mouseappendage;
extern dword mouseicon;

// **************************************************************************** >gui<
// ***																		***
// ***									Label								***
// ***																		***
// ****************************************************************************
gLabel::gLabel(char *_text,long _x, long _y, dword _flags, gWindow *_parent)
{	overlaytype = overlayLabel;

	strcpy(text,_text);
	x=_x;
	y=_y;
	flags = _flags;
	parent = _parent;

	getfont(&fontdata);
	parent->addelement(this);
}

gLabel::~gLabel(void)
{	if (parent) parent->removeelement(this);
}

void gLabel::draw(void)
{	setfont(&fontdata);
	long xpos;
	switch (flags & label_justifyMask)
	{	case label_justifyLeft: xpos = x; break;
		case label_justifyCenter: xpos = x - (gettextwidth(text)>>1); break;
		case label_justifyRight : xpos = x - gettextwidth(text); break;
	}
	textout(xpos+parent->CanvasX,y+parent->CanvasY,text);
}

dword gLabel::edittext(void)
{	if (guiKBfocus==this && keyready)
	{	byte kg = readkey();
		long tl = (long)strlen(text);
		long i;
		if (kg>31 && kg<128)
		{	// Insert character at this position
			if (tl>0)
			{	for (i=tl; i>=cursorX && i>=0; i--)
					text[i+1]=text[i];
				text[cursorX++]=kg;
			}	else
			{	text[0]=kg;
				text[1]=0;
				cursorX=1;
			}
			if (handler) handler(this,label_changed);
			if (parent->callback) parent->callback(this,guicmd_edit);
			return label_changed;
		}
		if (kg==203 && cursorX>0) cursorX--;	// Left
		if (kg==205 && cursorX<tl) cursorX++;	// Right
		if (kg==199) cursorX=0;					// Home
		if (kg==207) cursorX=tl;				// End
		if (kg==211)
		{	// Delete Key
			if (cursorX<tl)
			{	for (i=cursorX; i<=tl; i++)
					text[i]=text[i+1];
				if (handler) handler(this,label_changed);
				if (parent->callback) parent->callback(this,guicmd_edit);
				return label_changed;
			}	else
			{	if (handler) handler(this,label_deleteRight);
				if (parent->callback) parent->callback(this,guicmd_edit);
			}
		}
		if (kg==8)
		{	// Backspace Key
			if (cursorX>0)
			{	cursorX--;
				for (i=cursorX; i<=tl; i++)
					text[i]=text[i+1];
				if (handler) handler(this,label_changed);
				if (parent->callback) parent->callback(this,guicmd_edit);
				return label_changed;
			}	else
			{	if (handler) handler(this,label_backspaceLeft);
				if (parent->callback) parent->callback(this,guicmd_edit);
			}
		}
		if (kg==13) // ENTER
		{	if (handler) handler(this,label_enter);
			if (parent->callback) parent->callback(this,guicmd_edit);
		}
		if (kg==27) // ESC
		{	strcpy(text,editbuffer);
			if (handler) handler(this,label_cancel);
			if (parent->callback) parent->callback(this,guicmd_edit);
		}
	}
	return label_noaction;
}

// **************************************************************************** >gui<
// ***																		***
// ***								Edit Box								***
// ***																		***
// ****************************************************************************
gEditBox::gEditBox(long _x,long _y, long _width, char *_text, void (*_handler)(gOverlay* editbox,dword command), gWindow *_parent)
{	overlaytype = overlayEditBox;
	strcpy(text, _text);
	x=_x;
	y=_y;
	width = _width;
	flags = label_justifyLeft;
	parent = _parent;
	getfont(&fontdata);
	height = fontdata.bm[0].height+6;
	handler = _handler;
	parent->addelement(this);
	facecolor = 0xffffffff;
}

void gEditBox::draw(void)
{	long _clipx1=clipx1,_clipx2=clipx2,_clipy1=clipy1,_clipy2=clipy2;
	x+=parent->CanvasX;
	y+=parent->CanvasY;

	setclipper(x,y,x+width,y+height);
	hline(x,y,width,color_darkoutline);
	hline(x+1,y+height,width,color_brightoutline);
	vline(x,y,height,color_darkoutline);
	vline(x+width,y+1,height-1,color_brightoutline);
	if (parent->flags & winFilled)
	{	box(x+1,y+1,width-1,height-1,facecolor); 
	}

	setfont(&fontdata);
	if (guiKBfocus==this)
		textoutc(x+3,y+4,text,cursorX);
	else
		textout(x+3,y+4,text);
	
	x-=parent->CanvasX;
	y-=parent->CanvasY;
	resetclipper();
	setclipper(_clipx1,_clipy1,_clipx2,_clipy2);
}

bool gEditBox::run(bool react)
{	long xpos = x + parent->CanvasX;
	long ypos = y + parent->CanvasY;
	bool mouseover = (mousex>xpos && mousey>ypos && mousex<xpos+width && mousey<ypos+height);

	if (react && mouseover)
	{	mouseicon = mouseicon_textedit;
		if (mousebut&1)
		{	
			strcpy(editbuffer,text);
			guiKBfocus = this;
			cursorX = 0;
			return true;
		}
	}

	dword editcmd = edittext();
	if (editcmd)
	{	if (handler)
			handler(this,editcmd);
	}

	return false;
}

// **************************************************************************** >gui<
// ***																		***
// ***									Window								***
// ***																		***
// ****************************************************************************
enum windrag
{	windrag_whitespace	= 0,
	windrag_title		= 1,
	windrag_forcedword = 0x7fffffff
};

gWindow::gWindow(char *_title,long _x,long _y, long _width, long _height, dword _flags, 
				 bool (*_callback)(gOverlay *object, dword command), gWindow *_parent)
{	// Basic initialization
	construction = true;
	flags = _flags;
	overlaytype = overlayWindow;
	callback = _callback;

	// Assign to Parent
	parent = _parent;
	if (parent)
		parent->addchild(this);
	else
	{	desktopwin = this;
		deletedesktop = false;
	}
	

	// Assign dimensions of window
	x = _x;
	y = _y;
	width = _width;
	height= _height;

	_width-=1;
	_height-=1;

	facecolor = 0xc0c0c0;
	// Define title bar
	title = new gLabel(_title,width>>1,4,label_justifyCenter,this);

	closebox = NULL;
	if (flags & winHasClose)
	{	closebox = new gBitmapButton(0,0, 10,9, &guicross, NULL,this);
	}

	// Define pulldown menu
	menu = NULL;
	if (flags & winHasMenu)
	{	menu = new gMenu(this);
	}
	
	computesizes();
	CanvasWidth = PortalWidth;
	CanvasHeight= PortalHeight;
	focuswin = this;

	// Finish constructing window
	construction = false;
}

gWindow::~gWindow(void)
{	// Remove from the parent's list
	if (parent) 
	{	parent->removechild(this);
	}
	else
	{	desktopwin = NULL;
	}
	// Delete the window's build-in elements
	if (closebox) delete closebox;
	if (title) delete title;

	// Delete everything hanging off this window
	while (child.size()>0) delete *child.begin();
	while (element.size()>0) delete *element.begin();
	if (focuswin==this) 
	{	if (parent) parent->setfocus();
		else		focuswin = NULL;
	}
	if (callback)
		callback(this,guicmd_delete);
}

void gWindow::addelement(gOverlay *newelement)
{	if (!construction)
	{	element.push_back(newelement);
	}
}

void gWindow::removeelement(gOverlay *whichelement)
{	for (gOverlay **j=element.begin(); j<element.end(); j++)
	{	if (*j==whichelement)
		{	element.erase(j);
			break;
		}
	}
}

void gWindow::addchild(gWindow *newchild)
{	child.push_back(newchild);
}

void gWindow::removechild(gWindow *whichchild)
{	for (gWindow **j=child.begin(); j<child.end(); j++)
	{	if (*j==whichchild)
		{	child.erase(j);
			break;
		}
	}
}

void gWindow::setfocus(void)
{	if (focuswin==this) return;
	if (focuswin)
		if (focuswin->callback) focuswin->callback(focuswin,guicmd_losefocus);
	if (callback) callback(this,guicmd_getfocus);
	guiKBfocus = this;
	focuswin = this;
}

void gWindow::bringforward(void)
{	// !!!RECURSIVE!!!
	long i;
	if (!parent) return;
	if (*parent->child.end()-1==this) return;

	for (i=parent->child.size()-2; i>=0; i--)
	{	if (parent->child[i]==this)
		{	parent->child[i]=parent->child[i+1];
			parent->child[i+1]=this;
		}
	}
	parent->bringforward();
}

void gWindow::computesizes(void)
{	// Computes the sizes of all window elements (frame, title bar, menu system, scroll bars, etc)
	
	// Window Title
	if (flags & winHasTitle)
	{	titlelines = 6+title->fontdata.bm->height;	// 13 = pixel height of font
		//msg("xxx",buildstr("%i",titlelines));
		if (titlelines<11) titlelines = 11;	// Must never have less than 11 title lines
	}	else
	{	titlelines = 0;
	}

	// Border Frame
	if (flags & winHasFrame)
	{	xofs = 1;
		yofs = titlelines+1;
	}	else
	{	xofs = 0;
		yofs = titlelines;
	}
	
	if (closebox)
	{	if (titlelines<19)
		{	closebox->x = width-12;
			closebox->y = 1+((titlelines-11)>>1);
			closebox->width = 10;
			closebox->height=  9;
		}	else
		{	closebox->x = width-14;
			closebox->y = ((titlelines-11)>>1);
			closebox->width = 12;
			closebox->height= 11;
		}
	}
	if (menu)
		yofs+=24;

	PortalWidth = width -xofs;
	PortalHeight= height-yofs;
}

void gWindow::draw(void)
{	// !!!RECURSIVE!!!

	// Store clipper settings
	long _clipx1=clipx1,_clipx2=clipx2,_clipy1=clipy1,_clipy2=clipy2;
	long i;

	if (parent)
	{	x += parent->CanvasX;
		y += parent->CanvasY;
	}

	CanvasX = x;
	CanvasY = y;

	// Draw outter frame
	if (flags & winHasFrame)
	{	hline(x      ,y       ,width   ,color_brightoutline);
		hline(x+1    ,y+height,width   ,color_darkoutline);
		vline(x      ,y+1     ,height  ,color_brightoutline);
		vline(x+width,y+1     ,height-1,color_darkoutline);
	}

	// Draw Title Bar
	if (flags & winHasTitle)
	{	float colinc = 228.0f / (float)titlelines;
		float curcol = 27;
		if (focuswin == this)
		{	// Draw focused window title bar
			for (dword i=1; i<titlelines; i++) 
			{	dword col = (dword)curcol;
				curcol+=colinc;
				hline(x+1,y+i,width-1,col);
			}
		}	else
		{	// Draw unfocused window title bar
			for (dword i=1; i<titlelines; i++) 
			{	dword col = (dword)(curcol*0.5f);
				dword drawcol = (col<<16)+(col<<8)+(col);
				curcol+=colinc;
				hline(x+1,y+i,width-1,drawcol);
			}
		}

		// Draw the seperator line between the title and the canvas / Menu
		hline(x+1,y+titlelines,width-1,color_brightoutline);

		title->draw();			// Display the window title (gLabel)
		if (closebox)			
		{	closebox->draw();	// Display the closebox (gBitmapButton)
		}
		if (menu)
		{	CanvasY+=titlelines+1;
			menu->draw();		// Display the TOP PORTION only of the pull-down menu
		}
	}

	// Work out the top left corner of the canvas
	CanvasX = x+xofs;
	CanvasY = y+yofs;

	// Set clipper to block out everything except the canvas (so we can't draw outside the window - unless we really want to)
	setclipper(CanvasX,CanvasY,CanvasX+PortalWidth,CanvasY+PortalHeight);

	if (flags & winFilled)
	{	// Fill the window with it's face color
		box(x+xofs,y+yofs,PortalWidth,PortalHeight,facecolor);
	}

	// Draw each element
	for (i=0; i<element.size(); i++)
	{	element[i]->draw();
	}

	// Draw each child window
	for (i=0; i<child.size(); i++)
	{	child[i]->draw();
	}

	// Reset the clipper back to what it was before we started drawing this window
	if (parent)
	{	x -= parent->CanvasX;
		y -= parent->CanvasY;
	}
	resetclipper();
	setclipper(_clipx1,_clipy1,_clipx2,_clipy2);
}

bool gWindow::getmouseover(void)
{	if (mouseover) return false;
	
	long i;
	long _clipx1=clipx1,_clipx2=clipx2,_clipy1=clipy1,_clipy2=clipy2;

	if (parent)
	{	x += parent->CanvasX;
		y += parent->CanvasY;
	}
	CanvasX = x;
	CanvasY = y;

	if (mousex>x && mousex>clipx1 && mousex<x+width  && mousex<clipx2 &&
		mousey>y && mousey>clipy1 && mousey<y+height && mousey<clipy2)
	{	// mouse is over this window ... now check children
		CanvasX = x+xofs;
		CanvasY = y+yofs;
		setclipper(CanvasX,CanvasY,x+width,y+height);
		// Process all child windows
		for (i=child.size()-1; i>=0; i--)
		{	if (child[i]->getmouseover()) break;
		}

		// Process all elements within the window (except children)
		if (!mouseover)
			for (i=0; i<element.size(); i++)
			{	if (element[i]->getmouseover()) break;
			}
		if (!mouseover) mouseover = this;
	}

	if (parent)
	{	x -= parent->CanvasX;
		y -= parent->CanvasY;
	}
	resetclipper();
	setclipper(_clipx1,_clipy1,_clipx2,_clipy2);
	if (mouseover) return true;
	return false;
}

bool gWindow::run(bool react)
{	// !!!RECURSIVE!!!

	// Use of the clipper in this function is to prevent reaction to mouseclicks outside the window
	long i;
	bool original_react = react;
	long _clipx1=clipx1,_clipx2=clipx2,_clipy1=clipy1,_clipy2=clipy2;

	if (parent)
	{	x += parent->CanvasX;
		y += parent->CanvasY;
	}
	CanvasX = x;
	CanvasY = y;
	
//	bool mouseover = mousex>x && mousex<x+width && mousey>y && mousey<y+height;
//	if (react && mouseover) react = true;
	if (mouseover==this && react) react = true;
		else react = false;

	setclipper(x,y,x+width,y+height);
//	if (mousex<clipx1 || mousex>clipx2 || mousey<clipy1 || mousey>clipy2) react = false;

	// Handle dragging the window
	if (dragging==this && flags & winMovable)
	{	switch(dragparam)
		{	case windrag_title:	
				x = mousex-dragofsX;
				y = mousey-dragofsY;
				break;
		}
	}

	// Process closebox
	if (closebox)
	{	if (closebox->run(react)) 
		{	react = false;
			if ((mousebut & 1)==0)
			{	// Close box has been clicked
				if (callback) 
				{	if (callback(this,guicmd_close))
					{	if (desktopwin==this) deletedesktop = true;
						else delete(this);
						return true;
					}
				}	else
				{	if (desktopwin==this) deletedesktop = true;
						else delete(this);
					return true;
				}
			}
		}
	}

	// Process title bar
	if ((flags & winHasTitle) && react && (mousebut & 1))
	{	if (mousex>x && mousex<x+width && mousey>y && mousey<y+titlelines)
		{	// User has clicked in title bar - start dragging window
			react = false;
			dragging = this;
			dragparam = windrag_title;
			dragofsX = mousex-x;
			dragofsY = mousey-y;
			setfocus();
		}
	}
	// Process pull down menu
	CanvasY+=titlelines+1;
	if (menu) 
	{	if (menu->run(react))
		{	react = false;
		}
	}
	CanvasX = x+xofs;
	CanvasY = y+yofs;
	// Process all child windows
	for (i=child.size()-1; i>=0; i--)
	{	if (child[i]->run(react))
			react = false;
	}

	// Process all elements within the window (except children)
	for (i=0; i<element.size(); i++)
	{	if (element[i]->run(react))
		{	react = false;
			focuswin = this; //setfocus();
		}
	}

	if (parent)
	{	x -= parent->CanvasX;
		y -= parent->CanvasY;
	}

	// Process clicks on the window itself
	if (original_react && react && mouseover && (mousebut & 1))
	{	// We've clicked inside the window, but nothing responded to it
		if (callback) callback(this,guicmd_click);
		setfocus();
		dragging = this;
		dragparam = windrag_whitespace;
		// From here we MUST return TRUE ... to get into this block, mouseover must be true.
	}

	resetclipper();
	setclipper(_clipx1,_clipy1,_clipx2,_clipy2);

	return (original_react && (original_react!=react));//false;
}

// **************************************************************************** >gui<
// ***																		***
// ***								Basic Button							***
// ***																		***
// ****************************************************************************
bool BasicButton_standardaccept(gBasicButton *receiver, dword _datatype, void *data)
{	return (_datatype == receiver->datatype);
}

void gBasicButton::init(long _x,long _y,long _width,long _height, void (*_handler)(gBasicButton *button),char *_tooltiptext,gWindow *_parent)
{	overlaytype = overlayButton;
	x = _x;
	y = _y;
	width = _width;
	height = _height;
	datatype = 0;
	handler = _handler;
	parent = _parent;
	facecolor = color_windowface;
	parent->addelement(this);
}

bool gBasicButton::run(bool react)
{	long tx = x+parent->CanvasX;
	long ty = y+parent->CanvasY;
	mouseover = mousex>tx && mousex<tx+width && mousey>ty && mousey<ty+height;

	if (datatype && dragging != this && dragging && mouseover && dragdatatype)
	{	if (accept_func(this,dragdatatype,dragdataptr))
		{	mouseappendage = appendage_button;
			if ((mousebut & 1) == 0)
			{	// dragdrop has just occured
				if (copy_func)
					copy_func(this,dragdatatype,dragdataptr);
				if (parent->callback) 
					parent->callback(this,guicmd_dragdropped);
			}
		}
	}

	if (dragging == this)
	{	if (mouseover)
		{	mouseicon = mouseicon_finger;
			mouseappendage = 0;
			pressed = true;
			if ((mousebut & 1) == 0)
			{	// The button has been clicked
				if (handler) handler(this);
				if (parent->callback) parent->callback(this,guicmd_click);
				return true;
			}
		}	else
		{	pressed = false;
		}
	}
	
	if (!react) return false;
	pressed = false;
	if (mouseover) 
	{	mouseicon = mouseicon_finger;
		//if (mouseover_fn) mouseover_fn(this);
		if (mousebut & 1) 
		{	pressed = true; 
			dragging = this;
			if (datatype)
			{	dragdatatype = datatype;
				dragdataptr	= dataptr;
			}
		} 
		//if (tooltiptext[0] && !pressed) tooltip = tooltiptext;
		return pressed;
	}
	return false;
}

void gBasicButton::enabledragdrop(dword _datatype, void *_dataptr, bool (*_accept_func)(gBasicButton *receiver, dword _datatype, void *data),void (*_copy_func)(gBasicButton *receiver, dword _datatype, void *data))
{	// datatype = user defined number used to help the user determine if the data being dragged can be accepted
	//            it must be non-zero (zero = object does not accept dragdrop)
	datatype = _datatype;
	dataptr = _dataptr;
	if (accept_func)
		accept_func = _accept_func;
	else
		accept_func = BasicButton_standardaccept;
	copy_func = _copy_func;
}

void gBasicButton::draw(void)
{	//if (this != parent->closebox) msg("xxx",buildstr("Parent x,y = %i, %i, button x,y = %i, %i",parent->xofs,parent->yofs,x,y));
	x+=parent->CanvasX;
	y+=parent->CanvasY;

	if (pressed)
	{	hline(x,y,width,color_darkoutline);
		hline(x+1,y+height,width,color_brightoutline);
		vline(x,y,height,color_darkoutline);
		vline(x+width,y+1,height-1,color_brightoutline);
	}	else
	{	hline(x,y,width,color_brightoutline);
		hline(x+1,y+height,width,color_darkoutline);
		vline(x,y,height,color_brightoutline);
		vline(x+width,y+1,height-1,color_darkoutline);
	}
	if (parent->flags & winFilled)
	{	box(x+1,y+1,width-1,height-1,facecolor); 
	}
	dword cx1 = clipx1;
	dword cx2 = clipx2;
	dword cy1 = clipy1;
	dword cy2 = clipy2;
	setclipper(x,y,(x+width),(y+height));
	customdraw();
	resetclipper();
	setclipper(cx1,cy1,cx2,cy2);

	x-=parent->CanvasX;
	y-=parent->CanvasY;
}

// **************************************************************************** >gui<
// ***																		***
// ***								Text Button								***
// ***																		***
// ****************************************************************************
gTextButton::gTextButton(long _x,long _y,long _width,long _height,char *_text, void (*_handler)(gBasicButton *button),gWindow *_parent)
{	buttontype = subtype_TextButton;
	init(_x,_y,_width,_height,_handler,"",_parent);
	changetext(_text);
}

void gTextButton::customdraw(void)
{	setfont(&fontdata);
	textout(x+textx,y+texty,text);
}

void gTextButton::changetext(void)
{	getfont(&fontdata);
	textx = (width - gettextwidth(text))>>1;
	texty = (height - fontdata.bm->height)>>1;
}

void gTextButton::changetext(char *_text)
{	strcpy(text,_text);
	changetext();
}

// **************************************************************************** >gui<
// ***																		***
// ***								Bitmap Button							***
// ***																		***
// ****************************************************************************
gBitmapButton::gBitmapButton(long _x,long _y,long _width,long _height,bitmap *_bitmap, void (*_handler)(gBasicButton *button),gWindow *_parent)
{	buttontype = subtype_BitmapButton;
	bm = _bitmap;
	init(_x,_y,_width,_height,_handler,"",_parent);
}

void gBitmapButton::customdraw(void)
{	dword xpos = x+((width-bm->width)>>1);
	dword ypos = y+((height-bm->height)>>1);
	if (pressed)
	{	if ((bm->flags & bitmap_bppmask) == bitmap_8bit) drawsprite(bm,xpos+1,ypos+1); else drawbitmap(bm,xpos+1,ypos+1);
	}
	else
	{	if ((bm->flags & bitmap_bppmask) == bitmap_8bit) drawsprite(bm,xpos,ypos);     else drawbitmap(bm,xpos,ypos);
	}
}

// **************************************************************************** >gui<
// ***																		***
// ***								Color Button							***
// ***																		***
// ****************************************************************************
gColorButton::gColorButton(long _x,long _y,long _width,long _height,dword _color, void (*_handler)(gBasicButton *button),gWindow *_parent)
{	buttontype = subtype_ColorButton;
	init(_x,_y,_width,_height,_handler,"",_parent);
	if ((_color & 0xff000000)==0) _color |= 0xff000000;
	facecolor = _color;
}

// **************************************************************************** >gui<
// ***																		***
// ***									Slider								***
// ***																		***
// ****************************************************************************
gSlider::gSlider(long _x,long _y,long _width,float _min,float _max, float _value, void (*_handler)(gSlider *slider),gWindow *_parent)
{	overlaytype = overlaySlider;
	x=_x;
	y=_y;
	width=_width;
	min=_min;
	max=_max;
	value = _value;
	handler = _handler;
	parent = _parent;
	scale = (float)width/(max-min);
	position = (value-min) * scale;
	facecolor = color_windowface;
	parent->addelement(this);
}

void gSlider::draw(void)
{	x+=parent->CanvasX;
	y+=parent->CanvasY;
	hline(x,y+6,width,0);
	vline(x,y+4,5,0);
	vline(x+width,y+4,5,0);
	
	// Draw the slider box
	vline(x+position-4,y,   14,color_brightoutline);
	hline(x+position-3,y,    7,color_brightoutline);
	hline(x+position-3,y+14, 8,color_darkoutline);
	vline(x+position+4,y+1, 13,color_darkoutline);
	//if (parent->flags & winFilled)
		box(x+position-3,y+1,7,12,facecolor);
	x-=parent->CanvasX;
	y-=parent->CanvasY;
}


// **************************************************************************** >gui<
// ***																		***
// ***								Menu System								***
// ***																		***
// ****************************************************************************
gMenu::gMenu(gWindow *_parent)
{	dword x,y;
	for (x=0; x<20; x++)
		for (y=0; y<40; y++)
		{	item[x][y].text = NULL;
			item[x][y].callback = NULL;
		}
	parent = _parent;
	
	select_column = -1;
	select_row = -1;
	expanded = false;
}

gMenu::~gMenu(void)
{
}

bool gMenu::run(bool react)
{	long x = parent->CanvasX;
	long y = parent->CanvasY;
	long column = 0;
	bool result = false;
	if (react)
	{	if (mousex>clipx1 && mousex<clipx2 && mousey>clipy1 && mousey<clipy2 && mousey>y && mousey<y+23)
			result = true;
		if (!expanded) select_column = -1;
		while (item[column][0].text)
		{	long mx = x+item[column][0].x;
			if (mousex>mx-5 && mousex<mx+item[column][0].width-8 && mousey>=y && mousey<=y+23)
				select_column = column;
			column++;
		}
	}
	return result;
}

void gMenu::draw(void)
{	long x = parent->CanvasX;
	long y = parent->CanvasY;
	box(x,y,parent->width,23,0xC8C8C8);
	hline(x,y+23,parent->width,color_darkoutline);
	setfont(systemfont);
	fontcolor(0x000000);
	setfontspacing(1);
	long column = 0;
	while (item[column][0].text)
	{	long mx = x+item[column][0].x;
		if (column==select_column)
		{	if (!expanded)
			{	// Highlight menu column, but don't expand it (mouse over)
				hline(mx-4,y+2,item[column][0].width-4,color_brightoutline);
				vline(mx-5,y+2,18,color_brightoutline);
				hline(mx-4,y+20,item[column][0].width-4,color_darkoutline);
				vline(mx+item[column][0].width-8,y+3,18,color_darkoutline);
				textoutu(mx,y+5,item[column][0].text);
			}	else
			{	// Highlight menu column, it is expanded, but don't draw the expansion here
				hline(mx-4,y+2,item[column][0].width-4,color_darkoutline);
				vline(mx-5,y+2,18,color_darkoutline);
				hline(mx-4,y+20,item[column][0].width-4,color_brightoutline);
				vline(mx+item[column][0].width-8,y+3,18,color_brightoutline);
				textoutu(mx+1,y+6,item[column][0].text);
			}
		}	else
		{	textoutu(mx,y+5,item[column][0].text);
		}
		column++;
	}
}

void gMenu::cleanup(void)
{	long column = 0;
	long textX = 10;
	setfont(systemfont);
	setfontspacing(1);
	while (item[column][0].text)
	{	item[column][0].x = textX;
		long w = gettextwidthu(item[column][0].text)+12;
		textX+=w;
		item[column][0].width = w;
		column++;
	}
}


// **************************************************************************** >gui<
// ***																		***
// ***								Misc Functions							***
// ***																		***
// ****************************************************************************
bool rungui2(void)
{	bool react;

	mouseicon = mouseicon_arrow;
	mouseover = NULL;

	if (!desktopwin) return false;

	gWindow *oldfocus = focuswin;
	
	if (dragging && dragdatatype)
		mouseappendage = appendage_cross;
	else
		mouseappendage = 0;

	if (dragging)
		react = false;
	else
		react = true;

	desktopwin->getmouseover();
	react = desktopwin->run(react);

	if (dragging && ((mousebut&1)==0))
	{	dragdatatype = 0;
		dragging = NULL;
	}

	if (focuswin!=oldfocus)
	{	if (focuswin)
			focuswin->bringforward();
	}
	if (deletedesktop)
	{	delete desktopwin;
		deletedesktop = false;
		desktopwin = NULL;
		resetclipper();
	}
	return !react;
}

void drawgui2(void)
{	if (desktopwin)
		desktopwin->draw();
}

/*
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
		default: drawsprite(&mousebitmap,mousex,mousey); break;
	}
//	drawtooltip();
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
*/
