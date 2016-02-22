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
/*------------------------------  W A R N I N G  -----------------------------------*/
/*  This source file is only for the use of Stephen Fraser.  Any other use without	*/
/* my express permission may result in legal action being taken!					*/
/************************************************************************************/

/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

// Note : For these loaders, I only provide feedback in either 8 bit paletised, or 32 bit ARGB
//		  formats, since converting from these 2 into 16 bit format is a trivial matter
//		  Where Alpha data is not present in the file, I set alpha to 0xff, but the bitmap_alpha
//		  flag is not set.  This is so the file can still be made into an alpha-enabled texture
//		  map if necessary
//		  In the comments where I list support, be aware that the file format may also support
//		  additional color formats, however the loader does not recognise them.  This is also true
//		  for compression methods, like in TGA, where Huffman compression is possible, however
//		  I currently have no documentation on their huffman encoding scheme.

#include "fc_h.h"

bitmap *readjpg(byte *,dword flags, dword datasize);		// Located in seperate LIB file
bitmap *readpng(byte *in, dword flags, dword inSize);

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
// Basic Info  : PCX uses a simplified form of Run Length Compression, when the 2 most
//				 significant bits of a byte are set, that is binary 11?????? (greater or
//				 equal 192) in the input stream, the following byte is repeated by the number
//				 of times indicated in the lower 6 bits of the first byte.  For true color
//				 images, this works best for dark pictures.  For photographs, or high quality
//				 3D renderrings, the compression achieved is almost always insignificant, and
//				 can often lead to substantial file expansion, instead of compression.
//				 Although rare, it is possible to see file expansion of 50 - 100%
// ----------------------------------------------------------------------------------------------

bitmap *readpcx(byte *pcx,dword flags, dword datasize)
{	long i;					// Standard iterator
	long imgaddr;			// Index into image array of the current pixel
	byte data,data2;		// Temporary variables used for reading color and run-length data
	dword bytesPerLine;		// bytes per line is the number of bytes per bit plane line required for decoding
	dword x,y;				// pixel location within the image being decoded
	byte numPlanes;			// Number of color planes, 256 color images have 1 plane, 24 bit has 3
	byte *image;			// Buffer which points to the memory where the image is stored
	byte *linebuffer;		// We need to decode to a temp buffer first because there may be some crap on the end of the line
	dword fileindex;		// Pointer to the index of the next PCX byte
	bitmap *dest = NULL;

	const char *PCXerrtxt = "PCX import error";

	// Validate file
	if (pcx[0]!=10)	msg(PCXerrtxt,"Missing PCX ID header");	// check PCX file
	if (pcx[1]<5)	msg(PCXerrtxt,"Only know Version 5");	// check for at least version 5
	if (pcx[2]!=1)	msg(PCXerrtxt,"Unknown Compression");	// check compression
	if (pcx[3]!=8)	msg(PCXerrtxt,"Bits per plane != 8");	// check bits per plane (8)

	// Calculate image dimensions and memory requirements
	numPlanes = pcx[65];
	dword width =(pcx[8]+(pcx[9]*256) - pcx[4]+(pcx[5]*256))+1;	  // 4:5 = xmin  8:9 = xmax
	dword height=(pcx[10]+(pcx[11]*256) - pcx[6]+(pcx[7]*256))+1; // 6:7 = ymin  10:11 = ymax
	bytesPerLine = pcx[66]+(pcx[67]*256);
	linebuffer=imageLoadingCell;//fcalloc(bytesPerLine,"PCX image (Garbage Collection Buffer)");
	fileindex = 128;

	// Decode 8 bit (256 color) paletised images
	if (numPlanes==1) 	// single color plane - 8 bit paletized
	{
		dest = newbitmap(CurrentBitmapName,width,height,flags | bitmap_RGB_8bit);
		image = dest->pixel;
		imgaddr=0;	// Set index to start of image
		for (y=0; y<height; y++)
		{	x = 0;
			while (x<bytesPerLine)
			{	data=pcx[fileindex++];			// Read in a color
				if (data>=192)					// If it's over 192, then it means "Repeat the next color <data-192> times"
				{	data2=pcx[fileindex++];		// ... So, read in the next color
		            for (i=1; i<=data-192; i++)
					{	linebuffer[x++]=data2;	// ... And write it <data-192> times
					}
				} else
				{	linebuffer[x++]=data;		// Otherwise, write it to the buffer
				}
			}
			// Now, copy the correct width from the linebuffer into the image
			memcopy(image+imgaddr,linebuffer,width);
			imgaddr+=dest->width;
		}

		// Now read the palette.  It should be indicated by a number 12.  If the next byte in the
		// stream is not 12, then the image has not been decoded correctly.  However, rather than
		// crashing out with an error message, I keep searching for that 12 byte.
		data=pcx[fileindex++];
		while (data!=12) data=pcx[fileindex++];

		// The next 768 bytes represent palete data in the following format ...
		// struct {byte Red, Green, Blue;} PaletteEntry[256];
		dest->palette = newPalette();
		dword *col = dest->palette->col32;
		for (i=0; i<256; i++)
		{	col[i]=	0xff000000 |
					((dword)(pcx[fileindex])<<16) |
					((dword)(pcx[fileindex+1])<<8) |
					((dword)(pcx[fileindex+2]));
			fileindex+=3;
		}
		//fcfree(linebuffer);		// Free up temporary memory
		return dest;
	}

	//	Decode 24 bit True color images
	if (numPlanes==3)
	{	dest = newbitmap(CurrentBitmapName,width,height,flags | bitmap_RGB_32bit);
		image = dest->pixel;
		dword *image32 = (dword *)image;	// Use this instead of complex type casting later
		intf imagesize = width*height;
		for (i=0; i<imagesize; i++)
			image32[i]=0xff000000;
	    imgaddr = 0;
		for (y=0; y<height;y++)
		{	for (long plane=0; plane<3; plane++)
			{	long planeroll = 8*(2-plane);
				x=0;
				while (x<width)
				{	data=pcx[fileindex++];
					if (data>=192)
					{	data2=pcx[fileindex++];
						for (i=1; i<=data-192; i++)
						{	linebuffer[x++]=data2;
						}
					} else
					{	linebuffer[x++]=data;
					}
				}
				// Now, copy the correct width from the linebuffer into the image
				for (x=0; x<width; x++)
					image32[imgaddr+x]|=(linebuffer[x]<<planeroll);
			} // for (plane)
			imgaddr+=width;
		} // for (y)
		dest->pixel = image;
		// fcfree(linebuffer);
		return dest;
	}

	// fcfree(linebuffer);
	msg("flname","Don't know how to process this PCX image");
	return NULL;
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

bitmap *readtga(byte *tga,dword flags, dword datasize)
{	long i;				// Standard iterator
	byte idlength;		// Length in bytes of the ImageID block
	//byte colmaptype;	// Indicates the type of colormap data used.  0 = None, 1 = standard color map
	byte imagetype;		// Image type for paletised/RGB, and compressed/uncompressed
	long palstart;		// The first entry in the palette table containing data
	long pallength;		// The number of defined entries in the color palette
	long palbits;		// Bits per color in the palette
	byte attrbits;		// Number of attribute bits per color (this is SOMETIMES alpha)
	byte orientation;	// Bit 0 true for RIGHT to LEFT,  bit 1 true for TOP to BOTTOM
	dword sofs,dofs;	// Source and Destinstion index values
	dword col = 0;		// The color to be written to the image buffer
	dword tcol;			// A temporary color used for 16->32 bit expansion
	dword run;			// Size of a packet remaining to be processed
	dword sadd=0;		// This value is added to the source index (sofs) after each color
	bool comp;			// specifies if the current packet is a comressed one
	byte imgbpp;		// Bits per pixel within the file
	dword x,y;			// co-ords within the image being processed
	bool compressed;	// Specifies if this file utilises compression
	dword *image32;		// Buffer to hold the 32 bit image \ These two buffers are mutually
	byte  *image8;		// Buffer to hold the  8 bit image / exclusive, only 1 is valid at a time
	bitmap *bm;
	dword	width;
	dword	height;

	idlength	= tga[0];
	//colmaptype	= tga[1];	// Never Used!
	imagetype	= tga[2];
	palstart	= tga[3]+tga[4]*256;
	pallength	= tga[5]+tga[6]*256;
	palbits		= tga[7];
	width		= tga[12]+tga[13]*256;
	height		= tga[14]+tga[15]*256;
	imgbpp		= tga[16];
	attrbits	= tga[17] & 0x0f;		 // Bits 0 to 3 contain the attribute bit size
	orientation = (tga[17] >> 4) & 0x03; // Bits 4 and 5 contain the orientation data

	// This code compensates for strange attribute data (which does happen) where we have 32 bits of COLOR and no alpha bits
	if (imgbpp == 32 && attrbits == 0)
	{	imgbpp = 24;
		attrbits = 8;
	}

	if (attrbits!=0 && attrbits!=8)
	{	msg("TGA File Error",buildstr("TGA File Importer: I don't know how to read %i bits of attribute data",imgbpp));
	}

	imgbpp+=attrbits;

	if (imgbpp== 8) flags |= bitmap_RGB_8bit;
	if (imgbpp==16) flags |= bitmap_RGB_32bit;
	if (imgbpp==24) flags |= bitmap_RGB_32bit;
	if (imgbpp==32) flags |= bitmap_RGB_32bit|bitmap_Alpha;
	if (imgbpp==40) flags |= bitmap_RGB_32bit|bitmap_Alpha;
	if ((flags&bitmap_DataInfoMask)==0)
		msg("TGA File Error",buildstr("TGA File Importer: I don't know how to read a %i bit TGA file",imgbpp));

	// Allocate the memory to store the decompressed image into
	if (imgbpp==8)
	{	bm = newbitmap(CurrentBitmapName,width,height,flags);
		bm->palette = newPalette();
		image8 = bm->pixel;
		image32 = NULL;
	}
	else
	{	bm = newbitmap(CurrentBitmapName,width,height,flags);
		image32 = (dword *)bm->pixel;
		image8 = NULL;
	}
	sofs = idlength+18;	// sofs now points to the palette data

	// Read in the palette
	if (pallength+palstart>256) msg("TGA File Error",buildstr("I can't handle a palette of more than 256 entries.\nIn File %s, %i palette entries were found",CurrentBitmapName,pallength+palstart));	// ### This line may be faulty
	for (i=palstart; i<pallength+palstart; i++)
	{	if (palbits==15 || palbits==16)				// 15 and 16 bit palettes are both xRGB 1555
		{	col = (tga[sofs]+tga[sofs+1]*256);		// Untested, but should be right
			bm->palette->col32[i] = 0xff000000 |
									((dword)(col & 0x7c00)<<8) |
									((dword)(col & 0x03e0)<<6) |
									((dword)(col & 0x001f)<<3);
			sofs+=2;
		}
		if (palbits==24)	// This one is dead simple ... byte for byte in reverse order
		{	bm->palette->col32[i] = 0xff000000 |((dword)(tga[sofs+2])<<16) |
												((dword)(tga[sofs+1])<<8) |
												((dword)(tga[sofs+0]));
			sofs+=3;
		}
		if (palbits==32)
		{	bm->palette->col32[i] = tga[sofs];		// Untested, but should be right
			sofs+=4;
		}
	}

	// Select starting position based on orientation
	dofs = 0;
	if (orientation == 0) dofs = bm->width*(bm->height-1);
	if (orientation == 1) dofs = (bm->width*bm->height)-1;
	if (orientation == 2) dofs = 0;
	if (orientation == 3) dofs = bm->width-1;

	//if (imgbpp==40) dest->height = 100;
	run = 0;
	comp=0;

	compressed = (imagetype==10 || imagetype==9);	// Is this file compressed?

	for (y=0; y<height; y++)			// For each row ...
	{	for (x=0; x<width; x++)			// For each column ...
		{	if (compressed && run==0)	// If compressed and we're about to start a new packet ...
			{	run = tga[sofs++];		// Read in the packet header
				if (run & 0x80)			// If this is a compressed packet ...
				{	run &= 0x7f;		// mask out the compression flag from the packet length
					comp=1;				// Set the compression flag
				} else
					comp=0;				// Clear the compression flag
				run++;					// A packet length is stored as 1 less than it should be, since a compression run of 0 is pointless
			}

			switch (imgbpp)
			{	case 40 : col = tga[sofs]+(tga[sofs+1]<<8)+(tga[sofs+2]<<16)+(tga[sofs+3]<<24);//*(dword *)(tga+sofs);	// Not yet working
						  if (col==0x00FFFFFF) col=0;
						  sadd=4;
						  break;
				case 32 : col = GetLittleEndianINT32(tga+sofs);	//tga[sofs]+(tga[sofs+1]<<8)+(tga[sofs+2]<<16)+(tga[sofs+3]<<24);//*(dword *)(tga+sofs);	// Plain DWORD copy of ARGB data
						  if (col==0x00FFFFFF) col=0;
						  sadd=4;
						  break;
				case 24 : // 24 bit data is stored as 1 byte for Blue, Green, Red, in that order
						  col = 0xff000000 + tga[sofs]+(tga[sofs+1]<<8)+(tga[sofs+2]<<16);
						  sadd=3;
						  break;
				case 16 : // 16 bit data is stored as .RRRRRGG GGGBBBBB
						  tcol = *(word *)(tga+sofs);
						  col = 0xff000000 +
							    ((tcol & 0x7c00)<<9) +
								((tcol & 0x03e0)<<6) +
								((tcol & 0x001f)<<3);
						  sadd=2;
						  break;
				case  8 : col = tga[sofs];
						  sadd=1;
						  break;
			}

			if (compressed)
			{	if (run!=0)
				{	if (comp==0)		// If the packet is not compressed ...
						sofs+=sadd;		// ... increase the source offset by the size of a pixel
					run--;				// Reduce the run counter
				}
				if (run==0 && comp==1)  // Compensate for compressed packets, once the run ...
					sofs+=sadd;			// reaches 0, increase the source offset
			} else
			sofs+=sadd;					// For uncompressed files, always increase source offset

			// Now write the color pixel to the image buffer
			if (imgbpp==8)
				image8[dofs]=(byte)col;
			else
				image32[dofs]=col;

			if (orientation & 1) dofs--; // If image goes from right to left, descrease dest index
			else dofs++;				 // otherwise, increase it like normal
		} // for each x in 0 -> dest->width

		// Now, we need to locate the start of the next row
		if (!(orientation & 2)) dofs-=bm->width*2;
		if (orientation & 1) dofs+=bm->width*2;
	}

	// clean up
	if (imgbpp==8)
		bm->pixel = image8;	// Write a pointer to the image into the bitmap structure
	else
		bm->pixel = (byte *)image32;
	return bm;
}

// ----------------------------------------------------------------------------------------------
//								#######  ####  #### ######
//								 ##   ## ## #### ## ##   ##
//								 ##   ## ##  ##  ## ##   ##
//								 ######  ##  ##  ## ######
//								 ##   ## ##      ## ##					(Incomplete)
//								 ##   ## ##      ## ##
//								#######  ##      ## ##
// ----------------------------------------------------------------------------------------------
// BMP Files, Designed by Microsoft and IBM for GUI based OS's on the Intel x86 based platforms
// Supports	   : 8 bit paletised, 24 bit RGB
// Compression : Uncompressed, Run Length
// Quirks	   : Image may be stored upside down (have not yet encountered a right-side up file
// Known Bugs  : Only implemented the uncompressed version for 8 bit images.
// Basic Info  : The RLE system used in 8 bit BMP files is a bit strange, and I have not yet
//				 deciphered it.  It does not appear that 24 bit images can support compression.
//				 Also, despite seeing Alpha data documented in BMP files in the DirectX SDK, I
//				 cannot seem to create a BMP with alpha data, nor can I find one with alpha data.
//				 I have also seen documentation describing 16 bit BMP files, but I have been
//				 unable to create one for testing purposes.
// ----------------------------------------------------------------------------------------------

bitmap *readbmp(byte *bmp,dword flags, dword datasize)
{	long i;									// i is the standard iterator
	dword x,y;
	dword color;
	byte  *image8;
	dword *image32;
	bitmap *bm;

//	uint16 identifier=GetLittleEndianUINT16(bmp+0x00);	// Should equal 0x424D for win3.x/9x/NT
	uint32 bmoffset	= GetLittleEndianUINT32(bmp+0x0a);	// Obtain the bitmap data offset from start of file
//	uint32 bmhdrsize= GetLittleEndianUINT32(bmp+0x0E);	// Obtain the bitmap header size
	uint32 width	= GetLittleEndianUINT32(bmp+0x12);	// Obtain image width
	uint32 height	= GetLittleEndianUINT32(bmp+0x16);	// Obtain image height (if negative, then image is top-down)
	uint16 planes	= GetLittleEndianUINT16(bmp+0x1A);	// Number of planes
	uint16 bpp		= GetLittleEndianUINT16(bmp+0x1C);	// Nunber of bits per pixel
	dword compression=GetLittleEndianUINT32(bmp+0x1E);	// Type of compression
	if (bpp==8 && compression==0)
	{	bm = newbitmap(CurrentBitmapName,width,height,flags | bitmap_RGB_8bit);
		image8=bm->pixel;
		// Load in palette
		for (i=0; i<256; i++)
		{	bm->palette->col32[i] = GetLittleEndianUINT32(bmp+0x36+(i<<2));
		}
		long sofs = 0x436;
		for (y=0;y<height;y++)
		{	while ((sofs-bmoffset) & 3) sofs++;	// 8 bit uncompressed must start on a dword boundary
			long dofs = ((height-y)-1)*width;
			for (x=0; x<width;x++)
			{	image8[dofs++]=bmp[sofs++];
			}
		}
		bm->flags=bitmap_RGB_8bit;
		bm->pixel=image8;
		return bm;
	}

	if (bpp==24 && compression==0)
	{	bm = newbitmap(CurrentBitmapName,width,height,flags | bitmap_RGB_32bit);
		image32=(dword *)bm->pixel;
		long sofs = bmoffset;
		for (y=0;y<height;y++)
		{	while ((sofs-bmoffset) & 3) sofs++;	// 32 bit uncomressed must start on a dword boundary
			long dofs = ((height-y)-1)*width;
			for (x=0; x<width;x++)
			{	color = 0xff000000 + (bmp[sofs+2]<<16) + (bmp[sofs+1]<<8) + (bmp[sofs+0]);
				image32[dofs++]=color;
				sofs+=3;
			}
		}
		bm->flags=bitmap_RGB_32bit;
		bm->pixel=(byte *)image32;
		return bm;
	}

	// If we reach here, we have not yet deciphered the BMP file, report an error
	msg("BMP Info",buildstr("%s (%i x %i) %i planes, %i bpp, compression method %i",CurrentBitmapName,width,height,planes,bpp,compression));
	return NULL;
}


// ----------------------------------------------------------------------------------------------
//									######   ######    #####
//									##   ##  ##   ##  ##   ##
//									##   ##  ##   ##  ##
//									##   ##  ##   ##   #####
//									##   ##  ##   ##       ##
//									##   ##  ##   ##  ##   ##
//									######   ######    #####
// ----------------------------------------------------------------------------------------------
// DirectDraw Surface, Designed by Microsoft, but typically using S3 Texture Compression formulas
// Supports	   : 32 bit ARGB, but you won't lose anything by converting to 16 bit
// Compression : 4x4 blocks of pixels, 2 x 16 bit base colors, 2 interpolated colors inbetween,
//				 then each pixel is represented by 2 bits
// Quirks	   :
// Known Bugs  :
// Basic Info  :

#ifndef MakeFourCC
    #define MakeFourCC(ch0, ch1, ch2, ch3)                              \
                ((dword)(byte)(ch0) | ((dword)(byte)(ch1) << 8) |       \
                ((dword)(byte)(ch2) << 16) | ((dword)(byte)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

struct sddsheader
{	uint32 dwSize;
	uint32 dwFlags;
	uint32 dwHeight;
	uint32 dwWidth;
	uint32 dwPitchOrLinearSize;
	uint32 dwDepth;
	uint32 dwMipMapCount;
	uint32 dwReserved1[11];
	uint32 pfSize;
	uint32 pfFlags;
	uint32 pfFourCC;
	uint32 pfRGBBitCount;
	uint32 pfRBitMask;
	uint32 pfGBitMask;
	uint32 pfBBitMask;
	uint32 pfRGBAlphaBitMask;
	uint32 dwCaps1;
	uint32 dwCaps2;
	uint32 dwCapsReserved[2];
	uint32 dwReserved2;
};

// This table is needed because we're not #including any D3D headers (may not be compiling in Windows)
enum {
    D3DFMT_G8R8_G8B8            = MakeFourCC('G', 'R', 'G', 'B'),
    D3DFMT_R8G8_B8G8            = MakeFourCC('R', 'G', 'B', 'G'),
    D3DFMT_UYVY                 = MakeFourCC('U', 'Y', 'V', 'Y'),
    D3DFMT_YUY2                 = MakeFourCC('Y', 'U', 'Y', '2'),
    D3DFMT_DXT1                 = MakeFourCC('D', 'X', 'T', '1'),
    D3DFMT_DXT2                 = MakeFourCC('D', 'X', 'T', '2'),
    D3DFMT_DXT3                 = MakeFourCC('D', 'X', 'T', '3'),
    D3DFMT_DXT4                 = MakeFourCC('D', 'X', 'T', '4'),
    D3DFMT_DXT5                 = MakeFourCC('D', 'X', 'T', '5'),
    D3DFMT_MULTI2_ARGB8         = MakeFourCC('M', 'E', 'T', '1'),
};

bitmap *readdds(byte *dds, dword flags, dword datasize)
{	bitmap reference, *bm;

	dds+=4;
	sddsheader ddsheader;
	uint32 *dest = (uint32 *)&ddsheader;
	for (uintf i=0; i<sizeof(sddsheader)>>2; i++)
	{	*dest++ = GetLittleEndianUINT32(dds);
		dds+=4;
	}
	uint32 width = ddsheader.dwWidth;
	uint32 height = ddsheader.dwHeight;
	if (ddsheader.dwMipMapCount>0) ddsheader.dwMipMapCount--;
	flags |= ((ddsheader.dwMipMapCount)<<bitmap_NumMipMapShift);
	if (ddsheader.pfFlags & 0x00000002)			// HARDCODED: DDPF_ALPHA (ddraw.h)
		flags |= bitmap_Alpha;
	if (ddsheader.pfFlags & 0x00000004)			// HARDCODED: DDPF_FOURCC (ddraw.h)
	{	switch(ddsheader.pfFourCC)
		{	case D3DFMT_DXT1:
			{	if (1) //flags & bitmap_KeepFormat)
				{	bm = newbitmap(CurrentBitmapName, width,height, flags | bitmap_DataTypeCompressed | bitmap_compDXT1);
					uintf bytesneeded = 0;
					for (uintf i=0; i<=ddsheader.dwMipMapCount; i++)
						bytesneeded += GetBitmapMipSize(bm, i);
					memcopy(bm->pixel,dds,bytesneeded);
				}	else
				{	reference.height = height;
					reference.width = width;
					reference.flags = flags | bitmap_DataTypeCompressed | bitmap_compDXT1;
					reference.pixel = dds;
					bm = SwizzleBitmap(&reference, bitmap_DataTypeRGB | bitmap_RGB_32bit | bitmap_Alpha);
				}
				break;
			}	// case D3DFMT_DXT1
			default:
			{	bm = newbitmap(CurrentBitmapName,4,4,flags);
			}	// Default
		}
	}	else
	{	bm = newbitmap(CurrentBitmapName,4,4,flags);
	}
	return bm;
}

/*
// ----------------------------------------------------------------------------------------------
//									 #####   ####  #######
//									##   ##   ##   ##
//									##        ##   ##
//									##  ###   ##   #####
//									##   ##   ##   ##
//									##   ##   ##   ##
//									 #####   ####  ##
// ----------------------------------------------------------------------------------------------
// GIF Files, Designed by Compuserve, and suffer from a nasty patent situation
// Supports	   : 8 bit paletised
// Compression : LZW Sliding Window
// Quirks	   : Protected by patents, not sure whether or not a license is required for use.
// Known Bugs  : not yet operational
// Basic Info  : A good understanding of BOOLEAN ALGEBRA is required to understand this loader.
//				 A GIF file can contain multiple images, however these images can all be of
//				 different sizes.  To handle this, the GIF file format specifies a 'window' size
//				 and then each image must exist entirely within that window.  It may begin at
//				 any position, but must not pass the boundary of that window.  The window may have
//				 a palette attatched to it (known as the GLOBAL COLOR TABLE), and then each image
//				 may optionally contain a palette (known as a LOCAL COLOR TABLE).  If an image
//				 does not contain a local color table, the global one is used.  If an image does
//				 not contain a local color table, and there is no global one, then the global
//				 palette from the previous GIF image loaded should be used, or a decoder-defined
//				 color table may be used.
//
//				 A "string table" of up to 4096 entries is maintained.  It has an initial state
//				 which may be restored at any time.
//
//				 Initial State:
//				 stringtable starts off containing [(2^bits per pixel)+2] entries, known as root
//				 entries.  These root entries contain no previous data to call upon, so the 'prev'
//				 pointer is NULL.  The first [2^bpp] entries contain their index value in 'next'.
//				 That is, stringtable[i].next = i.
//				 stringtable[2^bpp].next = Clear code, when this is detected, reset the
//										   stringtable to this initial state.
//				 stringtable[2^bpp+1].next = End of stream.
//				 read_size is set to [bpp+1] to begin with.
//
//				 Data Blocks:
//				 The start of the encoded stream is a single byte specifying the initial
//				 read_size.  This is then followed by a series of data blocks.  Each data block
//				 contains a byte value of the size of the block.  A size of 0 indicates the end
//				 of data blocks.  Then, [block_size] bytes of data from the input stream are
//				 decoded.
//
//				 Decode Loop:
//
// ----------------------------------------------------------------------------------------------

// These variables are used for the readbits macro
long ReadBitCounter;
byte bitpointer;			// Used to point to the next BIT in the input stream
byte bitmask[8]={0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

// The readbits macro reads 'numbits' bits from 'src' (bitpointer) and stores the result in 'dest'
#define readbits(dest,src,numbits) 										\
	dest = 0;															\
	for (ReadBitCounter=0; ReadBitCounter<numbits; ReadBitCounter++)	\
	{	byte bit = ((*src)&bitmask[bitpointer])>>bitpointer;			\
		bitpointer++;													\
		if (bitpointer==8)												\
		{	bitpointer=0;												\
			src++;														\
		}																\
		dest |= bit<<ReadBitCounter;									\
	}																	\
\	//consoleadd("%x: %x",numcodes++,dest);

void readgif(byte *gif,bitmap *dest, dword datasize)
{	struct stringentry
	{	stringentry *prev;		// Pointer to a string entry containing the rest of the string
		byte		 next;		// The next character to add to the string
	} stringtable[4096];		// There are a maximum of 4096 string entries

	long width,height;			// Dimensions of the GIF 'window' which all images must be contained within
	long i;						// Standard iterator
	dword compcode;				// Holds the current compression code (can be up to 0xfff for GIF)
	byte control;				// A set of flags to indicate how to process each window
	byte colres;				// Resolution (in bits) of the color data (doesn't appear to be needed though)
	byte bpp;					// Number of bits per image for the 'window'
	byte sortflag;				// Set to 1 to indicate if the palette is sorted in order of importance
	byte background;			// Index of color to use as the background color of the 'window'
	byte version = 0;			// The file format version number.  0 = undefined

	// Check GIF header details
	if (gif[0]!='G' || gif[1]!='I' || gif[2]!='F') msg("GIF File Error","Invalid header in file");
	if (gif[3]=='8' && gif[4]=='7' && gif[5]=='a') version = 1;
	if (gif[3]=='8' && gif[4]=='9' && gif[5]=='a') version = 2;
	if (version==0)
		msg("GIF File Error",buildstr("Unrecognised Version number '%c%c%c'",gif[3],gif[4],gif[5]));

	dest->width = gif[6]+(gif[7]*256);
	dest->height= gif[8]+(gif[9]*256);
	control = gif[10];	// MCCCSPPP : M = 1, Global color map follows Descriptor
						//			  C+1 = # bits of color resolution
						//			  S = 1, Palette is sorted in order of importance
						//			  P+1 = # bits/pixel in image
	colres	 = ((control & 0x70)>>4)+1;	// Calculate # of bits of color resolution (C)
	sortflag = ((control & 0x80)>>3);	// Calculate palette sorting flag
	bpp		 = (control & 0x07) + 1;	// Calculate # of bits per pixel in image  (P)

	// Background color comes next ...
	background = gif[11];

	// Allocate the image array and clear it with the background color
	dest->pixel= fcalloc(dest->width*dest->height+sizeof(Palette),"GIF Image");
	dest->palette = (Palette *)(dest->pixel+dest->width*dest->height);
	dest->palette->flags = 0;
	memfill(dest->pixel,background,dest->width*dest->height);

	// gif[12] represents the aspect ratio which we don't really care about ...
	//   if aspect>0, Aspect Ratio is (aspect+15):64
	//	 (ie : a Value of 1 = 16:64, or 1:4 ratio, ranging from 1:4 to 4:1).
	//   An aspect value of 0 (as appears in 87a format) indicates no aspect data is given.

	long maxcolors = 1<<bpp;			// Calculate maximum number of colors (bpp of 8 results in 256 colors : 2^8)
	if (maxcolors>256) maxcolors=256;	// Can't yet handle more than 256 colors (neither does GIF right now)

	// From this point on, data is not in fixed positions, so use a pointer to our current position
	byte *src = &gif[13];

	// Read in palette, which is fortunately in EXACTLY the same format as my bitmap structure
	if (control && 0x80)
	{	//for (i=0; i<maxcolors*3; i++)
		//{	dest->palette[i]=*src++;
		//}
		memcpy(dest->palette,src,maxcolors*3);
		src+=maxcolors*3;
	}	else
	{	// If no global color map is present, we should use some default palette
		// For a program engineered to read a sequence of GIFs, we would use the palette from the last GIF file
		byte *pal = (byte *)dest->palette->col16;
		pal[0]=0;	pal[1]=0;	pal[2]=0;	// First color should always be black
		pal[3]=255;	pal[4]=255;	pal[5]=255;	// Second color should always be white
		for (i=0; i<254; i++)				// Set remaining colors to grey-scale
		{	pal[i*3+6]=(byte)i;
			pal[i*3+7]=(byte)i;
			pal[i*3+8]=(byte)i;
		}
	}

	// Repeat this for each image
	struct animframe
	{	long x,y,width,height;
		byte bpp,*pixel;
	} frame;

	// Step 1 - Find image seperator 0x2c which is the ',' character.  Everything prior to it is to be ignored
	long skip = 0;
	while ((*src++)!=',') skip++;	// I'm maintaining a skip counter for debugging purposes

	// Step 2 - Obtain the x,y offsets and the width and height of this image / animation frame
	dword xofs = src[0]+(src[1]*256); src+=2;
	dword yofs = src[0]+(src[1]*256); src+=2;
	width	   = src[0]+(src[1]*256); src+=2;
	height	   = src[0]+(src[1]*256); src+=2;
	control	   = gif[*src++];	// MIS00PPP : M = 0: Use Global color map, 1: Use local color map
								//            I = 0: No Interlace, 1: Image is stored in interlace order
								//			  S = 1, Palette is sorted in order of importance
								//			  P+1 = # bits per pixel for this image
	// ### TODO : import local color map.  For now, just skip it if it exists
	if (control & 0x80) src += (1<<((control & 0x07)+1))*3;

	byte *pixel = frame.pixel = fcalloc(width*height,"GIF frame 0");

	long codesize = *src++;
	dword clearcode = (1<<codesize);	// Reset all compression tables
	dword endOfInfo = clearcode+1;		// Speficies the end of an information stream
	long  readsize=codesize+1;
	dword nextcomp,lastcomp,freecode;

	// For each data block
	long blocksize = *src++;			// Read in size of this data block
	byte *nextblock = src+blocksize;	// pointer to the next data block
	bitpointer = 0;						// Start reading bits from LSB to MSB

//console_outfile("GIF.TXT");
	// Read in first compression code (first ever compression code should always be the clear code)
	lastcomp = clearcode;
long numcodes=0;
	readbits(compcode,src,readsize);
	while (compcode!=endOfInfo)
	{	if (compcode==clearcode)
		{	// Special case : Clear the stringtable
			for (i=0; i<(1<<codesize); i++)
			{	stringtable[i].prev=NULL;
				stringtable[i].next=(byte)i;
			}
			nextcomp  = clearcode+2;	// The next available compression code
			readsize  = codesize+1;		// Number of bits to read for each compression code
			freecode  = codesize+2;		// Next free compression code slot
		} else
		{	// This is a standard compression code, write it to the output array
			if (compcode<freecode)
			{	// Write this compression code to the destination file
//				stringentry *codeptr = &stringtable[compcode];
//				while (codeptr)
//				{	*pixel++=codeptr->next;
//					codeptr=codeptr->prev;
//				}
			} else
			{	// This is an unknown code, obtain the next code and add it to the last stream
//				dword next;
//				readbits(next,src,readsize);
//				stringtable[compcode].prev=&stringtable[lastcomp];
//				stringtable[compcode].next=(byte)next;
//				freecode=compcode+1;
				// output this code to the dest buffer
//				stringentry *codeptr = &stringtable[compcode];
//				while (codeptr)
//				{	*pixel++=codeptr->next;
//					codeptr=codeptr->prev;
//				}
			}
		}
		lastcomp = compcode;
		readbits(compcode,src,readsize);
	}

	// Proceed to next data block
	src = nextblock;

	// Read in [codesize+1] bits from input stream to obtain a compression code
	// Compression codes < clearcode represent root numbers (ie : no compression)
	// When we output code [2**(compression size)-1)

//	msg("GIF stream",buildstr("codesize = %l, compcode = %x\n%x %x %x %x %x %x %x %x",codesize,compcode,src[0],src[1],src[2],src[3],src[4],src[5],src[6],src[7]));
//  0331 : FE 00 01 08 1C 48 B0  A0 C1 83 08 13 2A 5C C8
//		   1111 111e  0000 0000 0000 0001
//	msg("GIF is fine",
//		buildstr("Version %l : %l,%l\n%l bits if color resolution, %l bits per pixel,\nGlobal color map = %l, 0 bit = %l\nSkipped bytes = %l\nOffset %l,%l, dimensions %l,%l\nCodesize = %l",
//				  version,width,height,colres,bpp,(control>>7),(control & 0x08)>>3,skip,xofs,yofs,width,height,codesize));
//	for (i=0; i<height; i++)
//	{	memcpy(dest->pixel+xofs+(yofs+i)*dest->width,
//			   frame.pixel+i*width,
//			   width*height);
//	}
//	console_closefile();
}
*/

void initimageplugins(void)
{	if (!_fcplugin) msg("Engine Initialization Failure","InitImagePlugins Called before Plugins System is Initialized");
	addGenericPlugin((void *)readpcx, PluginType_ImageReader, ".pcx");		// Register the PCX loader
	addGenericPlugin((void *)readtga, PluginType_ImageReader, ".tga");		// Register the TGA loader
	addGenericPlugin((void *)readbmp, PluginType_ImageReader, ".bmp");		// Register the BMP loader
	addGenericPlugin((void *)readjpg, PluginType_ImageReader, ".jpg");		// Register the JPG loader
	addGenericPlugin((void *)readdds, PluginType_ImageReader, ".dds");		// Register the DDS loader
	addGenericPlugin((void *)readpng, PluginType_ImageReader, ".png");		// Register the PNG loader
//	addGenericPlugin((void *)readgif, PluginType_ImageReader, ".gif");		// Register the GIF loader
}

