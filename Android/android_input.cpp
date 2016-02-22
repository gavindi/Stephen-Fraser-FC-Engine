/************************************************************************************/
/*						   ANDROID Input Control file.								*/
/*	    				   ---------------------------								*/
/*  					   (c) 2013 by Stephen Fraser								*/
/*																					*/
/* Contains the Android-Specific code to handle user input events					*/
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

#include <keycodes.h>

#include <fcio.h>
#include "fc_win.h"

// -----------------------------------------------------------------------------------------------
//												Keyboard Scancodes
// -----------------------------------------------------------------------------------------------
uintf	keyconv[] = {	
//					0			1			2			3			4			5			6			7			8			9
//					--------------------------------------------------------------------------------------------------------------
					0,			0,			0,			0,			0,			0,			0,		key_0,		key_1,		key_2,	//   0 -   9
				key_3,		key_4,		key_5,		key_6,		key_7,		key_8,		key_9,			0,			0,	   key_UP,	//  10 -  19
			 key_DOWN,	 key_LEFT,	key_RIGHT,			0,			0,			0,			0,			0,			0,		key_A,	//  20 -  29
				key_B,		key_C,		key_D,		key_E,		key_F,		key_G,		key_H,		key_I,		key_J,		key_K,	//  30 -  39
				key_L,		key_M,		key_N,		key_O,		key_P,		key_Q,		key_R,		key_S,		key_T,		key_U,	//  40 -  49
				key_V,		key_W,		key_X,		key_Y,		key_Z,	key_COMMA,key_PERIOD, key_LEFTALT,key_RIGHTALT,key_LEFTSHIFT,//  50 -  59
	   key_RIGHTSHIFT,	  key_TAB,	key_SPACE,			0,			0,			0,			0,key_BACKSPACE,key_TILDE,	key_MINUS,	//  60 -  69
			key_EQUAL,key_OPSQUARE,key_CLSQUARE,key_BACKSLASH,key_SEMICOLON,key_INVCOMMA,key_SLASH,		0,			0,			0,	//  70 -  79
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	//  80 -  89
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	//  90 -  99
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 100 - 109
					0,			0,			0,key_LEFTCTRL,key_RIGHTCTRL,key_CAPS,			0,key_LEFTWND,key_RIGHTWND,			0,	// 110 - 119
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 120 - 129
					0,		key_F1,		key_F2,		key_F3,		key_F4,		key_F5,		key_F6,		key_F7,		key_F8,		key_F9,	// 130 - 139
				key_F10,	key_F11,	key_F12,		0,			0,			0,			0,			0,			0,			0,	// 140 - 149
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 150 - 159
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 160 - 169
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 170 - 179
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 180 - 189
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 190 - 199
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 200 - 209
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 210 - 219
					0,			0,			0,			0,			0,			0,			0,			0,			0,			0,	// 220 - 229

};

uintf keyconvSize = sizeof(keyconv) / sizeof(uintf);

// Notes:  Up/Down/Left/Right arrow keys map to the DPAD.  DPAD also supports AKEYCODE_DPAD_CENTER (index 23)

/*
AKEYCODE_UNKNOWN         = 0,
    AKEYCODE_SOFT_LEFT       = 1,
    AKEYCODE_SOFT_RIGHT      = 2,
    AKEYCODE_HOME            = 3,
    AKEYCODE_BACK            = 4,
    AKEYCODE_CALL            = 5,
    AKEYCODE_ENDCALL         = 6,
    AKEYCODE_STAR            = 17,
    AKEYCODE_POUND           = 18,
    AKEYCODE_DPAD_CENTER     = 23,
    AKEYCODE_VOLUME_UP       = 24,
    AKEYCODE_VOLUME_DOWN     = 25,
    AKEYCODE_POWER           = 26,
    AKEYCODE_CAMERA          = 27,
    AKEYCODE_CLEAR           = 28,
    AKEYCODE_SYM             = 63,
    AKEYCODE_EXPLORER        = 64,
    AKEYCODE_ENVELOPE        = 65,
    AKEYCODE_ENTER           = 66,
    AKEYCODE_AT              = 77,
    AKEYCODE_NUM             = 78,
    AKEYCODE_HEADSETHOOK     = 79,
    AKEYCODE_FOCUS           = 80,   // *Camera* focus
    AKEYCODE_PLUS            = 81,
    AKEYCODE_MENU            = 82,
    AKEYCODE_NOTIFICATION    = 83,
    AKEYCODE_SEARCH          = 84,
    AKEYCODE_MEDIA_PLAY_PAUSE= 85,
    AKEYCODE_MEDIA_STOP      = 86,
    AKEYCODE_MEDIA_NEXT      = 87,
    AKEYCODE_MEDIA_PREVIOUS  = 88,
    AKEYCODE_MEDIA_REWIND    = 89,
    AKEYCODE_MEDIA_FAST_FORWARD = 90,
    AKEYCODE_MUTE            = 91,
    AKEYCODE_PAGE_UP         = 92,
    AKEYCODE_PAGE_DOWN       = 93,
    AKEYCODE_PICTSYMBOLS     = 94,
    AKEYCODE_SWITCH_CHARSET  = 95,
    AKEYCODE_BUTTON_A        = 96,
    AKEYCODE_BUTTON_B        = 97,
    AKEYCODE_BUTTON_C        = 98,
    AKEYCODE_BUTTON_X        = 99,
    AKEYCODE_BUTTON_Y        = 100,
    AKEYCODE_BUTTON_Z        = 101,
    AKEYCODE_BUTTON_L1       = 102,
    AKEYCODE_BUTTON_R1       = 103,
    AKEYCODE_BUTTON_L2       = 104,
    AKEYCODE_BUTTON_R2       = 105,
    AKEYCODE_BUTTON_THUMBL   = 106,
    AKEYCODE_BUTTON_THUMBR   = 107,
    AKEYCODE_BUTTON_START    = 108,
    AKEYCODE_BUTTON_SELECT   = 109,
    AKEYCODE_BUTTON_MODE     = 110,
*/

uintf keyconvSize = sizeof(keyconv) / sizeof(uintf);

// -----------------------------------------------------------------------------------------------
//												Mouse Handler
// -----------------------------------------------------------------------------------------------
bool cursorShown = false;

void setMouse(intf x,intf y)
{	// Not possible as yet
}

void showMouse(bool show)
{	// Not possible as yet
}

// -----------------------------------------------------------------------------------------------
//												Joystick Handler
// -----------------------------------------------------------------------------------------------
#define maxJoysticks 1
sJoystick joystickData[maxJoysticks];

float getjoypos(uintf data)
{	// Create a deadzone 
	if (data>32767 && data<33500) data = 32767;
	if (data<32767 && data>32000) data = 32767;
	return ((float)(data)-32767)*oo32767;
}

void getJoystick(uintf index)	// Retrieve the information (axis/rudder/button data) for joystick number 'index'.  This can take a lot of time, so only probe the Joystick when you need to.
{	
}

uintf initJoysticks(void)
{	uintf numJoysticks = 0;
	joystick = joystickData;
	
	while (numJoysticks<maxJoysticks)
	{	joystick[numJoysticks].Connected = false;
		getJoystick(numJoysticks);
		if (!joystick[numJoysticks].Connected) return numJoysticks;
		else numJoysticks++;
	}
	return numJoysticks;
}

