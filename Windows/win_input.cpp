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

#include <windows.h> 
#include <mmsystem.h>		// Needed for joystick support

#include <fcio.h>
#include "fc_win.h"

// -----------------------------------------------------------------------------------------------
//												Keyboard Scancodes
// -----------------------------------------------------------------------------------------------
uintf	keyconv[] = {	
					0,			0,			0,			0,			0,			0,			0,			0,	//   0 - 8
	    key_BACKSPACE,	  key_TAB,			0,			0,			0, key_RETURN,			0,	 key_EXIT, 	//   8 - 15
	    key_LEFTSHIFT,key_LEFTCTRL,key_LEFTALT,			0,	 key_CAPS,key_RIGHTSHIFT,key_RIGHTCTRL,key_RIGHTALT,//  16 - 23
					0,			0,			0,	  key_ESC,			0,			0,			0,			0,  //	24 - 31
		    key_SPACE, key_PAGEUP,key_PAGEDOWN,	  key_END,	 key_HOME,	 key_LEFT,	   key_UP,	key_RIGHT,	//	32 - 39
		     key_DOWN,			0,			0,			0,			0, key_INSERT, key_DELETE,		    0,	//	40 - 47
			    key_0,	    key_1,	    key_2,	    key_3,	    key_4,	    key_5,	    key_6,	    key_7, 	//  48 - 55
			    key_8,	    key_9,			0,			0,			0,			0,			0,			0, 	//  56 - 63
					0,      key_A,	    key_B,	    key_C,	    key_D,	    key_E,	    key_F,	    key_G,	//	64 - 71
			    key_H,      key_I,	    key_J,	    key_K,	    key_L,	    key_M,	    key_N,	    key_O,	//	72 - 79
			    key_P,      key_Q,	    key_R,	    key_S,	    key_T,	    key_U,	    key_V,	    key_W,	//	80 - 87
			    key_X,      key_Y,	    key_Z,			0,			0,			0,			0,			0,	//	88 - 95
			  key_KP0,	  key_KP1,	  key_KP2,	  key_KP3,	  key_KP4,	  key_KP5,	  key_KP6,	  key_KP7, 	//  96 - 103
			  key_KP8,	  key_KP9,key_KPASTER, key_KPPLUS,		    0,key_KPMINUS,key_KPPERIOD,key_KPSLASH,//104-111
			   key_F1,	   key_F2,	   key_F3,	   key_F4,	   key_F5,	   key_F6,	   key_F7,	   key_F8, 	// 112 - 119
			   key_F9,	  key_F10,	  key_F11,	  key_F12,			0,			0,			0,			0, 	// 120 - 127
					0,			0,			0,			0,			0,			0,			0,			0,	// 128 - 135
					0,			0,			0,			0,			0,			0,			0,			0,	// 136 - 143
					0, key_SCROLL,			0,			0,			0,			0,			0,			0,	// 144 - 151
					0,			0,			0,			0,			0,			0,			0,			0,	// 152 - 159
					0,			0,			0,			0,			0,			0,			0,			0,	// 160 - 167
					0,			0,			0,			0,			0,			0,			0,			0,	// 168 - 175
					0,			0,			0,			0,			0,			0,			0,			0,	// 176 - 183
					0,		  0,key_SEMICOLON,  key_EQUAL,  key_COMMA,  key_MINUS, key_PERIOD,  key_SLASH,	// 184 - 191
		    key_TILDE,			0,			0,			0,			0,			0,			0,			0,	// 192 - 199
					0,			0,			0,			0,			0,			0,			0,			0,	// 200 - 207
					0,			0,			0,			0,			0,			0,			0,			0,	// 208 - 215
					0,			0,			0,key_OPSQUARE,key_BACKSLASH,key_CLSQUARE,key_INVCOMMA,		0,	// 216 - 223
};

uintf keyconvSize = sizeof(keyconv) / sizeof(uintf);

// -----------------------------------------------------------------------------------------------
//												Mouse Handler
// -----------------------------------------------------------------------------------------------
bool cursorShown = false;

void setMouse(intf x,intf y)
{	if (winVars.factive!=WA_ACTIVE) return;
	if (winVars.isWindowMode)	
		mouse_event( MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 
				 ((x+winVars.winPosX)*65536+32768)/desktopMetrics->width, ((y+winVars.winPosY)*65536+32768)/desktopMetrics->height,
				 0, GetMessageExtraInfo() );
	else
		mouse_event( MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 
				 ((x)*65536+32768)/screenWidth, ((y)*65536+32768)/screenHeight,
				 0, GetMessageExtraInfo() );

}

void showMouse(bool show)
{	//if (show!=cursorshown)
	{	ShowCursor(show);
		cursorShown = show;
	}
}

// -----------------------------------------------------------------------------------------------
//												Joystick Handler
// -----------------------------------------------------------------------------------------------
#define maxJoysticks 2
float oo32767 = 1.0f / 32767.0f;
JOYINFOEX joystickinfo;
MMRESULT joystickresult;
dword jsIDlookup[maxJoysticks] = {JOYSTICKID1,JOYSTICKID2};
sJoystick joystickData[maxJoysticks];

float getjoypos(uintf data)
{	// Create a deadzone 
	if (data>32767 && data<33500) data = 32767;
	if (data<32767 && data>32000) data = 32767;
	return ((float)(data)-32767)*oo32767;
}

void getJoystick(uintf index)	// Retrieve the information (axis/rudder/button data) for joystick number 'index'.  This can take a lot of time, so only probe the Joystick when you need to.
{	if (joystick[index].Connected)
	{	if (index>=maxJoysticks) return;
		sJoystick *j = &joystickData[index];
		memfill(&joystickinfo,0,sizeof(JOYINFOEX));
		joystickinfo.dwSize = sizeof(JOYINFOEX);
		joystickinfo.dwFlags  = JOY_RETURNBUTTONS | JOY_RETURNPOVCTS | JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_USEDEADZONE;//JOY_RETURNALL;
		joystickresult = joyGetPosEx(jsIDlookup[index],&joystickinfo);
		if (joystickresult==JOYERR_NOERROR)
		{	j->Connected = true;
			j->X = getjoypos(joystickinfo.dwXpos);
			j->Y = getjoypos(joystickinfo.dwYpos);
			j->Z = getjoypos(joystickinfo.dwZpos);
			j->Rudder = getjoypos(joystickinfo.dwRpos);
			j->U = getjoypos(joystickinfo.dwUpos);
			j->V = getjoypos(joystickinfo.dwVpos);
			j->Buttons = joystickinfo.dwButtons;
			if (joystickinfo.dwPOV<=36000)
				j->POV = joystickinfo.dwPOV/10;
			else j->POV = -1;
		}	else
		j->Connected = false;
	}
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

