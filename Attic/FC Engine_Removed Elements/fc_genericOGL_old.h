#ifdef WIN32
#include <windows.h>
#include <gl/gl.h>
#include "glext.h"

extern	PFNGLMULTITEXCOORD1FARBPROC		glMultiTexCoord1fARB;
extern	PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2fARB;
extern	PFNGLMULTITEXCOORD3FARBPROC		glMultiTexCoord3fARB;
extern	PFNGLMULTITEXCOORD4FARBPROC		glMultiTexCoord4fARB;
extern	PFNGLACTIVETEXTUREARBPROC		glActiveTextureARB;
extern	PFNGLCLIENTACTIVETEXTUREARBPROC	glClientActiveTextureARB;
extern	PFNGLCOMPRESSEDTEXIMAGE2DPROC	glCompressedTexImage2D;
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include "glext.h"
#define	glMultiTexCoord1fARB glMultiTexCoord1f
#define glMultiTexCoord2fARB glMultiTexCoord2f
#define glMultiTexCoord3fARB glMultiTexCoord3f
#define glMultiTexCoord4fARB glMultiTexCoord4f
#define glActiveTextureARB	 glActiveTexture
#define glClientActiveTextureARB glClientActiveTexture
#define ForceRollAlpha
#endif

#define fake2Dlocks						// Define this statement for systems that cannot draw directly to the screen

#include "fc_h.h"

void ogl_getvideoinfo(void *data);
void ogl_updatecammatrix(void);
void ogl_sethorizon(float h);
void ogl_setshademode(dword shademode);
void ogl_cls(void);
void ogl_zcls(void);
void ogl_enableZbuffer(void);
void ogl_disableZbuffer(void);
void ogl_enableZwrites(void);
void ogl_disableZwrites(void);
void ogl_enabledithering(void);
void ogl_disabledithering(void);
void ogl_enableAA(void);
void ogl_disableAA(void);
void ogl_changeviewport(intf x, intf y, intf width, intf height);
void ogl_setfillmode(dword _fillmode);
void ogl_setambientlight(dword col);
void ogl_restorestate(void *state);
void ogl_fakelock3d(void);
void ogl_fakeunlock3d(void);
long ogl_getstatesize(void);
float ogl_test_fov(float3 *pos,float rad);
dword ogl_testobjvis(obj3d *obj);
void ogl_bucketobject(obj3d *obj);
byte ogl_renderobj3d(obj3d *obj);
void ogl_flushZsort(void);
void ogl_newobj3d(obj3d *obj, sObjCreateInfo *objinfo);
bool ogl_world2screen(float3 *pos,float *sx, float *sy);
void ogl_cleartexture(Texture *tex);
void ogl_downloadbmtex(Texture *_tex, bitmap *bm, dword miplevel);
void ogl_disablelights(uintf firstlight, uintf lastlight);
void ogl_changelight(light3d *thislight);
void ogl_convertobj3d(obj3d *obj);
void ogl_deleteobj3d(obj3d *obj);
void ogl_lockmesh(obj3d *obj, uintf lockflags);
void ogl_unlockmesh(obj3d *obj);

// Private to FC
void ogl_apprestore(void);
void InitOGLExtensions(void);	// Initialize the extnsions only (in case FC needs to perform other platform-specific inits based on extensions)
void InitOGL(void *_classfinder, long width, long height, long bpp);
void ogl_newtextures(Texture *_tex, dword numtex);
bool ogl_ExtensionSupported(char *ext);
void ogl_shutdown(void);

// Usage directions:
// * Initialize OpenGL to the point where you can retrieve the extensions list
// * Call InitOGLExtensions, to fetch all available extensions
// * Finalize initializations so that an RGB OpenGL renderable surface is ready to accept drawing commands
// * Call InitOGL
// * Finish any other platform-specific initializations and return to FC
// * Upon Completion, call ogl_shutdown
// * Perform any platform-specific shutdowns and return to FC

