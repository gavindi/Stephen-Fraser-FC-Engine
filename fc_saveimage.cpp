/************************************************************************************/
/*								2D Image Importers									*/
/*	    						------------------									*/
/*	 					(c) 1998-2002 by Stephen Fraser								*/
/*																					*/
/* Contains the functions for loading standard file formats into FC's BITMAP struct	*/
/*																					*/
/* Init Dependancies:	Plugins														*/
/* Init Function:		void initimageplugins(void)									*/
/* Term Function:		<none>														*/
/*																					*/
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#include "fc_h.h"

void savejpg(char *flname,bitmap *srcpic);					// Located in seperate LIB file
void savepng(char *flname,bitmap *srcpic);					// Located in seperate LIB file

extern char CurrentBitmapName[128];							// Filename of bitmap currently being imported

// ----------------------------------------------------------------------------------------------
//								######	  #####	 ##   ##
//								##   ##	 ##   ## ##   ##
//								##   ##	##        ## ##
//								######	##         ###
//								##      ##        ## ##
//								##       ##   ## ##   ##
//								##        #####  ##   ##
// ----------------------------------------------------------------------------------------------
// PCX Files, Designed by ZSoft for the PC Paintbrush program
// Supports	   : 8 bit paletised, 24 bit RGB
// Compression : Run Length
// Quirks	   : Number of encoded bytes per line is always even, regardless of width or BPP
// Known Bugs  : none
// ----------------------------------------------------------------------------------------------
void encodepcxpigment(dword width,byte *indata, byte *buffer, long *ofs)
{	dword x,i;
	x=0;
	while (x<width)
	{	if (indata[x]>=0xc0 || (x<(width+1) && indata[x]==indata[x+1]))
		{	i=1;
			while(indata[x+i]==indata[x] && (x+i)<width && i<0x3f)
			{	i++;
			}
			buffer[ofs[0]++] = 0xc0+(byte)i;
			buffer[ofs[0]++] = indata[x];
			x+=i;
		} else
		{	buffer[ofs[0]++] = indata[x];
			x++;
		}
	}
}

void savepcx8(char *filename, bitmap *bm)
{	uintf x,y;

	sFileHandle *handle = fileCreate(filename);
	byte pcxheader[128];
	uint16 *p;
	long bufsize = bm->width*7;
	tprintf((char *)pcxheader,sizeof(pcxheader),"Save Bitmap (8 bit): %s",filename);
	byte *buffer = fcalloc(bufsize,(char *)pcxheader);
	byte *ibuf = buffer+bm->width*6;

	for (long count=0; count<128; count++)
		pcxheader[count]=0;

	pcxheader[ 0]=10;			// Manufacturer
	pcxheader[ 1]= 5;			// Version Information
	pcxheader[ 2]= 1;			// Encoding (PCX)
	pcxheader[ 3]= 8;			// Bits per pixel per plane
	p = (uint16 *)(&pcxheader[4]);
	p[0] = 0;					// ( 4) Xmin
	p[1] = 0;					// ( 6) Ymin
	p[2] = (uint16)bm->width-1;	// ( 8) Xmax
	p[3] = (uint16)bm->height-1;// (10) Ymax
	p[4] = 47;					// (12) Horizontal DPI (800x600 on 17")
	p[5] = 35;					// (14) Horizontal DPI (800x600 on 17")
	// Bytes 16 -> 63			// colormap
	pcxheader[64]= 0;			// Reserved (should be 0)
	pcxheader[65]= 1;			// Number of color planes
	p = (word *)(&pcxheader[66]);
	p[0] = (word)bm->width;		// (66) Bytes to allocate for each scanline
	p[1] = 1;					// (68) How to interpret palette
	p[2] = (word)bm->width;		// (70) Screen Width
	p[3] = (word)bm->height;	// (72) Screen Height

	fileWrite(handle,pcxheader,128);

	byte *src8 = bm->pixel;
	for (y=0; y<bm->height; y++)
	{	for (x=0; x<bm->width; x++)
		{	ibuf[x] = *src8++;
		}
		long ofs=0;
		// Encode each pigment
		encodepcxpigment(bm->width,ibuf,buffer,&ofs);
		fileWrite(handle,buffer,ofs);
	}

	// Indicate that a palette follows
	buffer[0] = 12;
	fileWrite(handle, buffer, 1);

	// Output the palette
	for (x=0; x<256; x++)
	{	dword rgb = bm->palette->col32[x];
		byte r = (byte)((rgb & 0x00ff0000)>>16);
		byte g = (byte)((rgb & 0x0000ff00)>> 8);
		byte b = (byte)((rgb & 0x000000ff)    );
		fileWrite(handle, &r, 1);
		fileWrite(handle, &g, 1);
		fileWrite(handle, &b, 1);
	}
	fcfree(buffer);
	fileClose(handle);
}

void savepcx32(char *filename, bitmap *bm)
{	uintf x,y;
	dword col;

	sFileHandle *handle = fileCreate(filename);
	byte pcxheader[128];
	uint16 *p;
	long bufsize = bm->width*9;
	tprintf((char *)pcxheader,sizeof(pcxheader),"Save Bitmap (32 bit): %s",filename);
	byte *buffer = fcalloc(bufsize,(char *)pcxheader);
	byte *rbuf = buffer+bm->width*6;
	byte *gbuf = buffer+bm->width*7;
	byte *bbuf = buffer+bm->width*8;

	for (long count=0; count<128; count++)
		pcxheader[count]=0;

	pcxheader[ 0]=10;			// Manufacturer
	pcxheader[ 1]= 5;			// Version Information
	pcxheader[ 2]= 1;			// Encoding (PCX)
	pcxheader[ 3]= 8;			// Bits per pixel per plane
	p = (word *)(&pcxheader[4]);
	p[0] = 0;					// ( 4) Xmin
	p[1] = 0;					// ( 6) Ymin
	p[2] = (uint16)bm->width-1;	// ( 8) Xmax
	p[3] = (uint16)bm->height-1;// (10) Ymax
	p[4] = 47;					// (12) Horizontal DPI (800x600 on 17")
	p[5] = 35;					// (14) Horizontal DPI (800x600 on 17")
	// Bytes 16 -> 63			// colormap
	pcxheader[64]= 0;			// Reserved (should be 0)
	pcxheader[65]= 3;			// Number of color planes
	p = (word *)(&pcxheader[66]);
	p[0] = (word)bm->width;		// (66) Bytes to allocate for each scanline
	p[1] = 1;					// (68) How to interpret palette
	p[2] = (word)bm->width;		// (70) Screen Width
	p[3] = (word)bm->height;	// (72) Screen Height

	fileWrite(handle,pcxheader,128);

	dword *src32 = (dword *)bm->pixel;
	for (y=0; y<bm->height; y++)
	{	for (x=0; x<bm->width; x++)
		{	col = *src32++;
			bbuf[x] = (byte)((col & 0x0000ff));
			gbuf[x] = (byte)((col & 0x00ff00) >> 8);
			rbuf[x] = (byte)((col & 0xff0000) >> 16);
		}
		long ofs=0;
		// Encode each pigment
		encodepcxpigment(bm->width,rbuf,buffer,&ofs);
		encodepcxpigment(bm->width,gbuf,buffer,&ofs);
		encodepcxpigment(bm->width,bbuf,buffer,&ofs);

		fileWrite(handle,buffer,ofs);
	}
	fcfree(buffer);
	fileClose(handle);
}

void savepcx(char *filename, bitmap *bm)
{	if ((bm->flags & bitmap_DataInfoMask) == bitmap_RGB_8bit)
	{	// This is an 8 bit image
		savepcx8(filename,bm);
		return;
	}

	if ((bm->flags & bitmap_DataInfoMask) == bitmap_RGB_32bit)
	{	// This is a 32 bit image
		savepcx32(filename,bm);
		return;
	}
}

// ----------------------------------------------------------------------------------------------
//								########  #####	   ###
//								   ##    ##   ##  ## ##
//								   ##	##	     ##   ##
//								   ##	##  #### #######
//								   ##   ##    ## ##   ##
//								   ##    ##   ## ##   ##
//								   ##     #####  ##   ##
// ----------------------------------------------------------------------------------------------
// TGA Files, Designed by AT&T for a program, or service called "Truevision"
// Supports	   : 8 bit paletised, 16 bit xRGB 1555, 24 bit RGB, 32 bit ARGB
// Compression : Uncompressed, Run Length
// Quirks	   : Image may be stored upside down, or mirrored, or both.
// Known Bugs  : There is still some confusion over 32 bit images, since there is a 32 bit
//				 ARGB mode, and also a 32 bit RGB mode, as well as a 40 bit ARGB mode.
//				 Anything which has 32 bits allocated to RGB will end up being treated as
//				 32 bit ARGB, and 40 bit images just don't work.
// Basic Info  : TGA optionally uses packet-based Run Length Compression, there are two types
//				 of packets, compressed, and raw.  Both have a single byte header.  The most
//				 significant bit (1???????) is set to 1 for compressed, 0 for raw.  The lower
//				 7 bits contain the packet size (in pixels).  Compressed packets are followed
//				 by a single color entry (8, 16, 24, or 32 bits) which is repeated for <n>
//				 pixels where  n = (header & 0x7f)+1.  Raw packets are followed by <n> color
//				 entries which are copied on a one for one basis.
//				 The overhead of this compression system is slightly less than 1%, so even in
//				 a worst case scenario, file expansion is limited to 1% or less.  However
//				 under normal conditions, the compression savings can be quite impressive.
// ----------------------------------------------------------------------------------------------

struct TGAheader
{	uint8	idlength;			// Length in bytes of the ImageID block (Can be 0 when writing a TGA file)
	uint8	colmaptype;			// Indicates the type of colormap data used.  0 = None, 1 = standard color map
	uint8	imagetype;			// Greyscale / Paletised / RGB,  Compressed / Uncompressed
	uint8	palstartLo;			// Low order byte of the Palette start offset (that is, if you start at palette entry 27 instead of 0)
	uint8	palstartHi;			// Hi order byte of the Palette start offset
	uint8	pallengthLo;		// Low order byte of the Palette length (Total number of palette map entries)
	uint8	pallengthHi;		// Hi order byte of the Palette length
	uint8	palbits;			// Bit depth of the palette
	uint8	XstartLo;			// X co-ordinate of the lower left corner of the image
	uint8	XstartHi;			// X co-ordinate of the lower left corner of the image
	uint8	YstartLo;			// Y co-ordinate of the lower left corner of the image
	uint8	YstartHi;			// Y co-ordinate of the lower left corner of the image
	uint8	WidthLo;			// Width in pixels of the image
	uint8	WidthHi;			// Width in pixels of the image
	uint8	HeightLo;			// Height in pixels of the image
	uint8	HeightHi;			// Height in pixels of the image
	uint8	imgbpp;				// Number of bits per pixel in the image data
	uint8	attrbits;			// Image Attributes (Including Alpha channel size)
};

#define TGAheadersize 18		// Don't use sizeof in case the compiler has padded data onto the end of it.

void savetga(char *filename, bitmap *bm)
{	TGAheader	header;
	uintf		i,x,y;//,pixremain;
	uint8		*src8, *lastrawrun8;
	uint32		*src32, *lastrawrun32;
	bool		runfound;
	uint8		runlength;

	if ((bm->flags & bitmap_DataTypeMask)!=bitmap_DataTypeRGB)
	{	errorslog->log("Warning: Cannot save bitmap to TGA file - Bitmap is not of Data Type RGB");
		return;
	}

	memfill(&header, 0, sizeof(TGAheader));
	header.colmaptype = 1;						// TGA file has standard color map
	header.WidthHi  = (byte)((bm->width  & 0xff00)>>8);
	header.WidthLo  = (byte)((bm->width  & 0x00ff)   );
	header.HeightHi = (byte)((bm->height & 0xff00)>>8);
	header.HeightLo = (byte)((bm->height & 0x00ff)   );
	switch (bm->flags & bitmap_DataInfoMask)
	{	case	bitmap_RGB_8bit:
				header.imgbpp = 8;
				header.attrbits = 0x20;			// Top Left orientation, 0 bits for Alpha
				if (bm->palette)
				{	header.palbits = 24;
					header.imagetype = 9;		// Paletised Image (RLE)
					header.pallengthHi = 1;		// Palette is 256 bytes in size
				}	else
				{	header.imagetype = 11;		// Greyscale Image (RLE)
				}
				break;

		case	bitmap_RGB_16bit:
				errorslog->log("Warning: Cannot save bitmap to TGA file (not yet) - Bitmap is 16 bit");
				return;
				break;

		case 	bitmap_RGB_32bit:
				if (bm->flags & bitmap_Alpha)
				{	header.imgbpp = 32;
					header.attrbits = 0x28;		// Top Left orientation, 8 bits for Alpha
				}	else
				{	header.imgbpp = 24;
					header.attrbits = 0x29;		// Top Left orientation, 0 bits for Alpha
				}
				header.imagetype = 10;
				break;
	}

	sFileHandle *handle = fileCreate(filename);
	fileWrite(handle, &header, TGAheadersize);
	if (bm->palette)
	{	// Save out the palette
		for (i=0; i<256; i++)
		{	uint32 col = bm->palette->col32[i];
			byte r = (byte)((col & 0xff0000) >> 16);
			byte g = (byte)((col & 0x00ff00) >>  8);
			byte b = (byte)((col & 0x0000ff)      );
			fileWrite(handle, &b, 1);
			fileWrite(handle, &g, 1);
			fileWrite(handle, &r, 1);
		}
	}

	src8 = bm->pixel;
	src32 = (uint32 *)bm->pixel;

	runfound = false;

	for (y=0; y<bm->height; y++)
	{	// pixremain = bm->width;	// Never Used
		runlength = 0;
		x = 0;
		switch (bm->flags & bitmap_DataInfoMask)
		{	case	bitmap_RGB_8bit:
					lastrawrun8 = src8;
					while (x<bm->width)
					{	// Detect if this is a compression run?
						if ((x+2)<bm->width)
							runfound = (src8[0]==src8[1]) && (src8[0]==src8[2]);
						else
							runfound = false;

						// If it is a compressed run, or the raw run is now too long ...
						if (runfound || runlength==128)
						{	// If we have a raw run, write it out
							if (src8!=lastrawrun8)
							{	runlength--;
								fileWrite(handle,&runlength,1);
								while (src8!=lastrawrun8)
								{	fileWrite(handle,lastrawrun8,1);
									lastrawrun8++;
								}
							}

							// If we have a compressed run, write it out
							if (runfound)
							{	runlength=0;
								lastrawrun8 = src8;
								while ((*lastrawrun8==*src8)&&(runlength<128)&&(x<bm->width))
								{	src8++;
									runlength++;
									x++;
								}
								runlength--;
								runlength |= 128;
								fileWrite(handle,&runlength,1);
								fileWrite(handle,lastrawrun8,1);
							}

							// Now reset all counters for the next run
							runlength=0;
							lastrawrun8 = src8;
						}	else
						{	// No compressed run detected ... continue counting raw run
							runlength++;
							src8++;
							x++;
						}
					}
					// Save out any remaining bytes
					if (src8!=lastrawrun8)
					{	runlength--;
						fileWrite(handle,&runlength,1);
						while (src8!=lastrawrun8)
						{	fileWrite(handle,lastrawrun8,1);
							lastrawrun8++;
						}
					}
					break;

			case	bitmap_RGB_32bit:
					if (bm->flags & bitmap_Alpha)
					{	//		------------------------
						//		32 Bit ARGB (with Alpha)
						//		------------------------
						lastrawrun32 = src32;
						while (x<bm->width)
						{	// Detect if this is a compression run?
							if ((x+2)<bm->width)
								runfound = (src32[0]==src32[1]) && (src32[0]==src32[2]);
							else
								runfound = false;

							// If it is a compressed run, or the raw run is now too long ...
							if (runfound || runlength==128)
							{	// If we have a raw run, write it out
								if (src32!=lastrawrun32)
								{	runlength--;
									fileWrite(handle,&runlength,1);
									while (src32!=lastrawrun32)
									{
										uint32 color = *lastrawrun32;
										uint8  tmp;
										tmp = (uint8)((color & 0x000000ff)      );	// Blue
										fileWrite(handle,&tmp,1);
										tmp = (uint8)((color & 0x0000ff00) >>  8);	// Green
										fileWrite(handle,&tmp,1);
										tmp = (uint8)((color & 0x00ff0000) >> 16);	// Red
										fileWrite(handle,&tmp,1);
										tmp = (uint8)((color & 0xff000000) >> 24);	// Alpha
										fileWrite(handle,&tmp,1);
										lastrawrun32++;
									}
								}

								// If we have a compressed run, write it out
								if (runfound)
								{	runlength=0;
									lastrawrun32 = src32;
									while ((*lastrawrun32==*src32)&&(runlength<128)&&(x<bm->width))
									{	src32++;
										runlength++;
										x++;
									}
									runlength--;
									runlength |= 128;
									fileWrite(handle,&runlength,1);
									uint32 color = *lastrawrun32;
									uint8  tmp;
									tmp = (uint8)((color & 0x000000ff)      );	// Blue
									fileWrite(handle,&tmp,1);
									tmp = (uint8)((color & 0x0000ff00) >>  8);	// Green
									fileWrite(handle,&tmp,1);
									tmp = (uint8)((color & 0x00ff0000) >> 16);	// Red
									fileWrite(handle,&tmp,1);
									tmp = (uint8)((color & 0xff000000) >> 24);	// Alpha
									fileWrite(handle,&tmp,1);
								}

								// Now reset all counters for the next run
								runlength=0;
								lastrawrun32 = src32;
							}	else
							{	// No compressed run detected ... continue counting raw run
								runlength++;
								src32++;
								x++;
							}
						}
						// Save out any remaining bytes
						if (src32!=lastrawrun32)
						{	runlength--;
							fileWrite(handle,&runlength,1);
							while (src32!=lastrawrun32)
							{	uint32 color = *lastrawrun32;
								uint8  tmp;
								tmp = (uint8)((color & 0x000000ff)      );	// Blue
								fileWrite(handle,&tmp,1);
								tmp = (uint8)((color & 0x0000ff00) >>  8);	// Green
								fileWrite(handle,&tmp,1);
								tmp = (uint8)((color & 0x00ff0000) >> 16);	// Red
								fileWrite(handle,&tmp,1);
								tmp = (uint8)((color & 0xff000000) >> 24);	// Alpha
								fileWrite(handle,&tmp,1);
								lastrawrun32++;
							}
						}
					}	else
					{	//		------------------------
						//		24 Bit RGB (without Alpha)
						//		------------------------
						lastrawrun32 = src32;
						while (x<bm->width)
						{	// Detect if this is a compression run?
							if ((x+2)<bm->width)
								runfound = (src32[0]==src32[1]) && (src32[0]==src32[2]);
							else
								runfound = false;

							// If it is a compressed run, or the raw run is now too long ...
							if (runfound || runlength==128)
							{	// If we have a raw run, write it out
								if (src32!=lastrawrun32)
								{	runlength--;
									fileWrite(handle,&runlength,1);
									while (src32!=lastrawrun32)
									{
										uint32 color = *lastrawrun32;
										uint8  tmp;
										tmp = (uint8)((color & 0x000000ff)      );	// Blue
										fileWrite(handle,&tmp,1);
										tmp = (uint8)((color & 0x0000ff00) >>  8);	// Green
										fileWrite(handle,&tmp,1);
										tmp = (uint8)((color & 0x00ff0000) >> 16);	// Red
										fileWrite(handle,&tmp,1);
										lastrawrun32++;
									}
								}

								// If we have a compressed run, write it out
								if (runfound)
								{	runlength=0;
									lastrawrun32 = src32;
									while ((*lastrawrun32==*src32)&&(runlength<128)&&(x<bm->width))
									{	src32++;
										runlength++;
										x++;
									}
									runlength--;
									runlength |= 128;
									fileWrite(handle,&runlength,1);
									uint32 color = *lastrawrun32;
									uint8  tmp;
									tmp = (uint8)((color & 0x000000ff)      );	// Blue
									fileWrite(handle,&tmp,1);
									tmp = (uint8)((color & 0x0000ff00) >>  8);	// Green
									fileWrite(handle,&tmp,1);
									tmp = (uint8)((color & 0x00ff0000) >> 16);	// Red
									fileWrite(handle,&tmp,1);
								}

								// Now reset all counters for the next run
								runlength=0;
								lastrawrun32 = src32;
							}	else
							{	// No compressed run detected ... continue counting raw run
								runlength++;
								src32++;
								x++;
							}
						}
						// Save out any remaining bytes
						if (src32!=lastrawrun32)
						{	runlength--;
							fileWrite(handle,&runlength,1);
							while (src32!=lastrawrun32)
							{	uint32 color = *lastrawrun32;
								uint8  tmp;
								tmp = (uint8)((color & 0x000000ff)      );	// Blue
								fileWrite(handle,&tmp,1);
								tmp = (uint8)((color & 0x0000ff00) >>  8);	// Green
								fileWrite(handle,&tmp,1);
								tmp = (uint8)((color & 0x00ff0000) >> 16);	// Red
								fileWrite(handle,&tmp,1);
								lastrawrun32++;
							}
						}
					}
					break;
		}
	}
	fileClose(handle);
}

void initimagesaveplugins(void)
{	if (!_fcplugin) msg("Engine Initialization Failure","InitImageSavePlugins Called before Plugins System is Initialized");
	addGenericPlugin((void *)savepcx, PluginType_ImageWriter, ".pcx");		// Register the PCX writer
	addGenericPlugin((void *)savetga, PluginType_ImageWriter, ".tga");		// Register the TGA writer
	addGenericPlugin((void *)savejpg, PluginType_ImageWriter, ".jpg");		// Register the JPG writer
	addGenericPlugin((void *)savepng, PluginType_ImageWriter, ".png");		// Register the PNG writer (SLOW)
}
