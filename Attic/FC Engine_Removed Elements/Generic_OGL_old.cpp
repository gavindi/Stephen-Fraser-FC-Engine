#include "fc_GenericOGL.h"
#include <bucketsystem.h>

#define EngineNotLocal
#define bucket_memmanager memmanager

//#define DriverName		"OpenGL Display Driver"
//#define DriverShort		"OpenGL"
//#define DriverAuthor	"Stephen Fraser"

#define features_multitexture		0x00000001
#define features_env_combine		0x00000002
#define features_env_dot3			0x00000004
#define features_env_cubemap		0x00000008
#define features_tex_compression	0x00000010
#define features_border_clamp		0x00000020
#define	features_env_add			0x00000040
#define	features_anisotropic		0x00000080

#define pi 3.1415926535897932384626433832795f
#define RENDERSTATE_ZBUFFER		0x0001
#define RENDERSTATE_ZWRITES		0x0002
#define RENDERSTATE_SPECULAR	0x0010
#define RENDERSTATE_FOG			0x0080
#define RENDERSTATE_DITHER		0x0100
#define RENDERSTATE_ANTIALIAS	0x0200

// OGL Structures
//
// State data
struct oglstate
{	uintf			vpx, vpy, vpwidth, vpheight;			// Viewport dimensions
	dword			renderstate;//,rendermode;
	dword			fillmode;
	dword			ambientlight;//,fogcolor;
	//GLfloat			specularcolor[4];
	dword			shademode;
	GLenum			cullmode;
	float			/*fogstart,fogend,*/horizon,/*bumpscale,*/zoom;
};

// Texture data
struct ogltexture
{	GLuint	glTex;			// OpenGL's reference to the texture map
	byte	mapping;		// The last mapping value used on this texture (caching purposes)
	byte	flags;			// Memory Usage for this reference
	byte	padding[2];		// unused padding
	uintf	width, height;	// Dimensions of the texture map
	ogltexture	*next,*prev;
} *usedogltexture = NULL, *freeogltexture = NULL;

#define ogltex_memstruct 128
#define ogltex_bitmap	   0
#define ogltex_dxt1		   1
#define ogltex_typemask	   1

// Material data
struct oglMaterial
{	float diffuse[4];
	float ambient[4];
	float emissive[4];
	float specular[4];
};

struct sblenddata			
{	GLint		GLBlendOp;
	GLint		RGBScale;
} blenddata[]= {
	{0,				0},	//  0	blendop_terminate
	{GL_REPLACE,	1},	//  1	blendop_selecttex
	{GL_PREVIOUS,	1},	//  2	blendop_unchanged
	{GL_MODULATE,	1},	//  3	blendop_modulate
	{GL_MODULATE,	2},	//  4	blendop_modulate2x
	{GL_MODULATE,	4},	//  5	blendop_modulate4x
	{GL_ADD,		1},	//  6	blendop_add
	{GL_ADD_SIGNED,	1},	//  7	blendop_addsigned
	{GL_ADD_SIGNED,	2},	//  8	blendop_addsigned2x
	{GL_ADD_SIGNED,	1},	//  9	blendop_addtosigned
	{GL_ADD_SIGNED,	2},	// 10	blendop_addtosigned2x
	{GL_SUBTRACT,	1},	// 11	blendop_subtract
	{GL_SUBTRACT,	1}, // 12	*blendop_addtonegative
	{GL_ADD,		1},	// 13	*blendop_addsmooth
	{GL_MODULATE,	1},	// 14	*blendop_moddifalpha
	{GL_MODULATE,	1},	// 15	*blendop_modtexalpha
	{GL_MODULATE,	1},	// 16	*blendop_modfacalpha
	{GL_DECAL,		1},	// 17	*blendop_modalpha
	{GL_MODULATE,	1},	// 18	*blendop_moddifinva
	{GL_MODULATE,	1},	// 19	*blendop_modtexinva
	{GL_MODULATE,	1},	// 20	*blendop_modfacinva
	{GL_DECAL,		1},	// 21	*blendop_modinva
	{GL_DOT3_RGBA,	1},	// 22	blendop_dotproduct3
	{GL_INTERPOLATE,1},	//  2	*blendop_unchanged
};

// OGL Capabilities
char	*ogl_extension[256];									// A listing of all supported OGL extensions
uintf	numextensions;
GLint	texunits;												// Number of texture units in this hardware

// FC Engine interface variables
void *(*oglclassfinder)(char *);
fc_system	*sysmanager;
fc_matrix	*matmanager;
fc_bitmap	*bmmanager;
fc_graphics	*gfxmanager;
fc_memory	*memmanager;
fc_material	*mtlmanager;
fc_lighting *lgtmanager;

dword		*oglbackgroundrgb;
long		*ogllfbpitch;
void		**ogllfb;
Matrix		*ogluvmodifier;

// Global OGL variables
logfile *ogllog;											// Log file to write diagnostic reports to
bitmap	*rendersurface2d;									// Pointer to a fake 2D render surface
float	oo255;
float	ogl_camd;

float	*_cammat;											// Pointer to the global camera matrix
float	*oglhorizon;										// Pointer to the horizon
long	*oglhfov, *oglvfov;									// Horizontal and Vertical fields of view
bool	world2screenmatvalid;								// Is the current world-screen matrix valid?
oglstate currentstate,backupstate;							// The current render state, and a backup

Matrix	oglnegmtx;											// A Z=-1 matrix needed for D3D->OGL conversions of matrices
Matrix	invcammat;											// Invert of the camera matrix
Matrix	projectionmat;										// Projection Matrix
Matrix	invcamprojmat;										// Invert camera * Projection (needed for fast software vertex transforms)
GLenum	ColorTexFormat;										// Texture format to use for color-based textures

Material *NullMtl;											// NULL material for when an object is without a material (very bad idea)
Matrix	NullMtx;											// NULL Matrix is used for any object which does not have it's own matrix (this is OK)
GLuint	NullTexture;										// Plain white texture for use when the texture could not be loaded (non-fatal error)

// Field of view planes
// 0 = Near plane
// 1 = Far plane
// 2 = Left plane
// 3 = Right plane
// 4 = Top plane
// 5 = Bottom Plane
float fovnx[6],fovny[6],fovnz[6],fovd[6];

// --------------------------------------------------------
// ---				Utility Functions					---
// --------------------------------------------------------
void ColorDwordToFloat(dword col, float *flt)
{	flt[0] = (float)((col & 0x00ff0000) >> 16) * oo255;
	flt[1] = (float)((col & 0x0000ff00) >>  8) * oo255;
	flt[2] = (float)((col & 0x000000ff)      ) * oo255;
	flt[3] = (float)((col & 0xff000000) >> 24) * oo255;
}

void calcvisvert(float3 *v,long fx,long fy)
{	// Calculates a visibility vertex
	float mat[16];
	memcpy(mat, _cammat, sizeof(float) * 16);
	matmanager->rotatearoundy(mat,(long)fx);
	matmanager->rotatearoundx(mat,(long)fy);
	matmanager->fixrotations(mat);
	v->x = *oglhorizon * mat[mat_zvecx] + mat[mat_xpos];
	v->y = *oglhorizon * mat[mat_zvecy] + mat[mat_ypos];
	v->z = *oglhorizon * mat[mat_zvecz] + mat[mat_zpos];
}

void setfov(long plane, float px, float py, float pz, float nx, float ny, float nz)
{	// Sets the field of view plane.
	fovnx[plane]=nx;
	fovny[plane]=ny;
	fovnz[plane]=nz;
	fovd [plane]=px*nx + py*ny + pz*nz;
}

void calcfov(long plane, float3 *v0,float3 *v1,float *tempmat)
{	// Given 2 points and a matrix, calculate a fov plane

	// Generate 2 vectors from the provided data
	float v1x = v1->x - v0->x;
	float v1y = v1->y - v0->y;
	float v1z = v1->z - v0->z;

	float v2x = tempmat[mat_xpos] - v0->x;
	float v2y = tempmat[mat_ypos] - v0->y;
	float v2z = tempmat[mat_zpos] - v0->z;

	// Use those 2 vectors to calculate a normal
	float nx,ny,nz;
	crossproduct(n,v1,v2);
	_normalize(n);
	// Send this data to setfov function
	setfov(plane,tempmat[12], tempmat[13], tempmat[14], nx,ny,nz);
}

// --------------------------------------------------------
// ---					OGL Functions					---
// --------------------------------------------------------
void ogl_updatecammatrix(void)
{	Matrix tmp;
	matmanager->matmult(tmp,_cammat,oglnegmtx);
	matmanager->matrixinvert(invcammat,tmp);
	matmanager->matmult(invcamprojmat, projectionmat, invcammat);
	world2screenmatvalid = false;

	float3 v[4];
	
	calcvisvert(&v[0],3600-(*oglhfov),3600-(*oglvfov));
	calcvisvert(&v[1],3600-(*oglhfov),     (*oglvfov));
	calcvisvert(&v[2],     (*oglhfov),3600-(*oglvfov));
	calcvisvert(&v[3],     (*oglhfov),     (*oglvfov));

	// Near Plane
	setfov(5,_cammat[12], 
			 _cammat[13],
			 _cammat[14],
			 _cammat[8], _cammat[9], _cammat[10]);
	// Far Plane
	setfov(0,_cammat[12] + _cammat[ 8] * (*oglhorizon), 
			 _cammat[13] + _cammat[ 9] * (*oglhorizon),
			 _cammat[14] + _cammat[10] * (*oglhorizon),
			-_cammat[8], - _cammat[9], - _cammat[10]);
	// Left Plane
	calcfov(1,&v[0],&v[1],_cammat);
	// Right Plane
	calcfov(2,&v[3],&v[2],_cammat);
	// Top Plane
	calcfov(3,&v[2],&v[0],_cammat);
	// Bottom Plane
	calcfov(4,&v[1],&v[3],_cammat);
	ogl_camd = _cammat[8]*_cammat[mat_xpos] + _cammat[9]*_cammat[mat_ypos] + _cammat[10]*_cammat[mat_zpos];

	if (!lgtmanager) lgtmanager = (fc_lighting *)oglclassfinder("lighting");
	if (lgtmanager)
	{	light3d *l = lgtmanager->getfirstlight(light_freerange);
		while (l)
		{	changelight(l);
			l=l->next;
		}
	}
}

void ogl_GenerateProjectionMatrix(void)
{	// Generate D3D Projection Matrix
	float fFOV = 0.392699081698724154807830422909938f/currentstate.zoom;	// pi / 8 = 22.5 degrees ... This can be considdered a ZOOM factor (0.4 = standard, 0.1 = zoom in, 1.0 = zoom out
	float aspect = (float)currentstate.vpheight / (float)currentstate.vpwidth;
	float w = aspect * (float)( cos(fFOV)/sin(fFOV) );		// 0.75 * (0.9238795 / 0.38268343)
	float h =   1.0f * (float)( cos(fFOV)/sin(fFOV) );		// 1.0  * (0.9238795 / 0.38268343)
	float Q = *oglhorizon / ( *oglhorizon - 1 );
	
	float fFOVdeg = (fFOV/(pi*2))*3600;
	*oglhfov = (long)(fFOVdeg / aspect); // (180/8 = 22.5.  aspect = 0.75.  22.5/aspect = 30).
	*oglvfov = (long)(fFOVdeg);			 // (180/8 = 22.5).  
	projectionmat[ 0] = w;
	projectionmat[ 5] = h;
	projectionmat[10] = Q;
	projectionmat[11] = -Q;				// item 11 and 14 are swapped around from D3D version
	projectionmat[14] = 1.0f;
	glMatrixMode( GL_PROJECTION);
	glLoadMatrixf( projectionmat);
	glMatrixMode(GL_MODELVIEW);
	matmanager->matmult(invcamprojmat, projectionmat, invcammat);
	world2screenmatvalid = false;
}

void ogl_sethorizon(float h)
{	currentstate.horizon = *oglhorizon = h;
    ogl_GenerateProjectionMatrix();
}

void ogl_setshademode(dword shademode)
{	currentstate.shademode = shademode;
	switch(shademode)
	{	case shademode_flat:	
			glShadeModel( GL_FLAT );
			break;
		case shademode_gouraud:	
			glShadeModel( GL_SMOOTH );
			break;
		case shademode_phong:	
			glShadeModel( GL_SMOOTH );
			break;
		case shademode_metal:
			glShadeModel( GL_SMOOTH );
			break;
		case shademode_blinn:
			glShadeModel( GL_SMOOTH );
			break;
		default:
			glShadeModel( GL_SMOOTH );
	}
}

void ogl_cls(void)
{	glClearColor(((*oglbackgroundrgb & 0x00ff0000) >> 16) * oo255,
				 ((*oglbackgroundrgb & 0x0000ff00) >>  8) * oo255,
				 ((*oglbackgroundrgb & 0x000000ff)      ) * oo255,
				 ((*oglbackgroundrgb & 0xff000000) >> 24) * oo255);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ogl_zcls(void)
{	glClear(GL_DEPTH_BUFFER_BIT);
}

void ogl_enableZbuffer(void)
{	glEnable(GL_DEPTH_TEST);
	currentstate.renderstate |= RENDERSTATE_ZBUFFER;
}

void ogl_disableZbuffer(void)
{	glDisable(GL_DEPTH_TEST);
	currentstate.renderstate &= ~RENDERSTATE_ZBUFFER;
}

void ogl_enableZwrites(void)
{	glDepthMask(true);
	currentstate.renderstate |= RENDERSTATE_ZBUFFER;
}

void ogl_disableZwrites(void)
{	glDepthMask(false);
	currentstate.renderstate &= ~RENDERSTATE_ZBUFFER;
}

void ogl_enabledithering(void)
{	glEnable(GL_DITHER);
	currentstate.renderstate |= RENDERSTATE_DITHER;
}

void ogl_disabledithering(void)
{	glDisable(GL_DITHER);
	currentstate.renderstate &= ~RENDERSTATE_DITHER;
}

void ogl_enableAA(void)
{	glEnable(GL_POLYGON_SMOOTH);
	currentstate.renderstate |= RENDERSTATE_ANTIALIAS;
}

void ogl_disableAA(void)
{	glDisable(GL_POLYGON_SMOOTH);
	currentstate.renderstate &= ~RENDERSTATE_ANTIALIAS;
}

void ogl_changeviewport(intf x, intf y, intf width, intf height)
{	glViewport(x,y,width,height);									// Reset The Current Viewport
}

void ogl_setfillmode(dword _fillmode)
{	currentstate.fillmode = _fillmode;
	switch(_fillmode)
	{	case fillmode_wireframe:
			glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );
			break;
		default:
			glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );
	}
}

void ogl_setambientlight(dword col)
{	float ambient[4];
	currentstate.ambientlight = col;
	ColorDwordToFloat(col, ambient);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
}

long ogl_getstatesize(void)
{	return sizeof(oglstate);
}

void ogl_fakelock3d(void)
{	*ogllfb = (word *)rendersurface2d->pixel;
	uintf flags = rendersurface2d->flags & bitmap_DataInfoMask;
	if (flags == bitmap_RGB_8bit)
		*ogllfbpitch = rendersurface2d->width;
	if (flags == bitmap_RGB_16bit)
		*ogllfbpitch = rendersurface2d->width<<1;
	if (flags == bitmap_RGB_32bit)
		*ogllfbpitch = rendersurface2d->width<<2;
}

void ogl_fakeunlock3d(void)
{	*ogllfb = NULL;
	*ogllfbpitch = 0;
}

void ogl_cleartexture(Texture *tex)
{	if (tex) 
	{	ogltexture *ogltex = (ogltexture *)tex->oemdata;
		if (ogltex)
		{	glDeleteTextures(1, &ogltex->glTex);
			deletebucket(ogltexture,ogltex);
		}
		tex->oemdata = NULL;
	}
}

void ogl_newtextures(Texture *_tex, dword numtex)
{//	if (numtex!=texturealloc) oglmsg("Engine Version Conflict","The version information sent from the FC engine does not match this FC display driver, they appear to be incompatible");
//	GLuint texnames[texturealloc];
//	ogltexture *ogltex = memmanager->fcalloc(numtex * sizeof(ogltexture),"OpenGL Texture Cache");
//	for (uintf i=0; i<numtex; i++)
//	{	
//	}
}

void ogl_downloadbmtex(Texture *_tex, bitmap *bm, dword miplevel)
{	dword *src;
	dword	*tmp = NULL;					// Temporary storage location for mipmaps
	GLint	components = GL_RGB8;
	if (bm->flags & bitmap_Alpha)
		components = GL_RGBA8;
	
	ogltexture *tex = (ogltexture *)_tex->oemdata;
	if (!tex)
	{	allocbucket(ogltexture,tex,flags,ogltex_memstruct,128,"OpenGL Texture Cache");
		glGenTextures(1, &tex->glTex);
		tex->mapping = 0xff;				// Specify an impossible mapping algorithm to force reloading of texture mapping
		_tex->oemdata = tex;
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->glTex);

	if (_tex->flags & texture_envbump)	// ### Pertubated UV Bump Mapping (not yet supported)
	{	return;
	}

	if ((bm->flags & bitmap_DataTypeMask)==bitmap_DataTypeCompressed)
	{	if ((bm->flags & bitmap_DataInfoMask) == bitmap_compDXT1) 
		{	glCompressedTexImage2D(GL_TEXTURE_2D, miplevel, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, bm->width, bm->height, 0, (bm->width*bm->height)>>1, bm->pixel);
		}
		return;
	}

	if (ColorTexFormat == GL_RGBA)
	{	//glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);		// Although this works, it's horribly slow compared to my alpha-roll code
		//glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);		// Although this works, it's horribly slow compared to my alpha-roll code
		//dword *pix = (dword *)bm->pixel;
		//for (uintf i=0; i<bm->width * bm->height; i++)
		//{	pix[i]=0xffff0000;
		//}
		//glTexImage2D(GL_TEXTURE_2D, 0, components, bm->width, bm->height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bm->pixel);
		//src = (dword *)bm->pixel;

		uintf texsize = bm->width * bm->height;
		tmp = (dword *)memmanager->alloctempbuffer(texsize<<2,"Texture Map Color Conversion");
		dword *dwordcols = (dword *)bm->pixel;
		for (uintf i=0; i<texsize; i++)
		{	tmp[i] = (dwordcols[i] << 8) | (dwordcols[i]>>24);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, components, bm->width, bm->height, 0, ColorTexFormat, GL_UNSIGNED_BYTE, tmp);
		src = tmp;
	}	else
	{	glTexImage2D(GL_TEXTURE_2D, 0, components, bm->width, bm->height, 0, ColorTexFormat, GL_UNSIGNED_BYTE, bm->pixel);
		src = (dword *)bm->pixel;
	}

	if (_tex->flags & texture_manualmip)
	{	if (tmp) freetempbuffer(tmp);
		return;
	}

	dword owidth = bm->width;
	dword oheight = bm->height;
	dword newwidth = bm->width>>1;
	dword newheight = bm->height>>1;
	if (newwidth+newheight==0) return;
	if (newwidth==0) newwidth = 1;
	if (newheight==0) newheight = 1;
	if (!tmp) tmp = (dword *)memmanager->alloctempbuffer(newwidth*newheight*4,"Texture Mipmap Generation");

	dword level = 1;

	while(newwidth+newheight>0)
	{	if (newwidth==0) newwidth = 1;
		if (newheight==0) newheight = 1;
		dword srcyofs = 0;
		dword dstyofs = 0;
		for (uintf y=0; y<newheight; y++)
		{	for (uintf x=0; x<newwidth; x++)
			{	dword srcofs = srcyofs + (x<<1);
				dword a = ((src[srcofs] >> 2) & 0x3fc00000) + ((src[srcofs+1] >> 2) & 0x3fc00000) +
						  ((src[srcofs+owidth] >> 2) & 0x3fc00000) + ((src[srcofs+owidth+1] >> 2) & 0x3fc00000);
				dword r = ((src[srcofs] & 0x00fc0000) + (src[srcofs+1] & 0x00fc0000) + 
						   (src[srcofs+owidth] & 0x00fc0000) + (src[srcofs+owidth+1] & 0x00fc0000))>>2;
				dword g = ((src[srcofs] & 0x0000fc00) + (src[srcofs+1] & 0x0000fc00) + 
						   (src[srcofs+owidth] & 0x0000fc00) + (src[srcofs+owidth+1] & 0x0000fc00))>>2;
				dword b = ((src[srcofs] & 0x000000fc) + (src[srcofs+1] & 0x000000fc) + 
						   (src[srcofs+owidth] & 0x000000fc) + (src[srcofs+owidth+1] & 0x000000fc))>>2;
				tmp[dstyofs+x] = (a&0xff000000) + (r&0x00ff0000) + (g&0x0000ff00) + (b&0x000000ff);
			}
			dstyofs += newwidth;
			srcyofs += owidth << 1;
		}
		glTexImage2D(GL_TEXTURE_2D, level, components, newwidth, newheight, 0, ColorTexFormat, GL_UNSIGNED_BYTE, tmp);
		owidth = newwidth;
		oheight = newheight;
		newwidth>>=1;
		newheight>>=1;
		level++;
		src = tmp;
	}
	memmanager->freetempbuffer(tmp);
}

GLenum FinalBlendLUT[] =
{	GL_ZERO,				// mtl_srcZero
	GL_ONE,					// mtl_srcOne
	GL_SRC_COLOR,			// mtl_srcCol
	GL_ONE_MINUS_SRC_COLOR,	// mtl_srcInvCol
	GL_SRC_ALPHA,			// mtl_srcAlpha
	GL_ONE_MINUS_SRC_ALPHA,	// mtl_srcInvAlpha
	GL_DST_ALPHA,			// mtl_srcOtherAlpha
	GL_ONE_MINUS_DST_COLOR,	// mtl_srcInvOtherAlpha
	GL_DST_COLOR,			// mtl_srcOtherCol
	GL_ONE_MINUS_DST_COLOR,	// mtl_srcInvOtherCol
	GL_SRC_ALPHA_SATURATE	// mtl_srcAlphaSat
};

#define rendervertex(vertnum)												\
	vert = (float *)getVertexPtr(obj, _vert, vertnum);						\
	v = (float3*)(vert + obj->posofs);										\
	n = (float3*)(vert + obj->normofs);										\
	texchan = 0;															\
	while (m->colorop[texchan]!=blendop_terminate && texchan<texunits)		\
	{	if ((m->mapping[texchan] & mtl_coordmask)==0)						\
		{	coordset = m->channel[texchan];									\
			t = vert + obj->texofs[coordset];								\
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB+texchan, t[0], t[1]);		\
		}																	\
		texchan++;															\
	}																		\
	glNormal3d(n->x, n->y, n->z);											\
	glVertex3f(v->x, v->y, v->z);											

void renderpolys(obj3d *obj, Material *m)
{	uintf i;
	GLint texchan;
	uintf coordset;
	if (obj->flags & obj_quadlist)
	{	texchan = 0;
		// ### Unfinished code - no support for scripted shaders
		FixedPixelShader *ps = (FixedPixelShader *)m->PixelShader;
		FixedVertexShader *vs = (FixedVertexShader *)m->VertexShader;
		while (ps->colorop[texchan]!=blendop_terminate && texchan<texunits)
		{	glClientActiveTextureARB(GL_TEXTURE0_ARB+texchan);
			Texture *t = m->texture[texchan];
			if ((t->mapping & texmapping_coordmask)==0)
			{	coordset = m->channel[texchan];
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(obj->texelements[texchan],GL_FLOAT, obj->vertstride, (float *)obj->texcoord[coordset]);
			}	else
			{	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			texchan++;
		}
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, obj->vertstride, (float *)obj->vertnorm);
		glVertexPointer(3, GL_FLOAT, obj->vertstride, (float *)obj->vertpos);
		glEnableClientState(GL_VERTEX_ARRAY);
		glDrawArrays(GL_QUADS, 0, obj->numverts);
	}	else
	{	glBegin(GL_TRIANGLES);
		for (i=0; i<obj->numfaces; i++)
		{	float *vert;
			float3 *v;
			float3 *n;
			float *t;

			rendervertex(obj->face[i].v1);
			rendervertex(obj->face[i].v2);
			rendervertex(obj->face[i].v3);
		}
		glEnd();
	}
}

float ogl_test_fov(float3 *pos,float rad)
{	// Returns > 0 if object is visible
	for (long i=0; i<5; i++) // 6
	{	if ((pos->x*fovnx[i] + pos->y*fovny[i] + pos->z*fovnz[i] - fovd[i]) < -rad) 
			return -1;
	}
	float dist = -rad + pos->x*fovnx[5] + pos->y*fovny[5] + pos->z*fovnz[5] - fovd[5];
	if (dist>-rad) return dist+rad;
	return 0;
}


dword ogl_testobjvis(obj3d *obj)
{	// Returns non-zero if this object is visible
	float x,y,z;
	if (currentstate.cullmode == GL_CW) return 1;

	if (obj->matrix)
	{	x = obj->matrix[mat_xpos];
		y = obj->matrix[mat_ypos];
		z = obj->matrix[mat_zpos];
	}	else
	{	x = obj->worldpos.x;
		y = obj->worldpos.y;
		z = obj->worldpos.z;
	}
	
	float rad = -obj->radius;

	for (long i=0; i<6; i++) // 6
	{	if ((x*fovnx[i] + y*fovny[i] + z*fovnz[i] - fovd[i]) < rad) 
			return 0;
	}
	return 1;
}

bool Zbucketing = true;
struct zbuckettag
{	obj3d *obj;
	Material *mtl;
	float matrix[16];
	void (*drawingfunc)(obj3d *obj);
	float z;
	uintf uset;
	void *parent;
	zbuckettag *less,*more;
} *zbucket;

dword numzbuckets, nextzbucket;

void initzbuckets(void)
{//	usedzbuckettag = NULL;	// New
//	freezbuckettag = NULL;	// New

	nextzbucket = 0;
	zbucket = (zbuckettag *)memmanager->fcalloc(zbucketsperalloc*sizeof(zbuckettag),"Z Buckets for 3D Renderer");
	numzbuckets = zbucketsperalloc;
}

void killZbuckets(void)
{	if (zbucket) 
	{	memmanager->free(zbucket);
		zbucket = NULL;
	}
}

// New obj3d creation code - automatic generation of optimized mesh
void ogl_newobj3d(obj3d *obj, sObjCreateInfo *objinfo)
{	uintf i;
	uintf vertsize = 6*sizeof(float);
	uintf lasttex = 0;
	for (i=0; i<8; i++)
	{	if (objinfo->NumTexComponents[i]>0)
		{	vertsize += objinfo->NumTexComponents[i]*sizeof(float);
			lasttex = i;
		}	else break;
	}

	uintf memneeded = (objinfo->numverts * vertsize) + (objinfo->numfaces * 3 * 4);
	byte *buf = fcalloc(memneeded,"Geometry Buffer");
	obj->_vert = (float *)buf;		buf += objinfo->numverts * vertsize;
	obj->face = (Face *)buf;		buf += objinfo->numfaces * 3 * 4;
	obj->convvert = (void *)glGenLists(1);
	obj->convface = NULL;

	// initialize faces
	obj->numfaces	= objinfo->numfaces;
	obj->facestride = 0;

	// initialize vertices
	obj->numverts	= objinfo->numverts;
	obj->vertstride	= vertsize;

	obj->posofs = 0;
	obj->normofs = 3;

	// Initialize Texture Channels
	obj->numtexchans= lasttex+1;
	i=0;
	uintf currentofs = 6;
	while (i<8 && objinfo->NumTexComponents[i])
	{	obj->texelements[i] = objinfo->NumTexComponents[i];
		obj->texofs[i] = currentofs;
		obj->texcoord[i] = obj->_vert + currentofs;
		currentofs += objinfo->NumTexComponents[i];
		i++;
	}
}

void ogl_deleteobj3d(obj3d *obj)
{	fcfree(obj->_vert);
	if (obj->convvert)
		glDeleteLists((GLuint)obj->convvert, 1);
	obj->convvert = NULL;
}

void ogl_unlockmesh(obj3d *obj)
{	if (!(obj->flags & obj_meshlocked)) msg("Locking Error","UnlockMesh called on object which is not locked");
	
	if (obj->flags & lockflag_write)
	{	GLuint drawlist = (GLuint)obj->convvert;
	    glNewList(drawlist, GL_COMPILE);
		Material *m = obj->material;
		if (!m) 
		{	if (!NullMtl)
			{	mtlmanager = (fc_material *) oglclassfinder("material");
				NullMtl = mtlmanager->newmaterial("NULL Material");
				NullMtl->diffuse = 0xffffffff;
				NullMtl->ambient = 0xffffffff;
			}
			m = NullMtl;
		}
		renderpolys(obj, m);
		glEndList();
	}
	obj->flags &= ~obj_meshlocked;
}

void ogl_lockmesh(obj3d *obj, uintf lockflags)
{	if (lockflags==0) lockflags = lockflag_everything;
	if (obj->flags & obj_meshlocked)
	{	msg("Locking Error",buildstr("LockMesh called on object already locked %s",obj->name));
		return;
	}
	
	if (lockflags & lockflag_read) obj->flags |= obj_meshlockread;
	if (lockflags & lockflag_write) obj->flags |= obj_meshlockwrite;

	if (lockflags & lockflag_faces)
	{	obj->flags |= obj_faceslocked;
		obj->facestride = 3*4;
	}

	if (lockflags & lockflag_verts)
	{	obj->flags |= obj_vertslocked;
		obj->vertpos = (float3 *)obj->_vert;
		obj->vertnorm = (float3 *)((byte *)obj->_vert + sizeof(float3));
		obj->posofs = 0;
		obj->normofs = 3;
		uintf currentofs = 6;
		for (uintf i=0; i<obj->numtexchans; i++)
		{	obj->texcoord[i] = (float *)obj->_vert + currentofs;
			obj->texofs[i] = currentofs;
			currentofs += obj->texelements[i];
		}
	}
}

void renderbucket(zbuckettag *zb)
{	if (zb->more) renderbucket(zb->more);
	
	obj3d *obj = zb->obj;
	float *prevmatrix = obj->matrix;
	Material *prevmtl = obj->material;

	obj->matrix = zb->matrix;
	obj->material = zb->mtl;
	if (zb->drawingfunc)
	{	obj->uset = zb->uset;
		obj->parent = zb->parent;
		zb->drawingfunc(obj);
	}
	else
		gfxmanager->drawobj3d(obj);
	obj->matrix = prevmatrix;
	obj->material = prevmtl;
	if (zb->less) renderbucket(zb->less);
}

void ogl_flushZsort(void)
{//	while (usedzbuckettag)							// New
//		deletebucket(zbuckettag,freezbuckettag);	// New
	if (nextzbucket>0)
	{	bool wasbuf = Zbucketing;
		Zbucketing = false;
		renderbucket(&zbucket[0]);
		Zbucketing = wasbuf;
		nextzbucket = 0;
	}
}

void ogl_bucketobject(obj3d *obj)
{	// Store object and it's matrix and a pointer to it's material in a Z bucket
	if (nextzbucket>=numzbuckets)
	{	ogl_flushZsort();	// Forced to empty Z buckets because resizing the container causes a crash
		numzbuckets+=zbucketsperalloc;
		if (zbucket) fcfree(zbucket);
		zbucket = (zbuckettag *)memmanager->fcalloc(numzbuckets*sizeof(zbuckettag),"Z Sort Buffers");
	}

	zbuckettag *zb = &zbucket[nextzbucket++];
	zb->obj = obj;
	float *mat = obj->matrix;
	if (mat)
	{	memcpy(zb->matrix,mat,16*sizeof(float));
		//zb->matrixptr = mat;
		zb->z = _cammat[8]*mat[mat_xpos] + _cammat[9]*mat[mat_ypos] + _cammat[10]*mat[mat_zpos] - ogl_camd;
	} else
	{	zb->z = _cammat[8]*obj->worldpos.x + _cammat[9]*obj->worldpos.y + _cammat[10]*obj->worldpos.z - ogl_camd;
		//zb->matrixptr = NULL;
		float *mat = zb->matrix;
		mat[ 0]=1;				mat[ 1]=0;				mat[ 2]=0;				mat[ 3]=0;
		mat[ 4]=0;				mat[ 5]=1;				mat[ 6]=0;				mat[ 7]=0;
		mat[ 8]=0;				mat[ 9]=0;				mat[10]=1;				mat[11]=0;
		mat[12]=obj->worldpos.x;mat[13]=obj->worldpos.y;mat[14]=obj->worldpos.z;mat[15]=1;
	}
	zb->mtl = obj->material;
	zb->less = NULL;
	zb->more = NULL;
	zb->drawingfunc = obj->zsDrawFunc;
	zb->uset = obj->uset;
	zb->parent = obj->parent;
	float z = zb->z;

	if (nextzbucket!=1)
	{	// This is not the head bucket, so insert it into the binary tree
		zbuckettag *cb = &zbucket[0];
		while (1)
		{	if (z<=cb->z)
			{	if (cb->less) 
				{	cb = cb->less;
					continue;
				}	
				cb->less = zb;
				break;
			}
			if (cb->more)
			{	cb = cb->more;
				continue;
			}
			cb->more = zb;
			break;	
		}
	}
}

byte ogl_renderobj3d(obj3d *obj)
{	Matrix		rendermtx;
	Material	*m;
	GLint		texchan;

	// Test Visibility
	if (!(obj->flags & obj_nofov))
		if (!ogl_testobjvis(obj)) return 0;

	// Prepare Material
	m = obj->material;
	if (!m) 
	{	if (!NullMtl)
		{	mtlmanager = (fc_material *) oglclassfinder("material");
			NullMtl = mtlmanager->newmaterial("NULL Material");
			NullMtl->diffuse = 0xffffffff;
			NullMtl->ambient = 0xffffffff;
		}
		m = NullMtl;
	}
	{	
		if ((m->flags & mtl_zsort) && Zbucketing)
		{	ogl_bucketobject(obj);
			return 2;
		}	
		if (m->flags & mtl_2sided)
			glDisable(GL_CULL_FACE);
		else
			glEnable(GL_CULL_FACE);
		oglMaterial *oemMtl = (oglMaterial *)m->oemmat;
		glMaterialfv(GL_FRONT, GL_AMBIENT, oemMtl->ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, oemMtl->diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, oemMtl->specular);
		glMaterialfv(GL_FRONT, GL_EMISSION, oemMtl->emissive);
		glBlendFunc(FinalBlendLUT[(m->flags & mtl_srcamask) >> mtl_srcshift], FinalBlendLUT[(m->flags & mtl_dstamask) >> mtl_dstshift]);
		glAlphaFunc(GL_GEQUAL, m->minalpha*oo255);

		// Set up texture channels
		texchan = 0;
		while (m->colorop[texchan]!=blendop_terminate && texchan<texunits)
		{	glActiveTextureARB(GL_TEXTURE0_ARB + texchan);
			glEnable(GL_TEXTURE_2D);
			Texture *_tex = m->texture[texchan];
			ogltexture *tex = (ogltexture *)_tex->oemdata;
			if (tex)
			{	glBindTexture(GL_TEXTURE_2D, tex->glTex);
				byte mapping = m->mapping[texchan];
				if (mapping!=tex->mapping)
				{	tex->mapping = mapping;
					if (mapping & mtl_bilinear)	
						glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
					else
						glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
					if (_tex->flags & texture_nonmipped)
					{	if (mapping & mtl_mipmap)	
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
						else
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
					}	else
					{	if (mapping & mtl_mipmap)	
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
						else
							glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
					}
				}
				switch (mapping & mtl_coordmask)
				{	case mtl_screenxyz:
						glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
						glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
						glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
						glEnable(GL_TEXTURE_GEN_S);
						glEnable(GL_TEXTURE_GEN_T);
						glEnable(GL_TEXTURE_GEN_R);
						break;
					default:
						glDisable(GL_TEXTURE_GEN_S);
						glDisable(GL_TEXTURE_GEN_T);
						glDisable(GL_TEXTURE_GEN_R);
						break;
				}
			}
			else
				glBindTexture(GL_TEXTURE_2D, NullTexture);
			uintf colorop = m->colorop[texchan];
			uintf alphaop = m->alphaop[texchan];
			glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT,	 blenddata[colorop].GLBlendOp);	
			glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, blenddata[alphaop].GLBlendOp);	
			glTexEnvi (GL_TEXTURE_ENV, GL_RGB_SCALE,		 blenddata[colorop].RGBScale);
			glTexEnvi (GL_TEXTURE_ENV, GL_ALPHA_SCALE,		 blenddata[alphaop].RGBScale);
			glMatrixMode(GL_TEXTURE);
			byte uvmod = m->modifier[texchan];
			if (uvmod == 0)
			{	glLoadIdentity();
			}	else
			{	Matrix tmp;
				matmanager->matmult(tmp,ogluvmodifier[uvmod],oglnegmtx);
				glLoadMatrixf(tmp);
			}
			texchan++;
		}
		// Disable next texture channel
		while (texchan<texunits)
		{	glActiveTextureARB(GL_TEXTURE0_ARB + texchan);
			glDisable(GL_TEXTURE_2D);				
			//glDisable(GL_TEXTURE_CUBE_MAP_EXT);
			texchan++;
		}
	}

	// Prepare Matrix
	if (obj->matrix)
	{	matmult(rendermtx, invcammat, obj->matrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(rendermtx);
	}	else
	{	NullMtx[mat_xpos] = obj->worldpos.x;
		NullMtx[mat_ypos] = obj->worldpos.y;
		NullMtx[mat_zpos] = obj->worldpos.z;
		matmult(rendermtx, invcammat, NullMtx);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(rendermtx);
	}

	// Render Polygons
	if (obj->convvert)
		glCallList((GLint)obj->convvert);
	else
		renderpolys(obj, m);
	return 1;
}

void InitOGLExtensions(void)
{	char *extensions = (char *)glGetString(GL_EXTENSIONS);
	numextensions = 0;
	char *extptr = extensions;
	while (*extptr)
	{	if (*extptr==' ')
		{	*extptr = 0;
			ogl_extension[numextensions++] = extensions;
			extensions = extptr+1;
		}
		extptr++;
	}
	bool rerunsort = numextensions>0;
	while (rerunsort)
	{	rerunsort = false;
		for (uintf i=0; i<numextensions-1; i++)
			if (stricmp(ogl_extension[i],ogl_extension[i+1])>0)
			{	char *tmp = ogl_extension[i];
				ogl_extension[i] = ogl_extension[i+1];
				ogl_extension[i+1] = tmp;
				rerunsort = true;
			}
	}
}

void ogl_getvideoinfo(void *data)
{	videoinfostruct vi;
	strcpy(vi.DriverName, "OpenGL Display Driver");
	strcpy(vi.DriverNameShort, "OpenGL");
	strcpy(vi.AuthorName, "Stephen Fraser");
#ifdef STATICDRIVERS
	strcpy(vi.LoadingMethod,"Static");
#else
	strcpy(vi.LoadingMethod,"Dynamic");
#endif
	char *Description = (char *)glGetString(GL_RENDERER);
	if (strlen(Description)>sizeof(vi.DeviceDescription))
		Description[sizeof(vi.DeviceDescription)-1] = 0;
	strcpy(vi.DeviceDescription,Description);
	
	GLint tmp;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tmp);
	vi.MaxTextureWidth	= tmp;
	vi.MaxTextureHeight	= tmp;
	vi.NumTextureUnits	= texunits;
	vi.TextureMemory	= 0;
	vi.NumLights		= 8;
	vi.Features			= 0;
	videoinfostruct *dest = (videoinfostruct *)data;
	dword size = dest->structsize;
	if (size>sizeof(videoinfostruct)) size = sizeof(videoinfostruct);
	memcpy(dest,&vi,size);
	dest->structsize = size;
}

bool ogl_ExtensionSupported(char *ext)
{	for (uintf i=0; i<numextensions; i++)
		if (stricmp(ogl_extension[i],ext)==0) return true;
	return false;
}

void ogl_disablelights(uintf firstlight, uintf lastlight)
{	for (uintf i=firstlight; i<lastlight; i++)
		glDisable(GL_LIGHT0 + i);
}

void ogl_changelight(light3d *thislight)
{	GLfloat lightparam[4];

	// OpenGL fucks you in the bum by needing lighting co-ordinates specified in EYE-SPACE 
	// so translate with the inverse camera matrix first.
	// TODO: Pull off some fucking miracle to make sure this is not necessary for all lights in every object
	glMatrixMode (GL_MODELVIEW);
	glLoadMatrixf(invcammat);

	uintf index = thislight->index;
	if (index<0 || index>8) return;
	
	uintf lighttype = thislight->flags & light_typemask;
	switch (lighttype)
	{	case light_spot:
		{	// Set light position
			lightparam[0] = thislight->matrix[mat_xpos];
			lightparam[1] = thislight->matrix[mat_ypos];
			lightparam[2] = thislight->matrix[mat_zpos];
			lightparam[3] = 1;	// 1 = positional light type
			glLightfv(GL_LIGHT0 + index, GL_POSITION, lightparam);

			// Set light Direction
			lightparam[0] = thislight->matrix[ 8];
			lightparam[1] = thislight->matrix[ 9];
			lightparam[2] = thislight->matrix[10];
			lightparam[3] = 0;
			glLightfv(GL_LIGHT0 + index, GL_SPOT_DIRECTION, lightparam);
			float angle = thislight->outerangle * 180 / 3.14159f;
			glLightf(GL_LIGHT0 + index, GL_SPOT_CUTOFF, angle);
			glLightf(GL_LIGHT0 + index, GL_SPOT_EXPONENT, 256.0f-angle*4.0f);//thislight->falloff);
			break;
		}

		case light_omni:
			// Set light position
			lightparam[0] = thislight->matrix[mat_xpos];
			lightparam[1] = thislight->matrix[mat_ypos];
			lightparam[2] = thislight->matrix[mat_zpos];
			lightparam[3] = 1;	// 1 = positional light type
			glLightfv(GL_LIGHT0 + index, GL_POSITION, lightparam);
			glLightf(GL_LIGHT0 + index, GL_SPOT_CUTOFF, 180);
			glLightf(GL_LIGHT0 + index, GL_SPOT_EXPONENT, 256);
			break;
			
		default:	// must be a directional light
			lightparam[0] = -thislight->matrix[ 8];
			lightparam[1] = -thislight->matrix[ 9];
			lightparam[2] = -thislight->matrix[10];
			lightparam[3] = 0;	// 0 = directional light type (specify direction instead of position)
			glLightfv(GL_LIGHT0 + index, GL_POSITION, lightparam);
	}
	
	// Set Diffuse Values
	lightparam[0] = thislight->colr;
	lightparam[1] = thislight->colg;
	lightparam[2] = thislight->colb;
	lightparam[3] = 1;
	glLightfv(GL_LIGHT0 + index, GL_DIFFUSE, lightparam);
	
	// Clear Ambient light (this is the default on OpenGL)
	
	// Set Specular values
	lightparam[0] = thislight->specr;
	lightparam[1] = thislight->specg;
	lightparam[2] = thislight->specb;
	lightparam[3] = 1;
	glLightfv(GL_LIGHT0 + index, GL_SPECULAR, lightparam);
	
	// Set Attenuation Values
	glLightf(GL_LIGHT0 + index, GL_CONSTANT_ATTENUATION,  thislight->attenuation0);
	glLightf(GL_LIGHT0 + index, GL_LINEAR_ATTENUATION,    thislight->attenuation1);
	glLightf(GL_LIGHT0 + index, GL_QUADRATIC_ATTENUATION, thislight->attenuation2*10);
	
	// Set Outer angles, and Range/falloff
	// thislight->innerangle;
	// thislight->range;
	// thislight->falloff;
	
	// Enable or disable the light
	if (thislight->flags & light_enabled)
		glEnable(GL_LIGHT0 + index);
	else
		glDisable(GL_LIGHT0 + index);
}

void ogl_apprestore(void)
{	fcapppaused = 0;
	ogl_restorestate(NULL);
}

void ogl_restorestate(void *state)
{	if (state)
	{	// Copy this state to the default state variables
		memcpy(&currentstate,state,sizeof(oglstate));
	}
	if (currentstate.cullmode)
	{	glEnable(GL_CULL_FACE);
		glFrontFace(currentstate.cullmode);
	}	else
		glDisable(GL_CULL_FACE);
	ogl_setshademode(currentstate.shademode);
	ogl_sethorizon	(currentstate.horizon);
	glEnable(GL_LIGHTING);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	ogl_setfillmode(currentstate.fillmode);
	ogl_setambientlight(currentstate.ambientlight);

	ogl_changeviewport(currentstate.vpx, currentstate.vpy, currentstate.vpwidth, currentstate.vpheight);	

	if (currentstate.renderstate & RENDERSTATE_ZBUFFER) ogl_enableZbuffer();
		else ogl_disableZbuffer();
	if (currentstate.renderstate & RENDERSTATE_ZWRITES) ogl_enableZwrites();
		else ogl_disableZwrites();
	if (currentstate.renderstate & RENDERSTATE_ANTIALIAS) ogl_enableAA();
		else ogl_disableAA();

	if (currentstate.renderstate & RENDERSTATE_DITHER) ogl_enabledithering();
		else ogl_disabledithering();

	for (GLint i=0; i<texunits; i++)
	{	glActiveTextureARB(GL_TEXTURE0_ARB + i);
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
	}
/*	setrenderstate( D3DRS_NORMALIZENORMALS,	TRUE );
	setrenderstate( D3DRS_LOCALVIEWER,		TRUE );
	d3d_setspecularcolor(currentstate.specularcolor);
	if (currentstate.renderstate & RENDERSTATE_SPECULAR) ogl_enablespecular();
		else ogl_disablespecular();
	if (currentstate.renderstate & RENDERSTATE_FOG)
	{	setrenderstate(D3DRS_FOGENABLE, TRUE); 
		d3d_setfog(currentstate.fogcolor, currentstate.fogstart, currentstate.fogend);
	}	else d3d_disablefog();
*/

//	light3d *l = usedlightsF;
//	while (l)
//	{	d3d_changelight(l);
//		l = l->next;
//	}

/*	for (long i=0; i<8; i++)
	{	// Luminance Scale (only on tex formats with luminance)
		SetTextureStageState( i, D3DTSS_BUMPENVLSCALE, F2DW(4.0f) );
		// Luminance Offset - doesn't appear to do much at all
		SetTextureStageState( i, D3DTSS_BUMPENVLOFFSET, F2DW(0.0f) );
		// Lumpiness scale
		SetTextureStageState( i, D3DTSS_BUMPENVMAT00, F2DW(currentstate.bumpscale) );
		SetTextureStageState( i, D3DTSS_BUMPENVMAT01, F2DW(0.0f) );
		SetTextureStageState( i, D3DTSS_BUMPENVMAT10, F2DW(0.0f) );
		SetTextureStageState( i, D3DTSS_BUMPENVMAT11, F2DW(currentstate.bumpscale) );
		// Set up anisotropic filterring factor
		if (bestmin == D3DTEXF_ANISOTROPIC)
			SetTextureStageState( i, D3DTSS_MAXANISOTROPY, devicecaps.MaxAnisotropy );	
	}
	
	cleartexturecache();
*/
}

void InitOGL(void *_classfinder, long width, long height, long bpp)
{	oglclassfinder = (void *(*)(char(*)))_classfinder;
	matmanager	= (fc_matrix*)		oglclassfinder("matrix");
	bmmanager	= (fc_bitmap*)		oglclassfinder("bitmap");
	gfxmanager	= (fc_graphics *)	oglclassfinder("graphics");
	memmanager	= (fc_memory *)		oglclassfinder("memory");
	sysmanager	= (fc_system *)		oglclassfinder("system");

	// These 2 classes are not available until after OGL is initialised
	mtlmanager	= NULL;
	lgtmanager	= NULL;

	_cammat			= matmanager->getcameramatrix();
	oglhorizon		= (float *)gfxmanager->getvarptr("horizon");
	oglbackgroundrgb= (dword *)gfxmanager->getvarptr("backgroundrgb");
//	wglscreenbpp	= (long *)gfxmanager->getvarptr("screenbpp");
	oglhfov			= (long *)gfxmanager->getvarptr("hfov");
	oglvfov			= (long *)gfxmanager->getvarptr("vfov");
	ogluvmodifier	= (Matrix *)gfxmanager->getvarptr("uvmodifier");
	ogllfbpitch		= (long *)gfxmanager->getvarptr("lfbpitch");
	ogllfb			= (void **)gfxmanager->getvarptr("lfb");

	NullMtl = NULL;
	glGenTextures(1, &NullTexture);
	glBindTexture(GL_TEXTURE_2D, NullTexture);
	float col[4] = {1,1,1,1};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, ColorTexFormat, GL_UNSIGNED_BYTE, col);

	dword features = 0;
	GLint maxanisotropy = 0;
	texunits = 1;
	if (ogl_ExtensionSupported("GL_ARB_multitexture"))
	{	features |= features_multitexture;
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&texunits);
		if (ogl_ExtensionSupported("GL_ARB_texture_border_clamp"))	features |= features_border_clamp;
		if (ogl_ExtensionSupported("GL_ARB_texture_compression"))	features |= features_tex_compression;
		if (ogl_ExtensionSupported("GL_ARB_texture_cube_map"))		features |= features_env_cubemap;
		if (ogl_ExtensionSupported("GL_ARB_texture_env_add"))		features |= features_env_add;
		if (ogl_ExtensionSupported("GL_ARB_texture_env_combine"))	features |= features_env_combine;
		if (ogl_ExtensionSupported("GL_ARB_texture_env_dot3"))		features |= features_env_dot3;
		if (ogl_ExtensionSupported("GL_ARB_texture_anisotropic"))	features |= features_anisotropic;

		if (features & features_anisotropic)
		{	glGetIntegerv(/*GL_TEXTURE_MAX_ANISOTROPY_EXT*/GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxanisotropy);
		}
	}
	GLint comptexformats;
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &comptexformats);
//#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
//#define GL_COMPRESSED_TEXTURE_FORMATS     0x86A3

	ogllog = newlogfile("logs/OpenGL.txt");
	ogllog->log("Starting Trace on OpenGL.");
	ogllog->log("Vendor = %s",(char *)glGetString(GL_VENDOR));
	ogllog->log("Renderer = %s",(char *)glGetString(GL_RENDERER));
	ogllog->log("Version = %s",(char *)glGetString(GL_VERSION));
	ogllog->log("Texture Units = %i",texunits);
	ogllog->log("Max Anisotropy = %i",maxanisotropy);
	ogllog->log("Number Compressed Texture Formats = %i (%s)",comptexformats, (char *)glGetString(GL_COMPRESSED_TEXTURE_FORMATS));
	ogllog->log("Sizeof(Matrix) = %i",sizeof(Matrix));
	ogllog->log("");

	oo255 = 1.0f / 255.0f;
	currentstate.horizon = *oglhorizon;
	currentstate.vpx = 0;
	currentstate.vpy = 0;
	currentstate.vpwidth = width;
	currentstate.vpheight = height;
	currentstate.zoom = 1.0f;
	currentstate.cullmode = GL_CCW;
	currentstate.shademode = shademode_gouraud;
	currentstate.fillmode = fillmode_solid;
	currentstate.renderstate = RENDERSTATE_ZBUFFER | RENDERSTATE_ZWRITES | RENDERSTATE_DITHER;
	currentstate.ambientlight = 0x00ffffff;
	ogl_restorestate(NULL);

	glClearDepth(0.0f);
    glDepthFunc(GL_GEQUAL);
    glFlush();

	matmanager->makeidentity(NullMtx);
	matmanager->makeidentity(oglnegmtx);
	oglnegmtx[10] = -1;

#ifdef ForceRollAlpha
	ogllog->log("Forced to perform Alpha-Rolling");
	ColorTexFormat = GL_RGBA;
#else
	if (!ogl_ExtensionSupported("GL_EXT_BGRA"))
	{	ogllog->log("Warning: OpenGL Extension 'GL_EXT_BGRA' is not supported - Texture maps will be discolored!");
		ColorTexFormat = GL_RGBA;
	}
	ColorTexFormat = GL_BGRA_EXT;
#endif

#ifdef fake2Dlocks
	switch (bpp)
	{	case 0:	
			rendersurface2d = bmmanager->newbitmap(width, height, bitmap_16bit | bitmap_rendertarget);
			break;
		case 16:
			rendersurface2d = bmmanager->newbitmap(width, height, bitmap_16bit | bitmap_rendertarget);
			break;
		case 32:
			rendersurface2d = bmmanager->newbitmap(width, height, bitmap_32bit | bitmap_rendertarget);
			break;
	}
#else
	rendersurface2d = NULL;
#endif
	initzbuckets();
}

void ogl_shutdown(void)
{	ogllog->log("Shutting Down 3D System");
	killbucket(ogltexture,flags,ogltex_memstruct);
	if (rendersurface2d)
	{	bmmanager->_deletebitmap(rendersurface2d);
		rendersurface2d = NULL;
	}
	killZbuckets();

	ogllog->log("The following extensions were available ...");
	for (uintf i=0; i<numextensions; i++)
		ogllog->log(ogl_extension[i]);
	ogllog->log("");
	ogllog->log("Exiting Video Driver");
	delete ogllog;
}

// Incomplete Functions (stubs)
bool ogl_world2screen(float3 *pos,float *sx, float *sy)
{	return false;
}

void ogl_drawbitmap(bitmap *image)
{//	dword *cols = (dword *)image->pixel;
//	glPixelStorei(GL_UNPACK_LSB_FIRST, 1);
	glRasterPos2i(0,0);
	//glPixelTransferi(GL_MAP_COLOR, 1);
	glDrawPixels(image->width, image->height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, image->pixel);
}

/*
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(myvertex), (byte *)(texstart+channel*2));
	glVertexPointer(3, GL_FLOAT, sizeof(myvertex), (byte *)(vertstart));
	glEnableClientState(GL_VERTEX_ARRAY);
	glDrawArrays(GL_QUADS, 0, 24);
*/
