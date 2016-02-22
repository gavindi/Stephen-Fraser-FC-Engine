// **************************************************************************************************************
// ***																							 			  ***
// ***  FC_BitmapTypes - Part of FC_Bitmap, code to handle each of the different types (RGB, YUV, DXT1, etc)  ***
// ***																										  ***
// **************************************************************************************************************
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <fcio.h>

#define maxBitmapTypes 16						// Maximum number of bitmap data types to support
extern sBitmapType BitmapType[maxBitmapTypes];
extern uintf	NumBitmapTypes;

void datatypeerror(void)
{	msg("Error Initializing Bitmap System","The data types system is generating corrupt data!");
}

// Table of Contents:
//  bitmap_DataTypeYUV	(Index 1)	// This comes first because so much is dependant on it's Luminance calculation
//	bitmap_DataTypeHSL	(Index 2)
//  bitmap_DataTypeRGB  (Index 0)
//	bitmap_DataTypeCMP	(Index 3)	// Compressed (ie: DXT1)
//
//
// ---------------------------------------------------------------------------------------------------------------------------
//
//	YCC <--> RGB Conversion    (YCC = Luminance, Chrominance Red, Chrominance Blue)
//
//  YCC is ideal for color compression, since the human eye has trouble seeing Chrominance detail
//  so you can downsample chrominance to half the resolution vertically and horizontally, and you can barely tell (if at all)
//
// ---------------------------------------------------------------------------------------------------------------------------

const char *YUVbitmapTypeName = "YUV";

intf YUV_RtoY[256];		// Red   to Luminance lookup table
intf YUV_GtoY[256];		// Green to Luminance lookup table
intf YUV_BtoY[256];		// Blue  to Luminance lookup table
intf YUV_RtoCr[256];	// Red	 to Red Chrominance lookup table
intf YUV_GtoCr[256];	// Green to Red Chrominance lookup table
intf YUV_BtoCr[256];	// Blue	 to Red Chrominance lookup table
intf YUV_RtoCb[256];	// Red	 to Blue Chrominance lookup table
intf YUV_GtoCb[256];	// Green to Blue Chrominance lookup table
intf YUV_BtoCb[256];	// Blue	 to Blue Chrominance lookup table

intf YUV_CrtoR[256];
intf YUV_CrtoG[256];
intf YUV_CbtoG[256];
intf YUV_CbtoB[256];

bitmap	*YUVCreate(const char *name, uintf width, uintf height, uintf flags)
{	uintf pixels=0;
	switch (flags & bitmap_DataInfoMask)
	{	case bitmap_YUV_411:  if (((width&3)==0)&&((height&3)==0)) pixels = width*height + ((width>>2)*(height>>2))*2; else msg("Error Creating YUV Image","Width and Height must be multiples of 4 for YUV_411"); break;
		case bitmap_YUV_422:  if (((width&1)==0)&&((height&1)==0)) pixels = width*height + ((width>>1)*(height>>1))*2; else msg("Error Creating YUV Image","Width and Height must be multiples of 2 for YUV_422"); break;
		case bitmap_YUV_444:  pixels = width*height*3; break;
		default: msg("Error in YUVCreate","Unknown YUV data"); break;
	}
	dword memneeded = sizeof(bitmap)+pixels;
	if (flags & bitmap_RenderTarget) memneeded+=height*sizeof(dword);
	byte *buf = fcalloc(memneeded,buildstr("New YUV Bitmap: %s",name));
	bitmap *dst = (bitmap *)buf;	buf+=sizeof(bitmap);
	dst->pixel = buf;				buf+=pixels;
	dst->palette = NULL;
	dst->renderinfo = NULL;
	dst->flags = flags | bitmap_memstruct;
	dst->width = width;
	dst->height = height;
	return dst;
}

void YUVSwizzleIn(bitmap *dst, uintf x, uintf y)
{	uintf xx, yy,halfwidth;
	uint32 *srcpix = swizzleBuffer;
	byte *YPlane,*CbPlane,*CrPlane;
	uintf width = 16;
	uintf height = 16;
	uintf planesize;
	if (x+width>dst->width) width = dst->width-x;				
	if (y+height>dst->height) height = dst->height-y;		
	switch (dst->flags & bitmap_DataInfoMask)
	{	
		case bitmap_YUV_411:		// YUV411
			halfwidth = dst->width>>2;
			YPlane = dst->pixel + y*dst->width+x;
			CbPlane = dst->pixel + dst->width * dst->height + (y>>2)*halfwidth + (x>>2);
			CrPlane = CbPlane + halfwidth * (dst->height>>2);
			for (yy=0; yy<height; yy++)
			{	for (xx=0; xx<width; xx++)
				{	byte *rgb = (byte *)&srcpix[xx];
					uintf r = rgb[2];
					uintf g = rgb[1];
					uintf b = rgb[0];
					YPlane[xx] = (byte)(YUV_RtoY[r]  + YUV_GtoY[g]  + YUV_BtoY[b]);
				}
				YPlane += dst->width;
				srcpix += 16;
			}
			srcpix = swizzleBuffer;
			for (yy=0; yy<height; yy+=4)
			{	uintf xxx=0;
				for (xx=0; xx<width; xx+=4)
				{	byte *rgb = (byte *)&srcpix[xx];
					uintf r = (rgb[2] + rgb[10] + rgb[130] + rgb[138])>>2;
					uintf g = (rgb[1] + rgb[ 9] + rgb[129] + rgb[137])>>2;
					uintf b = (rgb[0] + rgb[ 8] + rgb[128] + rgb[136])>>2;
					CbPlane[xxx] = (byte)(YUV_RtoCb[r] + YUV_GtoCb[g] + YUV_BtoCb[b] + 128);
					CrPlane[xxx] = (byte)(YUV_RtoCr[r] + YUV_GtoCr[g] + YUV_BtoCr[b] + 128);
					xxx++;
				}
				CbPlane += halfwidth;
				CrPlane += halfwidth;
				srcpix += 64;
			}
			break;

		case bitmap_YUV_422:		// YUV422
			halfwidth = dst->width>>1;
			YPlane = dst->pixel + y*dst->width+x;
			CbPlane = dst->pixel + dst->width * dst->height + (y>>1)*halfwidth + (x>>1);
			CrPlane = CbPlane + halfwidth * (dst->height>>1);
			for (yy=0; yy<height; yy++)
			{	for (xx=0; xx<width; xx++)
				{	byte *rgb = (byte *)&srcpix[xx];
					uintf r = rgb[2];
					uintf g = rgb[1];
					uintf b = rgb[0];
					YPlane[xx] = (byte)(YUV_RtoY[r]  + YUV_GtoY[g]  + YUV_BtoY[b]);
				}
				YPlane += dst->width;
				srcpix += 16;
			}
			srcpix = swizzleBuffer;
			for (yy=0; yy<height; yy+=2)
			{	uintf xxx=0;
				for (xx=0; xx<width; xx+=2)
				{	byte *rgb = (byte *)&srcpix[xx];
					uintf r = (rgb[2] + rgb[6] + rgb[66] + rgb[70])>>2;
					uintf g = (rgb[1] + rgb[5] + rgb[65] + rgb[69])>>2;
					uintf b = (rgb[0] + rgb[4] + rgb[64] + rgb[68])>>2;
					CbPlane[xxx] = (byte)(YUV_RtoCb[r] + YUV_GtoCb[g] + YUV_BtoCb[b] + 128);
					CrPlane[xxx] = (byte)(YUV_RtoCr[r] + YUV_GtoCr[g] + YUV_BtoCr[b] + 128);
					xxx++;
				}
				CbPlane += halfwidth;
				CrPlane += halfwidth;
				srcpix += 32;
			}
			break;

		case bitmap_YUV_444:		// YUV444
			planesize = dst->width * dst->height;
			YPlane = dst->pixel + y*dst->width + x;
			CbPlane = YPlane + planesize;
			CrPlane = CbPlane + planesize;
			for (yy=0; yy<height; yy++)
			{	for (xx=0; xx<width; xx++)
				{	uint32 rgb = srcpix[xx];
					uintf r = (rgb&0x00ff0000)>>16;
					uintf g = (rgb&0x0000ff00)>>8;
					uintf b = (rgb&0x000000ff);
					YPlane [xx] = (byte)(YUV_RtoY[r]  + YUV_GtoY[g]  + YUV_BtoY[b]);
					CbPlane[xx] = (byte)(YUV_RtoCb[r] + YUV_GtoCb[g] + YUV_BtoCb[b] + 128);
					CrPlane[xx] = (byte)(YUV_RtoCr[r] + YUV_GtoCr[g] + YUV_BtoCr[b] + 128);
				}
				YPlane += dst->width;
				CbPlane += dst->width;
				CrPlane += dst->width;
				srcpix += 16;
			}
			break;

		default:
			msg("Error in YUVSwizzleIn","Unknown YUV format");
			break;
	}
}

void YUVSwizzleOut(bitmap *src, uintf x, uintf y)
{	uintf xx, yy, halfwidth;
	uint32 *dstpix = swizzleBuffer;
	uintf width = 16;
	uintf height = 16;											
	uintf yofs = 0;
	uintf planesize;
	byte *YPlane,*CbPlane,*CrPlane;
	if (x+width>src->width) width = src->width-x;				
	if (y+height>src->height) height = src->height-y;		
	switch(src->flags & bitmap_DataInfoMask)
	{	
		case bitmap_YUV_411:
		{	halfwidth = src->width>>2;
			byte *srcpix = src->pixel + y*src->width + x;
			CbPlane = src->pixel + src->width * src->height + (y>>2)*halfwidth + (x>>2);
			CrPlane = CbPlane + halfwidth * (src->height>>2);
			for (yy=0; yy<height; yy++)
			{	for (xx=0; xx<width; xx++)
				{	intf y = srcpix[xx];
					intf cb= CbPlane[yofs+(xx>>2)];
					intf cr= CrPlane[yofs+(xx>>2)];
					intf R = y + YUV_CrtoR[cr];
					intf G = y - YUV_CbtoG[cb] - YUV_CrtoG[cr];
					intf B = y + YUV_CbtoB[cb];
					if ((R | G | B) & 0x7FFFFF00)
					{	// Clipping has occured
						if (R<0)	R = 0;
						if (G<0)	G = 0;
						if (B<0)	B = 0;
						if (R>255)	R = 255;
						if (G>255)  G = 255;
						if (B>255)	B = 255;
					}
					dstpix[xx] = 0xff000000 + (R<<16) + (G<<8) + B;
				}
				if ((yy&3)==3) yofs+=halfwidth;
				dstpix += 16;
				srcpix += src->width;
			}
			break;	
		}

		case bitmap_YUV_422:
		{	halfwidth = src->width>>1;
			byte *srcpix = src->pixel + y*src->width + x;
			CbPlane = src->pixel + src->width * src->height + (y>>1)*halfwidth + (x>>1);
			CrPlane = CbPlane + halfwidth * (src->height>>1);
			for (yy=0; yy<height; yy++)
			{	for (xx=0; xx<width; xx++)
				{	intf y = srcpix[xx];
					intf cb= CbPlane[yofs+(xx>>1)];
					intf cr= CrPlane[yofs+(xx>>1)];
					intf R = y + YUV_CrtoR[cr];
					intf G = y - YUV_CbtoG[cb] - YUV_CrtoG[cr];
					intf B = y + YUV_CbtoB[cb];
					if ((R | G | B) & 0x7FFFFF00)
					{	// Clipping has occured
						if (R<0)	R = 0;
						if (G<0)	G = 0;
						if (B<0)	B = 0;
						if (R>255)	R = 255;
						if (G>255)  G = 255;
						if (B>255)	B = 255;
					}
					dstpix[xx] = 0xff000000 + (R<<16) + (G<<8) + B;
				}
				if (yy&1) yofs+=halfwidth;
				dstpix += 16;
				srcpix += src->width;
			}
			break;	
		}

		case bitmap_YUV_444:		// YUV444
			planesize = src->width * src->height;
			YPlane = src->pixel + y*src->width + x;
			CbPlane = YPlane + planesize;
			CrPlane = CbPlane + planesize;
			for (yy=0; yy<height; yy++)
			{	for (xx=0; xx<width; xx++)
				{	intf y = YPlane[xx];
					intf cb= CbPlane[xx];
					intf cr= CrPlane[xx];
					intf R = y + YUV_CrtoR[cr];
					intf G = y - YUV_CbtoG[cb] - YUV_CrtoG[cr];
					intf B = y + YUV_CbtoB[cb];
					if ((R | G | B) & 0x7FFFFF00)
					{	// Clipping has occured
						if (R<0)	R = 0;
						if (G<0)	G = 0;
						if (B<0)	B = 0;
						if (R>255)	R = 255;
						if (G>255)  G = 255;
						if (B>255)	B = 255;
					}
					dstpix[xx] = 0xff000000 + (R<<16) + (G<<8) + B;
				}
				YPlane += src->width;
				CrPlane += src->width;
				CbPlane += src->width;
				dstpix += 16;
			}
			break;

		default:
			msg("Error in YUVSwizzleOut","Unknown YUV format");
			break;
	}
}

void InitBitmapTypeYUV(void)
{	// Initialize the YUV tables
	for (intf i=0; i<256; i++)
	{	YUV_RtoY[i]  =  (intf)((float)i * 0.299f);
		YUV_GtoY[i]  =  (intf)((float)i * 0.587f);
		YUV_BtoY[i]  =  (intf)((float)i * 0.114f);

		YUV_RtoCb[i] = -(intf)((float)i * 0.1687f);
		YUV_GtoCb[i] = -(intf)((float)i * 0.3313f);
		YUV_BtoCb[i] =  (intf)((float)i * 0.5000f);

		YUV_RtoCr[i] =  (intf)((float)i * 0.5000f);
		YUV_GtoCr[i] = -(intf)((float)i * 0.4187f);
		YUV_BtoCr[i] = -(intf)((float)i * 0.0813f);

		YUV_CrtoR[i] = (intf)((float)(i-128)*1.40200f);
		YUV_CbtoG[i] = (intf)((float)(i-128)*0.34414f);
		YUV_CrtoG[i] = (intf)((float)(i-128)*0.71414f);
		YUV_CbtoB[i] = (intf)((float)(i-128)*1.77200f);
	}
	sBitmapType YUV = {(char *)YUVbitmapTypeName, YUVCreate, YUVSwizzleIn, YUVSwizzleOut,NULL};
	if (addBitmapType(&YUV) != bitmap_DataTypeYUV) datatypeerror();
}

// YUV Service Functions
byte calcLuminance(dword RGB)
{	return (byte)(YUV_RtoY[(RGB & 0xff0000)>>16]  + YUV_GtoY[(RGB & 0x00ff00)>>8]  + YUV_BtoY[RGB & 0x0000ff]);
}

byte calcChrominanceR(dword RGB)
{	return (byte)(YUV_RtoCr[(RGB & 0xff0000)>>16]  + YUV_GtoCr[(RGB & 0x00ff00)>>8]  + YUV_BtoCr[RGB & 0x0000ff] + 128);
}

byte calcChrominanceB(dword RGB)
{	return (byte)(YUV_RtoCb[(RGB & 0xff0000)>>16]  + YUV_GtoCb[(RGB & 0x00ff00)>>8]  + YUV_BtoCb[RGB & 0x0000ff] + 128);
}


// ---------------------------------------------------------------------------------------------------------------------------
//
//	HSL <--> RGB Conversion    (HSL = Hue, Saturation, Lightness)
//
//  RGB cannot easily be set up as a color picker, since you need 3 dimensions to store color.  So, HSL is used instead.
//  A 2D Array is shown with L set at maximum, allowing the user to pick just H and S values.  Then a slider can be used
//  to specify Lightness.  HSL is still not the perfect solution for color picking, as it makes it hard to pick a good brown.
//
// ---------------------------------------------------------------------------------------------------------------------------

const char *HSLbitmapTypeName = "HSL";

bitmap	*HSLCreate(const char *name, uintf width, uintf height, uintf flags)
{	uintf pixels;
	pixels = width*height*3;
	dword memneeded = sizeof(bitmap)+pixels;
	byte *buf = fcalloc(memneeded,buildstr("New HSL Bitmap: %s",name));
	bitmap *dst = (bitmap *)buf;	buf+=sizeof(bitmap);
	dst->pixel = buf;				buf+=pixels;
	dst->palette = NULL;
	dst->renderinfo = NULL;
	dst->flags = flags | bitmap_memstruct;
	dst->width = width;
	dst->height = height;
	return dst;
}

void HSLSwizzleIn(bitmap *dst, uintf x, uintf y)
{	uint32 *srcpix = swizzleBuffer;
	uintf width = 16;
	uintf height = 16;											
	if (x+width>dst->width) width = dst->width-x;				
	if (y+height>dst->height) height = dst->height-y;		
	uintf planesize = dst->width * dst->height;
	byte *HPlane = dst->pixel;
	byte *SPlane = HPlane + planesize;
	byte *LPlane = SPlane + planesize;
	uintf ofs = y*dst->width + x;
	float3 HSL;
	uintf srcindex = 0;
	for (uintf yy=0; yy<height; yy++)
	{	for (uintf xx=0; xx<width; xx++)
		{	RGBtoHSL(srcpix[srcindex+xx],&HSL);
			HPlane[ofs+xx] = (byte)(HSL.x*255.0f);
			SPlane[ofs+xx] = (byte)(HSL.y*255.0f);
			LPlane[ofs+xx] = (byte)(HSL.z*255.0f);
		}
		ofs += dst->width;
		srcindex += 16;
	}
}

void HSLSwizzleOut(bitmap *src, uintf x, uintf y)
{	uint32 *dstpix = swizzleBuffer;
	uintf width = 16;
	uintf height = 16;											
	if (x+width>src->width) width = src->width-x;				
	if (y+height>src->height) height = src->height-y;		
	uintf planesize = src->width * src->height;
	byte *HPlane = src->pixel;
	byte *SPlane = HPlane + planesize;
	byte *LPlane = SPlane + planesize;
	uintf ofs = y*src->width + x;
	uintf dstindex = 0;
	for (uintf yy=0; yy<height; yy++)
	{	for (uintf xx=0; xx<width; xx++)
		{	float H = (float)HPlane[ofs+xx] / 255.0f;
			float S = (float)SPlane[ofs+xx] / 255.0f;
			float L = (float)LPlane[ofs+xx] / 255.0f;
			dstpix[dstindex+xx] = HSLtoRGB(H,S,L);
			
		}
		ofs += src->width;
		dstindex+=16;
	}
}

void InitBitmapTypeHSL(void)
{	sBitmapType HSL = {(char *)HSLbitmapTypeName, HSLCreate, HSLSwizzleIn, HSLSwizzleOut,NULL};
	if (addBitmapType(&HSL) != bitmap_DataTypeHSL) datatypeerror();
}

uint32 HSLtoRGB(float H, float S, float L)
{	float var_r, var_g, var_b;
	
    float var_h = H * 5.9999f;
	uintf var_i = (uintf)var_h;
	float var_1 = L * ( 1.0f - S );
	float var_2 = L * ( 1.0f - S * ( var_h - (float)var_i ) );
	float var_3 = L * ( 1.0f - S * ( 1.0f - ( var_h - (float)var_i ) ) );

	if      ( var_i == 0 ) { var_r = L     ; var_g = var_3 ; var_b = var_1 ;}
	else if ( var_i == 1 ) { var_r = var_2 ; var_g = L     ; var_b = var_1 ;}
	else if ( var_i == 2 ) { var_r = var_1 ; var_g = L     ; var_b = var_3 ;}
	else if ( var_i == 3 ) { var_r = var_1 ; var_g = var_2 ; var_b = L     ;}
	else if ( var_i == 4 ) { var_r = var_3 ; var_g = var_1 ; var_b = L     ;}
	else                   { var_r = L     ; var_g = var_1 ; var_b = var_2 ;}
			
	return 0xff000000 + ((uintf)(var_r * 255)<<16) + ((uintf)(var_g * 255)<<8) + (uintf)(var_b * 255);
}

bitmap *buildHSLgrid(float L, uintf width, uintf height,bitmap *bm)
{	if (!bm) bm = newbitmap("HSL Grid",width, height, bitmap_RGB_32bit);
	uint32 *pix32 = (uint32*)bm->pixel;
	float S=1.0f;
	float H=0.0f;
	float deltaS =-1.0f / (float)height;
	float deltaH = 1.0f / (float)width;
	for (uintf y=0; y<height; y++)
	{	H = 0;
		for (uintf x=0; x<width; x++)
		{	*pix32++ = HSLtoRGB(H,S,L);
			H += deltaH;
		}
		S += deltaS;
	}
	return bm;
}

bitmap *buildHSL_Lcolumn(float H, float S, uintf width, uintf height, bitmap *bm)
{	if (bm)
	{	if (bm->width != width || bm->height!=height) 
		{	deleteBitmap(bm)
			bm = NULL;
		}
	}
	if (!bm) 
	{	bm = newbitmap("HSL L Column",width, height, bitmap_RGB_32bit);
	}
	uint32 *pix32 = (uint32 *)bm->pixel;
	float deltaL =-1.0f / height;
	float L=1.0f;
	for (uintf y=0; y<height; y++)
	{	uint32 col = HSLtoRGB(H,S,L);
		for (uintf x=0; x<width; x++)
			*pix32++ = col;
		L+= deltaL;
	}
	return bm;
}

void RGBtoHSL(uint32 RGB, float3 *HSL)
{	float var_R = ( (float)((RGB & 0xff0000)>>16) / 255.0f );
	float var_G = ( (float)((RGB & 0x00ff00)>> 8) / 255.0f );
	float var_B = ( (float)((RGB & 0x0000ff)    ) / 255.0f );

	float var_Min = var_R; if (var_G < var_Min) var_Min = var_G; if (var_B < var_Min) var_Min = var_B;
	float var_Max = var_R; if (var_G > var_Max) var_Max = var_G; if (var_B > var_Max) var_Max = var_B;
	float del_Max = var_Max - var_Min;

	HSL->z = var_Max;

	if ( del_Max <= 0.01f )
	{	HSL->x = 0;
		HSL->y = 0;
	}	else
	{	HSL->y = del_Max / var_Max;

		float del_R = ( ( ( var_Max - var_R ) / 6 ) + ( del_Max / 2 ) ) / del_Max;
		float del_G = ( ( ( var_Max - var_G ) / 6 ) + ( del_Max / 2 ) ) / del_Max;
		float del_B = ( ( ( var_Max - var_B ) / 6 ) + ( del_Max / 2 ) ) / del_Max;

		if		( (fabs)(var_B - var_Max )<0.01) HSL->x = ( 2.0f / 3.0f ) + del_G - del_R;
		else if ( (fabs)(var_G - var_Max )<0.01) HSL->x = ( 1.0f / 3.0f ) + del_R - del_B;
		else if ( (fabs)(var_R - var_Max )<0.01) HSL->x = del_B - del_G;
		
		if ( HSL->x < 0.0f ) HSL->x = 0.0f;
		if ( HSL->x > 1.0f ) HSL->x = 1.0f;
		if ( HSL->y < 0.0f ) HSL->y = 0.0f;
		if ( HSL->y > 1.0f ) HSL->y = 1.0f;
		if ( HSL->z < 0.0f ) HSL->z = 0.0f;
		if ( HSL->z > 1.0f ) HSL->z = 1.0f;
	}
}

// ---------------------------------------------------------------------------------------------------------------------------
//														bitmap_DataTypeRGB
// ---------------------------------------------------------------------------------------------------------------------------
const char *RGBbitmapTypeName = "RGB";

bitmap *RGBCreate(const char *name, uintf width, uintf height, uintf flags)
{	dword bpp = 0;
	switch (flags & bitmap_DataInfoMask)
	{	case bitmap_RGB_8bit:  bpp=1; break;
		case bitmap_RGB_16bit: bpp=2; break;
		case bitmap_RGB_32bit: bpp=4; break;
	}
	if (bpp==0)
	{	// Create a bitmap the same depth as the display screen
		if (screenbpp==16)
		{	flags |= bitmap_RGB_16bit;
			bpp = 2;
		}	else
		{	flags |= bitmap_RGB_32bit;
			bpp = 4;
		}
	}
	dword memneeded = sizeof(bitmap)+width*height * bpp;
	if (flags & bitmap_RenderTarget) memneeded+=height*sizeof(dword);
	byte *buf = fcalloc(memneeded,buildstr("New Bitmap: %s",name));
	bitmap *dst = (bitmap *)buf;	buf+=sizeof(bitmap);
	dst->pixel = buf;				buf+=width*height*bpp;
	dst->palette = NULL;
	if (flags & bitmap_RenderTarget)
	{	dst->renderinfo = (void *)buf; buf+=height*sizeof(uintf);
		uint32 *ri = (uint32 *)dst->renderinfo;
		for (uintf i=0; i<height; i++)
			ri[i]=i*width*bpp;
	}	else
		dst->renderinfo = NULL;
	dst->flags = flags | bitmap_memstruct;
	dst->width = width;
	dst->height = height;
	return dst;
}

void RGBSwizzleIn(bitmap *dst, uintf x, uintf y)
{	uintf xx, yy;
	uint32 *srcpix = swizzleBuffer;
	uintf width = 16;
	uintf height = 16;											
	if (x+width>dst->width) width = dst->width-x;				
	if (y+height>dst->height) height = dst->height-y;		
	switch(dst->flags & bitmap_DataInfoMask)
	{	case bitmap_RGB_8bit:
		{	uint8 *dstpix = dst->pixel + (y*dst->width+x);
			if (dst->flags & bitmap_Alpha)
			{	// All Alpha
				for (yy=0; yy<height; yy++)
				{	for (xx=0; xx<width; xx++)
					{	uint32 col = srcpix[xx];
						dstpix[xx] = (uint8)(col>>24);
					}
					dstpix += dst->width;
					srcpix += 16;
				}
			}	else
			if (!dst->palette)
			{	// Greyscale
				for (yy=0; yy<height; yy++)
				{	for (xx=0; xx<width; xx++)
					{	uint32 col = srcpix[xx];
						dstpix[xx] = (byte)(YUV_RtoY[(col & 0xff0000)>>16]  + YUV_GtoY[(col & 0x00ff00)>>8]  + YUV_BtoY[col & 0x0000ff]);
					}
					dstpix += dst->width;
					srcpix += 16;
				}
			}
			// ### TODO: Swizzle in to a paletted bitmap
			break;
		}
		
		case bitmap_RGB_16bit:
		{	uint16 *dstpix = (uint16*)(dst->pixel + ((y*dst->width + x)<<1));
			if (dst->flags & bitmap_Alpha)
			{	if (dst->flags & bitmap_Alpha1Bit)
				{	// ARGB 1555
					for (yy=0; yy<height; yy++)
					{	for (xx=0; xx<width; xx++)
						{	uintf col = (uintf)srcpix[xx];
							dstpix[xx] = (uint16)(rgbto1555(col));
						}
						dstpix += dst->width;
						srcpix += 16;
					}	
				}	else
				{	// ARGB 4444
					for (yy=0; yy<height; yy++)
					{	for (xx=0; xx<width; xx++)
						{	uintf col = (uintf)srcpix[xx];
							dstpix[xx] = (uint16)(rgbto4444(col));
						}
						dstpix += dst->width;
						srcpix += 16;
					}	
				}
			}	else
			{	// RGB 565
				for (yy=0; yy<height; yy++)
				{	for (xx=0; xx<width; xx++)
					{	uintf col = (uintf)srcpix[xx];
						dstpix[xx] = (uint16)(rgbto565(col));
					}
					dstpix += dst->width;
					srcpix += 16;
				}
			}
			break;
		}
		
		case bitmap_RGB_32bit:
		{	uint32 *dstpix = (uint32*)(dst->pixel + ((y*dst->width + x)<<2));
			if (dst->flags & bitmap_Alpha)
			{	// 32 Bits with Alpha
				for (yy=0; yy<height; yy++)
				{	for (xx=0; xx<width; xx++)
					{	dstpix[xx] = srcpix[xx];
					}
					dstpix += dst->width;
					srcpix += 16;
				}
			}	else
			{	// 32 bits without Alpha
				for (yy=0; yy<height; yy++)
				{	for (xx=0; xx<width; xx++)
					{	dstpix[xx] = srcpix[xx] | 0xff000000;
					}
					dstpix += dst->width;
					srcpix += 16;
				}				
			}
			break;	
		}
	}	
}

//	***		findbesttexformat(adapter, tex_RGB_8bitPal,   D3DFMT_P8,       false, texfind_16bit, texfind_32bit);
//	***		findbesttexformat(adapter, tex_RGB_8bitAlpha, D3DFMT_A8,       true,  texfind_16bit, texfind_32bit);
//	***		findbesttexformat(adapter, tex_RGB_8bitLum,   D3DFMT_L8,       false, texfind_16bit, texfind_32bit);
//	***		findbesttexformat(adapter, tex_RGB_16bit565,  D3DFMT_R5G6B5,   false, texfind_16bit, texfind_32bit);
//	***		findbesttexformat(adapter, tex_RGB_16bit1555, D3DFMT_A1R5G5B5, true,  texfind_16bit, texfind_32bit);
//	***		findbesttexformat(adapter, tex_RGB_16bit4444, D3DFMT_A4R4G4B4, true,  texfind_16bit, texfind_32bit);
//	***		findbesttexformat(adapter, tex_RGB_32bitX888, D3DFMT_X8R8G8B8, false, texfind_32bit, texfind_16bit);
//	***		findbesttexformat(adapter, tex_RGB_32bit8888, D3DFMT_A8R8G8B8, true,  texfind_32bit, texfind_16bit);
//			findbesttexformat(adapter, tex_BUMP_WithLum,  D3DFMT_X8L8V8U8, false, texfind_bump,  texfind_giveup);
//			findbesttexformat(adapter, tex_BUMP_NoLum,    D3DFMT_V8U8,     false, texfind_bump,  texfind_giveup);
//	***		findbesttexformat(adapter, tex_COMP_DXT1,	  D3DFMT_DXT1,	   true,  texfind_16bit, texfind_32bit);

void RGBSwizzleOut(bitmap *src, uintf x, uintf y)
{	uintf xx, yy;
	uint32 *dstpix = swizzleBuffer;
	uintf width = 16;
	uintf height = 16;
	if (x+width>src->width) width = (src->width-x);
	if (y+height>src->height) height = (src->height-y);
	switch(src->flags & bitmap_DataInfoMask)
	{	case bitmap_RGB_8bit:
		{	uint8 *srcpix = src->pixel + (y*src->width+x);
			if (src->flags & bitmap_Alpha)
			{	// All Alpha
				for (yy=0; yy<height; yy++)
				{	for (xx=0; xx<width; xx++)
					{	uint32 col = srcpix[xx];
						dstpix[xx] = col<<24 | 0x00ffffff;
					}
					dstpix += 16;
					srcpix += src->width;
				}
			}	else
			if (!src->palette)
			{	// Greyscale
				for (yy=0; yy<height; yy++)
				{	for (xx=0; xx<width; xx++)
					{	uint32 col = srcpix[xx];
						dstpix[xx] = 0xff000000 | (col<<16) | (col<<8) | col;
					}
					dstpix += 16;
					srcpix += src->width;
				}
			}	else
			{	// 8 bit paletised
				uint32 *palcols = src->palette->col32;
				for (yy=0; yy<height; yy++)
				{	for (xx=0; xx<width; xx++)
					{	dstpix[xx] = palcols[srcpix[xx]];
					}
					dstpix += 16;
					srcpix += src->width;
				}
			}
			break;
		}
		
		case bitmap_RGB_16bit:
		{	uint16 *srcpix = (uint16*)(src->pixel + ((y*src->width + x)<<1));
			if (src->flags & bitmap_Alpha)
			{	if (src->flags & bitmap_Alpha1Bit)
				{	// 16 bit with 1 bit alpha  (1555)
					for (yy=0; yy<height; yy++)
					{	for (xx=0; xx<width; xx++)
						{	uintf col = (uintf)srcpix[xx];
							dstpix[xx] = 0xff000000 | (uint32)(rgbfrom1555(col));
						}
						dstpix += 16;
						srcpix += src->width;
					}
				}	else
				{	// 16 bit with 4 bits alpha	(4444)
					for (yy=0; yy<height; yy++)
					{	for (xx=0; xx<width; xx++)
						{	uintf col = (uintf)srcpix[xx];
							dstpix[xx] = 0xff000000 | (uint32)(rgbfrom4444(col));
						}
						dstpix += 16;
						srcpix += src->width;
					}				
				}
			}	else
			{	// 16 bit no alpha	(0565)
				for (yy=0; yy<height; yy++)
				{	for (xx=0; xx<width; xx++)
					{	uintf col = (uintf)srcpix[xx];
						dstpix[xx] = 0xff000000 | (uint32)(rgbfrom565(col));
					}
					dstpix += 16;
					srcpix += src->width;
				}
				break;
			}
		}
		
		case bitmap_RGB_32bit:
		{	uint32 *srcpix = (uint32*)(src->pixel + ((y*src->width + x)<<2));
			for (yy=0; yy<height; yy++)
			{	for (xx=0; xx<width; xx++)
				{	dstpix[xx] = srcpix[xx];
				}
				dstpix += 16;
				srcpix += src->width;
			}
			break;	
		}
	}
}

void InitBitmapTypeRGB(void)
{	sBitmapType RGB = {(char *)RGBbitmapTypeName, RGBCreate, RGBSwizzleIn, RGBSwizzleOut,NULL};
	if (addBitmapType(&RGB) != bitmap_DataTypeRGB) datatypeerror();
}

// ---------------------------------------------------------------------------------------------------------------------------
//										bitmap_DataTypeCompressed : bitmap_compDXT1
// ---------------------------------------------------------------------------------------------------------------------------
struct dxt1_block
{	word	col0;
	word	col1;
	byte	row[4];
};

// DXT1 compression/decompression tables
uintf div3LUT[256];
uintf div3x2LUT[256];
byte dxt1LUT0[4];	// DXT1 compression lookup table for when maximum luminance occurs in color_0
byte dxt1LUT1[4];	// DXT1 compression lookup table for when maximum luminance occurs in color_1
byte dxt1LUT2[4];	// DXT1 compression lookup table for when maximum luminance occurs in color_1
byte dxt1LUT3[4];	// DXT1 compression lookup table for when maximum luminance occurs in color_1

bitmap *DXT1Create(const char *name, uintf width, uintf height, uintf flags)
{	uintf memneeded = 0;
	uintf nummips = (flags & bitmap_NumMipMapMask) >> bitmap_NumMipMapShift;
	uintf mipwidth = width;
	uintf mipheight = height;
	for (uintf i=0; i<=nummips; i++)
	{	uintf blockwidth = (mipwidth+3)>>2;
		uintf blockheight = (mipheight+3)>>2;
		memneeded += blockwidth*blockheight * 8;
		mipwidth>>=1;
		mipheight>>=1;
		if (mipwidth<1) mipwidth=1;
		if (mipheight<1) mipheight=1;
	}

	byte *buf = fcalloc(sizeof(bitmap) + memneeded,buildstr("New Bitmap: %s",name));
	bitmap *dst = (bitmap *)buf;	buf+=sizeof(bitmap);
	dst->pixel = buf;				buf+=memneeded;
	dst->palette = NULL;
	dst->renderinfo = NULL;
	dst->flags = flags | bitmap_memstruct;
	dst->width = width;
	dst->height = height;
	return dst;
}

uintf DXT1GetMipMapSize(bitmap *pix, uintf miplevel)
{	uintf w = pix->width;
	uintf h = pix->height;
	for (uintf i=0; i<miplevel; i++)
	{	w >>= 1;
		h >>= 1;
		if (w<1) w=1;
		if (h<1) h=1;
	}
	return ((w+3)>>2) * ((h+3)>>2) * 8;
}

void DXT1SwizzleOut(bitmap *src,uintf x, uintf y)
{	dword col[4];
	uintf width = 16;
	uintf height = 16;
	if (x+width>src->width) width = (src->width-x);
	if (y+height>src->height) height = (src->height-y);
	uintf blockwidth = (width+3)>>2;
	uintf blockheight = (height+3)>>2;
	uintf imageblockwidth = (src->width+3)>>2;
	byte *srcblock = src->pixel + ( ( ( (y>>2)*imageblockwidth) + (x>>2) ) << 3);
	for (uintf yy=0; yy<blockheight; yy++)
	{	for (uintf xx=0; xx<blockwidth; xx++)
		{	dxt1_block *data = (dxt1_block *)(srcblock + (xx<<3));
			uint32 *dst = &swizzleBuffer[(yy<<6)+(xx<<2)];
			col[0] = rgbfrom565(data->col0);
			col[1] = rgbfrom565(data->col1);

			if (col[0]>col[1])
			{	col[2] = 0xff000000 |
					((div3x2LUT[(col[0]&0x00ff0000)>>16] + div3LUT[(col[1]&0x00ff0000)>>16]) << 16) |
					((div3x2LUT[(col[0]&0x0000ff00)>> 8] + div3LUT[(col[1]&0x0000ff00)>> 8]) <<  8) |
					((div3x2LUT[(col[0]&0x000000ff)    ] + div3LUT[(col[1]&0x000000ff)    ])      ) ;

				col[3] = 0xff000000 | 
					((div3x2LUT[(col[1]&0x00ff0000)>>16] + div3LUT[(col[0]&0x00ff0000)>>16]) << 16) |
					((div3x2LUT[(col[1]&0x0000ff00)>> 8] + div3LUT[(col[0]&0x0000ff00)>> 8]) <<  8) |
					((div3x2LUT[(col[1]&0x000000ff)    ] + div3LUT[(col[0]&0x000000ff)    ])      ) ;
			} else
			{	col[2] = 0xff000000 |
					((((col[0]&0x00fe0000)>>17) + ((col[1]&0x00fe0000)>>17)) << 16) |
					((((col[0]&0x0000fe00)>> 9) + ((col[1]&0x0000fe00)>> 9)) <<  8) |
					((((col[0]&0x000000fe)>> 1) + ((col[1]&0x000000fe)>> 1))      ) ;
		
				col[3] = 0;
			}
			for (uintf i=0; i<4; i++)
			{	dst[0] = col[(data->row[i]&0x03)   ];
				dst[1] = col[(data->row[i]&0x0c)>>2];
				dst[2] = col[(data->row[i]&0x30)>>4];
				dst[3] = col[(data->row[i]&0xc0)>>6];
				dst += 16;
			}
		}
		srcblock += imageblockwidth<<3;
	}
}

void DXT1SwizzleIn(bitmap *dst,uintf x, uintf y)
{	uintf width = 16;
	uintf height = 16;
	if (x+width>dst->width) width = (dst->width-x);
	if (y+height>dst->height) height = (dst->height-y);
	if (((width | height)&3)>0) PadSwizzleBuffer(width,height);
	uintf blockwidth = (width+3)>>2;
	uintf blockheight = (height+3)>>2;
	uintf imageblockwidth = (dst->width+3)>>2;
	byte *dstblock = dst->pixel + ( ( ( (y>>2)*imageblockwidth) + (x>>2) ) << 3);
	for (uintf yy=0; yy<blockheight; yy++)
	{	for (uintf xx=0; xx<blockwidth; xx++)
		{	byte *dxt = (byte *)(dstblock + (xx<<3));
			uint32 *src = &swizzleBuffer[(yy<<6)+(xx<<2)];
			word *wcols;
			dword *dcols;
			byte *intenselookup;
			byte minlum = 255;
			byte maxlum = 0;
			byte maxlumi = 0, minlumi = 0;
			byte index;
			byte lum[16];
			dword cols[16];

			// Calculate minimum and maximum luminance
			index = 0;
			bool needsalpha = false;
			byte numalpha = 0;
			for (uintf j=0; j<4; j++)
			{	for (uintf i=0; i<4; i++)
				{	cols[index] = src[i];
					if (cols[index] < 0x01000000) 
					{	needsalpha = true;
						numalpha++;
					}
					byte tmp = (byte)(YUV_RtoY[(cols[index]&0x00ff0000)>>16]  + YUV_GtoY[(cols[index]&0x0000ff00)>>8]  + YUV_BtoY[(cols[index]&0x000000ff)]);
					if (tmp>maxlum) { maxlum = tmp;	maxlumi = index; }
					if (tmp<minlum) { minlum = tmp; minlumi = index; }
					lum[index++] = tmp;
				}
				src += 16;
			}

			if (numalpha==16)
			{	minlum = 0;
			}
			// Obtain 565 variants of the minimum and maximum colors
			word mincol = (word)rgbto565(cols[minlumi]);
			word maxcol = (word)rgbto565(cols[maxlumi]);
	
			// Protect against Divide by 0
			if (minlum==maxlum || mincol==maxcol)	
			{	wcols = (word *)dxt;
				if (needsalpha)
				{	*wcols++    = 0;
					*wcols++    = 0xffff;
					dcols = (dword *)wcols;
					*dcols = 0xffffffff;
				}
				else
				{	*wcols++    = maxcol;
					*wcols++    = maxcol;
					dcols = (dword *)wcols;
					*dcols = 0x00;
				}
			}	else
			{	intf intensity;
				dword colbits = 0;
				uintf shiftbits = 0;
				float lumscale;

				// Write the color0 and color1 values (color0 must be integer-greater than color1
				wcols = (word *)dxt;
				if (needsalpha)
				{	// Calculate the multiplier scale
					lumscale = 3.999f / (float)(maxlum-minlum);
					if (maxcol>mincol)	// Just because luminance of one color is greater than luminance of another, doesn't mean the RGB value is
					{	intenselookup = dxt1LUT2;
						*wcols++ = mincol;
						*wcols++ = maxcol;
					}	else
					{	intenselookup = dxt1LUT3;
						*wcols++ = maxcol;
						*wcols++ = mincol;
					}
					for (index=0; index<16; index++)
					{	if (cols[index]<0x01000000)
						{	colbits |= 3 << shiftbits;
						}	else
						{	intensity = (intf)((float)(lum[index]-minlum)*lumscale); // 0.0 - 0.9 = 00		1.0 - 1.9 = 01
							colbits |= intenselookup[intensity]<<shiftbits;
						}
						shiftbits += 2;
					}
					dcols = (dword *)wcols;
					*dcols = colbits;
				}	else
				{	// Calculate the multiplier scale
					lumscale = 3.999f / (float)(maxlum-minlum);
					if (maxcol>mincol)	// Just because luminance of one color is greater than luminance of another, doesn't mean the RGB value is
					{	intenselookup = dxt1LUT0;
						*wcols++ = maxcol;
						*wcols++ = mincol;
					}	else
					{	intenselookup = dxt1LUT1;
						*wcols++ = mincol;
						*wcols++ = maxcol;
					}
					for (index=0; index<16; index++)
					{	intensity = (intf)((float)(lum[index]-minlum)*lumscale); // 0.0 - 0.9 = 00		1.0 - 1.9 = 01
						colbits |= intenselookup[intensity]<<shiftbits;
						shiftbits += 2;
					}
					dcols = (dword *)wcols;
					*dcols = colbits;
				}
			}
		}
		dstblock += imageblockwidth<<3;
	}
}

// ---------------------------------------------------------------------------------------------------------------------------
//												bitmap_DataTypeCompressed
// ---------------------------------------------------------------------------------------------------------------------------
const char *CMPbitmapTypeName = "Compressed";

bitmap *CMPCreate(const char *name, uintf width, uintf height, uintf flags)
{	switch (flags & bitmap_DataInfoMask)
	{	case bitmap_compDXT1:
			return DXT1Create(name,width,height,flags);
			break;
		default:
			msg("Bitmap Error","Unknown compression method");
			return NULL;
	}
}

void CMPSwizzleIn(bitmap *dst,uintf x, uintf y)
{	switch (dst->flags & bitmap_DataInfoMask)
	{	case bitmap_compDXT1:
			DXT1SwizzleIn(dst,x,y);
			break;
		default:
			msg("Bitmap Error","Unknown compression method");
	}
}

void CMPSwizzleOut(bitmap *src,uintf x, uintf y)
{	switch (src->flags & bitmap_DataInfoMask)
	{	case bitmap_compDXT1:
			DXT1SwizzleOut(src,x,y);
			break;
		default:
			msg("Bitmap Error","Unknown compression method");
	}
}

uintf CMPGetMipSize(bitmap *pix, uintf miplevel)
{	switch (pix->flags & bitmap_DataInfoMask)
	{	case bitmap_compDXT1:
			return DXT1GetMipMapSize(pix, miplevel);
			break;
		default:
			msg("Bitmap Error","Unknown compression method");
	}
	return 0;
}

void InitBitmapTypeCMP(void)
{	for (uintf i=0; i<256; i++)
	{	div3LUT[i] = (uintf)(i/3);
		div3x2LUT[i] = (uintf)((i*2)/3);
	}
	dxt1LUT0[0] = 1;	dxt1LUT0[1] = 3;	dxt1LUT0[2] = 2;	dxt1LUT0[3] = 0;
	dxt1LUT1[0] = 0;	dxt1LUT1[1] = 2;	dxt1LUT1[2] = 3;	dxt1LUT1[3] = 1;
	dxt1LUT2[0] = 0;	dxt1LUT2[1] = 2;	dxt1LUT2[2] = 2;	dxt1LUT2[3] = 1;
	dxt1LUT3[0] = 1;	dxt1LUT3[1] = 2;	dxt1LUT3[2] = 2;	dxt1LUT3[3] = 0;

	sBitmapType CMP = {(char *)CMPbitmapTypeName, CMPCreate, CMPSwizzleIn, CMPSwizzleOut, CMPGetMipSize};
	if (addBitmapType(&CMP) != bitmap_DataTypeCompressed) datatypeerror();
}

// =====================================================================================
// Initialization Code for BitmapTypes
void NULLBitmapTypeError(void)
{	msg("Non-Existant Plugin Error","A call was made to a Bitmap Data-Type Plugin which does not exist");
}

bitmap	*NULLCreate(const char *name, uintf width, uintf height, uintf flags)
{	NULLBitmapTypeError();
	return NULL;
}

void	NULLSwizzle(bitmap *,uintf x, uintf y)
{	NULLBitmapTypeError();
}

void initbitmaptypes(void)
{	// Initialize all types to point to invalid data
	sBitmapType BmType = {(char *)"",NULLCreate,NULLSwizzle,NULLSwizzle};
	for (uintf i=0; i<maxBitmapTypes; i++)
		memcopy(&BitmapType[NumBitmapTypes], &BmType, sizeof(sBitmapType));

	// Now initialize each of the predefined types
	InitBitmapTypeRGB();
	InitBitmapTypeYUV();
	InitBitmapTypeHSL();
	InitBitmapTypeCMP();
}
