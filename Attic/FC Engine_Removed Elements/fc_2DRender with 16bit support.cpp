/********************************************************************************/
/*							 2D Render system for FC							*/
/*	    				-------------------------------							*/
/*						(c) 1998-2002 by Stephen Fraser							*/
/*																				*/
/* Contains the 2D Renderring routines (eg: primitives, bitmaps, fonts, etc	)	*/
/* Completely Generic, does not communicate with the hardware					*/
/*---------------------------  W A R N I N G  ----------------------------------*/
/*  This file is only for the use of Stephen Fraser.  Any other use without		*/
/* my express permission may result in legal action being taken!				*/
/********************************************************************************/

#include <stdarg.h>
#include "fc_h.h"
#include <bucketSystem.h>

const char *viddrv_error = "2D Video Driver Error";

// Palette and color variables
uint32	*paldata32;						// Pointer to current palette 32 bit entries
uint16	*paldata16;						// Pointer to current palette 16 bit entries (RGB565 format)

float			oo255 = 1.0f/255.0f;

//*****************************************************************************
//***								RenderTarget 2D							***
//*****************************************************************************
sRenderTarget2D	rt2d_ScreenDisplay;				// Render Target for the screen display
sRenderTarget2D	*current2DRenderTarget;
intf			rt2d_width,rt2d_height;			// Width and height of the 2D render target (in pixels)
uintf			rt2d_bpp;						// Bit depth of the 2D render target
bitmap			*rt2d_bitmap;					// The current 2D rendertarget
uintf			*ylookup;						// Lookup table to point to scanlines within the screen display
void			*lfb;							// Linear Frame Buffer of the current render target
intf			lfbpitch;						// Increment in bytes between scanlines on the display
intf			clipx1,clipx2,clipy1,clipy2;	// The current 2D clipping window

void select2DRenderTarget(sRenderTarget2D *rt)
{	if (!rt) rt = &rt2d_ScreenDisplay;
	current2DRenderTarget = rt;
	rt2d_width  = rt->width;
	rt2d_height = rt->height;
	rt2d_bpp    = rt->depth;
	clipx1		= rt->clipX1;
	clipx2		= rt->clipX2;
	clipy1		= rt->clipY1;
	clipy2		= rt->clipY2;
	rt2d_bitmap	= rt->bm;
	if (rt->bm)
	{	lfb		= rt->bm->pixel;
		ylookup	= (uintf *)rt->bm->renderinfo;
		lfbpitch= ylookup[1];
	}
	else
	{	lfb		= NULL;
		ylookup	= NULL;
		lfbpitch= 0;
	}
}

void create2DRenderTarget(sRenderTarget2D *rt, bitmap *bm)
{	rt->depth = 0;
	if ((bm->flags & bitmap_DataTypeMask) == bitmap_DataTypeRGB)
	{	uintf flags = bm->flags & bitmap_DataInfoMask;
		if (flags == bitmap_RGB_16bit)
			rt->depth = 16;
		if (flags == bitmap_RGB_32bit)
			rt->depth = 32;
	}
	if (rt->depth==0)
		msg("Illegal 2D Render Target","2D Render Targets must be either 16 bit or 32 bit RGB");

	rt->bm = bm;
	rt->width = bm->width;
	rt->height = bm->height;
	rt->clipX1 = 0;
	rt->clipX2 = bm->width-1;
	rt->clipY1 = 0;
	rt->clipY2 = bm->height-1;
}

//*****************************************************************************
//***									Clipper								***
//*****************************************************************************
void clipperSet(uintf x1, uintf y1, uintf x2, uintf y2)
{	if (x1>=(uintf)rt2d_width) x1 = rt2d_width-1;
	if (x2>=(uintf)rt2d_width) x2 = rt2d_width-1;
	if (y1>=(uintf)rt2d_height) y1 = rt2d_height-1;
	if (y2>=(uintf)rt2d_height) y2 = rt2d_height-1;
	if (x1>x2) x1=x2;
	current2DRenderTarget->clipX1 = clipx1 = x1;
	current2DRenderTarget->clipX2 = clipx2 = x2;
	if (y1>y2) y1=y2;
	current2DRenderTarget->clipY1 = clipy1 = y1;
	current2DRenderTarget->clipY2 = clipy2 = y2;
}

void clipperSetMax(void)
{	current2DRenderTarget->clipX1 = clipx1 = 0;
	current2DRenderTarget->clipY1 = clipy1 = 0;
	current2DRenderTarget->clipX2 = clipx2 = rt2d_width-1;
	current2DRenderTarget->clipY2 = clipy2 = rt2d_height-1;
}

void clipperShrink(uintf x1, uintf y1, uintf x2, uintf y2)
{	if (x1>(uintf)clipx1) clipx1 = x1;
	if (x2<(uintf)clipx2) clipx2 = x2;
	if (clipx1>clipx2) clipx1=clipx2;
	if (y1>(uintf)clipy1) clipy1 = y1;
	if (y2<(uintf)clipy2) clipy2 = y2;
	if (clipy1>clipy2) clipy1=clipy2;
	current2DRenderTarget->clipX1 = clipx1;
	current2DRenderTarget->clipX2 = clipx2;
	current2DRenderTarget->clipY1 = clipy1;
	current2DRenderTarget->clipY2 = clipy2;
}

// --------------------------- cls2D API Command ------------------------------
void cls2D(uint32 color)
{	box(0,0,rt2d_width, rt2d_height, color);
}

//*****************************************************************************
//***								2D Primitives							***
//*****************************************************************************
void pset(intf x, intf y, uint32 col)
{	if (x<clipx1 || x>clipx2 || y<clipy1 || y>clipy2) return;

	if (rt2d_bpp==16)
	{	uint16 *writeaddr = (uint16 *)(((byte *)lfb)+ylookup[y]+x+x);
		*writeaddr = (uint16)(rgbto565(col));
		return;
	}
	if (rt2d_bpp==32)
	{	dword *writeaddr = (dword *)(((byte *)lfb)+ylookup[y]+(x<<2));
		*writeaddr = col;
	}
}

void box(intf x, intf y, intf ow, intf h, uint32 col)
{	// box : using 16 bit color

	if (x>clipx2 || y>clipy2) return;
	if (x<clipx1)
	{	ow+=(x-clipx1);
		x=clipx1;
	}

	if (y<clipy1)
	{	h+=(y-clipy1);
		y=clipy1;
	}
	if (x+ow>clipx2) ow=(clipx2-x)+1;
	if (y+h>clipy2) h=(clipy2-y)+1;

	if (rt2d_bpp==16)
	{	/* DO SOMETHING FOR 16 BIT */
		dword col16 = rgbto565(col);
		dword col32 = (col16<<16)+col16;

		if (ow<=0 || h<0) return;
		if (x & 1)
		{	vline(x,y,h,col);
			x++;
			ow--;
			if (ow<=0) return;
		}

		if (ow & 1)
		{	vline(x+ow-1,y,h,col);
			ow--;
			if (!ow) return;
		}

		uint16 *data16 = (uint16 *)((byte *)lfb+ylookup[y]+x+x);
		ow>>=1;	// We'll write 2 pixels at a time
		for (intf yy=y; yy<y+h; yy++)
		{	uint32 *data32 = (uint32 *)data16;
			for (intf i=0; i<ow; i++)
			{	*data32++=col32;
			}
			data16 = (uint16 *)((byte *)data16 + lfbpitch);
		}
	}

	if (rt2d_bpp==32)
	{	byte *dest8 = ((byte *)lfb)+ylookup[y]+(x<<2);
		for (intf yy=0; yy<h; yy++)
		{	uint32 *dest = (uint32 *)dest8;
			for (intf xx=0; xx<ow; xx++)
				*dest++=col;
			dest8+=lfbpitch;
		}
	}
}

// --------------------------- hline API Command -------------------------------
void hline(intf x, intf y, intf width, uint32 color)
{	if (x>clipx2 || y<clipy1 || y>clipy2) return;

	if (x<clipx1)
    {	width-=(clipx1-x);
    	x=clipx1;
    }
    if ((x+width)>(clipx2+1))
    {	width=(clipx2+1)-x;
    }
    if (width<1) return;

	if (rt2d_bpp==16)
	{	/* DO SOMETHING FOR 16 BIT */
		uint32 col16 = (uint32)rgbto565(color);
		uint16 *data16 = (uint16 *)((byte *)lfb+ylookup[y]+x+x);
		uint32 col32 = (col16<<16)+col16;

		// Write out a any non-aligned pixels at the beginning
		if (((intptr)data16 & 3)!=0)	// We needed to typecast the pointer to a dword for alignment check
		{	*data16++=(uint16)col16;
			width--;
		}

		// Find out if there is an odd number of pixels remaining
		uintf widthlsb = width & 1;

		// Write out all the aligned pixels as 32 bit dwords (>2x speed of writing single pixels)
		width>>=1;
		uint32 *data32 = (uint32 *)data16;
		for (intf i=0; i<width; i++)
		{	*data32++=col32;
		}

		// If necessary, write out the final pixel at the end
		if (widthlsb)
		{	data16 = (uint16 *)data32;
			*data16 = (uint16)col16;
		}
	}
	if (rt2d_bpp==32)
	{	uint32 *dest = (uint32*)((byte *)lfb+ylookup[y]+(x<<2));
		for (intf i=0; i<width; i++)
			*dest++=color;
	}
}

// --------------------------- vline API Command -------------------------------
void vline(intf x, intf y, intf height, uint32 color)
{	if (y<clipy1)
    {	height-=(clipy1-y);
    	y=clipy1;
    }
    if (y+height>(clipy2+1))
    {	height=(clipy2+1)-y;
    }
    if (height<1 || y>clipy2 || x<clipx1 || x>clipx2) return;

	if (rt2d_bpp==16)
	{	/* DO SOMETHING FOR 16 BIT */
		dword col = (dword)rgbto565(color);
		byte *data16 = (byte *)lfb+ylookup[y]+x+x;
		for (intf i=0; i<height; i++)
		{	*(uint16 *)data16=(uint16)col;
			data16+=lfbpitch;
		}
	}
	if (rt2d_bpp==32)
	{	byte *dest = (byte *)lfb+ylookup[y]+(x<<2);
		for (intf i=0; i<height; i++)
		{	*(dword*)dest=color;
			dest+=lfbpitch;
		}
	}
}

void drawline16(intf x1,intf y1,intf x2,intf y2,uint32 col32)	// Co-ords must be clipped
{	intf error1, error2,d;
	uint16 col = (uint16)rgbto565(col32);
	intf yofs,yadd;

	uint16 *lfb16 = (uint16 *)lfb;

	if (x2<x1)	// For optimization later - handle all octants (remove this IF statement)
	{	// Swap points around
		intf tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	// Write first pixel
	yofs = ylookup[y1]>>1;
	yadd = lfbpitch>>1;
	lfb16[x1+yofs] = col;

	intf deltax = x2-x1;
	intf deltay = y2-y1;
	if (deltax == 0 && deltay == 0) return;

	//if (deltax>=0)	// For optimization later - handle all octants
	{	// could be in octant 2,3,4, or 5
		if (deltay<0)
		{	// could be in octant 2 or 3
			if (deltax>-deltay)
			{	// detected line in octant 3
				error1 = -deltay*2;
				error2 = error1-deltax*2;
				d = error1 - deltax;
				while (deltax>0)
				{	if (d<0)
					{	d+=error1;
					}	else
					{	yofs-=yadd;
						d+=error2;
					}
					x1++;
					deltax--;
					lfb16[x1+yofs] = col;
				}
			}	else
			{	// detected line in octant 2
				error1 = deltax*2;
				error2 = error1+deltay*2;
				d = error1 + deltay;
				while (deltay<0)
				{	if (d<0)
					{	d+=error1;
					}	else
					{	x1++;
						d+=error2;
					}
					yofs-=yadd;
					deltay++;
					lfb16[x1+yofs] = col;
				}
			}	// > Detected line in octant 2 or 3
		}	else
		{	// could be in octant 4 or 5
			if (deltax>deltay)
			{	// Detected line in octant 4
				error1 = deltay*2;
				error2 = error1 - deltax * 2;
				d = error1 - deltax;
				while (deltax>0)
				{	if (d<0)
					{	d+=error1;
					}	else
					{	d+=error2;
						yofs+=yadd;
					}
					x1++;
					deltax--;
					lfb16[x1+yofs] = col;
				}
			}	else
			{	// Detected line in octant 5
				error1 = deltax*2;
				error2 = error1 - deltay * 2;
				d = error1 - deltay;
				while (deltay>0)
				{	if (d<0)
					{	d+=error1;
					}	else
					{	d+=error2;
						x1++;
					}
					yofs+=yadd;
					deltay--;
					lfb16[x1+yofs] = col;
				}
			}
		}
	}
}

void drawline32(intf x1,intf y1,intf x2,intf y2,uint32 col32)	// Co-ords must be clipped
{	intf error1, error2,d;
	intf yofs,yadd;

	dword *lfb32 = (dword *)lfb;

	if (x2<x1)	// For optimization later - handle all octants (remove this IF statement)
	{	// Swap points around
		intf tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	// Write first pixel
	yofs = ylookup[y1]>>2;
	yadd = lfbpitch>>2;
	lfb32[x1+yofs] = col32;

	intf deltax = x2-x1;
	intf deltay = y2-y1;
	if (deltax == 0 && deltay == 0) return;

	//if (deltax>=0)	// For optimization later - handle all octants
	{	// could be in octant 2,3,4, or 5
		if (deltay<0)
		{	// could be in octant 2 or 3
			if (deltax>-deltay)
			{	// detected line in octant 3
				error1 = -deltay*2;
				error2 = error1-deltax*2;
				d = error1 - deltax;
				while (deltax>0)
				{	if (d<0)
					{	d+=error1;
					}	else
					{	yofs-=yadd;
						d+=error2;
					}
					x1++;
					deltax--;
					lfb32[x1+yofs] = col32;
				}
			}	else
			{	// detected line in octant 2
				error1 = deltax*2;
				error2 = error1+deltay*2;
				d = error1 + deltay;
				while (deltay<0)
				{	if (d<0)
					{	d+=error1;
					}	else
					{	x1++;
						d+=error2;
					}
					yofs-=yadd;
					deltay++;
					lfb32[x1+yofs] = col32;
				}
			}	// > Detected line in octant 2 or 3
		}	else
		{	// could be in octant 4 or 5
			if (deltax>deltay)
			{	// Detected line in octant 4
				error1 = deltay*2;
				error2 = error1 - deltax * 2;
				d = error1 - deltax;
				while (deltax>0)
				{	if (d<0)
					{	d+=error1;
					}	else
					{	d+=error2;
						yofs+=yadd;
					}
					x1++;
					deltax--;
					lfb32[x1+yofs] = col32;
				}
			}	else
			{	// Detected line in octant 5
				error1 = deltax*2;
				error2 = error1 - deltay * 2;
				d = error1 - deltay;
				while (deltay>0)
				{	if (d<0)
					{	d+=error1;
					}	else
					{	d+=error2;
						x1++;
					}
					yofs+=yadd;
					deltay--;
					lfb32[x1+yofs] = col32;
				}
			}
		}
	}
}

bool Clip_Line(intf &x1,intf &y1,intf &x2, intf &y2)
{
// this function clips the sent line using the globally defined clipping
// region

// internal clipping codes
#define CLIP_CODE_C  0x0000
#define CLIP_CODE_N  0x0008
#define CLIP_CODE_S  0x0004
#define CLIP_CODE_E  0x0002
#define CLIP_CODE_W  0x0001

#define CLIP_CODE_NE 0x000a
#define CLIP_CODE_SE 0x0006
#define CLIP_CODE_NW 0x0009
#define CLIP_CODE_SW 0x0005

int xc1=x1,
    yc1=y1,
	xc2=x2,
	yc2=y2;

int p1_code=0,
    p2_code=0;

// determine codes for p1 and p2
if (y1 < clipy1)
	p1_code|=CLIP_CODE_N;
else
if (y1 > clipy2)
	p1_code|=CLIP_CODE_S;

if (x1 < clipx1)
	p1_code|=CLIP_CODE_W;
else
if (x1 > clipx2)
	p1_code|=CLIP_CODE_E;

if (y2 < clipy1)
	p2_code|=CLIP_CODE_N;
else
if (y2 > clipy2)
	p2_code|=CLIP_CODE_S;

if (x2 < clipx1)
	p2_code|=CLIP_CODE_W;
else
if (x2 > clipx2)
	p2_code|=CLIP_CODE_E;

// try and trivially reject
if ((p1_code & p2_code))
	return(false);

// test for totally visible, if so leave points untouched
if ((p1_code+p2_code)==0)
	return(true);

// determine end clip point for p1
switch(p1_code)
	  {
	  case CLIP_CODE_C: break;

	  case CLIP_CODE_N:
		   {
		   yc1 = clipy1;
		   xc1 = (intf)(x1 + 0.5+(clipy1-y1)*(x2-x1)/(y2-y1));
		   } break;
	  case CLIP_CODE_S:
		   {
		   yc1 = clipy2;
		   xc1 = (intf)(x1 + 0.5+(clipy2-y1)*(x2-x1)/(y2-y1));
		   } break;

	  case CLIP_CODE_W:
		   {
		   xc1 = clipx1;
		   yc1 = (intf)(y1 + 0.5+(clipx1-x1)*(y2-y1)/(x2-x1));
		   } break;

	  case CLIP_CODE_E:
		   {
		   xc1 = clipx2;
		   yc1 = (intf)(y1 + 0.5+(clipx2-x1)*(y2-y1)/(x2-x1));
		   } break;

	// these cases are more complex, must compute 2 intersections
	  case CLIP_CODE_NE:
		   {
		   // north hline intersection
		   yc1 = clipy1;
		   xc1 = (long)(x1 + 0.5+(clipy1-y1)*(x2-x1)/(y2-y1));

		   // test if intersection is valid, of so then done, else compute next
			if (xc1 < clipx1 || xc1 > clipx2)
				{
				// east vline intersection
				xc1 = clipx2;
				yc1 = (intf)(y1 + 0.5+(clipx2-x1)*(y2-y1)/(x2-x1));
				} // end if

		   } break;

	  case CLIP_CODE_SE:
      	   {
		   // south hline intersection
		   yc1 = clipy2;
		   xc1 = (intf)(x1 + 0.5+(clipy2-y1)*(x2-x1)/(y2-y1));

		   // test if intersection is valid, of so then done, else compute next
		   if (xc1 < clipx1 || xc1 > clipx2)
		      {
			  // east vline intersection
			  xc1 = clipx2;
			  yc1 = (intf)(y1 + 0.5+(clipx2-x1)*(y2-y1)/(x2-x1));
			  } // end if

		   } break;

	  case CLIP_CODE_NW:
      	   {
		   // north hline intersection
		   yc1 = clipy1;
		   xc1 = (intf)(x1 + 0.5+(clipy1-y1)*(x2-x1)/(y2-y1));

		   // test if intersection is valid, of so then done, else compute next
		   if (xc1 < clipx1 || xc1 > clipx2)
		      {
			  xc1 = clipx1;
		      yc1 = (intf)(y1 + 0.5+(clipx1-x1)*(y2-y1)/(x2-x1));
			  } // end if

		   } break;

	  case CLIP_CODE_SW:
		   {
		   // south hline intersection
		   yc1 = clipy2;
		   xc1 = (intf)(x1 + 0.5+(clipy2-y1)*(x2-x1)/(y2-y1));

		   // test if intersection is valid, of so then done, else compute next
		   if (xc1 < clipx1 || xc1 > clipx2)
		      {
			  xc1 = clipx1;
		      yc1 = (intf)(y1 + 0.5+(clipx1-x1)*(y2-y1)/(x2-x1));
			  } // end if

		   } break;

	  default:break;

	  } // end switch

// determine clip point for p2
switch(p2_code)
	  {
	  case CLIP_CODE_C: break;

	  case CLIP_CODE_N:
		   {
		   yc2 = clipy1;
		   xc2 = x2 + (clipy1-y2)*(x1-x2)/(y1-y2);
		   } break;

	  case CLIP_CODE_S:
		   {
		   yc2 = clipy2;
		   xc2 = x2 + (clipy2-y2)*(x1-x2)/(y1-y2);
		   } break;

	  case CLIP_CODE_W:
		   {
		   xc2 = clipx1;
		   yc2 = y2 + (clipx1-x2)*(y1-y2)/(x1-x2);
		   } break;

	  case CLIP_CODE_E:
		   {
		   xc2 = clipx2;
		   yc2 = y2 + (clipx2-x2)*(y1-y2)/(x1-x2);
		   } break;

		// these cases are more complex, must compute 2 intersections
	  case CLIP_CODE_NE:
		   {
		   // north hline intersection
		   yc2 = clipy1;
		   xc2 = (intf)(x2 + 0.5+(clipy1-y2)*(x1-x2)/(y1-y2));

		   // test if intersection is valid, of so then done, else compute next
			if (xc2 < clipx1 || xc2 > clipx2)
				{
				// east vline intersection
				xc2 = clipx2;
				yc2 = (intf)(y2 + 0.5+(clipx2-x2)*(y1-y2)/(x1-x2));
				} // end if

		   } break;

	  case CLIP_CODE_SE:
      	   {
		   // south hline intersection
		   yc2 = clipy2;
		   xc2 = (intf)(x2 + 0.5+(clipy2-y2)*(x1-x2)/(y1-y2));

		   // test if intersection is valid, of so then done, else compute next
		   if (xc2 < clipx1 || xc2 > clipx2)
		      {
			  // east vline intersection
			  xc2 = clipx2;
			  yc2 = (intf)(y2 + 0.5+(clipx2-x2)*(y1-y2)/(x1-x2));
			  } // end if

		   } break;

	  case CLIP_CODE_NW:
      	   {
		   // north hline intersection
		   yc2 = clipy1;
		   xc2 = (intf)(x2 + 0.5+(clipy1-y2)*(x1-x2)/(y1-y2));

		   // test if intersection is valid, of so then done, else compute next
		   if (xc2 < clipx1 || xc2 > clipx2)
		      {
			  xc2 = clipx1;
		      yc2 = (intf)(y2 + 0.5+(clipx1-x2)*(y1-y2)/(x1-x2));
			  } // end if

		   } break;

	  case CLIP_CODE_SW:
		   {
		   // south hline intersection
		   yc2 = clipy2;
		   xc2 = (intf)(x2 + 0.5+(clipy2-y2)*(x1-x2)/(y1-y2));

		   // test if intersection is valid, of so then done, else compute next
		   if (xc2 < clipx1 || xc2 > clipx2)
		      {
			  xc2 = clipx1;
		      yc2 = (intf)(y2 + 0.5+(clipx1-x2)*(y1-y2)/(x1-x2));
			  } // end if

		   } break;

	  default:break;

	  } // end switch

// do bounds check
if ((xc1 < clipx1) || (xc1 > clipx2) ||
	(yc1 < clipy1) || (yc1 > clipy2) ||
	(xc2 < clipx1) || (xc2 > clipx2) ||
	(yc2 < clipy1) || (yc2 > clipy2) )
	{
	return(false);
	} // end if

// store vars back
x1 = xc1;
y1 = yc1;
x2 = xc2;
y2 = yc2;

return(true);

} // end Clip_Line

void drawLine(intf x1,intf y1,intf x2,intf y2,uint32 col32)
{	if (Clip_Line(x1,y1,x2,y2))
	switch(rt2d_bpp)
	{	case 16: drawline16(x1,y1,x2,y2,col32);	break;
		case 32: drawline32(x1,y1,x2,y2,col32);	break;
	}
}

// ********************************************************************************
// ***									Palettes								***
// ********************************************************************************
Palette *usedPalette, *freePalette;
Palette *GreyScalePalette;

void setpalette(Palette *pal)
{	paldata32 = pal->col32;
	paldata16 = pal->col16;
}

void updatePalette(Palette *p)
{	for (uintf i=0; i<256; i++)
	{	p->col16[i] = (uint16)(rgbto565(p->col32[i]));
	}
}

Palette *newPalette(const char *flname)
{	Palette *pal;
	allocbucket(Palette, pal, flags, Generic_MemoryFlag32, 15, "Palette Cache");
	if (flname)
	{	sFileHandle *handle = fileOpen(flname);
		fileRead(handle,(byte *)pal->col16,768);
		byte *p = (byte *)pal->col16;
		for (long i=255; i>=0; i--)
		{	dword ii = i+i+i;
			pal->col32[i] = 0xff000000 | (((dword)p[ii])<<16) | (((dword)p[ii+1])<<8) | ((dword)p[ii+2]);
		}
		fileClose(handle);
	}
	return pal;
}

void _deletePalette(Palette *pal)
{	if (!pal) return;
	deletebucket(Palette, pal);
}

void initPalette(void)
{	uint32 i;
	usedPalette = NULL;
	freePalette = NULL;
	GreyScalePalette = newPalette();
	for (i=0; i<256; i++)
	{	GreyScalePalette->col32[i] = 0xff000000 | (i<<16) | (i<<8) | i;
	}
	updatePalette(GreyScalePalette);
	updatePalette(&systempal);
}

void killPalette(void)
{	killbucket(Palette, flags, Generic_MemoryFlag32);
}

// ------------------------------ Bitmap Functions ------------------------------------------
byte *alpha_addarray;		//[512];
byte *alpha_modulatearray;	//[65536];
byte *alpha_modulate4xarray;//[65536];

void initalphalookups(byte *memPtr)
{	dword i,j;

	alpha_addarray = memPtr;		memPtr += 512;
	alpha_modulatearray = memPtr;	memPtr += 65536;
	alpha_modulate4xarray = memPtr;	memPtr += 65536;
	// init alpha_addarray
	for (i=0; i<256; i++)
		alpha_addarray[i]=(byte)i;
	for (i=256; i<512; i++)
		alpha_addarray[i]=255;

	// init alpha_modulatearray
	for (i=0; i<256; i++)
	{	for (j=0; j<256; j++)
		{	alpha_modulatearray[i+(j<<8)] = (byte)((i*j)>>8);
			uintf intense = (i*j)>>6;
			if (intense>255) intense=255;
			alpha_modulate4xarray[i+(j<<8)] = (byte)intense;
		}
	}
}

// --------------------------- Bitmap Drawing Functions ----------------------------------
// This clip macro is used by all the blit functions and clips the blit operation to the clipping
// rectangle (which is usually the whole screen)
#define clipmacro 								\
{	long pitch = width+stride;					\
	long tmp;									\
												\
	if (ypos>clipy2 || xpos>clipx2) return;		\
	/* Perform top clipping */					\
	if (ypos<clipy1)							\
    {   height-=(clipy1-ypos);					\
		while (ypos<clipy1)						\
		{	data+=pitch;						\
			ypos++;								\
		}										\
    }											\
	/* Perform bottom clipping */				\
    if (ypos+height>clipy2)						\
    {	height-=((ypos+height)-clipy2)-1;		\
    }											\
	if (height<0) return;						\
    /* Perform left clipping */					\
    if (xpos<clipx1)							\
    {	tmp = (clipx1-xpos);					\
		width-=tmp;								\
		stride+=tmp;							\
    	data+=tmp;								\
		xpos=clipx1;							\
	}											\
    /* Perform right clipping */				\
    if (xpos+width>clipx2)						\
    {	tmp = (xpos+width-clipx2)-1;			\
    	width-=tmp;								\
		stride+=tmp;							\
	}											\
	if (width<=0) return;						\
}

// blt32to16 - blit a 32 bit bitmap to a 16 bit screen
void blt32to16(dword *data,long xpos, long ypos, long width, long height,long stride)
{	// Optimisations - Forced 32 bit aligned DWORD writes to memory during main loop
	long x,y,yaddscreen,yaddbmap;
	dword *datacopy,*addr32;
	dword col1,col2;
	word *addr16;

	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt32to16");
	#endif

	clipmacro;

	byte *addr8 = (byte *)lfb+ylookup[ypos]+xpos+xpos;
	yaddscreen = lfbpitch>>1;
	yaddbmap = stride+width;

	// If not 32 bit aligned, draw a vertical line on the left, so the next pixel across will be
	// 32 bit aligned
	if (xpos & 1)
	{	addr16 = (word *)addr8;
		datacopy = data;
		for (y=0; y<height; y++)
		{	*addr16 = (word)(rgbto565(*datacopy));
			addr16 += yaddscreen;
			datacopy += yaddbmap;
		}
		addr8+=2;
		width--;
		stride++;
		data++;
	}

	// If width is not even, draw a vertical line on the right, so that the remaining width is
	// even, allowing for faster DWORD writes to memory.
	if (width & 1)
	{	addr16 = ((word *)addr8)+width-1;
		datacopy = data+width-1;
		for (y=0; y<height; y++)
		{	*addr16 = (word)(rgbto565(*datacopy));
			addr16 += yaddscreen;
			datacopy += yaddbmap;
		}
		width--;
		stride++;
	}

	// Draw the inner section - it is now guaranteed to be an even width, 32 bit aligned
	// Use DWORD writes, since a DWORD write is 1 clock cycle faster, and writes twice as much
	// data at a time to WORD writes
	width = width>>1;
	for (y=0; y<height; y++)
	{	addr32 = (dword *)addr8;
		for (x=0; x<width; x++)
		{	col1 = rgbto565(*data); data++;
			col2 = rgbto565(*data); data++;
			*addr32++ = (col2<<16)+col1;
		}
		data+=stride;
		addr8+=lfbpitch;
	}
}

// blt8to16 - blit a non-transparent 8 bit paletised image to a 16 bit screen
void blt8to16(byte *data, long xpos, long ypos, long width, long height, long stride)
{	// Optimisations - Forced 32 bit aligned DWORD writes to memory during main loop
	long x,y,yaddscreen,yaddbmap;
	dword *addr32;
	dword col1,col2;
	word *addr16;
	byte *datacopy;

	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt8to16t");
	#endif

	clipmacro;

	byte *addr8 = (byte *)lfb+ylookup[ypos]+xpos+xpos;
	yaddscreen = lfbpitch>>1;
	yaddbmap = stride+width;

	// If not 32 bit aligned, draw a vertical line on the left
	if (xpos & 1)
	{	addr16 = (word *)addr8;
		datacopy = data;
		for (y=0; y<height; y++)
		{	*addr16 = paldata16[*datacopy];
			addr16 += yaddscreen;
			datacopy += yaddbmap;
		}
		addr8+=2;
		width--;
		stride++;
		data++;
	}

	// If width is not even, draw a vertical line on the right
	if (width & 1)
	{	addr16 = ((word *)addr8)+width-1;
		datacopy = data+width-1;
		for (y=0; y<height; y++)
		{	*addr16 = paldata16[*datacopy];
			addr16 += yaddscreen;
			datacopy += yaddbmap;
		}
		width--;
		stride++;
	}

	// Draw the inner section - it is now guaranteed to be an even width, 32 bit aligned
	width = width>>1;
	for (y=0; y<height; y++)
	{	addr32 = (dword *)addr8;
		for (x=0; x<width; x++)
		{	col1 = paldata16[*data++];
			col2 = paldata16[*data++];
			*addr32++ = (col2<<16)+col1;
		}
		data+=stride;
		addr8+=lfbpitch;
	}
}

// blt8to32 - blit a non-transparent 8 bit paletised image to a 32 bit render target
void blt8to32(byte *data, long xpos, long ypos, long width, long height, long stride)
{	// Optimisations - Forced 32 bit aligned DWORD writes to memory during main loop
	long x,y;
	dword *addr32;

	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt8to32");
	#endif

	clipmacro;

	byte *addr8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);

	for (y=0; y<height; y++)
	{	addr32 = (dword *)addr8;
		for (x=0; x<width; x++)
		{	*addr32++ = paldata32[*data++];
		}
		data+=stride;
		addr8+=lfbpitch;
	}
}

// blt32to32 - blit a non-transparent 32 bit image to a 32 bit render target
void blt32to32(dword *data, long xpos, long ypos, long width, long height, long stride)
{	intf x,y;

	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt32to32");
	#endif

	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);

	if ((width&3)==0)
	{	// 4x pipeline Optimised
		for (y=0; y<height; y++)
		{	dword *dest32 = (dword *)dest8;
			for (x=0; x<width; x+=4)
			{	*dest32  =*data;
				dest32[1]=data[1];
				dest32[2]=data[2];
				dest32[3]=data[3];
				dest32+=4;
				data+=4;
			}
			data+=stride;
			dest8+=lfbpitch;
		}
	}	else
	{	// Regular version when pipelining isn't possible
		for (y=0; y<height; y++)
		{	dword *dest32 = (dword *)dest8;
			for (x=0; x<width; x++)
				*dest32++=data[x];
			data+=width+stride;
			dest8+=lfbpitch;
		}
	}
}

uint32 *blt_dest32, *blt_src32,blt_pix32_0,blt_pix32_1,blt_pix32_2,blt_pix32_3;
void blt32to32t(uint32 *data, long xpos, long ypos, long width, long height, long stride)
{	intf x,y;

	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt32to32");
	#endif

	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	blt_src32 = data;

	if ((width&3)==0)
	{	// 4x pipeline Optimised
		for (y=0; y<height; y++)
		{	blt_dest32 = (uint32 *)dest8;
			for (x=0; x<width; x+=4)
			{	blt_pix32_0 = *blt_src32;
				blt_pix32_1 = blt_src32[1];
				blt_pix32_2 = blt_src32[2];
				blt_pix32_3 = blt_src32[3];
				if (blt_pix32_0) *blt_dest32=blt_pix32_0;
				if (blt_pix32_1) blt_dest32[1]=blt_pix32_1;
				if (blt_pix32_2) blt_dest32[2]=blt_pix32_2;
				if (blt_pix32_3) blt_dest32[3]=blt_pix32_3;
				blt_src32+=4;
				blt_dest32+=4;
			}
			blt_src32+=stride;
			dest8+=lfbpitch;
		}
	}	else
	{	// Regular version when pipelining isn't possible
		for (y=0; y<height; y++)
		{	blt_dest32 = (uint32 *)dest8;
			for (x=0; x<width; x++)
			{	blt_pix32_0 = *blt_src32++;
				if (blt_pix32_0) *blt_dest32=blt_pix32_0;
				blt_dest32++;
			}
			blt_src32+=stride;
			dest8+=lfbpitch;
		}
	}
}

// blt16to32 - blit a non-transparent 32 bit image to a 16 bit render target
void blt16to32(word *data, long xpos, long ypos, long width, long height, long stride)
{	long x,y;

	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt16to32");
	#endif

	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);

	for (y=0; y<height; y++)
	{	dword *dest32 = (dword *)dest8;
		for (x=0; x<width; x++)
		{	dword col = (dword)(*data++);
			*dest32++ = rgbfrom565(col);
		}
		data+=stride;
		dest8+=lfbpitch;
	}
}

// blt16to16 - blit a non-transparent 16 bit image to a 16 bit screen
void blt16to16(word *data, long xpos, long ypos, long width, long height, long stride)
{	// Optimisations - Forced 32 bit aligned DWORD writes to memory during main loop
	long x,y,yaddscreen,yaddbmap;
	dword *addr32;
	word *addr16;
	word *datacopy;

	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt16to16");
	#endif

	clipmacro;

	byte *addr8 = (byte *)lfb+ylookup[ypos]+xpos+xpos;
	yaddscreen = lfbpitch>>1;
	yaddbmap = stride+width;

	// If not 32 bit aligned, draw a vertical line on the left
	if (xpos & 1)
	{	addr16 = (word *)addr8;
		datacopy = data;
		for (y=0; y<height; y++)
		{	*addr16 = *datacopy;
			addr16 += yaddscreen;
			datacopy += yaddbmap;
		}
		addr8+=2;
		width--;
		stride++;
		data++;
	}

	// If width is not even, draw a vertical line on the right
	if (width & 1)
	{	addr16 = ((word *)addr8)+width-1;
		datacopy = data+width-1;
		for (y=0; y<height; y++)
		{	*addr16 = *datacopy;
			addr16 += yaddscreen;
			datacopy += yaddbmap;
		}
		width--;
		stride++;
	}

	// Draw the inner section - it is now guaranteed to be an even width, 32 bit aligned
	width = width>>1;
	for (y=0; y<height; y++)
	{	addr32 = (dword *)addr8;
		dword *data32 = (dword *)data;
		for (x=0; x<width; x++)
		{	*addr32++ = *data32++;
		}
		data+=stride+width+width;
		addr8+=lfbpitch;
	}
}

// blt8to16t - blit a 0=transparent 8 bit paletised image to a 16 bit screen
void blt8to16t(byte *data, long xpos, long ypos, long width, long height, long stride)
{
	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt8to16t");
	#endif

	clipmacro;

	byte *addr8 = (byte *)lfb+ylookup[ypos]+xpos+xpos;
	for (long y=0; y<height; y++)
	{	word *addr16 = (word *)addr8;
		for (long x=0; x<width; x++)
		{	if (*data)
			{	*addr16 = paldata16[*data];
			}
			addr16++;
			data++;
		}
		data+=stride;
		addr8+=lfbpitch;
	}
}

// blt8to32t - blit a 0=transparent 8 bit paletized image to a 32 bit target
void blt8to32t(byte *data, long xpos, long ypos, long width, long height, long stride)
{
	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt8to32t");
	#endif

	clipmacro;

	byte *addr8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	for (long y=0; y<height; y++)
	{	dword *addr32 = (dword *)addr8;
		for (long x=0; x<width; x++)
		{	if (*data)
			{	*addr32 = paldata32[*data];
			}
			addr32++;
			data++;
		}
		data+=stride;
		addr8+=lfbpitch;
	}
}

// --------------------------- drawbitmap API Command ------------------------------
void drawbitmap(bitmap *pic,long x,long y)
{
	#ifdef fcdebug
		if (!pic) msg(viddrv_error,"NULL memory address was sent to function : drawbitmap");
	#endif

	dword bitdepth = pic->flags & bitmap_DataInfoMask;
	if (rt2d_bpp==16)
	{	if (bitdepth == bitmap_RGB_16bit)
		{	blt16to16((word *)pic->pixel,x,y,pic->width,pic->height,0);
			return;
		}

		if (bitdepth == bitmap_RGB_8bit)
		{	if (pic->palette)
				setpalette(pic->palette);
			else
				setpalette(GreyScalePalette);
			blt8to16(pic->pixel,x,y,pic->width,pic->height,0);
			return;
		}

		if (bitdepth == bitmap_RGB_32bit)
		{	blt32to16((dword *)pic->pixel,x,y,pic->width,pic->height,0);
			return;
		}
	}

	if (rt2d_bpp==32)
	{	if (bitdepth == bitmap_RGB_32bit)
		{	blt32to32((dword *)pic->pixel,x,y,pic->width,pic->height,0);
			return;
		}
		if (bitdepth == bitmap_RGB_16bit)
		{	blt16to32((word *)pic->pixel,x,y,pic->width,pic->height,0);
			return;
		}
		if (bitdepth == bitmap_RGB_8bit)
		{	if (pic->palette)
				setpalette(pic->palette);
			else
				setpalette(GreyScalePalette);
			blt8to32(pic->pixel,x,y,pic->width,pic->height,0);
			return;
		}
	}
}

// --------------------------- drawsprite API Command ------------------------------
void drawsprite(bitmap *pic,long x, long y)
{
	#ifdef fcdebug
		if (!pic) msg(viddrv_error,"NULL memory address was sent to function : drawsprite");
	#endif

	if ((pic->flags & bitmap_DataInfoMask) == bitmap_RGB_8bit)
	{	if (rt2d_bpp==16)
		{	if (pic->palette)
				setpalette(pic->palette);
			else
				setpalette(GreyScalePalette);
			blt8to16t(pic->pixel,x,y,pic->width,pic->height,0);
		}
		if (rt2d_bpp==32)
		{	if (pic->palette)
				setpalette(pic->palette);
			else
				setpalette(GreyScalePalette);
			blt8to32t(pic->pixel,x,y,pic->width,pic->height,0);
		}
	}	else
	if ((pic->flags & bitmap_DataInfoMask) == bitmap_RGB_32bit)
	{	if (rt2d_bpp==32)
		{	blt32to32t((uint32 *)pic->pixel,x,y,pic->width,pic->height,0);
		}
	}
}

// This clip macro is used by all the blit functions with XFlip and clips the blit operation to
// the clipping rectangle (which is usually the whole screen)
#define clipmacroXf								\
{	long pitch = width+stride;					\
	long tmp;									\
												\
	if (ypos>clipy2 || xpos>clipx2) return;		\
	/* Perform top clipping */					\
	if (ypos<clipy1)							\
    {   height-=(clipy1-ypos);					\
		while (ypos<clipy1)						\
		{	data+=pitch;						\
			ypos++;								\
		}										\
    }											\
	/* Perform bottom clipping */				\
    if (ypos+height>clipy2)						\
    {	height-=((ypos+height)-clipy2)-1;		\
    }											\
	if (height<0) return;						\
    /* Perform left clipping */					\
    data += width-1;							\
	stride += (width<<1);						\
	if (xpos<clipx1)							\
    {	tmp = (clipx1-xpos);					\
		width-=tmp;								\
		stride-=tmp;							\
    	xpos=clipx1;							\
		data-=tmp;								\
	}											\
    /* Perform right clipping */				\
    if (xpos+width>clipx2)						\
    {	tmp = (xpos+width-clipx2)-1;			\
		width-=tmp;								\
		stride-=tmp;							\
	}											\
	if (width<=0) return;						\
}
// blt8to32tXf - blit a X-Flipped, 0=transparent 8 bit paletized image to a 32 bit target
void blt8to32tXf(byte *data, long xpos, long ypos, long width, long height, long stride)
{
	#ifdef fcdebug
		if (!data) msg(viddrv_error,"NULL memory address was sent to function : blt8to32t");
	#endif

	clipmacroXf;

	byte *addr8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	for (long y=0; y<height; y++)
	{	dword *addr32 = (dword *)addr8;
		for (long x=0; x<width; x++)
		{	if (*data)
			{	*addr32 = paldata32[*data];
			}
			addr32++;
			data--;
		}
		data+=stride;
		addr8+=lfbpitch;
	}
}

void drawspriteXflip(bitmap *pic, long x, long y)
{
	#ifdef fcdebug
		if (!pic) msg(viddrv_error,"NULL memory address was sent to function : drawsprite");
	#endif

	if (rt2d_bpp==16)
	{	msg(viddrv_error,"drawspriteXflip does not yet support 16 bit surfaces");
		//if ((pic->flags & bitmap_bppmask) == bitmap_8bit)
		//{	if (pic->palette)
		//		setpalette(pic->palette);
		//	else
		//		setpalette(&systempal);
		//	blt8to16t(pic->pixel,x,y,pic->width,pic->height,0);
		//}
	}

	if (rt2d_bpp==32)
	{	if ((pic->flags & bitmap_DataInfoMask) == bitmap_RGB_8bit)
		{	if (pic->palette)
			{	setpalette(pic->palette);
				blt8to32tXf(pic->pixel,x,y,pic->width,pic->height,0);
			}
			else
			{	setpalette(&systempal);
				blt8to32tXf(pic->pixel,x,y,pic->width,pic->height,0);
			}
		}
	}
}

#define viddrv_not32bm " can only handle 32 bit RGB bitmaps"

#define validatealphaparam(bm,funcname)				\
	if ((bm->flags & bitmap_DataInfoMask)!=bitmap_RGB_32bit) msg(viddrv_error,funcname viddrv_not32bm); \
	if ((bm->flags & bitmap_DataTypeMask)!=bitmap_DataTypeRGB) msg(viddrv_error,funcname viddrv_not32bm);

void drawbitmap_add(bitmap *bm, long xpos, long ypos)
{
	#ifdef fcdebug
		if (!bm) msg(viddrv_error,"NULL pointer was sent to function : drawbitmap_add");
		if (rt2d_bpp!=32) msg(viddrv_error,"drawbitmap_add function called, but render target was not 32 bit");
		validatealphaparam(bm, "drawbitmap_add");
	#endif

	long width = bm->width;
	long height = bm->height;
	long  stride = 0;
	dword *data = (dword *)bm->pixel;
	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	for (long y=0; y<height; y++)
	{	byte *src8 = (byte *)data;
		byte *dst8 = (byte *)dest8;
		for (long x=0; x<width; x++)
		{	dst8[0] = alpha_addarray[dst8[0]+src8[0]];
			dst8[1] = alpha_addarray[dst8[1]+src8[1]];
			dst8[2] = alpha_addarray[dst8[2]+src8[2]];
			dst8+=4;
			src8+=4;
		}
		data+=width+stride;
		dest8+=lfbpitch;
	}

}

void drawbitmap_addscale(bitmap *bm, long xpos, long ypos,byte scale)
{
	#ifdef fcdebug
		if (!bm) msg(viddrv_error,"NULL memory address was sent to function : drawbitmap_addscale");
		if (rt2d_bpp!=32) msg(viddrv_error,"drawbitmap_addscale function called, but render target was not 32 bit");
		if ((bm->flags & bitmap_DataInfoMask)!=bitmap_RGB_32bit) msg(viddrv_error,"drawbitmap_addscale function called, but the bitmap to draw is not 32 bit");
	#endif

	long width = bm->width;
	long height = bm->height;
	long  stride = 0;
	dword *data = (dword *)bm->pixel;
	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	dword dscale = ((dword)scale)<<8;
	for (long y=0; y<height; y++)
	{	byte *src8 = (byte *)data;
		byte *dst8 = (byte *)dest8;
		for (long x=0; x<width; x++)
		{	dst8[0] = alpha_addarray[dst8[0]+alpha_modulatearray[src8[0]+dscale]];
			dst8[1] = alpha_addarray[dst8[1]+alpha_modulatearray[src8[1]+dscale]];
			dst8[2] = alpha_addarray[dst8[2]+alpha_modulatearray[src8[2]+dscale]];
			dst8+=4;
			src8+=4;
		}
		data+=width+stride;
		dest8+=lfbpitch;
	}
}

void drawbitmap_modulate(bitmap *bm, long xpos, long ypos)
{
	#ifdef fcdebug
		if (!bm) msg(viddrv_error,"NULL memory address was sent to function : drawbitmap_modulate");
		if (rt2d_bpp!=32) msg(viddrv_error,"drawbitmap_modulate function called, but render target was not 32 bit");
		if ((bm->flags & bitmap_DataInfoMask)!=bitmap_RGB_32bit) msg(viddrv_error,"drawbitmap_modulate function called, but the bitmap to draw is not 32 bit");
	#endif

	long width = bm->width;
	long height = bm->height;
	long  stride = 0;
	dword *data = (dword *)bm->pixel;
	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	for (long y=0; y<height; y++)
	{	byte *src8 = (byte *)data;
		uint32 *dst32 = (uint32 *)dest8;
		byte *dst8 = dest8;
		for (long x=0; x<width; x++)
		{	uint32 s1 = (*src8++ << 8) + *dst8++;
			uint32 s2 = (*src8++ << 8) + *dst8++;
			uint32 s3 = (*src8++ << 8) + *dst8++;
			*dst32++ =	(alpha_modulatearray[s1]) +
						(alpha_modulatearray[s2]<<8) +
						(alpha_modulatearray[s3]<<16) + 0xff000000;

			src8++;
			dst8++;
		}
		data+=width+stride;
		dest8+=lfbpitch;
	}
}

void drawbitmap_modulate4x(bitmap *bm, long xpos, long ypos)
{
	#ifdef fcdebug
		if (!bm) msg(viddrv_error,"NULL memory address was sent to function : drawbitmap_modulate4x");
		if (rt2d_bpp!=32) msg(viddrv_error,"drawbitmap_modulate4x function called, but render target was not 32 bit");
		if ((bm->flags & bitmap_DataInfoMask)!=bitmap_RGB_32bit) msg(viddrv_error,"drawbitmap_modulate4x function called, but the bitmap to draw is not 32 bit");
	#endif

	long width = bm->width;
	long height = bm->height;
	long  stride = 0;
	dword *data = (dword *)bm->pixel;
	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	for (long y=0; y<height; y++)
	{	byte *src8 = (byte *)data;
		uint32 *dst32 = (uint32 *)dest8;
		byte *dst8 = dest8;
		for (long x=0; x<width; x++)
		{	uint32 s1 = (*src8++ << 8) + *dst8++;
			uint32 s2 = (*src8++ << 8) + *dst8++;
			uint32 s3 = (*src8++ << 8) + *dst8++;
			*dst32++ =	(alpha_modulate4xarray[s1]) +
						(alpha_modulate4xarray[s2]<<8) +
						(alpha_modulate4xarray[s3]<<16) + 0xff000000;

			src8++;
			dst8++;
		}
		data+=width+stride;
		dest8+=lfbpitch;
	}
}

void drawcomposite_modulate4x(bitmap *src1, bitmap *src2, intf xpos, intf ypos)
{
	#ifdef fcdebug
		if ((!src1) || (!src2)) msg(viddrv_error,"NULL memory address was sent to function : drawcomposite_modulate4x");
		if (rt2d_bpp!=32) msg(viddrv_error,"drawcomposite_modulate4x function called, but render target was not 32 bit");
		if ((src1->flags & bitmap_DataInfoMask)!=bitmap_RGB_32bit) msg(viddrv_error,"drawcomposite_modulate4x function called, but the bitmap to draw is not 32 bit");
		if ((src1->width != src2->width) || (src1->height != src2->height)) msg(viddrv_error,"drawcomposite_modulate4x function called, but the bitmap to draw is not 32 bit");
	#endif

	intf tx = xpos;
	intf ty = ypos;
	intf width = src1->width;
	intf height = src1->height;
	intf stride = 0;
	uint32 *data = (uint32 *)src1->pixel;
	clipmacro;
	uint32 *src1data = data;

	xpos = tx;
	ypos = ty;
	width = src2->width;
	height = src2->height;
	data = (uint32 *)src2->pixel;
	stride = 0;
	clipmacro;
	uint32 *src2data = data;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	for (intf y=0; y<height; y++)
	{	byte *src1_8 = (byte *)src1data;
		byte *src2_8 = (byte *)src2data;
		uint32 *dst32 = (uint32 *)dest8;
		for (intf x=0; x<width; x++)
		{	uint32 s1 = (*src1_8++ << 8) + *src2_8++;
			uint32 s2 = (*src1_8++ << 8) + *src2_8++;
			uint32 s3 = (*src1_8++ << 8) + *src2_8++;
			*dst32++ =	(alpha_modulate4xarray[s1]) +
						(alpha_modulate4xarray[s2]<<8) +
						(alpha_modulate4xarray[s3]<<16) + 0xff000000;

			src1_8++;
			src2_8++;
		}
		src1data+=width+stride;
		src2data+=width+stride;
		dest8+=lfbpitch;
	}
}

void drawbitmap_modscale(bitmap *bm, long xpos, long ypos,byte scale)
{
	#ifdef fcdebug
		if (!bm) msg(viddrv_error,"NULL memory address was sent to function : drawbitmap_modscale");
		if (rt2d_bpp!=32) msg(viddrv_error,"drawbitmap_modscale function called, but render target was not 32 bit");
		if ((bm->flags & bitmap_DataInfoMask)!=bitmap_RGB_32bit) msg(viddrv_error,"drawbitmap_modscale function called, but the bitmap to draw is not 32 bit");
	#endif

	long width = bm->width;
	long height = bm->height;
	long  stride = 0;
	dword *data = (dword *)bm->pixel;
	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	dword sscale = ((dword)scale)<<8;
	dword dscale = ((dword)(255-scale))<<8;
	for (long y=0; y<height; y++)
	{	byte *src8 = (byte *)data;
		byte *dst8 = (byte *)dest8;
		for (long x=0; x<width; x++)
		{	dst8[0] = alpha_addarray[alpha_modulatearray[dst8[0]+dscale]+alpha_modulatearray[src8[0]+sscale]];
			dst8[1] = alpha_addarray[alpha_modulatearray[dst8[1]+dscale]+alpha_modulatearray[src8[1]+sscale]];
			dst8[2] = alpha_addarray[alpha_modulatearray[dst8[2]+dscale]+alpha_modulatearray[src8[2]+sscale]];
			dst8+=4;
			src8+=4;
		}
		data+=width+stride;
		dest8+=lfbpitch;
	}
}

void drawbitmap_modalpha(bitmap *bm, long xpos, long ypos)
{
	#ifdef fcdebug
		if (!bm) msg(viddrv_error,"NULL memory address was sent to function : drawbitmap_modalpha");
		if (rt2d_bpp!=32) msg(viddrv_error,"drawbitmap_modalpha function called, but render target was not 32 bit");
		if ((bm->flags & bitmap_DataInfoMask)!=bitmap_RGB_32bit) msg(viddrv_error,"drawbitmap_modalpha function called, but the bitmap to draw is not 32 bit");
	#endif

	long width = bm->width;
	long height = bm->height;
	long  stride = 0;
	dword *data = (dword *)bm->pixel;
	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	for (long y=0; y<height; y++)
	{	byte *src8 = (byte *)data;
		byte *dst8 = (byte *)dest8;
		for (long x=0; x<width; x++)
		{	dword salpha = ((dword)src8[3])<<8;
			dword dalpha = ((dword)(255-src8[3]))<<8;
			dst8[0] = alpha_addarray[alpha_modulatearray[dst8[0]+dalpha]+alpha_modulatearray[src8[0]+salpha]];
			dst8[1] = alpha_addarray[alpha_modulatearray[dst8[1]+dalpha]+alpha_modulatearray[src8[1]+salpha]];
			dst8[2] = alpha_addarray[alpha_modulatearray[dst8[2]+dalpha]+alpha_modulatearray[src8[2]+salpha]];
			dst8+=4;
			src8+=4;
		}
		data+=width+stride;
		dest8+=lfbpitch;
	}
}

void drawbitmap_modscalealpha(bitmap *bm, long xpos, long ypos,byte scale)
{
	#ifdef fcdebug
		if (!bm) msg(viddrv_error,"NULL memory address was sent to function : drawbitmap_modscalealpha");
		if (rt2d_bpp!=32) msg(viddrv_error,"drawbitmap_modscalealpha function called, but render target was not 32 bit");
		if ((bm->flags & bitmap_DataInfoMask)!=bitmap_RGB_32bit) msg(viddrv_error,"drawbitmap_modscalealpha function called, but the bitmap to draw is not 32 bit");
	#endif

	long width = bm->width;
	long height = bm->height;
	long  stride = 0;
	dword *data = (dword *)bm->pixel;
	clipmacro;

	byte *dest8 = (byte *)lfb+ylookup[ypos]+(xpos<<2);
	for (long y=0; y<height; y++)
	{	byte *src8 = (byte *)data;
		byte *dst8 = (byte *)dest8;
		for (long x=0; x<width; x++)
		{	dword s = alpha_modulatearray[src8[3]+(scale<<8)];
			dword salpha = s<<8;
			dword dalpha = (255-s)<<8;
			dst8[0] = alpha_addarray[alpha_modulatearray[dst8[0]+dalpha]+alpha_modulatearray[src8[0]+salpha]];
			dst8[1] = alpha_addarray[alpha_modulatearray[dst8[1]+dalpha]+alpha_modulatearray[src8[1]+salpha]];
			dst8[2] = alpha_addarray[alpha_modulatearray[dst8[2]+dalpha]+alpha_modulatearray[src8[2]+salpha]];
			dst8+=4;
			src8+=4;
		}
		data+=width+stride;
		dest8+=lfbpitch;
	}
}

//*****************************************************************************
//***									Fonts								***
//*****************************************************************************
extern	intf	_font_convert[];		// Font conversion table found in PREDEFINES.CPP
fontsettings	currentfont;

void setfont(fontsettings *fnt)
{	setfont(fnt->bm);
	setfontcolor(fnt->color);
	currentfont.curscol = fnt->curscol;
	currentfont.spacing = fnt->spacing;
	currentfont.underlinecol = fnt->underlinecol;
}

void getfont(fontsettings *fnt)
{	fnt->bm = currentfont.bm;
	fnt->color = currentfont.color;
	fnt->curscol = currentfont.curscol;
	fnt->spacing = currentfont.spacing;
	fnt->underlinecol = currentfont.underlinecol;
}

// --------------------------- setfont API Command ------------------------------
void setfont(bitmap *font)
{	if (font)
		currentfont.bm = font;
	else
	{	currentfont.bm = systemfont;
		currentfont.spacing = 1;
	}
}

// --------------------------- setcursorcol API Command ------------------------------
void setcursorcol(dword col)
{	currentfont.curscol = col;
}

void setfontspacing(intf spacing)
{	currentfont.spacing = spacing;
}

void setfontunderline(dword col)
{	currentfont.underlinecol = col;
}

// --------------------------- setfontcolor API Command ------------------------------
void setfontcolor(dword col)
{	currentfont.color = col;
	systempal.col16[1] = (word)(rgbto565(col));
	systempal.col32[1] = col;
}

void initFonts(void)
{	setfont(systemfont);
	setcursorcol(0xff000000);
	setfontcol(0xff20a020);
	setfontunderline(0xff000000);
	setfontspacing(1);
}

//*****************************************************************************
//***								Text Output								***
//*****************************************************************************
bool	underlining = false;	// Is the text system currently in underline mode

void _cdecl textout(long x, long y, const char *txt,...)
{	char tmp;
	long index;

	#ifdef fcdebug
		if (!txt) msg(viddrv_error,"Attempted to call textout but with a NULL parameter");
	#endif

	if ((y>clipy2) || (y+(intf)currentfont.bm[0].height<clipy1) || (x>clipx2)) return;

	char text[256];
	constructstring(text, txt, sizeof(text));
//	va_list val;
//	va_start(val,txt);
//	vsprintf(text,txt,val);
//	va_end(val);

	index=0;
	tmp=text[index++];
	while (tmp>0)
	{	tmp-=33;
		if (tmp<0) x+=currentfont.bm[30].width+currentfont.spacing;	// 30 maps to the letter 't'.  We use the width of lower-case 't' as a space
		else
		{	bitmap *tbmap=&currentfont.bm[_font_convert[(uint8)tmp]];
			drawsprite(tbmap,x,y);
			x+=tbmap->width+currentfont.spacing;
			if (x>clipx2) return;
		}
		tmp=text[index++];
	}
}

// --------------------------- textoutu API Command ------------------------------
void _cdecl textoutu(long x, long y, const char *txt,...)
{	// Text out with underlining (use & to toggle underlining on and off)
	char tmp;
	long index;

	#ifdef fcdebug
		if (!txt) msg(viddrv_error,"Attempted to call textoutu but with a NULL parameter");
	#endif

	char text[256];
	constructstring(text, txt, sizeof(text));
//	va_list val;
//	va_start(val,txt);
//	vsprintf(text,txt,val);
//	va_end(val);

	index=0;
	tmp=text[index++];
	while (tmp>0)
	{	if (tmp!='&')
		{	tmp-=33;
			if (tmp<0) x+=currentfont.bm[30].width+currentfont.spacing;
			else
			{	bitmap *tbmap=&currentfont.bm[_font_convert[(uint8)tmp]];
				drawsprite(tbmap,x,y);
				if (underlining) hline(x,y+tbmap->height-2,tbmap->width,currentfont.curscol);
				x+=tbmap->width+currentfont.spacing;
				if (x>clipx2) return;
			}
		}	else
			underlining = !underlining;
		tmp=text[index++];
	}
}

// --------------------------- textwrap API Command ------------------------------
intf textwrap(intf startX, intf startY, intf maxX, intf dy, const char *txt,...)
{	char tmp;
	long index;
	char text[256];
	bitmap *bm;

	#ifdef fcdebug
		if (!txt) msg(viddrv_error,"Attempted to call textwrap but with a NULL parameter");
	#endif

	index = 0;
	// if ((startY>clipy2) || (startX>clipx2)) return startY;
	constructstring(text, txt, sizeof(text));

	intf x = startX;
	intf y = startY;
	bool printedOnThisLine = false;
	while (text[index]!=0)
	{	// Find a space or eol marker
		int tmpX = x;
		for (int i=index; text[i]!=0 && text[i]!=' '; i++)
		{	tmp = text[i]-33;
			if (tmp<0)
				tmpX += currentfont.bm[30].width+currentfont.spacing;
			else
			{	bm = &currentfont.bm[_font_convert[(intf)tmp]];
				tmpX += bm->width + currentfont.spacing;
			}
			if (tmpX>maxX) break;
		}
		if (tmpX<=maxX || printedOnThisLine==false)
		{	while (text[index]!=0 && text[index]!=32)
			{	tmp = text[index++]-33;
				if (tmp>0)
				{	bm = &currentfont.bm[_font_convert[(intf)tmp]];
					drawsprite(bm,x,y);
				}	else
					bm=&currentfont.bm[30];
				x += bm->width + currentfont.spacing;
			}
			x += currentfont.bm[30].width+currentfont.spacing;
			if (text[index]==' ') index++;
			printedOnThisLine = true;
		}	else
		{	x = startX;
			y += dy;
			printedOnThisLine = false;
			//if (y>clipy2) return y;
		}
	}
	return y+dy;
}

// --------------------------- gettextwidth API Command ------------------------------
long gettextwidth(const char *text)
{	// Get the pixel width of a text string using the current font
	char tmp;
	long index,x;

	#ifdef fcdebug
		if (!text) msg(viddrv_error,"Attempted to call gettextwidth but with a NULL parameter");
	#endif

	index=0;
	x=0;
	tmp=text[index++];
	while (tmp>0)
	{	tmp-=33;
		if (tmp<0)
			x+=currentfont.bm[30].width+currentfont.spacing;
		else
		{	x+=currentfont.bm[_font_convert[(uint8)tmp]].width+currentfont.spacing;
		}
		tmp=text[index++];
	}
	return x;
}

// --------------------------- gettextwidthu API Command ------------------------------
long gettextwidthu(const char *text)
{	// Get the pixel width of a text string, ignoring & characters
	char tmp;
	long index,x;

	#ifdef fcdebug
		if (!text) msg(viddrv_error,"Attempted to call gettextwidthu but with a NULL parameter");
	#endif

	index=0;
	x=0;
	tmp=text[index++];
	while (tmp>0)
	{	if (tmp!='&')
		{	tmp-=33;
			if (tmp<0)
				x+=currentfont.bm[30].width+currentfont.spacing;
			else
			{	x+=currentfont.bm[_font_convert[(uint8)tmp]].width+currentfont.spacing;
			}
		}
		tmp=text[index++];
	}
	return x;
}

// --------------------------- getcharwidth API Command ------------------------------
long getcharwidth(char c)
{
	c-=33;
	if (c<0)
		return currentfont.bm[30].width+currentfont.spacing;
	else
	{	return currentfont.bm[_font_convert[(uint8)c]].width+currentfont.spacing;
	}
}

uintf getFontHeight(void)
{	return currentfont.bm->height;
}

// --------------------------- textoutc API Command ------------------------------
void textoutc(long x, long y, const char *text,long cpos)
{	// Textout with a cursor (thin vertical line) displayed within the text
	char tmp;
	long index;

	#ifdef fcdebug
		if (!text) msg(viddrv_error,"Attempted to call textoutc but with a NULL parameter");
	#endif

	index=0;
	tmp=text[index];
	while (tmp>0)
	{	if (cpos==index)
		{	box(x-2,y,2,currentfont.bm[_font_convert[',']].height,currentfont.curscol);
		}
		index++;
		tmp-=33;
		if (tmp<0) x+=currentfont.bm[30].width+currentfont.spacing;
		else
		{	bitmap *tbmap=&currentfont.bm[_font_convert[(uint8)tmp]];
			drawsprite(tbmap,x,y);
			x+=tbmap->width+currentfont.spacing;
		}
		tmp=text[index];
	}
	if (cpos==index)
	{	box(x-2,y,2,currentfont.bm[_font_convert[',']].height,currentfont.curscol);
	}
}

//*****************************************************************************
//***						Initialization & Shutdown						***
//*****************************************************************************
void init2DVideo(byte *memPtr)
{	// Initialize palettes and fonts
	initFonts();
	initPalette();
	initalphalookups(memPtr);
}

void kill2DVideo(void)
{	killPalette();
}

sEngineModule mod_2DRender =
{	"2DRender",					// Module Name
	512 +						// alpha_addarray
		65536 +					// alpha_modulatearray
		65536 ,					// alpha_modulate4xarray
	init2DVideo,
	kill2DVideo
};
