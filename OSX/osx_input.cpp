/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <fcio.h>

// -----------------------------------------------------------------------------------------------
//												Keyboard Scancodes
// -----------------------------------------------------------------------------------------------
// iBook notes:
// Enter key (small key) is referred to as Right CTRL
// Left and Right Command keys are both mapped as the right ALT
// Caps lock is considdered down whenever the caps light is on
// Can't determine difference between Left and Right shift keys - both activate both shift commands 

uintf keyconv[] =
{	fckey_A,	fckey_S,	fckey_D,	fckey_F,	fckey_H,	fckey_G,	fckey_Z,	fckey_X,	//  0 -  7
	fckey_C,	fckey_V,	0,			fckey_B,	fckey_Q,	fckey_W,	fckey_E,	fckey_R,	//  8 - 15
	fckey_Y,	fckey_T,	fckey_1,	fckey_2,	fckey_3,	fckey_4,	fckey_6,	fckey_5,	// 16 - 23
	fckey_EQUAL,fckey_9,	fckey_7,	fckey_MINUS,fckey_8,	fckey_0,	fckey_CLSQUARE,fckey_O,	// 24 - 31
	fckey_U,	fckey_OPSQUARE,fckey_I,	fckey_P,	fckey_RETURN,fckey_L,	fckey_J,fckey_INVCOMMA,	// 32 - 39
	fckey_K,fckey_SEMICOLON,fckey_BACKSLASH,fckey_COMMA,fckey_SLASH,fckey_N,fckey_M,fckey_PERIOD,	// 40 - 47
	fckey_TAB,	fckey_SPACE,fckey_TILDE,fckey_BACKSPACE,fckey_RIGHTCTRL,fckey_ESC,  0,	0,			// 48 - 55
	0,			0,			0,			0,			0,			0,			0,			0,			// 56 - 63
	0,			0,			0,			0,			0,			0,			0,			0,			// 64 - 71
	0,			0,			0,			0,			0,			0,			0,			0,			// 72 - 79
	fckey_LEFTSHIFT,fckey_RIGHTSHIFT,fckey_CAPS,fckey_LEFTALT,fckey_LEFTCTRL,fckey_RIGHTALT,0,0,	// 80 - 87
	0,			0,			0,			0,			0,			0,			0,			0,			// 88 - 95
	fckey_F5,	fckey_F6,	fckey_F7,	fckey_F3,	fckey_F8,	fckey_F9,	0,			fckey_F11,	// 96 -103
	0,			0,			0,			0,			0,			fckey_F10,	0,			fckey_F12,	//104 -111
	0,			0,			0,			0,			0,			0,			fckey_F4,	0,			//112 -119
	fckey_F2,	0,			fckey_F1,	fckey_LEFT,	fckey_RIGHT,fckey_DOWN, fckey_UP,	0			//120 -127
};

void getJoystick(uintf index)	// Retrieve the information (axis/rudder/button data) for joystick number 'index'.  This can take a lot of time, so only probe the Joystick when you need to.
{	memset(
}
