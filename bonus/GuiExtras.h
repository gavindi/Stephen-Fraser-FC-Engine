/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
void initGuiExtras(void);	// Must be called prior to using the GuiExtras library
void killGuiExtras(void);	// Release all memory being used by the GuiExtras library

// ******************************************************************************************************************************
// guiColorPicker - Used to allow the user to visually select a color.
// --------------
// intf posx = X position on the parent surface where it is to appear (picker is 300 pixels wide)
// intf posy = Y position on the parent surface where it is to appear (picker is 380 pixels high)
// uint32 startRGB = the color to display when the color picker opens
// void (*callback)(uint32 RGB, void *userdata, bool changed) = the Callback function to call once complete
// void *userdata = Pointer to be passed to the callback function upon completion
// guiWindow *parent = The parent window / system upon which to draw the color picker
//
// Callback Function takes 3 parameters ...
// uint32 RGB = the final color being displayed in the color picker at the time it was closed
// void *userdata = the value passed to the color picker in the userdata parameter when it was created
// bool changed = A boolean value to indicate whether the color change should occur.  Will be true is the color picker was closed by the user pressing the OK button, or false if the user hit cancel or closed the window.
//
// Example of use:
// Let's say you have a button containing a color, and when someone clicks that button, you display the color picker, 
// and when a color is selected, the button changes to that particular color, and whatever that button represents (let's say
// background color in this case), is updated to show the new color as well.
//
// Step 1, create the button, set it's facecolor variable to the starting color, and set it's clickfunc to a new function.
// Step 2, within the clickfunc callback function, you should call the guiColorPicker, and pass the button's pointer as the
//         userdata.  You will also need a function to handle the colorpicker's callback
// Step 3, In the colorpicker callback, check the 'changed' variable.  If it's false, return from the callback, since we don't 
//         want to do anything.  Otherwise, the changed variable is true, so we need to do something...
//		   Take the userdata pointer, and type-cast it back to a guiButton pointer.  Next, set the button's facecolor to the
//		   specified RGB value.  Now check the pointer against all pointers for color buttons that use this callback function,
//		   and when you find a match (in this case, the background color), do whatever is required to set that state
//		   (backgroundrgb = RGB).  Finally, before returning from the colorpicker callback, call the button's parent's 
//		   'setchanged' function, so that the updated color in the button will become visible.
void newGuiColorPicker(intf posx, intf posy, uint32 startRGB, void (*callback)(uint32 RGB, void *userdata, bool changed), void *userdata, guiWindow *parent);

