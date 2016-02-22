/******************************************************************************/
/*  					     FC_WIN Input Handler.                            */
/*	    				     --------------------                             */
/*  					(c) 1998-2002 by Stephen Fraser                       */
/*                                                                            */
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#include "fc_h.h"

uintf	inputSecurityCode;
mInput	oemInput;

// ----------------------------------------------------------------------------
// ---								Keyboard Control						---
// ----------------------------------------------------------------------------
#define keyQsize	32

struct	oemKeyboardInternals
{	void	(*hotKeyPtr[512])(uintf);
	void	(*anyKey)(uintf);
	byte	keyQueue[keyQsize];
	byte	QPosPress, QPosRead;
} oemkb;

extern	uintf	keyconv[];
extern	uintf	keyconvSize;

unsigned char ascconv[186] = // Convertion from FC keycodes to ascii characters (it's almost a 1-1 conversion except for non-ascii keys (like SHIFT)
{	  0,  0,  0,  0,  0,  0,  0,  0,  8,  9,  0,  0,  0, 13,  0,  0,	//   0 -  15
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 27,  0,  0,  0,  0,	//  16 -  31
	' ',  0,  0,  0,  0,  0,  0,'\'', 0,  0,  0,  0,',','-','.','/',	//  32 -  47
	'0','1','2','3','4','5','6','7','8','9',  0,';',  0,'=',  0,  0,	//  48 -  63
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	//  64 -  79
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'[','\\',']', 0,  0,	//  80 -  95
	'`','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',	//  96 - 111
	'p','q','r','s','t','u','v','w','x','y','z',  0,  0,  0,  0,127,	// 112 - 127 (127 is DEL)

	128,129,130,131,132,133,134,135,136,137,138,139,140, 13,142,143,	// F1 - F12, Print, keypad(Enter), Scroll Lock, Insert
	144,145,  0,  0,148,149,  0,  0,  0,153,154,155,156,  0,  0,  0,	// Home, Page Up, [numlck], 147, End, Page Dn, [CAPS], [LeftShift], [RightShift], up, down, left, right, [leftCTRL], [rightCTRL], [leftALT]
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'*','+',172,'-','.','/',	// [RightALT], [Pause], [LeftWND], [RightWND], [Menu], 165-169, keypad(*), keypad(+), 172, keypad(-), keypad(.), keypad(/)
	'0','1','2','3','4','5','6','7','8','9'								// keypad keys
};

unsigned char s_ascconv[186] = // Convertion from FC keycodes to SHIFTED ascii characters
{	  0,  0,  0,  0,  0,  0,  0,  0,  8,147,  0,  0,  0,161,  0,  0,	//   0 -  15
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 27,  0,  0,  0,  0,	//  16 -  31
	' ',  0,  0,  0,  0,  0,  0,'"',  0,  0,  0,  0,'<','_','>','?',	//  32 -  47
	')','!','@','#','$','%','^','&','*','(',  0,':',  0,'+',  0,  0,	//  48 -  63
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,	//  64 -  79
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'{','|', '}', 0,  0,	//  80 -  95
	'~','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',	//  96 - 111
	'P','Q','R','S','T','U','V','W','X','Y','Z',  0,  0,  0,  0,127,	// 112 - 127 (127 is DEL)

	128,129,130,131,132,133,134,135,136,137,138,139,140, 13,142,162,	// F1 - F12, Print, keypad(Enter), Scroll Lock, Insert
	163,164,  0,  0,165,166,  0,  0,  0,186,187,188,189,  0,  0,  0,	// Home, Page Up, [numlck], 147, End, Page Dn, [CAPS], [LeftShift], [RightShift], up, down, left, right, [leftCTRL], [rightCTRL], [leftALT]
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'*','+',172,'-','.','/',	// [RightALT], [Pause], [LeftWND], [RightWND], [Menu], 165-169, keypad(*), keypad(+), 172, keypad(-), keypad(.), keypad(/)
	143,148,154,149,155,  0,156,144,153,145								// keypad keys
};

const char *keyname[186] =
{	"","","","","","","","",							//   0 -   7
	"Back Space", "Tab","","","","Enter","","",			//   8 -  15
	"","","","","","","","",							//  16 -  23
	"","","","Esc","","","","",							//  24 -  31
	"Space","","","","","","","'",						//  32 -  39
	"","","","",",","-",".","/",						//  40 -  47
	"0","1","2","3","4","5","6","7",					//  38 -  55
	"8","9","",";","=","","","",						//  56 -  63
	"","","","","","","","",							//  64 -  71	
	"","","","","","","","",							//  72 -  79
	"","","","","","","","",							//  80 -  87	
	"","","","[","\\","]","","",						//  88 -  95
	"`","A","B","C","D","E","F","G",					//  96 - 103
	"H","I","J","K","L","M","N","O",					// 104 - 111
	"P","Q","R","S","T","U","V","W",					// 112 - 119
	"X","Y","Z","","","","","",							// 120 - 127
	"F1","F2","F3","F4","F5","F6","F7","F8",			// 128 - 135
	"F9","F10","F11","F12","PrtScr","KeyPad Enter","Scroll Lock","Insert",	// 136 - 143
	"Home","Page Up","Num Lock","Delete","End","Page Down","Caps Lock","Left Shift", // 144 - 151
	"Right Shift","Up","Down","Left","Right","Left CTRL", "Right CTRL", "Left Alt",	// 152 - 159
	"Right Alt","Pause","","","","","","",				// 160 - 167
	"","","Keypad *","Keypad +","","Keypad -","Keypad .","Keypad /", // 168 - 175
	"Keypad 0","Keypad 1","Keypad 2","Keypad 3","Keypad 4","Keypad 5","Keypad 6","Keypad 7", // 176 - 183
	"Keypad 8","Keypad 9"								// 184 - 185
};

sKeyboard	oemKeyboard;

// Keyboard values of the currently selected keyboard
uint8	keyReady;
uint8	*keyDown;

// ------------------------------------ Publicly Visible: keyHit -----------------------------------------------
// This function is called internally when a keyboard key is pressed.  It can be called by the application to simulate a keypress
void keyHit(uintf keyCode)
{	unsigned char asckey;

    if (oemKeyboard.keyDown[keyCode]<255) oemKeyboard.keyDown[keyCode]++;
	if (oemkb.anyKey) oemkb.anyKey(keyCode);
    if (oemkb.hotKeyPtr[keyCode]) oemkb.hotKeyPtr[keyCode](keyCode);

	// Place key in keyboard Queue
	if (oemKeyboard.keyDown[key_LEFTSHIFT] || oemKeyboard.keyDown[key_RIGHTSHIFT])
	{	asckey = s_ascconv[keyCode];
	} else asckey = ascconv[keyCode];

	if (asckey>0)
	{	oemkb.keyQueue[oemkb.QPosPress++]=asckey;
		if (oemkb.QPosPress==keyQsize) oemkb.QPosPress=0;
		if (oemkb.QPosPress==oemkb.QPosRead) oemkb.QPosRead++;
		if (oemkb.QPosRead==keyQsize) oemkb.QPosRead=0;
		oemKeyboard.keyReady = 1;
		oemInput.keyReady = oemKeyboard.keyReady;
	}
}

// ------------------------------------ Internal Function : keyhit ---------------------------------------------
void keyhit(uintf oemKeyCode)
{	// This function accepts the OEM keycode of the key that was pressed, performs some validation, and passes it onto the public 'keyHit' function
	uintf keyCode;

	if (applicationState!=AppState_preGFX && applicationState!=AppState_running) return;	// Don't process anything while paused
	if (oemKeyCode >= keyconvSize) return;

    keyCode=keyconv[oemKeyCode];
	keyHit(keyCode);
}

// ------------------------------------ Publicly Visible: keyRelease -----------------------------------------------
// This function is called internally when a keyboard key is released.  It can be called by the application to simulate a keyrelease (recommended for use only for releasing simulated key presses)
void keyRelease(uintf keyCode)
{	oemKeyboard.keyDown[keyCode]=0;
	keyCode += key_released;
    if (oemkb.hotKeyPtr[keyCode]) oemkb.hotKeyPtr[keyCode](keyCode);
}

// ------------------------------------ Internal Function : keyrelease --------------------------------------
void keyrelease(uintf oemKeyCode)
{	// This function accepts the OEM keycode of the key that was released, performs some validation, and passes it onto the public 'keyRelease' function
	uintf keyCode;

	if (applicationState!=AppState_preGFX && applicationState!=AppState_running) return;	// Don't process anything while paused
	if (oemKeyCode >= keyconvSize) return;

    keyCode=keyconv[oemKeyCode];
	keyRelease(keyCode);
}
// ------------------------------------ API Function : readkey ---------------------------------------------
unsigned char readkey(void)
{	while (!oemKeyboard.keyReady) waitmsg();
	uint8 result = oemkb.keyQueue[oemkb.QPosRead++];
	if (oemkb.QPosRead==keyQsize) oemkb.QPosRead=0;
	if (oemkb.QPosRead==oemkb.QPosPress) oemKeyboard.keyReady=0;
	oemInput.keyReady = oemKeyboard.keyReady;
	return result;
}

void setKeyboardCallback(void callback(uintf))
{	oemkb.anyKey = callback;
}

void sethotkey(uintf scancode, void handleradr(uintf scancode))
{	oemkb.hotKeyPtr[scancode]=handleradr;
}

void selectKeyboard(uintf keyboardNum)
{	// This function is deliberately left blank for now
}





// ----------------------------------------------------------------------------
// ---								Mouse Control							---
// ----------------------------------------------------------------------------
void setMouse(intf x,intf y);
void showMouse(bool show);

sMouse		oemMouse;

// Mouse values from the device driver
uintf	OS_cacheMouseX,OS_cacheMouseY;
intf	OS_cacheMouseBut;				
intf	OS_cacheMouseWheel = 0;

// Mouse values of the currently selected mouse
intf	mouseX,mouseY;								// Mouse position in relation to the application's window, or in the case of a full-screen application, they represent screen co-ordinates.
uintf	mouseButtons;								// Current button status of the mouse.  Left mouse button is 1, right mouse button is 2, and the middle mouse button is 4.  These values are combined using boolean OR to create the value seen in the 'buttons' member variable
intf	mouseWheel;									// Current position of the vertical mouse wheel.  This is in 'steps'.  This value starts at 0 when the application begine.  Be aware that this value is not bounds-limited, and ranges from -billion to +billion

void getMouse(void)
{	mouseWheel   = oemMouse.wheel   = OS_cacheMouseWheel;
	mouseButtons = oemMouse.buttons = OS_cacheMouseBut;
	mouseX       = oemMouse.x       = OS_cacheMouseX;
	mouseY       = oemMouse.y       = OS_cacheMouseY;
}

void selectMouse(uintf mouseNum)
{	// This function is deliberately left blank for now
}





// ----------------------------------------------------------------------------
// ---								Joystick Control						---
// ----------------------------------------------------------------------------
sJoystick *joystick;





// ----------------------------------------------------------------------------
// ---								Input Module Control					---
// ----------------------------------------------------------------------------
void inputShutdown(uintf securityCode)
{	// This function is deliberately left blank (no actions necessary)
	// Only shutdown if securityCode==inputSecurityCode
}

mInput *initinput(uintf securityCode)
{	inputSecurityCode = securityCode;

	uintf i,j;
	
	// Initialize Mice
	OS_cacheMouseBut = 0;
	OS_cacheMouseWheel = 0;

	oemInput.numMice		= 1;
	oemInput.mouse			= &oemMouse;
	oemInput.getMouse		= getMouse;
	oemInput.setMouse		= setMouse;
	oemInput.showMouse		= showMouse;
	oemInput.selectMouse	= selectMouse;

	mouseX		 = oemMouse.x=0;
	mouseY		 = oemMouse.y=0;
	mouseButtons = oemMouse.buttons=0;
	mouseWheel	 = oemMouse.wheel=0;


	// Initialize Keyboards
	keyDown = &oemKeyboard.keyDown[0];

	oemInput.numKeyboards	= 1;
	oemInput.keyboard		= &oemKeyboard;
	oemInput.readKey		= readkey;
	oemInput.setHotKey		= sethotkey;
	oemInput.selectKeyboard	= selectKeyboard;
	oemInput.keyReady		= 0;

	for (i=0; i<oemInput.numKeyboards; i++)
	{	sKeyboard *k = &oemInput.keyboard[i];
		oemKeyboardInternals *ok = &oemkb;

		for (j=0; j<512; j++)
		{	k->keyDown[j]=0;
	    	ok->hotKeyPtr[j]=NULL;
		}
		ok->anyKey=NULL;
		ok->QPosPress=0;
		ok->QPosRead=0;
		k->keyReady=0;
	}

	oemInput.shutdown		= inputShutdown;
	return &oemInput;
}
