/*******************************************************************************/
/*  				     FC_H Global Include file.                 */
/*	    				---------------------------            */
/*  				    (c) 1998 by Stephen Fraser                 */
/*                                                                             */
/* Contains private prototypes and externs                                     */
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#ifdef RENDER_EXPORTS
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#define FC_BuildVer 1000

#ifdef __ANDROID__
#include <stdarg.h>
#define UsingGLES
#endif

#include <cstdarg>
#include <stdio.h>
#include <fcio.h>

#define UnfinishedErr "Unfinished Engine Code"

struct sEngineModule
{	const	char *ModuleName;
	uintf	staticMemoryNeeded;
	void	(*initialize)(byte *memory);
	void	(*shutdown)(void);
};

// Engine warnings
enum EngineWarningParamType
{	EngineWarningParam_NoParam			= 0,
	EngineWarningParam_String,
	EngineWarningParam_FORCEDWORD		= 0x7fffffff
};

extern void	*EngineWarningParam;

// Plugins
extern char pluginfilespecs[][256];

void OSCloseEvent(void);

// System-Dependant redirected functions
void startupwarning(const char *title, const char *message);

// Video Global Variables
extern intf	fcapplaunched;		// 1 = FC application has been launched

void initvideodriver(intf screenwidth,intf screenheight,intf screenbpp);
void initbillboards(void);
void killbillboard(void);

// Video Driver functions that are not revealed to the app
//extern void (*vd_newobj3d)(obj3d *obj, vertexFormat objinfo, uintf numfaces, uintf numverts, uintf flags);	// Creates the vertex buffer and sets up the pointers
//extern void (*vd_deleteobj3d)(obj3d *obj);							// Deletes the vertex buffer
extern void (*apprestore)(void);							// Called to restore the application
extern void (*winmove)(intf x1, intf y1, intf x2, intf y2);	// Called when the application's window is moved around

// 3D system variables
extern float oo255;
extern void	*lfb;						// 3D Linear Frame Buffer
extern intf	lfbpitch,vd_lfbpitch;		// Increment in bytes between scanlines on the display
extern uintf framesdrawn;

// Shader Components
extern void (*vd_compileShaderComponent)(sShaderComponent *result, uintf componentType, const char *source);
// Shader Programs
extern void (*vd_linkShaders)(const char *name, sShaderComponent **shaders, uintf numShaders, sShaderProgram *result);
extern void (*vd_deleteShaderProgram)(sShaderProgram *shaderProgram);
extern void (*vd_findAttribute)(sShaderProgram *program, uintf elementID, const char *name);
extern void (*vd_findUniform)(sShaderProgram *program, uintf elementID, const char *name);

extern mInput		*input;
extern fc_bitmap	*_fcbitmap;
extern fc_texture	*_fctexture;
extern fc_text		*_fctext;
extern fc_lighting	*_fclighting;
extern fc_plugin 	*_fcplugin;
extern void			*_OEMpointer;

extern logfile		*errorslog;

void		initmemoryservices(void);		void killmemory(void);
void		 initdisk(void);				void killdisk(void);
mInput		*initinput(uintf securityCode);
fc_bitmap	*initbitmap(void);				void killbitmap(void);
void		initgeometry(void);				void killgeometry(void);
void		initmatrix(void);				void killmatrix(void);
fc_texture	*inittexture(void);				void killtexture(void);
fc_text		*inittext(void);				void killtext(void);
void		initgraphicssystem(void);		void killgraphics(void);
void		fc_initShaders(void);
void		initmaterial(void);				void killmaterial(void);
fc_lighting *initlighting(void);			void killlighting(void);
fc_printer	*initprinter(void);				void killprinter(void);
void		 initBucketGui(void);			void killBucketGui(void);
void		 initutils(void);				void killutils(void);

void *vd_init2D(void *classfinder,intf width, intf height, intf bpp);
extern sDynamicLib *videoDriverPlugin;

#ifdef STATICDRIVERS
DLL_EXPORT void *vd_init3D(void *classfinder,uintf width, uintf height, uintf depth, uintf bpp);	// Initialize 3D video driver.  Returns a ptr to a function which returns pointers to all other functions based on an ASCII name
#else
extern void *(*vd_init3D)(void *classfinder,uintf width, uintf height, uintf depth, uintf bpp);
#endif
extern void (*downloadbitmaptex)(Texture *tex, bitmap *bm, uintf miplevel);	// Loads a bitmap into a texture slot.  Allocate a texture slot first through newtexture.  tex = pointer to texture slot, bm = pointer to the bitmap, set flags to 0, except for bump maps which should be texture_envbump.  Bump mapped textures must be 8 bit paletised where the palette is greyscale.  Non-bump mapped textures must be 32 bit.  Miplevel is the level number of the mipmap (ignored unless texture_manualmip flag is set)


void initLUTs(void);
bool initswrender(void);
void resourceError(const char *res);

// Audio Global Variables
extern bool music_finished;	// Flag for triggering the event to call the mod finished function
void audio_update(void);
void killaudio(void);

// Keyboard prototypes
void keyhit(uintf keycode);
void keyrelease(uintf keycode);

// Windows Prototypes
void _exitfc();

// Video Driver Prototypes
void killvid(void);

// FC starting point
void fclaunch(void);
void fcmain(void);

#define state_zbuffer	0x01
#define state_texture   0x02
#define state_gouraud   0x04
#define state_bilinear  0x08
#define state_alpha		0x10
#define state_specular	0x20
#define state_mipmap	0x40
#define	state_aalias	0x80
#define state_fog		0x0100

// Music prototypes (should be removed from here later)
void midiinit(void);
void midiplay(char *flname);
void midistop(void);
void midiclose(void);

// predefines that I don't feel like sharing with the world
extern byte tolower_array[];	// Array for converting text to lower case : char lower = tolower_array[original_char];
extern byte toupper_array[];	// Array for converting text to lower case : char upper = toupper_array[original_char];

#define texpathsize 1024			// Size in bytes of the texture path

void myvstrintf(char *buffer, uintf size, const char *format, va_list argptr);

#define constructstring(dest,txt,size)	\
{	va_list x;							\
	va_start(x,txt);					\
	myvstrintf(dest,size,txt,x);		\
	va_end(x);							\
}

void _cdecl systemMessage(char *txt,...);

