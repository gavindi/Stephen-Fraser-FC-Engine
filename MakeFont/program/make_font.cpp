// *****************************************************************
// ***        MakeFont utility : (c) 1999 by Stephen Fraser      ***
// ***-----------------------------------------------------------***
// *** Part of the FC SDK distribution.							 ***
// ***															 ***
// *** Usage : Generate a PCX image with only the font graphics  ***
// *** in this order :											 ***
// ***															 ***
// *** `1234567890-=~!@#$%^&*()_+								 ***
// *** qwertyuiop[]QWERTYUIOP{}									 ***
// *** asdfghjkl;'ASDFGHJKL:"									 ***
// *** zxcvbnm,./\ZXCVBNM<>?|									 ***
// ***															 ***
// *** Notes : The last character (|) is a backslash (\) shifted ***
// ***		   There must be at least one blank horizontal line  ***
// ***            seperating the lines of text.					 ***
// ***         Should be at least 3 blank pixels to the right of ***
// ***            each character.								 ***
// ***		   Character height is calculated from the highest   ***
// ***			  lowest point on the first row of text.		 ***
// ***		   Be aware that some keyboards are layed out		 ***
// ***			  differently.  Be careful when typing.			 ***
// ***			  Especially with the backslash.				 ***
// ***		   The Quotes character (") must have at least a 1   ***
// ***			  pixel gap between the quotes.					 ***
// *** Output : <filename>.LDF contains the font bitmaps		 ***	
// *****************************************************************

#include <string.h>
#include "..\fcio.h"
#include "..\gui.h"

char *flname;

#define maxfontsize 1024*1024
#define numchars 94

byte zerobyte = 0;
char num[5]="00, ";
char eol[3]={"\r\n"};
char charorder[numchars] = 
	{ '`','1','2','3','4','5','6','7','8','9','0','-','=',
	  '~','!','@','#','$','%','^','&','*','(',')','_','+',
	  'q','w','e','r','t','y','u','i','o','p','[',']',
	  'Q','W','E','R','T','Y','U','I','O','P','{','}',
	  'a','s','d','f','g','h','j','k','l',';','\'',
	  'A','S','D','F','G','H','J','K','L',':','"',
	  'z','x','c','v','b','n','m',',','.','/','\\',
	  'Z','X','C','V','B','N','M','<','>','?','|' };

bitmap *fontbm,*sysfont;
long chrwidth[numchars],charconv[numchars];
long currchar=0;
byte *font,*fontptr;
word *fontptrw;
long x,y,linestart,linestop,rowstart,rowstop,height,scany;
word charheight,charwidth;

long scanline(long y)
{	// Returns 0 if line is blank, or non-zero if it contains graphics
	if (y>=fontbm->height) return 0;
	for (long x=0; x<fontbm->width; x++)
		if (fontbm->pixel[y*fontbm->width+x]>0) return 1;
	return 0;
}

long scanrow(long x)
{	// Returns 0 if line is blank, or non-zero if it contains graphics
	if (x>=fontbm->width) return 0;
	for (long y=linestart; y<=linestop; y++)
		if (fontbm->pixel[y*fontbm->width+x]>0) return 1;
	return 0;
}

void grabfont(void)
{	long count,thischar;

	// Load font bitmap file into memory
	flname = "d:\\Fixed16.pcx";//opendialog(openimagestring,NULL,0);
	fontbm = newbitmap(flname,0);
	
	font = fcalloc(maxfontsize,"font buffer");
	fontptr = font;
	
	for (count=0; count<94; count++)
	{	charconv[count]=0;
		chrwidth[count]=0;	
	}

	height = -1;
	scany=0;

	while (scany<fontbm->height)
	{	// Obtain top of character line
		while (scany<=fontbm->height && scanline(scany)==0) scany++;
		linestart=scany;

		if (scany>=fontbm->height) break;

		// Obtain bottom of character line
		while (scany<fontbm->height && scanline(scany)!=0) scany++;
		linestop=scany;

		if (height<0) height=linestop-linestart;

		// Center line if it's too small
		while (linestop-linestart<height)
		{	linestart--;
			if (linestop-linestart<height) 
				linestop++;
		}

		x=0;
		// Scan past first character
		while (x<fontbm->width && scanrow(x)==0) x++;
		while (x<fontbm->width && scanrow(x)!=0) x++;

		while (x<fontbm->width)
		{	// Obtain left edge of character
			while (x<fontbm->width && scanrow(x)==0) x++;
			rowstart=x;

			if (x<fontbm->width)
			{	// Obtain right edge of character
				if (charorder[currchar]=='"')
				{	while (x<fontbm->width && scanrow(x)!=0) x++;
					while (x<fontbm->width && scanrow(x)==0) x++;
				}
				while (x<fontbm->width && scanrow(x)!=0) x++;
				rowstop=x-1;
				lock3d();
				drawbitmap(fontbm,0,0);
				hline(0,linestart-1,640,0xffffff);
				hline(0,linestop+1,640,0xffffff);
				vline(rowstart-1,linestart,linestop-linestart+1,0xffffff);
				vline(rowstop+1,linestart,linestop-linestart+1,0xffffff);
				// Copy image into buffer
				charheight = (word)height;
				charwidth  = (rowstop - rowstart) + 1;
				thischar = charorder[currchar]-'!';
				chrwidth[thischar]=charwidth+1;
				charconv[thischar]=currchar;
						
				fontptrw = (word *)fontptr;
				fontptrw[0] = charwidth;
				fontptrw[1] = charheight;
				fontptr+=4;
				for (y=linestart; y<linestart+height; y++)
					for (x=rowstart; x<rowstart+charwidth; x++)
					{	*fontptr=fontbm->pixel[y*fontbm->width+x];
						if (*fontptr)
							pset(x-rowstart,y-linestart,0xffff);
						fontptr++;
					}
					
				unlock3d();
				update();
				//readkey();
				currchar++;
				if (currchar==numchars) scany=fontbm->height;
			}	// if x<fontbm->width
		}	// while x<fontbm->width
	}	// while scany<fontbm->height

//	consoleclearflags(conflag_stayonline);
	
	long ldfsize = fontptr-font;
//	consoleadd("LDF File Size = %l",ldfsize);
	filenameinfo(flname);
	FileCreate(buildstr("%s\\%s.ldf",filepath,fileactualname));
//	FileWrite(&zerobyte,1);
	FileWrite(font,ldfsize );
	FileClose();
//	consoleadd("LDF File Size = %l",filesize);

	cls();
//	consoleadd("Press ESC key to finish");
//
//	selectfont(1);
//	installfont(palette,buildstr("%s.ldf",flname),charconv,chrwidth,5);
//	textout(10,100,"`1234567890-= ~!@#$%^&*()_+");
//	textout(10,120,"qwertyuiop[] QWERTYUIOP{}");
//	textout(10,140,"asdfghjkl;'ASDFGHJKL:\"");
//	textout(10,160,"zxcvbnm,./\\ZXCVBNM<>?|");
//
//	textout(10,200,"`abcdefghijklmnopqrstuvwxyz'-=~!@#$%^&*()_+1234567890");
//	textout(10,220,"ABCDEFGHIJKLMNOPQRSTUVWXYZ,.<>/?\"\\[]{}:~;");
//	textout(10,240,"Testing (1234) ...");
	
//	textout(10,100,"`....56..90-. ~!@#$%^&*....");
//	textout(10,120,"............ Q....YU..P{}");
//	textout(10,140,"..........'......JK.:\"");
//	textout(10,160,".......,..\\..CV..M<>?|");
	
//	textout(10,190,"I went to the zoo the other day to play my xylaphone to the rhinoserous.");
//	textout(10,210,"Shouting in text looks like this ... HELLO WORLD");
//	textout(10,230,"I quickly asked my dog, fleebag to come here, and verely jog with me.");
//	textout(10,250,"FOG_TABLE[78]=DENSITY[12]/34+(X-Z);");
//	textout(10,270,"Remaining characters are : `5690-~!@#$%^&*QYUP{}'JK:\"\\CVM<>?|");
}

void fcmain(void)
{	initgraphics(3,640,480,0);

	grabfont();
//	while (!keydown[fckey_ESC])
//	{	getmouse();
//	}
	deletebitmap(sysfont);
}
