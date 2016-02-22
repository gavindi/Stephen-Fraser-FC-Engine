MakeFont Utility for FC Engine
------------------------------

(C) 1999 - 2002 Stephen Fraser

Converts bitmap images containing fonts into LDF files for FC engine
The default file, systemfont.pcx shows the layout of the characters.

The bitmap file you load in MUST BE 8 BIT PALETIZED!

As long as all characters are present and in the specified order, things should work
The first character on each line is not encoded into the font, it is a height guage, to
prevent fonts from having elevated characters.  It should ideally be a straight line from
the topmost pixel to the bottom most pixel.

Some keyboards place the backslash and pipe ('\' and '|') key on the top row of the
keyboard ... this program assumes the pipe is located after the period key.

This program uses AI to determine the exact font data, however it can make mistakes.  Essentially,
all characters, with the exception of the double-quote " character must not have any horizontal
gaps in them.  Also, a row of characters must not have a blank horizontal line.  Both of these
situations will confuse the AI and result in erroneous font files being created.

Color number 0 will be transparent.
If only one other color is used, in palette slot 1, then the font will be created as a MONO font
which the engine can draw in any color.  If additional colors are used, you will need to manage 
the palette yourself.

Any font being used in the GUI must be no more than 12 pixels high.  Otherwise, the GUI may not work.

