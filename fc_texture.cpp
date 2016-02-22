/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
// *****************************************************************
// ***						Texture Map Handling				 ***
// *****************************************************************
//
// TODO:	Allow manual handling of mipmaps
//
#include "fc_h.h"
#include <bucketSystem.h>

#define texturealloc 128							// number textures to allocate in a single hit
#define rt3Dalloc	 16								// number of 3D render targets to allocate in a single hit

#define texture_dontlog		texture_reserved1		// Don't send information to the textures log
#define texture_inuse		texture_reserved2		// This texture is in use (just a double-safety, in case you delete a texture twice)
#define texture_indriver	texture_reserved3		// This texture has been loaded into the video driver

class fctexture : public fc_texture
{	public:
	void setTexturePath(const char *path)			{::setTexturePath(path);};
	void _deletetexture(Texture *tex)				{::_deletetexture(tex);};
	void deletealltextures(void)					{::deletealltextures();};
	Texture *copytexture(Texture *tex)				{return ::copytexture(tex);};
	Texture *newTextureFromFile(const char *texname,uintf flags)	{return ::newTextureFromFile(texname,flags);};
	Texture *getfirsttexture(void)					{return ::getfirsttexture();};
} *OEMtexture = NULL;

extern void (*newtextures)(Texture *tex, dword numtex);
extern void (*vd_newRTTexture)(Texture *tex, uintf width, uintf height, uintf flags);
extern void (*vd_create3DRenderTarget)(sRenderTarget3D *rt, intf width, intf height, uintf flags);

char texpath[texpathsize];
logfile *texturelog;
Texture *freeTexture;
Texture *usedTexture;
uintf	estimatedtexmemused;

uintf maxtexwidth, maxtexheight, texturechannels;
uintf videoFeatures;

fc_texture *inittexture(void)
{	videoinfostruct vi;

	// Initialize texturemap log file
#ifdef fcdebug
	texturelog = newlogfile("logs/textures.log");
#else
	texturelog = newlogfile(NULL);
#endif
	// Initialize texture cache
	freeTexture = NULL;
	usedTexture = NULL;
	// Obtain 3D renderer's capabilities
	getvideoinfo(&vi,sizeof(vi));
	maxtexwidth = vi.MaxTextureWidth;
	maxtexheight= vi.MaxTextureHeight;
	texturechannels=vi.NumTextureUnits;
	estimatedtexmemused = 0;
	videoFeatures = vi.Features;
	// Create a default blank texture
	OEMtexture = new fctexture;
	return OEMtexture;
}

void killtexture(void)
{	if (!OEMtexture)
		return;					// OEMtexture not defined ... we haven't yet been initialized
	deletealltextures();
//	killbucket(Texture, flags, texture_memstruct);
{	Texture *tmpTexture, *nextTexture;
	while (usedTexture)
	{	tmpTexture = usedTexture;
		deletebucket(Texture, tmpTexture)
	}
	tmpTexture = freeTexture;
	while (tmpTexture)
	{	nextTexture = tmpTexture->next;
		if (tmpTexture->flags & Generic_MemoryFlag32)
		{	tmpTexture->next = usedTexture;
			usedTexture = tmpTexture;
		}
		tmpTexture = nextTexture;
	}
	tmpTexture = usedTexture;
	while (tmpTexture)
	{	nextTexture = tmpTexture->next;
		fcfree(tmpTexture);
		tmpTexture = nextTexture;
	}
}


	texturelog->log("Estimated Texture memory used = %i Megs",estimatedtexmemused / (1024*1024));
	delete texturelog;
	delete OEMtexture;
}

void setTexturePath(const char *path)			// Sets the texture map search path, directories are seperated by ';' characters, maximum of 1024 characters.  Each path must end in a foreward flash (Backslashes will work under Windows, but are not guaranteed to always work on other platforms).
{	txtcpy(texpath,texpathsize,path);
}

Texture *newTexture(const char *name, uintf value, uintf flags)			// value = depth for volume textures, and dimensions for cubemap textures
{	// This is the base 'newtexture' function which all others call
	Texture *tex;
	char flname[1024];

	if (name)
	{	if ((flags & texture_currentImporter) && (name[0]))
		{	// Try to find a texture with a matching name and return a pointer to that
			fileNameInfo(name);
			tprintf(flname,sizeof(flname),"%s.%s",fileactualname,fileextension);
			Texture *tex = usedTexture;
			while (tex)
			{	if (txticmp(flname,tex->name)==0)
				{	// Texture already loaded
					tex->refcount++;
					return tex;
				}
				tex = tex->next;
			}
		}
	}

	allocbucket(Texture,tex,flags,Generic_MemoryFlag32,texturealloc,"Texture Cache");
	tex->flags |= flags;
	if (!name)
		tprintf(tex->name,sizeof(tex->name),"Unnamed (0x%04x)",(long)tex);				// ### Typecast pointer to int
	else
		txtcpy(tex->name, sizeof(tex->name), name);
	// if (!(tex->flags & texture_dontlog)) texturelog->log("Created 0: %s",tex->name);
	tex->refcount = 1;
	tex->oemdata=NULL;
	tex->texmemused = 0;
	tex->flags |= flags | texture_inuse;
	tex->UVscale.x = 1.0f;
	tex->UVscale.y = 1.0f;
	tex->mapping = texmapping_default;
	switch (flags & texture_typemask)
	{	case texture_2D:
			tex->depth = 0;
			break;
		case texture_cubemap:
			tex->width = value;
			tex->height = value;
			tex->depth = value;
			break;
		case texture_volumemap:
			tex->depth = value;
			break;
	}
	tex->levels = 0;
	return tex;
}

bitmap *bm_MissingTexture = NULL;

void textureLoad(Texture *tex)
{	bitmap *bm;
	char flname[1024];

//	tex->flags &= texture_clearmask;
//	tex->flags |= flags;

//	texturelog->log("Attempting to load texture %s",tex->name);

	if (fileExists(tex->name))										// Check to see if the path + filename is valid ...
		txtcpy(flname, sizeof(flname), tex->name);					// if so, load that
	else
		fileFindInPath(flname, sizeof(flname), tex->name, texpath);	// Find the texture with our texture path

	if (!flname[0])													// nothing found ... fail
	{	texturelog->log("*** %s *** Missing texture.  Looked in %s",tex->name,texpath);
		if (!bm_MissingTexture)
		{	bm_MissingTexture = newbitmap("Missing Texture Standin",1,1,bitmap_ARGB32);
			*(uint32 *)(bm_MissingTexture->pixel) = 0xFFFF00FF;		// Purple
		}
		textureFromBitmap(bm_MissingTexture, tex);
		estimatedtexmemused += 4;
	}	else
	{	bm = newbitmap(flname,0);
		textureFromBitmap(bm, tex);
		deleteBitmap(bm);
	}
	if (tex->oemdata)
		texturelog->log("Create %i: %s",tex->texmemused/1024,flname);
}

Texture *newTextureFromFile(const char *texname,uintf flags)
{	// Search the current texture directory to locate a suitable texture file
	bitmap *bm;
	char flname[1024];
	if (!texname) return NULL;
	if (!texname[0]) return NULL;

	// Look for an already loaded texture with the same name
	fileNameInfo(texname);
	tprintf(flname,sizeof(flname),"%s.%s",fileactualname,fileextension);
	Texture *tex = usedTexture;
	while (tex)
	{	if (txticmp(flname,tex->name)==0)
		{	// Texture already loaded
			tex->refcount++;
			return tex;
		}
		tex = tex->next;
	}

	// Allocate a new texture cell
	tex = newTexture(flname, 0, 0);	// ### This forces the texture to be 2D!!!
	tex->flags &= texture_clearmask;
	tex->flags |= flags;

	if (fileExists(texname))										// Check to see if the path + filename is valid ...
		txtcpy(flname, sizeof(flname), texname);					// if so, load that
	else
		fileFindInPath(flname, sizeof(flname), tex->name, texpath);	// Find the texture with our texture path

	if (!flname[0])													// nothing found ... fail
	{	texturelog->log("*** %s *** Missing texture.  Looked in %s",tex->name,texpath);
		if (!bm_MissingTexture)
		{	bm_MissingTexture = newbitmap("Missing Texture Standin",1,1,bitmap_ARGB32);
			*(uint32 *)(bm_MissingTexture->pixel) = 0xFFFF00FF;		// Purple
		}
		textureFromBitmap(bm_MissingTexture, tex);
		estimatedtexmemused += 4;
	}	else
	{	bm = newbitmap(flname,0);
		textureFromBitmap(bm, tex);
		deleteBitmap(bm);
	}
	if (tex->oemdata)
		texturelog->log("Create %i: %s",tex->texmemused/1024,flname);
	return tex;
}

//struct sRenderTarget3D
//{	intf	width,height;				// Width and Height in pixels of the render target
//	Texture	*tex;						// Texture Map to render to
//	byte	OEMData[64];				// Video Driver specific variables (Do not modify!)
//};
//extern sRenderTarget3D *current3DRenderTarget;	// This variable is READ-ONLY, use selectRenderTarget3D to change the 3D render target
//void select3DRenderTarget(sRenderTarget3D *rt);	// Select a render target to be used for drawing to.  Specify rt=NULL to render to the screen display

sRenderTarget3D *current3DRenderTarget;

sRenderTarget3D *new3DRenderTarget(intf width, intf height, uintf flags)	// Creates the texture and fills in the rt pointer with valid data to make the render target usable.  Flags - set to 0 for now, in the future, you will be able to enable/disable Z buffer, stencil, etc.  Setting to 0 enables a basic Z buffer and no stencil.
{	sRenderTarget3D *result;
	result = (sRenderTarget3D *)fcalloc(sizeof(sRenderTarget3D),"3D Render Target");
	vd_create3DRenderTarget(result,width,height,flags);
	return result;
}

/*
Texture *newRenderTexture(uintf width, uintf height, uintf flags)
{	flags |=  texture_rendertarget | texture_nonmipped;
	Texture *result = newTexture("Render Target Texture",0,flags);
	vd_newRTTexture(result,width,height,flags);
	return result;
}
*/

extern void (*cleartexture)(Texture *tex);	// Clears a texture from the memory
void _deletetexture(Texture *tex)
{	// Clears out the specified texture map from memory
	if (!(tex->flags & texture_inuse))
	{	texturelog->log("*** Error: Attempted to delete a texture which is not allocated (0x%08x)",tex);
		return;
	}
	tex->refcount--;
	if (tex->refcount>0) return;
	tex->flags &= ~texture_inuse;
	texturelog->log("Delete %s",tex->name);
	cleartexture(tex);
	deletebucket(Texture,tex);
}

void deletealltextures(void)	// This is a lazy man's function - you should never call this
{	// Clears out all textures from memory
	while (usedTexture)
	{	Texture *tmp = usedTexture;
		_deletetexture(tmp);
	}
}

Texture *copytexture(Texture *tex)
{	if (!tex) return NULL;
	tex->refcount++;
	return tex;
}

// Internal function - Called to load up a texture map - file is known to exist
Texture *textureFromBitmap(bitmap *loadbm, Texture *tex)
{	bitmap *swizzleBm, *scaleBm, *resizeCanvasSrc;
	bool mustSwizzle = false;
	bool mustScale = false;

	// Step 1: Check for a need to swizzle (if the video card doesn't support this texture mode)
	uintf dataType = loadbm->flags & (bitmap_DataTypeMask | bitmap_DataInfoMask);
	// ### This code is not yet complete, must leave this block with 'swizzleBM' pointing to swizzled bitmap data

	// If Video card only handles 'Power-of-2' texture dimensions
	bool resizeCanvas=false;
	// if (GLESWarnings) //!(videoFeatures & videodriver_nonP2Tex))
	{	uintf newCanvasWidth = loadbm->width;
		uintf newCanvasHeight = loadbm->height;

		if (!isPow2(loadbm->width))
		{	resizeCanvas=true;
			newCanvasWidth=nextPow2(loadbm->width);
		}
		if (!isPow2(loadbm->height))
		{	resizeCanvas=true;
			newCanvasHeight=nextPow2(loadbm->height);
		}
		if (resizeCanvas)
		{	resizeCanvasSrc = loadbm;
			loadbm = newbitmap("resizeCanvasP2Tex",newCanvasWidth,newCanvasHeight,bitmap_ARGB32);
			uintf x,y;
			uint32 *src32 = (uint32 *)resizeCanvasSrc->pixel;
			uint32 *dst32 = (uint32 *)loadbm->pixel;
			for (y=0; y<resizeCanvasSrc->height; y++)
			{	uint32 *src = &src32[y*(resizeCanvasSrc->width)];
				uint32 *dst = &dst32[y*newCanvasWidth];
				for (x=0; x<resizeCanvasSrc->width; x++)
					*dst++ = *src++;
				for (;x<newCanvasWidth; x++)
					*dst++ = 0;
			}
			for (;y<newCanvasHeight; y++)
			{	uint32 *dst = &dst32[y*newCanvasWidth];
				for (x=0; x<newCanvasWidth; x++)
					*dst++=0;
			}
		}

/*		// Work out X scale
		uintf size = 1;
		while (size<=maxtexwidth)
		{	if (newx<=size) break;
			size <<=1;
		}
		if (size>maxtexwidth) size = maxtexwidth;
		newx = size;

		// Work out Y scale
		size = 1;
		while (size<=maxtexheight)
		{	if (newy<=size) break;
			size <<=1;
		}
		if (size>maxtexheight) size = maxtexheight;
		newy = size;
*/
	}

	// Step 2: Check if we need to resize - this may change swizzle mode
	uintf newx = loadbm->width;
	uintf newy = loadbm->height;
	if (newx>maxtexwidth)
		newx = maxtexwidth;
	if (newy>maxtexheight)
		newy = maxtexheight;

	if (newx!=loadbm->width || newy!=loadbm->height)
	{	dataType = bitmap_DataTypeRGB | bitmap_RGB_32bit;
		mustSwizzle = true;
		mustScale = true;
	}

	if (mustSwizzle)
	{	dataType |= loadbm->flags & bitmap_AlphaMask;
		swizzleBm = SwizzleBitmap(loadbm, dataType);
	}	else
		swizzleBm = loadbm;

	if (mustScale)
	{	// Bitmap needs to be resized before hardware will accept it
		scaleBm = scalebitmap(swizzleBm,newx,newy);
	}	else
		scaleBm = swizzleBm;

	// If we don't have a texture provided, create a new one
	if (!tex)
		tex = newTexture(NULL, 0, 0);
	downloadbitmaptex(tex, scaleBm, 0);
	estimatedtexmemused += tex->texmemused;
	if (mustScale)
		deleteBitmap(scaleBm);
	if (mustSwizzle)
		deleteBitmap(swizzleBm);
	if (resizeCanvas)
	{	deleteBitmap(loadbm);
		loadbm = resizeCanvasSrc;
		tex->flags |= texture_canvasSize;
		tex->UVscale.x = (float)loadbm->width / (float)tex->width;
		tex->UVscale.y = (float)loadbm->height/ (float)tex->height;
	}
	return tex;
}

Texture *getfirsttexture(void)
{	return usedTexture;
}

Texture *newNormalizeCubeMap(int size, float intensity, const char *name)
{	// Create a new Normalizing Cube Map (commonly used in Normal Mapping)
	// Size = Number of pixels in width, height, and depth
	// Intensity = Intensity of lighting.  Ranges from 0 (very little lighting) to 1 (heavy specular effect)
	// Name = the name you want to call this texture (optional)
	Texture *tex = newTexture("Unnamed Cubemap", size, texture_cubemap | texture_manualmip);
	if (name) txtcpy(tex->name,maxtexnamesize,name);

	float vector[3] = {0,0,0};
	intf side, x, y, mip;
	byte *pixels;
	intensity *= 127;

	bitmap *bm = newbitmap("Normalize CubeMap", size, size,bitmap_RGB_32bit);
	pixels = (byte *)bm->pixel;

	mip = 0;
	while (size>0)
	{	float oofsize = 1.0f / (float)size;
		bm->width = size;
		bm->height = size;
		for (side = 0; side < 6; side++)
		{	for (y = 0; y < size; y++)
			{	for (x = 0; x < size; x++)
				{	float s, t, sc, tc, mag;

					s = ((float)x + 0.5f) * oofsize; // / (float)size;
					t = ((float)y + 0.5f) * oofsize; // / (float)size;
					sc = s*2.0f - 1.0f;
					tc = t*2.0f - 1.0f;

					switch (side)
					{	case 0:
							vector[0] = 1.0;
							vector[1] = -tc;
							vector[2] = -sc;
							break;
						case 1:
							vector[0] = -1.0;
							vector[1] = -tc;
							vector[2] = sc;
							break;
						case 2:
							vector[0] = sc;
							vector[1] = 1.0;
							vector[2] = tc;
							break;
						case 3:
							vector[0] = sc;
							vector[1] = -1.0;
							vector[2] = -tc;
							break;
						case 4:
							vector[0] = sc;
							vector[1] = -tc;
							vector[2] = 1.0;
							break;
						case 5:
							vector[0] = -sc;
							vector[1] = -tc;
							vector[2] = -1.0;
							break;
					} // switch size

					mag = 1.0f/(float)sqrt(vector[0]*vector[0] + vector[1]*vector[1] + vector[2]*vector[2]);
					vector[0] *= mag;
					vector[1] *= mag;
					vector[2] *= mag;
					float alpha = intensity * vector[2] * 2;
					if (alpha<0) alpha = 0;
					pixels[4*(y*size+x) + 0] = (byte)alpha;//(128 + (byte)(intensity*vector[2]));	// A	(A taken from Z)
					pixels[4*(y*size+x) + 1] = (128 + (byte)(intensity*vector[0]));	// R
					pixels[4*(y*size+x) + 2] = (128 + (byte)(intensity*vector[1]));	// G
					pixels[4*(y*size+x) + 3] = (128 + (byte)(intensity*vector[2]));	// B
				} // for X
			}	// for Y
			downloadcubemapface(tex, side, bm, mip);
		}	// for side
		size >>= 1;
		mip++;
	}
	tex->mapping = texmapping_default | texmapping_clampUV;
	fcfree(bm);
	return tex;
}

/*
void copyTextureFromBitmap(Texture *tex, bitmap *bm, uintf level, uintf flags);	// Copy a Texture from a bitmap, if tex is NULL, a new texture is created.  Bitmap image is copied into the specified mipmap level.
Texture *getCubeTextureFace(Texture *tex, uintf face); // Obtain a non-renderable 2D texture from a cubemap
Texture *getVolumeTextureSlice(Texture *tex, uintf depth);	// Obtain a non-renderable slice of a volume texture.
Texture *copyTexture(Texture *source);			// Copy a texture from one material to another, and increase the texture's usage counter.  The usage counter is needed because as materials are deleted, so are the textures they contain.  When a texture is deleted, it's reference count is dropped by one and when it hits 0, it is removed from memory.
bitmap *copyTextureToBitmap(Texture *tex, bitmap *bm, uintf level);	// Copy a mipmap level of a texture into a bitmap.  If bm is NULL, a new bitmap is created and returned by the function
void _deleteTexture(Texture *tex);
Texture *getFirstTexture(void);					// Returns a pointer to the first texture in the linked list
void getTextureLevelDimensions(Texture *tex, uintf level, int3 *size);

cubemap face numbers, and where they would appear if you were looking north towards the cube:
	0 = Positive_X = Right_Side
	1 = Negative_X = Left_Side
	2 = Positive_Y = Top
	3 = Negative_Y = Bottom
	4 = Positive_Z = Rear
	5 = Positive_Z = Front
*/
