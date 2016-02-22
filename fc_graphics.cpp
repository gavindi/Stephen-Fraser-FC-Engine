/********************************************************************************/
/*					 FC_Graphics - Generic Graphics System						*/
/*	    			 -------------------------------------						*/
/*						(c) 1998-2002 by Stephen Fraser							*/
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#include <stddef.h>						// Needed for offsetof macro (May need to be cstddef for G++)
#include <stdlib.h>
#include "fc_h.h"

// Public Globals
float	mysin[3600+900],*mycos;			// Sin Lookup Table (cos table points to within sin table (sin + 90 degrees)
uintf	screenWidth,screenHeight;		// Integer versions of screen width & height
uintf	screenbpp;						// Current bit depth of the screen
uint32	backgroundrgb=0;				// Background colour in RGB format
float	horizon;						// Distance of the horizon - nothing can be seen past here
intf	hfov,vfov;						// Horizontal and vertical viewing angles (in FC Rotation Units)
long	objsdrawn;						// Number of objects drawn in this frame
long	objsdrawnopt,objsdrawnraw;		// Number of optimised / unoptimies objects drawn
uintf	polysdrawn;						// Number of polygons drawn in this frame

videoinfostruct		videoinfo;			// Information about the video driver (not available until after initgraphics is called)
sScreenMetrics		*desktopMetrics;	// Desktop width, height, and depth

// Video Driver prototypes
char	graphicsDriverFileName[128];	// filename of the video device driver

sDynamicLib *videoDriverPlugin;

// Private Globals
extern	sRenderTarget2D	rt2d_ScreenDisplay;				// Render Target for the screen display

bitmap	*fc_screenBitmap = NULL;
uintf	frametick,framesdrawn,framerate;// Used to calculate framerate (framerate=frames/frametick)

intf	rtwidth3d,rtheight3d;			// Integer Width and height of the 3D render target
bool	GLESWarnings = false;			// Set to true if you want to receive an error for GLES incompatibilities

// External References
//void preplighting(obj3d *o);
void initalphalookups(void);
extern void (*vd_update)(void);			// Update the screen

//******************************************************************************
//***                          Video Driver Routines                         ***
//******************************************************************************
#ifdef STATICDRIVERS
void *vd_init3D(void *classfinder, uintf width, uintf height, uintf depth, uintf bpp);	// Initialize video driver.  Returns a ptr to a function which returns pointers to all other functions based on an ASCII name
#else
void *(*vd_init3D)(void *classfinder, uintf width, uintf height, uintf depth, uintf bpp);	// Initialize video driver.  Returns a ptr to a function which returns pointers to all other functions based on an ASCII name
#endif
void (*video_interface)(const char *,void *);	// Interface to the video driver
//void (*positionwindow)(intf x, intf y);	// Position the application's window (Only useful in windowed mode)
void (*shutDownVideo)(void);			// Shut down the video driver
bitmap *(*getScreenBitmap)(void);
void (*vd_update)(void);				// Update the screen
word (*get3dpixel)(long x,long y);		// Fetch the color of the pixel at this location
void (*apprestore)(void);				// Called to restore the application
void (*winmove)(intf x1, intf y1, intf x2, intf y2);	// Called when the application's window is moved around
// Render state changes
void (*enableZbuffer)(void);			// The Z buffer allows 3D objects to be drawn out of order, and still allows them to appear in the right order
void (*disableZbuffer)(void);			//
void (*enableZwrites)(void);			// Disabling Z Writes means that objects being drawn to the screen do not write to the Z buffer, hence objects drawn later that are farther away may still appear on top of this object
void (*disableZwrites)(void);			//    Z Writing is enabled by default
void (*enablespecular)(void);			// Specular highlighting causes objects to exhibit a shiny surface.  For systems that must calculate lighting in software, specular lighting consumes twice as much processing time
void (*disablespecular)(void);			//    Specular is DISABLED by default
//void (*enableAA)(void);					// AA (AntiAliasing) is used to slightly blur lines as they are drawn to try to eliminate the jagged edges.  USE WITH CAUTION!  Drawing a solid, or textured polygion with this function enabled can cause unexpected results
//void (*disableAA)(void);				//    AA is DISABLED by default (don't confuse AA with FSAA 'Full Screen Anti Aliasing' which is handled elsewhere)
void (*setshademode)(void);				// Set the way polygons are shaded by lighting (what type of interpolation between vertices is implemented).  Under some circumstances, the video driver may simply ignore your settings, this is due to the way the video driver within the OS is written)
void (*setfillmode)(dword fillmode);	// Fill mode determines if the polygon is to be drawn as an outline or as a solid polygon.  (Solid is the default)
void (*sethorizon)(float horizon);		// Sets the distance of the horizon.  Any polygons beyond this point will not be drawn
void (*setambientlight)(dword col);		// Sets the ambient light of the scene
void (*setfog)(dword col, float Start, float End);	// Sets up the fog
void (*enablefog)(void);				// Enables the fog
void (*disablefog)(void);				// Disables the fog (Default)
void (*enabledithering)(void);			// Dithering allows the hardware to improve the quality of low bit-per-pixel screen modes by dithering colors.
void (*disabledithering)(void);			//    Dithering is enabled by default
void (*setzoom)(float zoom);			// Sets the 3D zoom factor
void (*setaspect)(float aspect);		// Sets the 3D aspect ratio
long (*getstatesize)(void);				// Returns the size (in bytes) of a renderstate.
void (*savestate)(void *state);			// Saves the renderstate to a buffer provided by the app.  The size of the buffer must be equal, or larger than the value returned by getstatesize
void (*restorestate)(void *state);		// Restores the renderstate from a buffer provided by the app.  The buffer must have previously been filled by a call to savestate.
void (*rendermode)(long modenum);		// Rendermode is for telling the renderer to perform in a series of strange ways.
//float (*setreflection)(obj3d *mirror,long level, float *mirrormat); // Set a reflection mode
void (*setbumpscale)(float scale);		// Set the Bump mapping scaling factor
// Texture Routines
void (*newtextures)(Texture *tex, dword numtex);	// New texture buckets have been allocated
void (*vd_create3DRenderTarget)(sRenderTarget3D *rt, intf width, intf height, uintf flags);	// Creates the texture and fills in the rt pointer with valid data to make the render target usable.  Flags - set to 0 for now, in the future, you will be able to enable/disable Z buffer, stencil, etc.  Setting to 0 enables a basic Z buffer and no stencil.
void (*select3DRenderTarget)(sRenderTarget3D *rt);
void (*downloadbitmaptex)(Texture *tex, bitmap *bm, uintf miplevel);	// Loads a bitmap into a texture slot
void (*downloadcubemapface)(Texture *_tex, dword facenum, bitmap *bm, uintf miplevel);	// Loads a bitmap into a face of a cubemap.  facenum ranges from 0 to 5.  0:+x  1:-x  2:+y  3:-y  4:+z  5:-z.  Face 0 MUST be loaded first as it sets the dimensions.  Loading different size bitmap into face 0 dumps all faces and resizes the texture, Miplevel is the level number of the mipmap (ignored unless texture_manualmip flag is set)
void (*cleartexture)(Texture *tex);		// Clears a texture from the memory
void (*setTextureFilter)(Texture *tex, uintf filterFlags);

// Matrix Routines
void (*updatecammatrix)(void);			// Indicates that the camera matrix has been updated

// Material Routines
void (*prepMaterial)(Material *mtl);

//void (*updateShaderInput)(ShaderInput *si);	// Indicate that the ShaderInput object has changed
void (*specmodecolor)(long color);		// Used with rendermode to set the specialised color

// Shader Components
void (*vd_compileShaderComponent)(sShaderComponent *result, uintf componentType, const char *source);
void (*deleteShaderComponent)(sShaderComponent *component);
// Shader Programs
void (*vd_linkShaders)(const char *name, sShaderComponent **shaders, uintf numShaders, sShaderProgram *result);
void (*vd_deleteShaderProgram)(sShaderProgram *shaderProgram);
void (*vd_findAttribute)(sShaderProgram *program, uintf elementID, const char *name);
void (*vd_findUniform)(sShaderProgram *program, uintf elementID, const char *name);

// Lighting Routines
void (*disablelights)(uintf firstlight, uintf lastlight);	// LightManagement Code only - Disable all unseen lights
void (*changelight)(light3d *thislight);// Change the settings in a light structure
// Viewport Routines
float (*test_fov)(float3 *pos,float rad); // Tests whether a sphere is within the visible viewport
//dword (*testobjvis)(obj3d *obj);		// Tests whether an object is within the visible viewport
void (*vd_changeviewport)(long x1,long y1,long x2,long y2); // Changes the 3D viewport on the screen, used to keep a renderring within a window

// Drawing Routines
void (*vd_zcls)(void);					// Clear the Z Buffer, but leave the display intact
void (*vd_cls)(uint32 color);			// Clear the screen and the Z buffer
void (*drawObj3D)(newobj *o, float *matrix, void *uniforms);
void (*flushRenderQueue)(eResourceType rt, void *resource);	// Flush a particular resource from the render queue.

//void (*beginscene)(void);				// Lock the screen for 3D rendering
//void (*endscene)(void);					// Unlock the screen for 3D renderring
//byte (*renderobj3d)(obj3d *obj);		// * Internal object renderer.  Apps should call drawobj3d
//void (*bucketobject)(obj3d *obj, void (*drawingfunc)(obj3d *obj));
// void (*flushZsort)(void);				// Flush out the Z sorted objects
// Geometry Routines
//sVertexStreamDepricated *(*createVertexStreamDepricated)(sVertexStreamDescription *description, int numVerts, int numKeyFrames, void *vertexData, int flags);
sVertexStream *(*createVertexStream)(void *rawData, uintf numVerts, uintf vertexSize, uintf flags);
sFaceStream *(*createFaceStream)(int numFaces, void *faceData, int flags);
void (*uploadVertexStream)(sVertexStream *stream);
void (*uploadFaceStream)(sFaceStream *stream);
//void (*vd_newobj3d)(obj3d *obj, vertexFormat objinfo, uintf numfaces, uintf numverts, uintf flags);// Creates the vertex buffer and sets up the pointers
//void (*vd_deleteobj3d)(obj3d *obj);							// Deletes the vertex buffer
//void (*lockmesh)(obj3d *obj, uintf lockflags);				// Lock a mesh for reading or writing
//void (*unlockmesh)(obj3d *obj);								// Unlock a mesh
// Renderer's Capabilities
void (*getvideoinfo)(void *data, uintf structSize);
// Co-ordinate Conversion
bool (*world2screen)(float3 *pos,float *sx, float *sy);
void (*screen2Vector)(float3 *vec, intf sx, intf sy);
void (*shaderUniformConstantInt32)(sShaderProgram *program, const char *name, int32 value);

struct funcfinder
{	const char *stringname;
	void *functionadr;
};

funcfinder vid2Dfunc[]={
	{"shutDownVideo",	(void *)&shutDownVideo},
	{"update",			(void *)&vd_update},
	{"getvideoinfo",	(void *)&getvideoinfo},
	{"getScreenBitmap",	(void *)&getScreenBitmap},
	{"apprestore",		(void *)&apprestore},
	{"winmove",			(void *)&winmove}
};
uintf num2Dvidfuncs = sizeof(vid2Dfunc)/sizeof(funcfinder);

funcfinder vid3Dfunc[]={
	// Initialisation
	{"getvideoinfo",			(void *)&getvideoinfo},
	{"shutDownVideo",			(void *)&shutDownVideo},

	// Matricies
	{"updatecammatrix",			(void *)&updatecammatrix},

	// Lighting
	{"setambientlight",			(void *)&setambientlight},
	{"changelight",				(void *)&changelight},

	// Materials
	{"prepMaterial",			(void *)&prepMaterial},

	// Shader Components
	{"compileShaderComponent",	(void *)&vd_compileShaderComponent},
	{"deleteShaderComponent",	(void *)&deleteShaderComponent},
	// Shader Programs
	{"linkShaders",				(void *)&vd_linkShaders},
	{"deleteShaderProgram",		(void *)&vd_deleteShaderProgram},
	{"findAttribute",			(void *)&vd_findAttribute},
	{"findUniform",				(void *)&vd_findUniform},
	// Shader Uniform Constants
	{"shaderUniformConstantInt32",(void *)&shaderUniformConstantInt32},

	// Renderring
	{"cls",						(void *)&vd_cls},
	{"drawObj3D",				(void *)&drawObj3D},
	{"update",					(void *)&vd_update},
	{"flushRenderQueue",		(void *)&flushRenderQueue},

	// Render States
	{"enableZbuffer",			(void *)&enableZbuffer},
	{"disableZbuffer",			(void *)&disableZbuffer},

	// Textures
	{"downloadbitmaptex",		(void *)&downloadbitmaptex},
	{"cleartexture",			(void *)&cleartexture},
	{"setTextureFilter",		(void *)&setTextureFilter},
	{"create3DRenderTarget",	(void *)&vd_create3DRenderTarget},
	{"select3DRenderTarget",	(void *)&select3DRenderTarget},

	// Geometry
	{"createVertexStream",		(void *)&createVertexStream},
	{"createFaceStream",		(void *)&createFaceStream},
	{"uploadVertexStream",		(void *)&uploadVertexStream},
	{"uploadFaceStream",		(void *)&uploadFaceStream},

	// Depricated Functions
//	{"createVertexStreamDepricat",	(void *)&createVertexStreamDepricated},

/*	{"positionwindow",			(void *)&positionwindow},
	{"get3dpixel",				(void *)&get3dpixel},
	{"apprestore",				(void *)&apprestore},
	{"winmove",					(void *)&winmove},
	// Render State Changes
	{"enableZwrites",			(void *)&enableZwrites},
	{"disableZwrites",			(void *)&disableZwrites},
	{"enablespecular",			(void *)&enablespecular},
	{"disablespecular",			(void *)&disablespecular},
	{"enableAA",				(void *)&enableAA},
	{"disableAA",				(void *)&disableAA},
	{"setshademode",			(void *)&setshademode},
	{"setfillmode",				(void *)&setfillmode},
	{"sethorizon",				(void *)&sethorizon},
	{"setfog",					(void *)&setfog},
	{"enablefog",				(void *)&enablefog},
	{"disablefog",				(void *)&disablefog},
	{"enabledithering",			(void *)&enabledithering},
	{"disabledithering",		(void *)&disabledithering},
	{"setzoom",					(void *)&setzoom},
	{"setaspect",				(void *)&setaspect},
	{"getstatesize",			(void *)&getstatesize},
	{"savestate",				(void *)&savestate},
	{"restorestate",			(void *)&restorestate},
	{"rendermode",				(void *)&rendermode},
	{"setreflection",			(void *)&setreflection},
	{"setbumpscale",			(void *)&setbumpscale},
	// Texture Routines
	{"newtextures",				(void *)&newtextures},
	{"downloadcubemapface",		(void *)&downloadcubemapface},
	{"specmodecolor",	(void *)&specmodecolor},
	{"updateShaderInput",(void*)&updateShaderInput},
	// Lighting Routines
	{"disablelights",	(void *)&disablelights},
	// Viewport Routines
	{"test_fov",		(void *)&test_fov},
	{"testobjvis",		(void *)&testobjvis},
	{"changeviewport",	(void *)&vd_changeviewport},
	// Drawing Routines
	{"zcls",			(void *)&vd_zcls},
	{"renderobj3d",		(void *)&renderobj3d},
	{"bucketobject",	(void *)&bucketobject},
	{"flushZsort",		(void *)&flushZsort},
	// Geometry Routines
	{"newobj3d",		(void *)&vd_newobj3d},
	{"deleteobj3d",		(void *)&vd_deleteobj3d},
	// Renderer's Capabilities
	// Co-ordinate conversion system
	{"world2screen",	(void *)&world2screen},
	{"screen2Vector",	(void *)&screen2Vector}
*/

};
uintf num3Dvidfuncs = sizeof(vid3Dfunc)/sizeof(funcfinder);

// To add another function:
//	1) extern the pointer in FCIO.H (unless it's only used internally)
//  2) Within all the device driver files, add a new entry into vidfuncs
//	3) Place the pointer into fcviddrv, and add in a new entry into initvideodriver

void invalidvideoplugin(void)
{	msg("Plugin Error","Error: This program attempted to access a function in the 3D Video driver plugin\r\nBut either the function was not found, or the driver is not initialized");
}

void invalid2DFunc(void)
{	msg("Invalid 2D Function","Error: This program attempted to access a 3D-only feature from a 2D graphics renderer");
}

void invalid3DFunc(void)
{	msg("Invalid 3D Function","Error: This program attempted to access a 2D-only feature from a 3D graphics renderer");
}

void noVideoDriverInited(void)
{	msg("No Video Driver Initialized","Error: This program attempted to perform a graphics operation, but no video driver has been initialized");
}

void NULL_winmove(intf x1, intf y1, intf x2, intf y2)
{}

void swrender_interface(char *fname, void **func);

void clear3Dfuncs(void (*callback)(void))
{	for (uintf i=0; i<num3Dvidfuncs; i++)
	{	void **x = (void **)vid3Dfunc[i].functionadr;
		*x = (void *)callback;
	}
	// Now initialize any functions which may be called before initialization is complete
	winmove = NULL_winmove;

	shutDownVideo = NULL;						// In case a shutdown is ordered during video driver initialization
}

void clear2Dfuncs(void (*callback)(void))
{	for (uintf i=0; i<num2Dvidfuncs; i++)
	{	void **x = (void **)vid2Dfunc[i].functionadr;
		*x = (void *)callback;
	}
	// Now initialize any functions which may be called before initialization is complete
	winmove = NULL_winmove;

	shutDownVideo = NULL;						// In case a shutdown is ordered during video driver initialization
}

void init3Ddriver(uintf screenwidth, uintf screenheight, uintf screenbpp)
{
#ifndef STATICDRIVERS
	if (!videoDriverPlugin)
	videoDriverPlugin   = loadPlugin(viddrvfilename);
	vd_init3D			= (void *(*)(void *,uintf, uintf, uintf, uintf))videoDriverPlugin->initPlugin;
#endif

	clear2Dfuncs((void (*)(void))invalid3DFunc);
	void *x = vd_init3D((void *)classfinder,screenwidth,screenheight,5000,screenbpp);	// If this is NULL, the current video driver isn't compatible with this machine		// ### Need to pass in depth instead of using 5000!
	video_interface = (void (*)(const char *,void *))x;							// Woohoo!  Check it out!
	for (uintf i=0; i<num3Dvidfuncs; i++)
	{	video_interface(vid3Dfunc[i].stringname, vid3Dfunc[i].functionadr);
	}
}


void init2Ddriver(uintf screenwidth, uintf screenheight, uintf screenbpp)
{	clear3Dfuncs((void (*)(void))invalid2DFunc);
	void *x = vd_init2D((void *)classfinder,screenwidth,screenheight,screenbpp);
	if (!x)
	{	// ### TODO - Report that the video driver failed to start up (May not be compatible with this machine ... possibly set up another return variable
		exit(1);
	}
	video_interface = (void (*)(const char *,void *))x;							// Woohoo!  Check it out!
	for (uintf i=0; i<num2Dvidfuncs; i++)
	{	video_interface(vid2Dfunc[i].stringname, vid2Dfunc[i].functionadr);
	}
}

void initgraphicssystem(void)
{	long i;

	// Initialize sin/cos tables
	mycos = &mysin[900];
	for (i=0; i<(3600+900); i++)
	{	mysin[i]=(float)sin(3.14159265358*2*((float)i/3600.0f));
	}
	horizon = 5000;

	makeidentity(cammat);

	// Clear out all video functions
	clear3Dfuncs(noVideoDriverInited);
	clear2Dfuncs(noVideoDriverInited);
	shutDownVideo = NULL;
}

void killgraphics(void)
{	//killbillboard();
	select2DRenderTarget(NULL);
	killmaterial();
	if (_fctexture)		{	killtexture();	_fctexture = NULL;	}
	if (_fclighting)	{	killlighting();	_fclighting= NULL;	}
	if (fc_screenBitmap) deleteBitmap(fc_screenBitmap);				// ### This is a bug, we don't own the screen bitmap, the video driver does!
	if (shutDownVideo)
	{	if (shutDownVideo!=invalidvideoplugin)
			shutDownVideo();
	}
}

void initGraphics(uintf graphicsMode,uintf width, uintf height, uintf bpp)
{	if (shutDownVideo) shutDownVideo();

	// Perform standard 3D world inits
	width = width&0xfffffffc;
	screenWidth	 = width;
	screenHeight = height;

	switch (graphicsMode & 0x0000000f)
	{	case gfxmode_GUI:	// Deliberate fallthrough to 2D mode for now

		case gfxmode_2D:
		{	init2Ddriver(width,height,bpp);
			fc_screenBitmap = getScreenBitmap();
			create2DRenderTarget(&rt2d_ScreenDisplay,fc_screenBitmap);
			select2DRenderTarget(&rt2d_ScreenDisplay);
			break;
		}

		case gfxmode_3D:
		{	// This is the currently supported mode
			fc_screenBitmap = newbitmap("2D RT Safety Net",1,1,bitmap_x8r8g8b8 | bitmap_RenderTarget);
			create2DRenderTarget(&rt2d_ScreenDisplay,fc_screenBitmap);
			select2DRenderTarget(&rt2d_ScreenDisplay);
			init3Ddriver(screenWidth,screenHeight,bpp);
			setcammatrix();
			_fclighting = initlighting();
			_fctexture = inittexture();
			fc_initShaders();
			initmaterial();
			initbillboards();
			break;
		}
		default:
		{	msg("Error","Unrecognised Graphics mode in call to initGraphics.\nDid you use the old method of requesting a render core?");
			break;
		}
	}

	getvideoinfo(&videoinfo,sizeof(videoinfo));
	fcapplaunched = 1;
}

void update(void)
{	checkmsg();
	vd_update();
//	checkmsg();
//	if (audioworks) audio_update();
	framesdrawn++;
}

long getframerate(void)
{	if (frametick>=(currentTimerHz))
	{	frametick-=currentTimerHz;
		framerate=framesdrawn;
		framesdrawn=0;
	}
	return framerate;
}

void enableGLESWarnings(void)
{	GLESWarnings=true;
}

// ****************************************************************************
// ***							Billboarding code							***
// ****************************************************************************
extern uint16		quadFacesIndices[];
extern sFaceStream	*quadFacesStream;

newobj				bbobj,hudobj;
Material			*BBMtl,*HUDMtl;
sShaderComponent	*BBVtxShader, *HUDVtxShader, *HUDandBBFrgShader;

const char *HudName = "Engine HUD Object";
const char *BBName = "Engine Billboard Object";

// -------------------------------- HUD Setup ------------------------------
struct sHudVertex
{	float x1,y1,x2,y2;
	float u,v;
} hudVertex[4] = {
	{	1, 0,0, 1,	0,1	},			// 0: Top Left
	{	0, 0,1, 1,	1,1	},			// 1: Top Right
	{	1, 1,0, 0,	0,0	},			// 2: Bottom Left
	{	0, 1,1, 0,	1,0	}			// 3: Bottom Right
};

const char *hudVSStr =
"varying	vec2	UVpassthru;\n"
"uniform	vec4	rect;\n"
"uniform	vec2	UVScale;\n"
"attribute	vec4	scale;\n"
"attribute	vec2	UV;\n"
"void main(void)\n"
"{	vec4 tmp = scale*rect;\n"
"	gl_Position = vec4(tmp.x+tmp.z, tmp.y+tmp.w, 0.0, 1.0);\n"
"	UVpassthru = UV*UVScale;\n"
//"	UVpassthru = UV;\n"
"}";
struct sHudUniform
{	float	x1,y1,x2,y2;
	float2	UVScale;
} HudUniforms;

// ------------------------------ Billboard Setup -------------------------
struct sBBVertex
{	float x,y;
	float u,v;
} bbVertex[4] = {
	{	-1, 1, 0,0	},		// 0: Top Left
	{	-1,-1, 0,1	},		// 1: Bottom Left
	{	 1, 1, 1,0	},		// 2: Top Right
	{	 1,-1, 1,1	}		// 3: Bottom Right
};

const char *bbVSStr =
"uniform	mat4	matrix;\n"
"uniform	vec2	scale;\n"
"uniform	vec3	worldView;\n"
"varying	vec2	UVpassthru;\n"
"attribute	vec2	position;\n"
"attribute	vec2	UV;\n"
"void main(void)\n"
"{	vec2 tmp = vec2(position * scale + worldView.xy);\n"	//  projection * (position + vec4(worldView[3].xyz, 0));
"	gl_Position = matrix * vec4(tmp, worldView.z, 1.0);\n"
"	UVpassthru = UV;\n"
"}";
struct sBBUniforms
{	float	*matrix;
	float	scaleX, scaleY;
	float3	worldView; //X, worldViewY, worldViewZ;
} bbUniforms;

// ---------------------- Billboard and HUD Fragment Shader ---------------------
const char *BBandHUD_FSStr =
"uniform sampler2D tex0;\n"
"varying vec2 UVpassthru;\n"
"void main(void)\n"
"{	vec4 col = texture2D(tex0, UVpassthru);\n"
//"	if (col.a<0.01) discard;\n"
"	gl_FragColor = col;\n"
"}";

void initbillboards(void)
{	sShaderProgram *shaderProgram;

	// ********************* BB and HUD Common Elements ********************************
	HUDandBBFrgShader = newShaderComponent(shaderComponent_Fragment,BBandHUD_FSStr);

	// *************************** Heads-Up Display ************************************
	// Create Shaders
	HUDVtxShader = newShaderComponent(shaderComponent_Vertex, hudVSStr);
	shaderProgram = newShaderProgram(HUDVtxShader, HUDandBBFrgShader, sizeof(sHudVertex));
	shaderUniformConstantInt32(shaderProgram, "tex0", 0);
	shaderAttribute(shaderProgram, "scale", vertex_float, 4, offsetof(sHudVertex,x1));
	shaderAttribute(shaderProgram, "UV", vertex_float, 2, offsetof(sHudVertex,u));
	shaderUniform(shaderProgram, "rect", vertex_float, 4, offsetof(sHudUniform,x1));
	shaderUniform(shaderProgram, "UVScale", vertex_float, 2, offsetof(sHudUniform,UVScale));

	// Create Material
	HUDMtl = newmaterial(HudName);
	HUDMtl->finalBlendOp = finalBlend_add;
	HUDMtl->finalBlendSrcScale = blendParam_SrcAlpha;
	HUDMtl->finalBlendDstScale = blendParam_InvSrcAlpha;
	HUDMtl->shader = shaderProgram;

	// Create HUD Object
	hudobj.name = (char *)HudName;
	hudobj.mtl = HUDMtl;
	hudobj.flags = 0;
	hudobj.lighting = NULL;
	hudobj.maxlights = 0;
	hudobj.preDrawCallback = NULL;
	makeidentity(hudobj.matrix);

	// Create Geometry
	hudobj.faces = quadFacesStream;
	hudobj.verts = createVertexStream((void *)&hudVertex, 4, sizeof(sHudVertex), vertexStreamFlags_modifyOnce);
	uploadVertexStream(hudobj.verts);

	// *************************** Billboard ************************************
	// Create Shaders
	BBVtxShader = newShaderComponent(shaderComponent_Vertex, bbVSStr);
	shaderProgram = newShaderProgram(BBVtxShader, HUDandBBFrgShader, sizeof(sBBVertex));
	shaderUniformConstantInt32(shaderProgram, "tex0", 0);
	shaderAttribute(shaderProgram, "position", vertex_float, 2, offsetof(sBBVertex,x));
	shaderAttribute(shaderProgram, "UV", vertex_float, 2, offsetof(sBBVertex,u));
	shaderUniform(shaderProgram, "matrix", vertex_mat4x4Ptr, 1, offsetof(sBBUniforms,matrix));
	shaderUniform(shaderProgram, "scale", vertex_float, 2, offsetof(sBBUniforms,scaleX));
	shaderUniform(shaderProgram, "worldView", vertex_float, 3, offsetof(sBBUniforms,worldView));
	bbUniforms.matrix = projectionMatrix;

	// Create Material
	BBMtl = newmaterial(BBName);
	BBMtl->finalBlendOp = finalBlend_add;
	BBMtl->finalBlendSrcScale = blendParam_SrcAlpha;
	BBMtl->finalBlendDstScale = blendParam_InvSrcAlpha;
	BBMtl->shader = shaderProgram;

	// Create Billboard Object
	bbobj.name = (char *)BBName;
	bbobj.mtl = BBMtl;
	bbobj.flags = 0;
	bbobj.lighting = NULL;
	bbobj.maxlights = 0;
	bbobj.preDrawCallback = NULL;
	makeidentity(bbobj.matrix);
	// Create Geometry
	bbobj.faces = quadFacesStream;
	bbobj.verts = createVertexStream((void *)&bbVertex, 4, sizeof(sBBVertex), vertexStreamFlags_modifyOnce);
	uploadVertexStream(bbobj.verts);
}

void drawHud(Texture *tex, float x1, float y1, float x2, float y2)
{	// Draw a HUD where the co-ordinates represent values between -1 to +1 (-1 is left / bottom of screen)

	hudobj.mtl->texture[0]=tex;					// ### Changing material properties
	HudUniforms.x1 = x1*2.0f-1.0f;
	HudUniforms.y1 = 1.0f-y1*2.0f;
	HudUniforms.x2 = x2*2.0f-1.0f;
	HudUniforms.y2 = 1.0f-y2*2.0f;
	HudUniforms.UVScale.x = 1.0f;
	HudUniforms.UVScale.y = 1.0f;
	drawObj3D(&hudobj, hudobj.matrix, &HudUniforms);
}

void drawHudScaledUV(Texture *tex, float x1, float y1, float x2, float y2)
{	// Draw a HUD where the co-ordinates represent values between -1 to +1 (-1 is left / bottom of screen)

	hudobj.mtl->texture[0]=tex;					// ### Changing material properties
	HudUniforms.x1 = x1*2.0f-1.0f;
	HudUniforms.y1 = 1.0f-y1*2.0f;
	HudUniforms.x2 = x2*2.0f-1.0f;
	HudUniforms.y2 = 1.0f-y2*2.0f;
	HudUniforms.UVScale = tex->UVscale;
	drawObj3D(&hudobj, hudobj.matrix, &HudUniforms);
}

void drawHudScreen(Texture *tex, intf x1, intf y1, intf x2, intf y2)
{	// Draw a HUD where the co-ordinates represent screen co-ordinates where the top left is 0,0
	hudobj.mtl->texture[0]=tex;					// ### Changing material properties
	HudUniforms.x1 = ((float)(x1*2)/(float)screenWidth)-1.0f;
	HudUniforms.y1 = -(((float)(y1*2)/(float)screenHeight)-1.0f);
	HudUniforms.x2 = ((float)(x2*2)/(float)screenWidth)-1.0f;
	HudUniforms.y2 = -(((float)(y2*2)/(float)screenHeight)-1.0f);
	drawObj3D(&hudobj, hudobj.matrix, &HudUniforms);
}

void setHudMaterial(Material *mtl)
{	hudobj.mtl = mtl;						// ### Changing object's material
}

void drawBillboard(Texture *tex, float *mtx, float sizeX, float sizeY)
{	float tmpMtx[16];
	bbobj.mtl->texture[0]=tex;					// ### Changing material properties
	bbUniforms.scaleX = sizeX;
	bbUniforms.scaleY = sizeY;
	matmult(tmpMtx, invcammat, mtx);
	bbUniforms.worldView.x = tmpMtx[mat_xpos];
	bbUniforms.worldView.y = tmpMtx[mat_ypos];
	bbUniforms.worldView.z = tmpMtx[mat_zpos];
	drawObj3D(&bbobj, mtx, &bbUniforms);
}

void zcls(void)
{	// Clears the Z buffer - Propper use of this requires some imagination
	vd_zcls();
	objsdrawn=0;
	objsdrawnopt=0;
	objsdrawnraw=0;
	polysdrawn=0;
}

void cls3D(uint32 color)
{	// Clear the screen AND the Z buffer
	vd_cls(color);
	objsdrawn=0;
	objsdrawnopt=0;
	objsdrawnraw=0;
	polysdrawn=0;
}

void changeviewport(long x1,long y1,long x2,long y2)
{	vd_changeviewport(x1,y1,x2,y2);
}

Matrix NULLmtx;

// -------------------------------- drawobj3d -------------------------------------------
/*
byte drawobj3d(obj3d *obj)
{	// This is the entry point to the most important function in the FC engine ... all 3D
	// graphics are passed through this function.
	byte render;

	if (obj->flags & obj_meshlockwrite) msg("Drawobj3d Error","Attempted to draw an obj3d which is locked");

	if (!(obj->flags & obj_nofov))
		if (!testobjvis(obj)) return 0;

	// Process animation
	if (obj->numkeyframes>1)
	{	if (obj->frame1>=(dword)obj->numkeyframes) obj->frame1=obj->numkeyframes-1;
		if (obj->frame2>=(dword)obj->numkeyframes) obj->frame2=obj->numkeyframes-1;
		obj->decodeframe(obj);
	}

	// Prepare Matrix (Temporary function - matricies will be compulsory soon)
	bool needsMtx;
	if (!obj->matrix)
	{	needsMtx = true;
		makeidentity(NULLmtx);
		NULLmtx[mat_xpos] = obj->worldpos.x;
		NULLmtx[mat_ypos] = obj->worldpos.y;
		NULLmtx[mat_zpos] = obj->worldpos.z;
		obj->matrix = NULLmtx;
	}	else
		needsMtx = false;

	// *** Note: The call to preplighting requires a valid material structure to be in place ***
	if (!obj->material)
	{	uint32 col = obj->wirecolor;
		NULLmtl->diffuse.a = (float)(( col & 0xff000000 ) >> 24) * oo255;
		NULLmtl->diffuse.r = (float)(( col & 0x00ff0000 ) >> 16) * oo255;
		NULLmtl->diffuse.g = (float)(( col & 0x0000ff00 ) >>  8) * oo255;
		NULLmtl->diffuse.b = (float)(( col & 0x000000ff )      ) * oo255;
		obj->material = NULLmtl;
		preplighting(obj);					// Prepare any managed lights required for this object
		render = renderobj3d(obj);			// Render the object (within the video driver)
		obj->material = NULL;
	}	else
	{	preplighting(obj);					// Prepare any managed lights required for this object
		render = renderobj3d(obj);			// Render the object (within the video driver)
	}

	if (needsMtx)
		obj->matrix = NULL;

	if (render)
	{	// Increase the count of obects drawn
		objsdrawn++;						// Total objects drawn
		polysdrawn+=obj->numfaces;
		objsdrawnraw++;					// Raw (unoptimised) objects drawn
	}
	return render;
}
*/

void uint32ToFloat4(float4 *col, uint32 icol)
{	col->a = (float)((icol & 0xff000000) >> 24) * oo255;
	col->r = (float)((icol & 0x00ff0000) >> 16) * oo255;
	col->g = (float)((icol & 0x0000ff00) >>  8) * oo255;
	col->b = (float)((icol & 0x000000ff)      ) * oo255;
}

uint32 Float4To32Clamp(float4 *col)
{	uint32 result;
	float tmp = col->a;
	if (tmp<0.0f) tmp = 0.0f;
	if (tmp>1.0f) tmp = 1.0f;
	result = (uint32)(tmp * 255.0f)<<24;

	tmp = col->r;
	if (tmp<0.0f) tmp = 0.0f;
	if (tmp>1.0f) tmp = 1.0f;
	result |= (uint32)(tmp * 255.0f)<<16;

	tmp = col->g;
	if (tmp<0.0f) tmp = 0.0f;
	if (tmp>1.0f) tmp = 1.0f;
	result |= (uint32)(tmp * 255.0f)<<8;

	tmp = col->b;
	if (tmp<0.0f) tmp = 0.0f;
	if (tmp>1.0f) tmp = 1.0f;
	result |= (uint32)(tmp * 255.0f);

	return result;
}

uint32 float4To32Abs(float4 *col)
{	uint32 result;
	float tmp;

	tmp = col->a;
	if (tmp<0.0f) tmp = -tmp;
	if (tmp>1.0f) tmp = 1.0f;
	result = (uint32)(tmp * 255.0f)<<24;

	tmp = col->r;
	if (tmp<0.0f) tmp = -tmp;
	if (tmp>1.0f) tmp = 1.0f;
	result = (uint32)(tmp * 255.0f)<<16;

	tmp = col->g;
	if (tmp<0.0f) tmp = -tmp;
	if (tmp>1.0f) tmp = 1.0f;
	result = (uint32)(tmp * 255.0f)<<8;

	tmp = col->b;
	if (tmp<0.0f) tmp = -tmp;
	if (tmp>1.0f) tmp = 1.0f;
	result = (uint32)(tmp * 255.0f);

	return result;
}

// ----------------------------- Screen Reading Functions -------------------------------------
/*
dword addscreencolors(void)
{	// Used to guage screen overdraw
	dword result;
	result = 0;
	uintf *ylookup = (uintf *)current2DRenderTarget->bm->renderinfo;
	for (uintf y=0; y<screenHeight; y++)
	{	word *pixel = (word *)(((byte *)lfb) + ylookup[y]);
		for (uintf x=0; x<screenWidth; x++)
			result+=pixel[x];
	}
	return result;
}
*/

/*
long getclickobj(group3d *g1,long x, long y)
{	dword backcol = backgroundrgb;
	backgroundrgb = 0;
	cls3D();
	rendermode(1);
	for (dword i=0; i<g1->numobjs; i++)
	{ 	specmodecolor(i+1);
		obj3d *o = g1->obj[i];
		drawobj3d(o);
	}
	rendermode(0);
	backgroundrgb = backcol;
	long result = get3dpixel(x,y)-1;
	if (result>(long)g1->numobjs) result = -1;	// Avoid a potential crash
	return result;
}
*/


/*
Command
0000	add		dest,src1,src2		dest = src1 + src2
0001	cmp		dest,src1,src2		if (r0.a > 0.5f) dest = src1 else dest = src2
0002	dot		dest,src1,src2		dest = src1 . scr2  (all filtered dest components = dot)
0003	lrp		dest,src1,src2,src3	dest = src1 * src2 + (1-src1)*src3
0004	mad		dest,src1,src2,src3	dest = src1 * src2 + src3
0006	mov		dest,src1
0007	mul		dest,src1,src2		dest = src1 * src2
0008    sub		dest,src1,src2		dest = src1 - src2


addTokenID(tokentype_cmd, 'add', 0);
addTokenID(tokentype_cmd, 'cmp', 1);

struct InterpretToken
{	uintf	type;
	char	name[16];
	uintf	value;
}

struct InterpreterData
{	uintf	numtokens;
	InterpretToken	tokens[128];
	uintf	tokenchecksum[128];
	interpreter *next, *pref
	uintf	flags;
}

struct InterpreterToken
{	uintf	tokenType;
	uintf	value;
}

class Interpreter
{	public:
		uintf	getNumTokens(uintf tokenType);
		uintf	getLastError(void)
		bool	addToken(uintf tokenType, char *name, uintf value);
		void	pushStatus(void);		// Push the current set of tokens to the stack
		bool	pullStatus(void);		// Pull the current set of tokens from the stack
		bool	addFile(char *filename);// Add a text file to be processed
		bool	addString(char *text);	// Add a string to be processed
		bool	getToken(InterpreterToken *result);
}

*/
