// ****************************************************************************************
// ***								Bitmap Tools										***
// ***                              ------------										***
// ***																					***
// *** Initialization Dependancies: None												***
// ***																					***
// *** External Functions declared here:												***
// ***	bitmap *newbitmap(byte *image,const char *flname, dword flags)					***
// ***	bitmap *newbitmap(const char *flname,dword flags)								***
// ***	void drawbitmap(bitmap *pic,long x, long y)										***
// ***	void drawsprite(bitmap *pic,long x, long y)										***
// ***	void setrendertarget2d(bitmap *rt)												***
// ***																					***
// ****************************************************************************************
#include "fc_h.h"
#include <bucketSystem.h>

#define bitmapLoadMem (512*1024)		// Allocate 512kb for bitmap-loading buffer
// References to fc_color.cpp
void initbitmaptypes(void);

uintf bitmapPrefs;

byte *imageLoadingCell;
uintf imageLoadingSize;

uintf	NumBitmapTypes;
sBitmapType BitmapType[256];
uint32 swizzleBuffer[256];

// ********************************************************************************
// ***							End of Palette Code								***
// ********************************************************************************

class fcbitmap : public fc_bitmap
{	public:
	bitmap *newbitmap(const char *name, dword width, dword height, dword flags)		{return ::newbitmap(name, width,height,flags);};
	bitmap *newbitmap(byte *image,const char *flname, dword flags, dword datasize)	{return ::newbitmap(image,flname,flags,datasize);};
	bitmap *newbitmap(const char *flname,dword flags)			{return ::newbitmap(flname,flags);};
	bitmap *scalebitmap(bitmap *src,long bmwidth,long bmheight)	{return ::scalebitmap(src,bmwidth,bmheight);};
	bitmap *aspectscale(bitmap *img, uintf x, uintf y) 			{return ::aspectscale(img,x,y);};
	bitmap *readldf(const char *filename,Palette *pal)			{return ::readldf(filename,pal);};
	bitmaps *readmbm(const char *filename)						{return ::readmbm(filename);};
	long findcluster(bitmaps *mbm,const char *name)				{return ::findcluster(mbm,name);};
	void drawbitmap(bitmap *pic,long x, long y)					{::drawbitmap(pic,x,y);};
	void drawsprite(bitmap *pic,long x, long y)					{::drawsprite(pic,x,y);};
	void savebitmap(const char *filename, bitmap *bm)			{::savebitmap(filename,bm);};
	void _deleteBitmap(bitmap *bm)								{::_deleteBitmap(bm);};
} *OEMbitmap;

void initbitmap(byte *memPtr)
{	OEMbitmap = new fcbitmap;
	bitmapPrefs = 95;
	NumBitmapTypes = 0;
	initbitmaptypes();

	imageLoadingSize = bitmapLoadMem;
	imageLoadingCell = memPtr;
	_fcbitmap = OEMbitmap;
}

void killbitmap(void)
{	imageLoadingCell = NULL;
	delete OEMbitmap;
}

uintf addBitmapType(sBitmapType *newType)						// Add a new bitmap type.  Returns the index number of this bitmap type
{	if (NumBitmapTypes>255) msg("Too Many Bitmap DataTypes","There is a limit of 256 bitmap data types, which has been exceeded");
	memcopy(&BitmapType[NumBitmapTypes],newType, sizeof(sBitmapType));
	NumBitmapTypes++;
	return NumBitmapTypes-1;	
}

bitmap *newbitmap(const char *name, dword width, dword height, dword flags)
{	flags &= ~bitmap_memmask;	// Mask out unwanted memory masks
	uintf datatype = (flags & bitmap_DataTypeMask) >> bitmap_DataTypeShift;
	return BitmapType[datatype].Create(name, width, height, flags);
}

uintf GetBitmapMipSize(bitmap *pic, uintf miplevel)
{	uintf datatype = (pic->flags & bitmap_DataTypeMask) >> bitmap_DataTypeShift;
	if (BitmapType[datatype].GetMipSize) 
		return BitmapType[datatype].GetMipSize(pic, miplevel);
	else
		return 0;
}

void _deleteBitmap(bitmap *bm)
{	if (!bm) return;
	if ((bm->flags & bitmap_mempixels) && bm->pixel)
		fcfree(bm->pixel);
//	if ((bm->flags & bitmap_mempalette) && bm->palette)
//		deletepalette(bm->palette);
	if ((bm->flags & bitmap_memrendinfo) && bm->renderinfo)
		fcfree(bm->renderinfo);
	if (bm->flags & bitmap_memstruct) fcfree(bm);
}

bitmap *SwizzleBitmap(bitmap *source, uintf destflags)
{	destflags &= (bitmap_DataTypeMask | bitmap_DataInfoMask | bitmap_AlphaMask | bitmap_MiscMask);
	bitmap *swizzlebm = newbitmap("Swizzle Bitmap", source->width, source->height, destflags);
	uintf sourcetype = (source->flags & bitmap_DataTypeMask) >> bitmap_DataTypeShift;
	uintf desttype = (swizzlebm->flags & bitmap_DataTypeMask) >> bitmap_DataTypeShift;
	for (uintf y=0; y<source->height; y+=16)
		for (uintf x=0; x<source->width; x+=16)
		{	BitmapType[sourcetype].SwizzleOut(source,x,y);
			BitmapType[desttype].SwizzleIn(swizzlebm,x,y);
		}
	return swizzlebm;
}

void PadSwizzleBuffer(uintf width, uintf height)
{	uintf i;
	if ((height==0) || (width==0)) return;
	uint32 *src = swizzleBuffer + ((height-1)<<4);
	uint32 *dst = src+16;
	while (height<16)
	{	for (i=0; i<16; i++)
		{	*dst++=*src++;
		}
		height++;
	}
	
	src = swizzleBuffer + width-1;
	dst = src+1;
	while (width<16)
	{	for (i=0; i<16; i++)
		{	dst[i<<4] = src[i<<4];
		}
		width++;
		src++;
		dst++;
	}
}

bitmap *scalebitmap(bitmap *src,uintf bmwidth,uintf bmheight)
{	// Scale a bitmap to the specified size, result is returned in a new bitmap
	// This function is a highly accurate scale, and consumes a considerable amount of CPU time.
	if ((src->flags & (bitmap_DataInfoMask | bitmap_DataTypeMask)) != (bitmap_DataTypeRGB | bitmap_RGB_32bit)) msg("scalebitmap Error","scalebitmap can only operate on 32 bit RGB bitmaps");

	if (bmwidth <1) bmwidth  = 1;
	if (bmheight<1) bmheight = 1;

	bitmap *tmpbm,*dst;

	// First, we must scale src into a temp bitmap (tmpbm) along the X axis (Y is done seperately)
	if (bmwidth!=src->width)
	{	tmpbm = newbitmap("Scale Bitmap (step 1)",bmwidth,src->height,src->flags);
		if (bmwidth>src->width)
		{	dword srcyofs = 0;
			dword *srcptr = (dword *)src->pixel;
			dword *dstptr = (dword *)tmpbm->pixel;
			dword deltax = (((src->width-1)<<16) / (bmwidth-1));	// How much does X increment on the source
			for	(uintf y=0; y<src->height; y++)
			{	dword sx = 0;
				for (uintf x=0; x<bmwidth; x++)
				{	//if ((sx>>16)+1>bmwidth) continue;
					dword col1 = srcptr[srcyofs+(sx>>16)];
					dword col2 = srcptr[srcyofs+(sx>>16)+1];
					dword rightpix = sx & 0xffff;
					dword leftpix  = 0x10000-rightpix;
					sx+=deltax;

					dword a = (((col1&0xff000000)>>24) * leftpix + ((col2&0xff000000)>>24) * rightpix) >> 16;
					dword r = (((col1&0x00ff0000)>>16) * leftpix + ((col2&0x00ff0000)>>16) * rightpix) >> 16;
					dword g = (((col1&0x0000ff00)>> 8) * leftpix + ((col2&0x0000ff00)>> 8) * rightpix) >> 16;
					dword b = (((col1&0x000000ff)    ) * leftpix + ((col2&0x000000ff)    ) * rightpix) >> 16;
					*dstptr++ = (a<<24)+(r<<16)+(g<<8)+b;
				}
				srcyofs+=src->width;
			}
		}

		if (bmwidth<src->width)
		{	dword srcyofs = 0;
			dword *srcptr = (dword *)src->pixel;
			dword *dstptr = (dword *)tmpbm->pixel;
			dword deltax = (src->width<<16) / bmwidth;	// How much does X increment on the source
			for (uintf y=0; y<src->height; y++)
			{	dword sx = 0;
				for (uintf x=0; x<bmwidth; x++)
				{	dword divisor=65536;
					dword col = srcptr[srcyofs+(sx>>16)];
					dword leftpix = 0x10000-(sx & 0xffff);
					dword a = ((col&0xff000000)>>24) * leftpix; 
					dword r = ((col&0x00ff0000)>>16) * leftpix;
					dword g = ((col&0x0000ff00)>> 8) * leftpix;
					dword b = ((col&0x000000ff)    ) * leftpix;
					dword nextsx = sx+deltax;
					for (dword tmp = sx+0x10000; tmp<nextsx; tmp+=0x10000)
					{	col = srcptr[srcyofs+(tmp>>16)];
						a += ((col&0xff000000)>> 8);
						r += ((col&0x00ff0000)    );
						g += ((col&0x0000ff00)<< 8);
						b += ((col&0x000000ff)<<16);
						divisor+=65536;
					}
					col = srcptr[srcyofs+(nextsx>>16)];
					dword rightpix = sx & 0xffff;
					a += ((col&0xff000000)>>24) * rightpix;
					r += ((col&0x00ff0000)>>16) * rightpix;
					g += ((col&0x0000ff00)>> 8) * rightpix;
					b += ((col&0x000000ff)    ) * rightpix;
					a/=divisor;
					r/=divisor;
					g/=divisor;
					b/=divisor;
					*dstptr++ = (a<<24)+(r<<16)+(g<<8)+b;
					sx = nextsx;
				}
				srcyofs+=src->width;
			}
		}
	}	else
		tmpbm = src;

	// Now we must scale tmpbm into dst, scaling the Y axis
	if (bmheight!=src->height)
	{	dst = newbitmap("Scale Bitmap (step 2)",bmwidth,bmheight,src->flags);
		dword *mydst = (dword *)dst->pixel;
		for (uintf i=0; i<dst->width*dst->height; i++)
			mydst[i]=0xff0000;

		if (bmheight>tmpbm->height)
		{	dword *srcptr = (dword *)tmpbm->pixel;
			dword *dstptr = (dword *)dst->pixel;
			dword deltay = ((src->height-1)<<16) / (bmheight-1);	// How much does X increment on the source
			for	(uintf x=0; x<bmwidth; x++)
			{	dword sy = 0;
				dword writey = 0;
				for (uintf y=0; y<bmheight; y++)
				{	long readpos = (sy>>16)*bmwidth+x;
					dword col1 = srcptr[readpos];
					dword col2;
					if (readpos+bmwidth<tmpbm->height*bmwidth)
						col2 = srcptr[readpos+bmwidth];
					else   
						col2 = col1;
					dword rightpix = sy & 0xffff;
					dword leftpix  = 0x10000-rightpix;
					sy+=deltay;

					dword a = (((col1&0xff000000)>>24) * leftpix + ((col2&0xff000000)>>24) * rightpix) >> 16;
					dword r = (((col1&0x00ff0000)>>16) * leftpix + ((col2&0x00ff0000)>>16) * rightpix) >> 16;
					dword g = (((col1&0x0000ff00)>> 8) * leftpix + ((col2&0x0000ff00)>> 8) * rightpix) >> 16;
					dword b = (((col1&0x000000ff)    ) * leftpix + ((col2&0x000000ff)    ) * rightpix) >> 16;
					dstptr[writey+x] = (a<<24)+(r<<16)+(g<<8)+b;
					writey+=bmwidth;
				}
			}
		}

		if (bmheight<src->height)
		{	dword *srcptr = (dword *)tmpbm->pixel;
			dword *dstptr = (dword *)dst->pixel;
			dword deltay = (src->height<<16) / (bmheight+1);	// How much does X increment on the source
			for (uintf x=0; x<bmwidth; x++)
			{	dword sy = 0;
				dword col = srcptr[x];
				dword destofs = x;
				for (uintf y=0; y<bmheight; y++)
				{	dword divisor=65536;
					dword leftpix = 0x10000-(sy & 0xffff);
					dword a = ((col&0xff000000)>>24) * leftpix; 
					dword r = ((col&0x00ff0000)>>16) * leftpix;
					dword g = ((col&0x0000ff00)>> 8) * leftpix;
					dword b = ((col&0x000000ff)    ) * leftpix;
					dword nextsy = sy+deltay;
					for (dword tmp = sy+0x10000; tmp<nextsy; tmp+=0x10000)
					{	col = srcptr[x+(tmp>>16)*bmwidth];
						a += ((col&0xff000000)>> 8);
						r += ((col&0x00ff0000)    );
						g += ((col&0x0000ff00)<< 8);
						b += ((col&0x000000ff)<<16);
						divisor+=65536;
					}
					col = srcptr[x+(nextsy>>16)*bmwidth];
					dword rightpix = sy & 0xffff;
					a += ((col&0xff000000)>>24) * rightpix;
					r += ((col&0x00ff0000)>>16) * rightpix;
					g += ((col&0x0000ff00)>> 8) * rightpix;
					b += ((col&0x000000ff)    ) * rightpix;
					a/=divisor;
					r/=divisor;
					g/=divisor;
					b/=divisor;
					dstptr[destofs] = (a<<24)+(r<<16)+(g<<8)+b;
					destofs+=bmwidth;
					sy = nextsy;
				}
			}
		}
	}	else dst = tmpbm;

	if (src==dst)
	{	// The new bitmap has the same dimensions, we must duplicate the original bitmap
		dst = newbitmap("ScaleBitmap (Duplicate)",src->width,src->height,src->flags);
		memcopy(dst->pixel,src->pixel,src->width*src->height<<2);
	}

	if (tmpbm!=src && tmpbm!=dst) deleteBitmap(tmpbm);
	return dst;	
}

bitmap *aspectscale(bitmap *img, uintf x, uintf y)
{	bitmap *result;
	float curaspect = (float)img->width / (float)img->height;
	if ((float)x / curaspect <= y)
	{	result = scalebitmap(img,x,(intf)((float)x / curaspect));
	}	else
	{	result = scalebitmap(img,(intf)((float)y*curaspect),y);
	}
	return result;
}

bitmap *readldf(const char *filename,Palette *pal)
{	uintf numbitmaps, index,i;
	long width,height,memneeded;

	// Step 1 - Load LDF file into memory
	byte *ldf = fileLoad(filename, genflag_usetempbuf);
	
	// Step 2 - Scan data to find number of bitmaps, and amount of required memory
	numbitmaps = 0;
	memneeded = 0;
	index = 0;
	while (index<filesize)
	{	numbitmaps++;
		width  = ldf[index  ] + (ldf[index+1]<<8);
		height = ldf[index+2] + (ldf[index+3]<<8);
		memneeded += width*height;
		index += 4 + width*height;
	}
	if (index>filesize)
		msg("File Corrupt",buildstr("LDF File '%s' is corrupt",filename));

	// Step 3 - Allocate memory and fill it with data
	memneeded+=sizeof(bitmap)*numbitmaps;
	byte *buf = fcalloc(memneeded,buildstr("LDF Bitmaps %s",filename));
	bitmap *bitmaps = (bitmap *)buf;	buf+=sizeof(bitmap)*numbitmaps;
	index = 0;
	i = 0;
	while (i<numbitmaps)
	{	bitmaps[i].width  = width  = ldf[index  ] + (ldf[index+1]<<8);
		bitmaps[i].height = height = ldf[index+2] + (ldf[index+3]<<8);
		bitmaps[i].flags  = bitmap_DataTypeRGB | bitmap_RGB_8bit;
		bitmaps[i].palette= pal;
		bitmaps[i].pixel  = buf;
		memcopy(buf,ldf+index+4,width*height);
		index+= 4 + width*height;
		buf+=width*height;
		i++;
	}
	freetempbuffer(ldf);
	bitmaps->flags |= bitmap_memstruct;
	return bitmaps;
}

#define mbm_8bit			0x00000001	// Bitmap is 8 bit paletised
#define mbm_16bit			0x00000002	// Bitmap is 16 bit ARGB
#define mbm_32bit			0x00000003	// Bitmap is 32 bit ARGB
#define mbm_bppmask			0x00000003	// Mask to indicate bit depth of the image 
#define mbm_alpha			0x00000004	// Bitmap contains valid alpha data (otherwise, all alpha is 0xff)
#define mbm_alpha1bit		0x00000008	// Bitmap alpha channel is only 1 bit (only valid if bitmap_alpha)
#define mbm_allalpha		0x00000010	// Bitmap is entirely alpha data (no color)
#define mbm_alphamask		0x0000001C	// Mask to indicate all alpha bits

bitmaps *readmbm(const char *filename)
{	
	struct MBMheader
	{	word		TextPoolSize;
		word		NumBitmapClusters;
		word		NumPalettes;
		word		NumBitmaps;
		dword		PixelMemory;
	} mbmhdr;

	sFileHandle *handle = fileOpen(filename);
	fileRead(handle,&mbmhdr,sizeof(MBMheader));
	//con->add("TextPoolSize = %i, NumClusters = %i, NumPalettes = %i, NumBitmaps = %i, PixelMemory = %i",
	//		mbmhdr.TextPoolSize, mbmhdr.NumBitmapClusters, mbmhdr.NumPalettes, mbmhdr.NumBitmaps,mbmhdr.PixelMemory);
	//con->add("");
	
	long memneeded = sizeof(bitmaps)+mbmhdr.TextPoolSize+mbmhdr.NumBitmapClusters*(4+ptrsize*2)+
						mbmhdr.NumPalettes*sizeof(Palette)+mbmhdr.NumBitmaps*sizeof(bitmap) + 
						mbmhdr.PixelMemory;
	
	byte *buf = fcalloc(memneeded,buildstr("Multiple BitMap file %s",filename));
	bitmaps *mbm = (bitmaps *)buf;		buf+=sizeof(bitmaps);
	char *names = (char *)buf;			buf+=mbmhdr.TextPoolSize;
	mbm->clustername = (char **)buf;	buf+=mbmhdr.NumBitmapClusters*ptrsize;
	mbm->clusterbitmap= (bitmap *)buf;	buf+=mbmhdr.NumBitmapClusters*ptrsize;
	mbm->clustersize = (long *)buf;		buf+=4*mbmhdr.NumBitmapClusters;
	bitmap *bmap = (bitmap *)buf;		buf+=sizeof(bitmap)*mbmhdr.NumBitmaps;
	Palette *pal = (Palette *)buf;		buf+=sizeof(Palette)*mbmhdr.NumPalettes;
	byte *pixelbuf = buf;				buf+=mbmhdr.PixelMemory;

	mbm->numclusters = mbmhdr.NumBitmapClusters;
	mbm->totalbitmaps= mbmhdr.NumBitmaps;

	word tmp;
	long nextpalette=0;
	long nextbitmap=0;

	// Read in the text pool
	fileRead(handle,names,mbmhdr.TextPoolSize);
	long ofs = 0;
	for (long name=0; name<mbmhdr.NumBitmapClusters; name++)
	{	mbm->clustername[name]=names+ofs;
		ofs+=txtlen(names+ofs)+1;
	}

	for (long cluster=0; cluster<mbmhdr.NumBitmapClusters; cluster++)
	{	// Read in each cluster
		//con->add("Cluster %i: %s",cluster,mbm->clustername[cluster]);
		mbm->clusterbitmap = &bmap[nextbitmap];
		fileRead(handle,&tmp,2);	// numbitmaps
		mbm->clustersize[cluster]=tmp;
		fileRead(handle,&tmp,2);	// flags
		dword flags=bitmap_DataTypeRGB;//tmp & mbm_bppmask;
		dword pixelsize = 0;
		if ((tmp & mbm_bppmask)==mbm_8bit ) {	pixelsize=1;	flags |= bitmap_RGB_8bit;	}
		if ((tmp & mbm_bppmask)==mbm_16bit) {	pixelsize=2;	flags |= bitmap_RGB_16bit;	}
		if ((tmp & mbm_bppmask)==mbm_32bit) {	pixelsize=4;	flags |= bitmap_RGB_32bit;	}

		// ### TODO: Check for pixelsize==0 -> Error

		if (tmp & mbm_alpha) flags |= bitmap_Alpha;
		if (tmp & mbm_alpha1bit) flags |= bitmap_Alpha1Bit;
		if ((tmp & mbm_bppmask) == mbm_8bit)
		{	fileRead(handle,&pal[nextpalette].col32,1024);	// palette
			nextpalette++;
			//con->add("Cluster is paletized");
		}	//else
			//con->add("Cluster is not paletized");
		
		// Now read in the bitmaps
		for (long i=0; i<mbm->clustersize[cluster]; i++)
		{	bitmap *bm = &bmap[nextbitmap];
			if ((tmp & mbm_bppmask)==mbm_8bit)
				bm->palette = &pal[nextpalette-1];
			else
				bm->palette = NULL;
			bm->flags = flags;
			
			fileRead(handle,&tmp,2);	// width
			bm->width = tmp;
			fileRead(handle,&tmp,2);	// height
			bm->height = tmp;
			bm->pixel = pixelbuf;
			fileRead(handle,pixelbuf,bm->width * bm->height*pixelsize);
			pixelbuf += bm->width * bm->height * pixelsize;
			nextbitmap++;
		}
	}

	fileClose(handle);
	return mbm;
}

long findcluster(bitmaps *mbm,const char *name)
{	for (long i=0; i<mbm->numclusters; i++)
		if (txticmp(mbm->clustername[i],name)==0) return i;
	return -1;
}

// ----------------------------------- Image Readers --------------------------------
char CurrentBitmapName[128];										// Filename of bitmap currently being imported
char *openimagestring=pluginfilespecs[PluginType_ImageReader];		// '*.TGA;' is 6 characters

bitmap *newbitmap(byte *image,const char *flname, dword flags, dword datasize)
{	fileNameInfo(flname);
	tprintf(CurrentBitmapName,sizeof(CurrentBitmapName),"%s.%s",fileactualname,fileextension);
	bitmap *dest = NULL;

	if (!image)
		msg("Illegal Operation","newbitmap (memory-based file) called with a NULL image pointer");

	const char *ext = flname;
	const char *fln = flname;
	// Scan through the filename to find the extension.
	while (*fln!=0)
	{	if (*fln=='.') ext = fln;	
		fln++;
	}
	
	bitmap *(*importer)(byte *,dword,dword) = (bitmap *(*)(byte *,dword,dword))getPluginHandler(ext, PluginType_ImageReader);
	if (importer)
	{	dest = importer(image,flags,datasize);
	}
	
	if (dest==NULL)	
		msg("Image File Error",buildstr("%s is either an unknown, or invalid 2D image file",flname));
	
	// Create a greyscale version of the data into the Alpha channel (RGB32 images only)
	if ((dest->flags & (bitmap_DataTypeMask | bitmap_DataInfoMask | bitmap_AlphaMask))==bitmap_RGB32)
	{	dest->flags |= bitmap_GreyscaleInAlpha;
		uintf numPixels = dest->width * dest->height;
		uint32 *pix = (uint32*)dest->pixel;
		for (uintf i=0; i<numPixels; i++)
		{	uint32 col = *pix;
			uint32 alpha = calcLuminance(col);
			*pix++ = (col&0x00ffffff) | (alpha<<24);
		}
	}

	dest->renderinfo = NULL;
	return dest;
}

bitmap *newbitmap(const char *flname,uintf flags)
{	fileNameInfo(flname);
	tprintf(CurrentBitmapName,sizeof(CurrentBitmapName),"%s.%s",fileactualname,fileextension);
	bitmap *dest = NULL;

//	const char *ext=flname;
//	const char *fln = flname;
//	// Scan through the filename to find the extension.
//	while (*fln!=0)
//	{	if (*fln=='.') ext = fln;	
//		fln++;
//	}
	
	bitmap *(*importer)(byte *,uintf,uintf) = (bitmap *(*)(byte *,uintf,uintf))getPluginHandler(fileextension, PluginType_ImageReader);
	if (importer)
	{	byte *image = fileLoad(flname);
		dest = importer(image,flags,filesize);
		fcfree(image);
	}

	if (dest==NULL)	
		msg("Image File Error",buildstr("%s is either an unknown, or invalid 2D image file",flname));
	return dest;
}

void savebitmap(const char *flname, bitmap *bm)
{	txtcpy(CurrentBitmapName,sizeof(CurrentBitmapName),flname);
	if ((bm->flags & bitmap_DataTypeMask)!=bitmap_DataTypeRGB) 
		msg("SaveBitmap Error","SaveBitmap only supports RGB images");
	const char *ext=flname;
	const char *fln = flname;
	// Scan through the filename to find the extension.
	while (*fln!=0)
	{	if (*fln=='.') ext = fln;	
		fln++;
	}
	
	void (*exporter)(const char *,bitmap *) = (void (*)(const char *,bitmap *))getPluginHandler(ext, PluginType_ImageWriter);
	if (exporter)
	{	exporter(flname,bm);
	}	else
	{	msg("Image Save Error",buildstr("%s is not supported for saving bitmaps",flname));
	}
}

sEngineModule mod_bitmap =
{	"bitmap",					// Module Name
	bitmapLoadMem,				// Memory needed for bitmap-loading memory cell
	initbitmap,
	killbitmap
};
