/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <fc_h.h>

EventTargetRef FCEventTarget;

void msg(char *title, char *thismessage)
{	printf ("%s\n--> %s", title, thismessage);
	ExitToShell();
}

void startupwarning(char *title, char *thismessage)
{	printf ("%s\n--> %s", title, thismessage);
}

void fclaunch(void);
void fcmain (void);
void videodriver_event (EventRecord *theEvent, Boolean *fProcessed);

// Timer Variables
AbsoluteTime fc_starttime;
intf		 fc_currenttimems;		// Number of milliseconds passed since last timer interrupt
intf		 fc_timerIntDuration;	// Number of milliseconds between timer interrupts (based on currenttimerHz)
extern uintf frametick;
extern void (*timertask)(void);
intf fcapppaused = 0;

char *cmdline = "";
char programflname[256];

intf winmousex, winmousey, winmousewheel;
dword winmousebut;

bool dumped = false;
void output_keys(void);

byte lastcustomkeys=0;

void checkmsg(void)
{	EventRecord theEvent;
	Boolean 	AlreadyProcessed;
	
	// Get custom keys that don't show up in keyup / keydown events
	KeyMap currentKeyboardState;
	GetKeys(currentKeyboardState);
	byte *keys = (byte *)&currentKeyboardState;
	byte currentcustomkeys = (keys[6] & 0x80 ) | (keys[7] & 0x0F);
	if (currentcustomkeys != lastcustomkeys)
	{	byte changedkeys = currentcustomkeys ^ lastcustomkeys;
		if (changedkeys & 0x01)
		{	// Left or Right shift keys
			if (currentcustomkeys & 0x01) 
			{	keyhit(80);
				keyhit(81);
			}	else
			{	keyrelease(80);
				keyrelease(81);
			}
		}
		if (changedkeys & 0x02)
		{	// Caps Lock
			if (currentcustomkeys & 0x02) 
			{	keyhit(82);
			}	else
			{	keyrelease(82);
			}
		}
		if (changedkeys & 0x04)
		{	// Alt (fckey_LEFTALT)
			if (currentcustomkeys & 0x04) 
			{	keyhit(83);
			}	else
			{	keyrelease(83);
			}
		}
		if (changedkeys & 0x08)
		{	// CTRL
			if (currentcustomkeys & 0x08) 
			{	keyhit(84);
			}	else
			{	keyrelease(84);
			}
		}
		if (changedkeys & 0x80)
		{	// Left/Right Command (fckey_RIGHTALT)
			if (currentcustomkeys & 0x80) 
			{	keyhit(85);
			}	else
			{	keyrelease(85);
			}
		}		
		lastcustomkeys = currentcustomkeys;
	}
	
	while (WaitNextEvent (everyEvent, &theEvent, 0, NULL))
	{	AlreadyProcessed = false;
		videodriver_event(&theEvent, &AlreadyProcessed);
//printf("Received Command %i (DSp = %i)\n",/*eventname[*/theEvent.what,AlreadyProcessed);
		if (!AlreadyProcessed)
		{	switch (theEvent.what)
			{
				case keyUp:
					keyrelease ((theEvent.message & keyCodeMask) >> 8);
					break;

				case keyDown:
				case autoKey:
					keyhit((theEvent.message & keyCodeMask) >> 8);
					break;
				case osEvt:
					if (theEvent.message & 0x01000000)	//	Suspend/resume event
					{	if (theEvent.message & 0x00000001)	//	Resume
						{	//gfFrontProcess = true;
						} else
						{	//gfFrontProcess = false;
						}
					}
					break;
					AEProcessAppleEvent (&theEvent);
					break;

				// Ignored Events
				case updateEvt:
				case diskEvt:
					break;
			}
		}
	}
	
	// Emulate timer ticks
	AbsoluteTime currtime = UpTime();
	fc_currenttimems -= AbsoluteDeltaToDuration(currtime, fc_starttime);
	fc_starttime = currtime;
	while (fc_currenttimems>=fc_timerIntDuration)
	{	fc_currenttimems-=fc_timerIntDuration;
		frametick++;
		timer--;
		globalTimer++;
		if (timertask && fcapppaused == 0) timertask();
	}	
}

void setTimerTask(void (*taskadr)(void), long ticks)
{   timertask = taskadr;
	currentTimerHz = ticks;
	fc_timerIntDuration = 1000000 / currentTimerHz;
	fc_currenttimems = 0;
}

void os_screenchanged(void)
{
}

void exitfc(void)
{	ExitToShell();
}

static pascal OSStatus appEvtHndlr (EventHandlerCallRef myHandler, EventRef event, void* userData)
{	EventMouseButton	button = 0;
	Point				mousedelta;
	
	UInt32 	kind = GetEventKind (event);
	switch(kind)
	{	case kEventMouseDown:
			GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &button);
			if (button>0) 
			{	winmousebut |= 1 << (button-1);
			}
			break;
		case kEventMouseUp:
			GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &button);
			if (button>0)
			{	winmousebut &= ~(1 << (button-1));
			}
			break;
		case kEventMouseMoved:
			GetEventParameter(event, kEventParamMouseDelta, typeQDPoint, NULL, sizeof(Point), NULL, &mousedelta);
			winmousex+=mousedelta.h;
			if (winmousex<0) winmousex=0;
			if (winmousex>=screenwidth) winmousex = screenwidth-1;
			winmousey+=mousedelta.v;
			if (winmousey<0) winmousex=0;
			if (winmousey>=screenheight) winmousex = screenheight-1;
			// paramtype = typeQDPoint kEventParamMouseDelta, kEventParamKeyModifiers
			break;
		case kEventMouseWheelMoved:
			break;
		default:
			//printf("appEvtHndlr (Not handled)\n");
			return eventNotHandledErr;
	}
    return noErr;
}

int main(void)
{	Point mousepos; 
	
	InitCursor();		// Make the mouse cursor an arrow
	HideCursor();		// Hide the cursor from view
	
	// Initialize Mouse
	GetMouse (&mousepos);
	winmousex = mousepos.h;
	winmousey = mousepos.v;
	winmousebut = 0;

//----------------------------------------------------
	EventHandlerUPP gEvtHandler;			// main event handler
    EventHandlerRef	ref;
    EventTypeSpec	list[] = { { kEventClassMouse, kEventMouseDown },// handle trackball functionality globaly because there is only a single user
							   { kEventClassMouse, kEventMouseUp }, 
							   { kEventClassMouse, kEventMouseMoved },
							   { kEventClassMouse, kEventMouseWheelMoved } };

	gEvtHandler = NewEventHandlerUPP(appEvtHndlr);
    InstallApplicationEventHandler (gEvtHandler, GetEventTypeCount (list), list, 0, &ref);
//----------------------------------------------------
	timertask = NULL;
	currentTimerHz = 100;
	fc_timerIntDuration = 1000000 / currentTimerHz;
	fc_starttime = UpTime();
	fc_currenttimems = 0;
//	timerFuncHandle = NewEventLoopTimerUPP(timeproc);
//	InstallEventLoopTimer(GetMainEventLoop(), 0, (kEventDurationSecond / 100.0f), timerFuncHandle, NULL, &timerHandle);

//	mkdir("logs");
//	if (!DeleteFile("logs\\exit.log"))
//	{	
//	}
	strcpy (programflname, "FC Engine");
	fclaunch();
	fcmain();

//	RemoveEventLoopTimer(timerHandle);
    QuitApplicationEventLoop();
	ExitToShell();
}

void setmouse (long x, long y)
{	winmousex = x;
	winmousey = y;
}

bool cursorshown = false;
void showmouse (bool show)
{	if (show!=cursorshown)
	{	cursorshown = show;
		if (show)
			InitCursor();
		else
			HideCursor();
	}
}

void stamptime(vlong *dest)
{	*dest = 1;
}

/*
void output_keys(void)
{	KeyMap currentKeyboardState;
	byte *keys = (byte *)&currentKeyboardState;
	GetKeys(currentKeyboardState);
	for (dword i=0; i<16; i++)
		printf("%i. %i\n",i,keys[i]);
}
*/
#endif
