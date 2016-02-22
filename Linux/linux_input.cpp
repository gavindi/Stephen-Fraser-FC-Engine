/************************************************************************************/
/*						   WINDOWS Input Control file.								*/
/*	    				   ---------------------------								*/
/*  					   (c) 2008 by Stephen Fraser								*/
/*																					*/
/* Contains the Windows-Specific code to handle user input events					*/
/*																					*/
/* Must provide the following Interfaces:											*/
/*	uintf keyconv[];	// An array for converting OS keycodes into FC keycodes		*/
/*	void setMouse(intf x,intf y);	// Position the mouse pointer on the screen		*/
/*	void showMouse(bool show);		// Show, or hide the mouse						*/
/*	uintf initJoysticks(void);		// Initialize Joystick handler, return count	*/
/*	void getJoystick(uintf index);	// Obtain joystick data for joystick[index]		*/
/*																					*/
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include <fcio.h>
#include "fc_linux.h"

// -----------------------------------------------------------------------------------------------
//												Keyboard Scancodes
// -----------------------------------------------------------------------------------------------
uintf	keyconv[] = {
0,				0,				0,				0,			0,				0,				0,			0,				0,				key_ESC,
key_1,			key_2,			key_3,			key_4,		key_5,			key_6,			key_7,		key_8,			key_9,			key_0,
key_MINUS,		key_EQUAL,		key_BACKSPACE,	key_TAB,	key_Q,			key_W,			key_E,		key_R,			key_T,			key_Y,
key_U,			key_I,			key_O,			key_P,		key_OPSQUARE,	key_CLSQUARE,	key_ENTER,	key_LEFTCTRL,	key_A,			key_S,
key_D,			key_F,			key_G,			key_H,		key_J,			key_K,			key_L,		key_SEMICOLON,	key_INVCOMMA,	key_TILDE,
key_LEFTSHIFT,	key_BACKSLASH,	key_Z,			key_X,		key_C,			key_V,			key_B,		key_N,			key_M,			key_COMMA,
key_PERIOD,		key_SLASH,		key_RIGHTSHIFT,	key_KPASTER,key_LEFTALT,	key_SPACE,		key_CAPS,	key_F1,			key_F2,			key_F3,
key_F4,			key_F5,			key_F6,			key_F7,		key_F8,			key_F9,			key_F10,	key_NUMLOCK,	key_SCROLL,		key_KP7,
key_KP8,		key_KP9,		key_KPMINUS,	key_KP4,	key_KP5,		key_KP6,		key_KPPLUS,	key_KP1,		key_KP2,		key_KP3,
key_KP0,		key_KPPERIOD,	0,				0,			0,				key_F11,		key_F12,	0,				0,				0,
0,				0,				0,				0,			key_KPENTER,	key_RIGHTCTRL,	key_KPSLASH,0,				key_RIGHTALT,	0,
key_HOME,		key_UP,			key_PAGEUP,		key_LEFT,	key_RIGHT,		key_END,		key_DOWN,	key_PAGEDOWN,	key_INSERT,		key_DELETE,
0,				0,				0,				0,			0,				0,				0,			key_PAUSE,		0,				0,
0,				0,				0,				0,			key_RIGHTWND,	key_MENU,		0,			0,				0,				0
};

uintf keyconvSize = sizeof(keyconv) / sizeof(uintf);

// -----------------------------------------------------------------------------------------------
//												Mouse Handler
// -----------------------------------------------------------------------------------------------
bool cursorShown = false;

void setMouse(intf x,intf y)
{	XWarpPointer(linuxVars.dpy, None, linuxVars.mainWin, 0,0,0,0, x, y);
}

void showMouse(bool show)
{
}

// -----------------------------------------------------------------------------------------------
//												Joystick Handler
// -----------------------------------------------------------------------------------------------
#define maxJoysticks 2
sJoystick joystickData[maxJoysticks];

float getjoypos(uintf data)
{	return 0.0f;
}

void getJoystick(uintf index)	// Retrieve the information (axis/rudder/button data) for joystick number 'index'.  This can take a lot of time, so only probe the Joystick when you need to.
{
}

uintf initJoysticks(void)
{	uintf numJoysticks = 0;
	joystick = joystickData;

	while (numJoysticks<maxJoysticks)
	{	joystick[numJoysticks].Connected = true;
		getJoystick(numJoysticks);
		if (!joystick[numJoysticks].Connected) return numJoysticks;
		else numJoysticks++;
	}
	return numJoysticks;
}

