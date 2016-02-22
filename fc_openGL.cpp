/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#ifdef _WIN32
// Windows
#include <windows.h>
#define needDefines
#define needProtos
#define glFetchFunc		wglGetProcAddress
#define glFuncParamType	LPCSTR
#include <GL/gl.h>
#include <GL/glu.h>
#ifndef GL_VERSION_2_0
	typedef char GLchar;
#endif
typedef ptrdiff_t GLsizeiptr;
#endif

#ifdef __ANDROID__
// Android
#include <android_fc.h>
#include <stdlib.h>
#include <string.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define UsingGLES
void glFrustum(float l,float r, float b,float t, float n,float f) {};
#define GL_HALF_FLOAT		0
#define GL_STACK_OVERFLOW 	0x503
#define GL_STACK_UNDERFLOW	0x504
#define GL_MIN				GL_MIN_EXT
#define GL_MAX				GL_MAX_EXT
#define GL_RGBA8			GL_RGBA
#define GL_CLAMP			GL_CLAMP_TO_EDGE
#define glCreateShaderObject glCreateShader
#define glGetShader 		glGetShaderiv
#else

// Linux
#ifdef __linux__
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <string.h>
#define glFetchFunc		glXGetProcAddress
#define glFuncParamType	const GLubyte *
typedef ptrdiff_t GLsizeiptr;
#endif	// ifdef __linux__
#endif  // ifdef __ANDROID__

#include "fc_h.h"
#include <bucketSystem.h>

#define ogl_maxLights	3			// We need to limit this to 3 for old-generation shader cards, 8 for current gen
//#define numTexturePBOs	2			// Number of Texture PBOs to allocate.  Need a minimum of 2

extern float projectionMatrix[];

const char *ogl_GetErrorText(int errorNumber);
#define GLPICKY() { GLenum err = glGetError(); if (GL_NO_ERROR != err) msg("GL Picky Error",buildstr("*** GL error in %s:%i - %s\n", __FILE__, __LINE__, ogl_GetErrorText(err))); }

#ifndef GL_ARB_shader_objects
	typedef int GLhandleARB;
	typedef char GLcharARB;
#endif

#ifdef needDefines
#define GL_HALF_FLOAT							0x140B
#define GL_FUNC_ADD								0x8006
#define GL_MIN									0x8007
#define GL_MAX									0x8008
#define GL_FUNC_SUBTRACT						0x800A
#define GL_FUNC_REVERSE_SUBTRACT				0x800B
#define GL_BGRA_EXT                             0x80E1
#define GL_CLAMP_TO_EDGE						0x812F
#define GL_GENERATE_MIPMAP						0x8191
#define GL_DEPTH_COMPONENT16					0x81A5
#define GL_DEPTH_COMPONENT24					0x81A6
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT			0x83F0
#define	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT		0x83F1
#define GL_SECONDARY_COLOR_ARRAY				0x845E
#define GL_TEXTURE0								0x84C0
#define GL_TEXTURE_MAX_ANISOTROPY_EXT			0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT		0x84FF
#define GL_MAX_VERTEX_ATTRIBS					0x8869
#define GL_MAX_TEXTURE_COORDS					0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS				0x8872
#define GL_ARRAY_BUFFER							0x8892
#define GL_ELEMENT_ARRAY_BUFFER					0x8893
#define GL_STREAM_DRAW							0x88E0
#define GL_STATIC_DRAW							0x88E4
#define GL_DYNAMIC_DRAW							0x88E8
#define GL_PIXEL_UNPACK_BUFFER					0x88EC
#define GL_FRAGMENT_SHADER						0x8B30
#define	GL_VERTEX_SHADER						0x8B31
#define GL_MAX_VARYING_FLOATS					0x8B4B
#define GL_COMPILE_STATUS						0x8B81
#define GL_LINK_STATUS							0x8B82
#define GL_VALIDATE_STATUS						0x8B83
#define GL_SHADING_LANGUAGE_VERSION				0x8B8C
#define GL_COLOR_ATTACHMENT0					0x8CE0
#define GL_DEPTH_ATTACHMENT						0x8D00
#define GL_FRAMEBUFFER							0x8D40
#define GL_RENDERBUFFER							0x8D41
#endif

// ------------------------------------------------------------------ OpenGL Error / Warning Text ----------------------------------------------------------
#define oglTextDef \
txtCode(ogltxt_noText,"No Error")													\
txtCode(ogltxt_noVersion,"Failed to obtain OpenGL version number")					\
txtCode(ogltxt_noShaderVersion,"Failed to obtain OpenGL Shader version")			\
txtCode(ogltxt_noExtensions,"Failed to obtain OpenGL extensions list")				\
txtCode(ogltxt_badVersion,"You need OpenGL 2.0 or better (3.2 preferred) to run this program")		\
txtCode(ogltxt_missingExt,"A Required OpenGL Extension: %s is required to run this program")

#define txtCode(name, text) name,
enum {
oglTextDef
};
#undef txtCode

#define txtCode(name, text) text,
const char *ogl_Text[]={
oglTextDef
};
#undef txtCode

void ogl_Error(uintf errorCode, const char *params="")
{	char buffer[256];
	tprintf(buffer,256,ogl_Text[errorCode],params);
	msg("OpenGL Error",buffer);
}

#ifdef needProtos
// ------------------------------------------------ OpenGL API Function List ------------------------------------------------------
#define oglprotos_1_3 \
glproto(void,		PFNGLACTIVETEXTUREARBPROC,			glActiveTexture,			"glActiveTexture",				(GLenum texture), true)		\
glproto(void,		PFNGLCOMPRESSEDTEXIMAGE2DPROC,		glCompressedTexImage2D,		"glCompressedTexImage2D",		(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data) ,true) \
glproto(void,		PFNGLBLENDEQUATIONEXTPROC,			glBlendEquation,			"glBlendEquation",				(GLenum mode), true) \

#define oglprotos_3_1 \
glproto(void,		PFNGLGENBUFFERSPROC,				glGenBuffers,				"glGenBuffers",					(GLsizei n, GLuint * buffers),	true)	\
glproto(void,		PFNGLBINDBUFFERPROC,				glBindBuffer,				"glBindBuffer",					(GLenum target, GLuint buffer),	true)	\
glproto(void,		PFNGLBUFFERDATAPROC,				glBufferData,				"glBufferData",					(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage), true)	\
\
glproto(void,		PFNGLGENERATEMIPMAPPROC,			glGenerateMipmap,			"glGenerateMipmap",				(GLenum texture), true) \
\
glproto(void,		PFNGLGENFRAMEBUFFERSPROC,			glGenFramebuffers,			"glGenFramebuffers",			(GLsizei n, GLuint * framebuffers), true) \
glproto(void,		PFNGLBINDFRAMEBUFFERPROC,			glBindFramebuffer,			"glBindFramebuffer",			(GLenum target, GLuint framebuffer), true) \
glproto(void,		PFNGLFRAMEBUFFERTEXTURE2DPROC,		glFramebufferTexture2D,		"glFramebufferTexture2D",		(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level), true) \
glproto(void,		PFNGLGENRENDERBUFFERSPROC,			glGenRenderbuffers,			"glGenRenderbuffers",			(GLsizei n, GLuint * renderbuffers), true) \
glproto(void,		PFNGLBINDRENDERBUFFERPROC,			glBindRenderbuffer,			"glBindRenderbuffer",			(GLenum target, GLuint renderbuffer), true) \
glproto(void,		PFNGLRENDERBUFFERSTORAGEPROC,		glRenderbufferStorage,		"glRenderbufferStorage",		(GLenum target, GLenum internalformat, GLsizei width, GLsizei height), true) \
glproto(void,		PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC,glFramebufferRenderbuffer,	"glFramebufferRenderbuffer",	(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer), true) \
\
glproto(GLhandleARB,PFNGLCREATESHADEROBJECTARBPROC,		glCreateShaderObject,		"glCreateShaderObjectARB",		(GLenum shaderType), true) \
glproto(void,		PFNGLSHADERSOURCEPROC,				glShaderSource,				"glShaderSource",				(GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length), true) \
glproto(void,		PFNGLCOMPILESHADERPROC,				glCompileShader,			"glCompileShader",				(GLuint shader), true) \
glproto(void,		PFNGLGETSHADERIVPROC,				glGetShader,				"glGetShaderiv",				(GLuint shader, GLenum pname, GLint * params), true) \
glproto(void,		PFNGLGETSHADERINFOLOGPROC,			glGetShaderInfoLog,			"glGetShaderInfoLog",			(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog), true) \
glproto(void,		PFNGLDELETESHADERPROC,				glDeleteShader,				"glDeleteShader",				(GLuint shader), true) \
\
glproto(GLuint,		PFNGLCREATEPROGRAMPROC,				glCreateProgram,			"glCreateProgram",				(), true)\
glproto(void,		PFNGLATTACHSHADERPROC,				glAttachShader,				"glAttachShader",				(GLuint program, GLuint shader), true) \
glproto(void,		PFNGLLINKPROGRAMPROC,				glLinkProgram,				"glLinkProgram",				(GLuint program), true) \
glproto(void,		PFNGLUSEPROGRAMPROC,				glUseProgram,				"glUseProgram",					(GLuint program), true) \
glproto(void,		PFNGLVALIDATEPROGRAMPROC,			glValidateProgram,			"glValidateProgram",			(GLuint program), true) \
glproto(void,		PFNGLGETPROGRAMIVPROC,				glGetProgramiv,				"glGetProgramiv",				(GLuint program, GLenum pname, GLint * params), true) \
glproto(void,		PFNGLGETPROGRAMINFOLOGPROC,			glGetProgramInfoLog,		"glGetProgramInfoLog",			(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog), true) \
glproto(void,		PFNGLDELETEPROGRAMPROC,				glDeleteProgram,			"glDeleteProgram",				(GLuint program), true) \
\
glproto(GLint,		PFNGLGETUNIFORMLOCATIONPROC,		glGetUniformLocation,		"glGetUniformLocation",			(GLuint program, const GLchar *name), true) \
glproto(void,		PFNGLUNIFORM1IPROC,					glUniform1i,				"glUniform1i",					(GLint location, GLint v0), true) \
glproto(void,		PFNGLUNIFORM1FPROC,					glUniform1f,				"glUniform1f",					(GLint location, GLfloat v0), true) \
glproto(void,		PFNGLUNIFORM2FPROC,					glUniform2f,				"glUniform2f",					(GLint location, GLfloat v0, GLfloat v1), true) \
glproto(void,		PFNGLUNIFORM3FPROC,					glUniform3f,				"glUniform3f",					(GLint location, GLfloat v0, GLfloat v1, GLfloat v2), true) \
glproto(void,		PFNGLUNIFORM4FPROC,					glUniform4f,				"glUniform4f",					(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3), true) \
glproto(void,		PFNGLUNIFORM1FVPROC,				glUniform1fv,				"glUniform1fv",					(GLint location, GLsizei count, const GLfloat * value), true) \
glproto(void,		PFNGLUNIFORM2FVPROC,				glUniform2fv,				"glUniform2fv",					(GLint location, GLsizei count, const GLfloat * value), true) \
glproto(void,		PFNGLUNIFORM3FVPROC,				glUniform3fv,				"glUniform3fv",					(GLint location, GLsizei count, const GLfloat * value), true) \
glproto(void,		PFNGLUNIFORM4FVPROC,				glUniform4fv,				"glUniform4fv",					(GLint location, GLsizei count, const GLfloat * value), true) \
glproto(void,		PFNGLUNIFORMMATRIX4FVPROC,			glUniformMatrix4fv,			"glUniformMatrix4fv",			(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value), true) \
\
glproto(GLint,		PFNGLGETATTRIBLOCATIONPROC,			glGetAttribLocation,		"glGetAttribLocation",			(GLuint program, const GLchar * name), true)	\
glproto(void,		PFNGLDISABLEVERTEXATTRIBARRAYPROC,	glDisableVertexAttribArray,	"glDisableVertexAttribArray",	(GLuint index), true) \
glproto(void,		PFNGLENABLEVERTEXATTRIBARRAYPROC,	glEnableVertexAttribArray,	"glEnableVertexAttribArray",	(GLuint index), true) \
glproto(void,		PFNGLVERTEXATTRIBPOINTERPROC,		glVertexAttribPointer,		"glVertexAttribPointer"	,		(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer),true)

#ifndef GL_VERSION_1_3
#define oglprotos \
oglprotos_1_3 \
oglprotos_3_1
#else
#define oglprotos \
oglprotos_3_1
#endif

// Define function types, eg: typedef void (APIENTRY* PFNGLGENBUFFERSPROC) (GLsizei n, GLuint * buffers);
#define glproto(returnType, dataType, funcName, txtName, parameters, critical) typedef returnType (APIENTRY* dataType) parameters;
oglprotos
#undef glproto

// Declare function pointers eg: PFNGLGENBUFFERSPROC glGenBuffers;
// Which equates to:  void (APIENTRY* glGenBuffers)(GLsizei n, GLuint * buffers);
#define glproto(returnType, dataType, funcName, txtName, parameters, critical) dataType funcName;
oglprotos
#undef glproto

// Retrieve function pointers
#define glproto(returnType, dataType, funcName, txtName, parameters, critical) \
	funcName = (dataType)glFetchFunc((glFuncParamType)txtName); \
	if ((funcName==NULL) && critical) msg("OpenGL Error","Critical Function " txtName " not present");
void oglFetchFunctions(void)
{	oglprotos

	// Next, determine support for several other features:
	// GL_EXT_texture_filter_anisotropic -> enables glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest); and glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
}
#undef glproto
#else	// needProtos
void oglFetchFunctions(void){}
#endif	// needProtos

void ogl_PrepMaterial(Material *mtl);

// --------------------------------------------------------------------- Internal OpenGL Utilities ----------------------------------------------------------
void ogl_colorI32toFloat(uint32 col, float4 *flt)
{	flt->r = (float)((col & 0x00ff0000) >> 16) * oo255;
	flt->g = (float)((col & 0x0000ff00) >>  8) * oo255;
	flt->b = (float)((col & 0x000000ff)      ) * oo255;
	flt->a = (float)((col & 0xff000000) >> 24) * oo255;
}

// ------------------------------------------------------------------------- OpenGL Structures --------------------------------------------------------------
GLenum ogl_FinalBlendOp[]=
{	GL_FUNC_ADD,				// finalBlend_add			(dest = dest + source)
	GL_FUNC_SUBTRACT,			// finalBlend_subtract		(dest = dest - source)
	GL_FUNC_REVERSE_SUBTRACT,	// finalBlend_revsubtract	(dest = source - dest)
	GL_MIN,						// finalBlend_min			(dest = min(dest,source))
	GL_MAX						// finalBlend_max			(dest = max(dest,source))
};

GLenum ogl_FinalBlendLUT[] =
{	GL_ZERO,					// blendParam_Zero			(   0,    0,    0,    0)
	GL_ONE,						// blendParam_One			(   1,    1,    1,    1)
	GL_SRC_COLOR,				// blendParam_SrcCol		(  Rs,   Gs,   Bs,   As)
	GL_ONE_MINUS_SRC_COLOR,		// blendParam_InvSrcCol		(1-Rs, 1-Gs, 1-Bs, 1-As)
	GL_SRC_ALPHA,				// blendParam_SrcAlpha		(  As,   As,   As,   As)
	GL_ONE_MINUS_SRC_ALPHA,		// blendParam_InvSrcAlpha	(1-As, 1-As, 1-As, 1-As)
	GL_DST_ALPHA,				// blendParam_DstAlpha
	GL_ONE_MINUS_DST_COLOR,		// blendParam_InvDstCol
	GL_DST_COLOR,				// blendParam_DstCol
	GL_ONE_MINUS_DST_COLOR,		// blendParam_InvDstCol
	GL_SRC_ALPHA_SATURATE		// blendParam_AlphaSaturation
};


struct sOglVertexStream
{	sVertexStream genericStream;
	GLuint	VBO;
	uintf	oglFlags;
	sOglVertexStream	*next,*prev;		// links for Global linked list
};
sOglVertexStream *freesOglVertexStream = NULL, *usedsOglVertexStream = NULL;

struct sOglFaceStream
{	sFaceStream	genericStream;
	GLuint	FBO;
	GLsizei	numVerts;					// Precalculated number of verticies (numfaces * 3)
	uintf	oglFlags;
	sOglFaceStream *prev,*next;			// links for Global Linked List
};
sOglFaceStream *freesOglFaceStream = NULL, *usedsOglFaceStream = NULL;

// ------------------------------------------------------------------------- OpenGL Variables --------------------------------------------------------------
// Caching Variables
float			ogl_Pi, ogl_PiDiv8;							// Precalculated values for pi and pi/8
Texture*		lastBindedTexture[32]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

// ----
int				lastDrawnVertexCache = 0;
//GLuint			TexturePBO[numTexturePBOs];
//int				lastTexturePBO = 0;

// Cache utilisation Code
intf			lastShaderProgramID = 0;
#define selectShaderProgram(id) if (lastShaderProgramID!=id) { glUseProgram(id); lastShaderProgramID=id;}

// View Perspective Variables
int		ogl_hfov, ogl_vfov;							// Horizontal and Vertical Field Of View (in degrees)
float	ogl_Zoom, ogl_horizon;						// Zoom factor, and horizon (near plane is always 1, far plane is the horizon)

// Matricies
float	ogl_ProjectionMat[16];						// Projection matrix
float	ogl_invcam[16];
float	ogl_ModelView[16];							// Matrix for holding the current viewing matrix (sent to the shaders)

// Display Dimensions
int		ogl_DisplayWidth, ogl_DisplayHeight;		// Width and Height of the OpenGL display window (which may be a fullscreen display)
float	ogl_ViewPortWidth, ogl_ViewPortHeight;		// *** Depricated ***

// OpenGL Information variables
int		ogl_VersionMajor, ogl_VersionMinor, ogl_ShaderVersionMajor, ogl_ShaderVersionMinor;
char	*ogl_ExtensionsStr = NULL;

extern light3d *activelight[];

// Prototypes to functions provided by the OS-Specific OpenGL file
void OSGL_preInit(uintf flags);			// OS-Specific pre-initialisation code
void OSGL_postInit(uintf flags);		// OS-Specific post-initialisation code
void OSGL_createOpenGLWindow(uintf width, uintf height);
void OSGL_shutDownVideo(void);			// Shut down the video driver
void OSGL_update(void);

// -------------------------------------------------------------------- Feature Checking Functions -------------------------------------------------------------
// Get the OpenGL version number
bool ogl_GetVersion(void)
{

#ifdef UsingGLES
	const char *szVersionString = (const char *)glGetString(GL_VERSION);
	bool success = fetchVersionMajorMinorFromString(szVersionString,&ogl_VersionMajor, &ogl_VersionMinor);
	if (!success) ogl_Error(ogltxt_noVersion);

	// Get shading language version
	const char *szShaderString = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
	success = fetchVersionMajorMinorFromString(szShaderString,&ogl_ShaderVersionMajor, &ogl_ShaderVersionMinor);
	if (!success) ogl_Error(ogltxt_noShaderVersion);

#else
	const char *szVersionString = (const char *)glGetString(GL_VERSION);
	if(szVersionString == NULL)	ogl_Error(ogltxt_noVersion);
	ogl_VersionMajor = atoi(szVersionString);
	ogl_VersionMinor = atoi(strstr(szVersionString, ".")+1);

	// Get shading language version
	const char *szShaderString = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
	if(szShaderString == NULL)
	{
		ogl_Error(ogltxt_noShaderVersion);
	}	else
	{	ogl_ShaderVersionMajor = atoi(szVersionString);
	ogl_ShaderVersionMinor = atoi(strstr(szVersionString, ".")+1);
	}
#endif

	ogl_ExtensionsStr = (char *)glGetString(GL_EXTENSIONS);
	if (ogl_ExtensionsStr == NULL) ogl_Error(ogltxt_noExtensions);

	if (ogl_VersionMajor<2)
		ogl_Error(ogltxt_badVersion);
	return true;
}

// This function determines if the named OpenGL Extension is supported
bool oglIsExtSupported(const char *extension)
{	const char *start;
	char *where, *terminator;

	if (ogl_ExtensionsStr == NULL) ogl_Error(ogltxt_noExtensions);

	start = ogl_ExtensionsStr;
	for (;;)
	{
		where = (char *) strstr((const char *) start, extension);
		if (!where)
			break;

		terminator = where + strlen(extension);

		if (where == start || *(where - 1) == ' ')
			{
			if (*terminator == ' ' || *terminator == '\0')
				return true;
			}
		start = terminator;
	}
	return false;
}

void ogl_setcammatrix(void)
{	// Reposition all lights
	float tmpmtx[16];
	matrixinvert( tmpmtx, ogl_invcam );
//	uintf i=0;
//	light3d *light;
//	while ((light = activelight[i]) && (i<ogl_maxLights))
//	{
//		float4 lightPos, mtxDir,lightDir;
//		mtxDir.x = light->matrix[mat_zvecx];
//		mtxDir.y = light->matrix[mat_zvecy];
//		mtxDir.z = light->matrix[mat_zvecz];
//		transformPointByMatrix((float3*)&lightPos, ogl_invcam, (float3 *)&light->matrix[mat_xpos]);
//		rotateVectorByMatrix((float3 *)&lightDir, tmpmtx, (float3*)&mtxDir);
//		lightPos.w = 1.0f;
//		glLightfv(GL_LIGHT0 + light->index, GL_POSITION, (GLfloat *)&lightPos);				// Position
//		glLightfv(GL_LIGHT0 + light->index, GL_SPOT_DIRECTION, (GLfloat *)&lightDir);		// Direction
//		i++;
//	}
/*
	// This is the known working code for DIRECTION
	float tmpmtx[16];
	matrixinvert( tmpmtx, ogl_invcam );
	uintf i=0;
	light3d *light;
	while ((light = activelight[i]) && (i<ogl_maxLights))
	{
		float4 lightPos, mtxDir,lightDir;
		mtxDir.x = -light->matrix[mat_zvecx];
		mtxDir.y = -light->matrix[mat_zvecy];
		mtxDir.z = -light->matrix[mat_zvecz];
		transformPointByMatrix((float3*)&lightPos, tmpmtx, (float3 *)&light->matrix[mat_xpos]);
		rotateVectorByMatrix((float3 *)&lightDir, tmpmtx, (float3*)&mtxDir);
		lightPos.w = 1.0f;
		glLightfv(GL_LIGHT0 + light->index, GL_POSITION, (GLfloat *)&lightPos);				// Position
		glLightfv(GL_LIGHT0 + light->index, GL_SPOT_DIRECTION, (GLfloat *)&lightDir);		// Direction
		i++;
	}
*/
}
void ogl_PrepMatrixDepricated(float *mtx)
{	matmult(ogl_ModelView,ogl_invcam,mtx);
	GLPICKY();
}

void ogl_GenerateProjectionMatrix(int _width, int _height)
{	float fFOV = ogl_PiDiv8/ogl_Zoom;				// pi / 8 = 22.5 degrees ... This can be considdered a ZOOM factor
	float aspect = (float)ogl_ViewPortHeight / (float)ogl_ViewPortWidth;
	float w = aspect * (float)( cos(fFOV)/sin(fFOV) );
	float h =   1.0f * (float)( cos(fFOV)/sin(fFOV) );
	float Q = ogl_horizon / ( ogl_horizon - 1 );
	float fFOVdeg = (fFOV/(ogl_Pi*2))*3600;
	ogl_hfov = (int)(fFOVdeg / aspect);		// (180/8 = 22.5.  aspect = 0.75.  22.5/aspect = 30).
	ogl_vfov = (int)(fFOVdeg);				// (180/8 = 22.5).
	makeidentity(projectionMatrix);
	projectionMatrix[ 0] = w;
	projectionMatrix[ 5] = h;
	projectionMatrix[10] = Q;
	projectionMatrix[11] = Q;				// item 11 and 14 are swapped around from D3D version
	projectionMatrix[14] = -1.0f;
	projectionMatrix[15] = 1.0f;

	glFrustum(0,w, 0,h, 1,ogl_horizon);		// Just in case the OGL implementation sucks

/*
//	0	1	2	3
//	4	5	6	7
//	8	9	10	11
//	12	13	14	15

	projectionMatrix[ 0] = w;
	projectionMatrix[ 5] = h;
	projectionMatrix[10] = -Q;
	projectionMatrix[11] = -Q;				// item 11 and 14 are swapped around from D3D version
	projectionMatrix[14] = -1.0f;
	projectionMatrix[15] = 1.0f;
*/
//	ogl_matrix->matmult(oglInvCamProjMat, projectionMatrix, invcammat);
//	oglWorld2ScreenMatValid = false;
}

// Window has changed size. Reset to match window coordiantes
void ogl_changeSize(uintf w, uintf h)
{	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if(h == 0)
		h = 1;

	ogl_ViewPortWidth = (float)w;
	ogl_ViewPortHeight = (float)h;
	ogl_DisplayWidth = w;
	ogl_DisplayHeight = h;

	glViewport(0, 0, w, h);
	ogl_GenerateProjectionMatrix(w,h);
}

void ogl_getvideoinfo(void *data, uintf structSize)
{	videoinfostruct *dest = (videoinfostruct *)data;
	if (structSize>sizeof(videoinfostruct)) structSize = sizeof(videoinfostruct);
	memcopy(dest,&videoinfo,structSize);
}

void ogl_cls(uint32 color)
{    glClearColor(	(float)((color & 0x00ff0000)>>16)/255.0f,	// Red
					(float)((color & 0x0000ff00)>> 8)/255.0f,	// Green
					(float)((color & 0x000000ff)    )/255.0f,	// Blue
					(float)((color & 0xff000000)>>24)/255.0f);	// Alpha
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

}

const char *ogl_GetErrorText(int errorNumber)
{	switch (errorNumber)
	{	case GL_NO_ERROR:			// 0x000
			return "No Error";
		case GL_INVALID_ENUM:		// 0x500
			return "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_INVALID_VALUE:		// 0x501
			return "GL_INVALID_VALUE: A numeric argument is out of range. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_INVALID_OPERATION:	// 0x502
			return "GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_STACK_OVERFLOW:		// 0x503
			return "GL_STACK_OVERFLOW: This function would cause a stack overflow. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_STACK_UNDERFLOW:	// 0x504
			return "GL_STACK_UNDERFLOW: This function would cause a stack underflow. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_OUT_OF_MEMORY:		// 0x505
			return "GL_OUT_OF_MEMORY: There is not enough memory left to execute the function. The state of OpenGL is undefined, except for the state of the error flags, after this error is recorded.";
		default:
			return "Unknown Error: An error occurred, but the documentation used to create this error report did not explain this scenario";
	}
}

void ogl_TestForError(int linenum)
{	GLenum glerr = glGetError();
	if (glerr==GL_NO_ERROR) return;
	const char *errText = ogl_GetErrorText(glerr);
/*	switch (glerr)
	{	case GL_INVALID_ENUM:		// 0x500
			errText = "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_INVALID_VALUE:		// 0x501
			errText = "GL_INVALID_VALUE: A numeric argument is out of range. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_INVALID_OPERATION:	// 0x502
			errText = "GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_STACK_OVERFLOW:		// 0x503
			errText = "GL_STACK_OVERFLOW: This function would cause a stack overflow. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_STACK_UNDERFLOW:	// 0x504
			errText = "GL_STACK_UNDERFLOW: This function would cause a stack underflow. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_OUT_OF_MEMORY:		// 0x505
			errText = "GL_OUT_OF_MEMORY: There is not enough memory left to execute the function. The state of OpenGL is undefined, except for the state of the error flags, after this error is recorded.";
			break;
		default:
			errText = "Unknown Error: An error occurred, but the documentation used to create this error report did not explain this scenario";
			break;
	}
*/
	msg("OpenGL Error",buildstr("Line %i: %s",linenum,errText));
}

extern intf VertexUsageSize[16];

GLenum vsdTypeConv[] =
{	GL_FLOAT,		// vertexData_float		0x00	// 32 bit float
	GL_FLOAT,		// vertexData_half		0x10	// 16 bit float (Not yet supported - potential patent issue for implementing this!)
	GL_INT,			// vertexData_int32		0x20	// 32 bit integer
	GL_INT,			// vertexData_uint32	0x30	// 32 bit unsigned integer
	GL_SHORT,		// vertexData_int16		0x40	// 16 bit integer
	GL_SHORT,		// vertexData_uint16	0x50	// 16 bit unsigned integer
	0,				// vertexData_int8		0x60	// 8 bit integer (Not yet supported, need additional OGL docs)
	0,				// vertexData_uint8		0x70	// 8 bit unsigned integer (Not yet supported, need additional OGL docs)
};

/*
void renderFromStreamsDepricated(sVertexStreamDepricated *verts, sFaceStream *faces, uintf frame0, uintf frame1)
{
	msg("Call to Depricated Function","a call was made to renderFromStreamsDepricated");
// To use vertex attributes, you need to use glBindAttribLocation or glGetAttribLocation - Future expansion possibility for extra speed
//	glEnableVertexAttribArray(0);
//	glEnableVertexAttribArray(1);
//	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, sizeof(sSimpleVertex), (GLvoid *)0);//offset);	offset += sizeof(float)*3;
//	glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, sizeof(sSimpleVertex), (GLvoid *)(3*sizeof(GLfloat)));//offset);	offset += sizeof(float)*3;

	sVertexStreamDescription *vsd = verts->desc;
	uintf vsdSize = vsd->size;
	uintf vsdAnimSize = vsd->animSize;
	uintf newCacheFlags = vsd->cacheFlags;
	if (newCacheFlags!=lastDrawnVertexCache)
	{	// Cache flags have changed since last draw - work out which client states to change
		uintf commonCacheFlags = newCacheFlags & lastDrawnVertexCache;
		uintf disableCacheFlags = lastDrawnVertexCache - commonCacheFlags;
		if (disableCacheFlags & 1) glDisableClientState(GL_VERTEX_ARRAY);			// vertexUsage_position
		if (disableCacheFlags & 2) glDisableClientState(GL_NORMAL_ARRAY);			// vertexNormal_position
		if (disableCacheFlags & 4) glDisableClientState(GL_COLOR_ARRAY);			// vertexUsage_color
		if (disableCacheFlags & 8) glDisableClientState(GL_SECONDARY_COLOR_ARRAY);	// vertexUsage_color2
		uintf enableCacheFlags = newCacheFlags - commonCacheFlags;
		if (enableCacheFlags & 1) glEnableClientState(GL_VERTEX_ARRAY);				// vertexUsage_position
		if (enableCacheFlags & 2) glEnableClientState(GL_NORMAL_ARRAY);				// vertexNormal_position
		if (enableCacheFlags & 4) glEnableClientState(GL_COLOR_ARRAY);				// vertexUsage_color
		if (enableCacheFlags & 8) glEnableClientState(GL_SECONDARY_COLOR_ARRAY);	// vertexUsage_color2

		// Set up texture channels
		uintf lastUVs = (lastDrawnVertexCache & 0x00ff0000) >> 16;
		uintf newUVs  = (newCacheFlags & 0x00ff0000) >> 16;
		if (lastUVs<newUVs)
		{	for (uintf i=lastUVs; i<newUVs; i++)
			{	glClientActiveTexture(GL_TEXTURE0+i);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			}
		}
		if (newUVs<lastUVs)
		{	for (uintf i=newUVs; i<lastUVs; i++)
			{	glClientActiveTexture(GL_TEXTURE0+i);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, ((sOglVertexStream *)verts)->VBO);
	uintf offset = 0;
	uintf numUVs = 0;
	uintf numAttribs = 0;
	uintf i=0;
	byte l = vsd->layout[i];
	while (l!=vertexUsage_end && l!=vertexUsage_nextStream)
	{	byte s = vsd->layout[i+1];
		uintf numElements = s&0x0f;
		uintf elementType = s>>4;
		GLenum glElementType = vsdTypeConv[elementType];
		switch (l)
		{	case 0:	break;
			case vertexUsage_position:	glVertexPointer		(numElements, glElementType, vsdSize, (GLvoid *)offset);	break;
			case vertexUsage_normal:	glNormalPointer		(glElementType, vsdSize, (GLvoid *)offset);	break;
			case vertexUsage_color:		glColorPointer		(numElements, glElementType, vsdSize, (GLvoid *)offset);	break;
			case vertexUsage_color2:	glSecondaryColorPointer(numElements, glElementType, vsdSize, (GLvoid *)offset); break;
			case vertexUsage_texture:	glClientActiveTexture(GL_TEXTURE0+numUVs++); glTexCoordPointer(numElements, glElementType, vsdSize, (GLvoid *)offset);	break;
			case vertexUsage_attribute:	numAttribs++; break; // ### Not yet supported
		}
		offset += VertexUsageSize[elementType]*(numElements);
		i+=2;
		l = vsd->layout[i];
	}
	if (offset!=vsdSize)
		msg("xxx",buildstr("Error renderring vsd %i, offset != vsdSize (%i != %i)",vsd->VertexDescID,offset,vsdSize));

	if (l==vertexUsage_nextStream)
	{	offset = 0;
		uintf frame0Offset = vsd->size * verts->numVerts + vsd->animSize * verts->numVerts * frame0;
		uintf frame1Offset = vsd->size * verts->numVerts + vsd->animSize * verts->numVerts * frame1;
		i++;
		l = vsd->layout[i];
		while (l!=vertexUsage_end)
		{	byte l2 = vsd->layout[i+1];
			byte s = vsd->layout[i+2];
			uintf numElements = s&0x0f;
			uintf elementType = s>>4;
			GLenum glElementType = vsdTypeConv[elementType];
			switch (l)
			{	case 0:	break;
				case vertexUsage_position:	glVertexPointer		(numElements, glElementType, vsdAnimSize, (GLvoid *)(offset+frame0Offset));	break;
				case vertexUsage_normal:	glNormalPointer		(glElementType, vsdAnimSize, (GLvoid *)(offset+frame0Offset));	break;
				case vertexUsage_color:		glColorPointer		(numElements, glElementType, vsdAnimSize, (GLvoid *)(offset+frame0Offset));	break;
				case vertexUsage_color2:	glSecondaryColorPointer(numElements, glElementType, vsdAnimSize, (GLvoid *)(offset+frame0Offset)); break;
				case vertexUsage_texture:	glClientActiveTexture(GL_TEXTURE0+numUVs++); glTexCoordPointer(numElements, glElementType, vsdAnimSize, (GLvoid *)(offset+frame0Offset));	break;
				case vertexUsage_attribute:	numAttribs++; break; // ### Not yet supported
			}
			switch (l2)
			{	case 0:	break;
				case vertexUsage_position:	glVertexPointer		(numElements, glElementType, vsdAnimSize, (GLvoid *)(offset+frame1Offset));	break;
				case vertexUsage_normal:	glNormalPointer		(glElementType, vsdAnimSize, (GLvoid *)(offset+frame1Offset));	break;
				case vertexUsage_color:		glColorPointer		(numElements, glElementType, vsdAnimSize, (GLvoid *)(offset+frame1Offset));	break;
				case vertexUsage_color2:	glSecondaryColorPointer(numElements, glElementType, vsdAnimSize, (GLvoid *)(offset+frame1Offset)); break;
				case vertexUsage_texture:	glClientActiveTexture(GL_TEXTURE0+numUVs++); glTexCoordPointer(numElements, glElementType, vsdAnimSize, (GLvoid *)(offset+frame1Offset));	break;
				case vertexUsage_attribute:	numAttribs++; break; // ### Not yet supported
			}
			offset += VertexUsageSize[elementType]*(numElements);
			i+=3;
			l = vsd->layout[i];
		}
		if (offset!=vsdAnimSize)
			msg("xxx",buildstr("Error renderring vsd %i, offset != vsdAnimSize (%i != %i + %i)",vsd->VertexDescID,offset,vsdAnimSize));
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ((sOglFaceStream *)faces)->FBO);

	glDrawElements(GL_TRIANGLES,faces->numFaces*3,GL_UNSIGNED_SHORT,0);
	ogl_TestForError(__LINE__);
}
*/

/*
sVertexStreamDepricated *ogl_createVertexStreamDepricated(sVertexStreamDescription *description, int numVerts, int numKeyFrames, void *vertexData, int flags)
{	msg("ogl_createVertexStreamDepricated is Depricated","Call to depricated function ogl_createVertexStreamDepricated");
	uintf memNeeded = sizeof(sOglVertexStream);
	uintf genStreamSize = sizeof(sVertexStreamDepricated);
	sOglVertexStream *oglStream;
	allocbucket(sOglVertexStream, oglStream, oglFlags, Generic_MemoryFlag32, 512, "VertexStream Cache");
	glGenBuffers(1,&oglStream->VBO);
	sVertexStreamDepricated *genStream = &(oglStream->genericStream);
	if (!vertexData) vertexData = fcalloc(numVerts * description->size + (numVerts * description->animSize * numKeyFrames),"Empty Vertex Stream");
	genStream->desc = description;
	genStream->numVerts = numVerts;
	genStream->numKeyFrames = numKeyFrames;
	genStream->flags = flags;
	genStream->vertices = vertexData;
	return (sVertexStream *)oglStream;
	return NULL;
}
*/

sVertexStream *ogl_createVertexStream(void *rawData, uintf numVerts, uintf vertexSize, uintf flags) // int numkeyframes
{	sOglVertexStream *oglStream;
	allocbucket(sOglVertexStream, oglStream, oglFlags, Generic_MemoryFlag32, 512, "VertexStream Cache");
	glGenBuffers(1,&oglStream->VBO);
	GLPICKY();
	sVertexStream *genStream = &(oglStream->genericStream);
	if (!rawData) rawData = fcalloc(numVerts * vertexSize /* + (numVerts * description->animSize * numKeyFrames) */,"Empty Vertex Stream");
	genStream->vertexSize = vertexSize;
	genStream->animSize = 0;
	genStream->numVerts = numVerts;
//	genStream->numKeyFrames = numKeyFrames;
	genStream->flags = flags;
	genStream->vertices = rawData;
	return (sVertexStream *)oglStream;
}

void ogl_uploadVertexStream(sVertexStream *stream)
{	glBindBuffer(GL_ARRAY_BUFFER, ((sOglVertexStream *)stream)->VBO);
	GLPICKY();
	uintf dataSize = stream->vertexSize * stream->numVerts;// ### TODO: Add animation code back in + (stream->numVerts * stream->desc->animSize * stream->numKeyFrames);
	switch (stream->flags & vertexStreamFlags_modifyMask)
	{	case vertexStreamFlags_modifyOnce:
			glBufferData(GL_ARRAY_BUFFER, dataSize, stream->vertices, GL_STATIC_DRAW);
			break;
		case vertexStreamFlags_modifyRarely:
			glBufferData(GL_ARRAY_BUFFER, dataSize, stream->vertices, GL_STREAM_DRAW);
			break;
		case vertexStreamFlags_modifyOften:
		default:
			glBufferData(GL_ARRAY_BUFFER, dataSize, stream->vertices, GL_DYNAMIC_DRAW);
			break;
	}
	GLPICKY();
}

sFaceStream *ogl_createFaceStream(int numFaces, void *faceData, int flags)
{	sOglFaceStream *oglStream;
	allocbucket(sOglFaceStream, oglStream, oglFlags, Generic_MemoryFlag32, 512, "FaceStream Cache");
	sFaceStream *genStream = &oglStream->genericStream;
	if (!faceData) faceData = fcalloc(numFaces * 2 * 6,"facedata");	// '2' was 'description+1' however description is being merged with flags, and at the moment, only 16 bit faces are supported
	glGenBuffers(1,&oglStream->FBO);
	GLPICKY();
	genStream->numFaces = numFaces;
	oglStream->numVerts = numFaces * 3;
	genStream->flags = flags;
	genStream->faces = faceData;
	return (sFaceStream *)oglStream;
}

void ogl_uploadFaceStream(sFaceStream *stream)
{	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ((sOglFaceStream *)stream)->FBO);
	GLPICKY();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*stream->numFaces, stream->faces, GL_STATIC_DRAW);
	GLPICKY();
}

// ------------------------------------------------------- Render Queue Code ----------------------------------------------------
void ogl_FlushQueue(eResourceType rt, void *resource)
{	// This function flushes the render queue so that synchronous jobs may be performed.  Where reasonable, the render queue will be
	// scanned to see if the specified resource is used in the queue, and if not, it doesn't flush the queue.
	// %%% empty for now
}

// ----------------------------------------------------- Texture mapping Code ---------------------------------------------------
void ogl_initTexture(Texture *tex)
{	// Allocate an OpenGL reference number to this texture
	GLuint ref;
	glGenTextures(1, &ref);
	GLPICKY();
	tex->oemreference = ref;
}

void ogl_setTextureFilter(Texture *tex, uintf filterFlags)
{	lastBindedTexture[0]=tex;
	glBindTexture(GL_TEXTURE_2D, tex->oemreference);
	GLPICKY();

	if (filterFlags & texMapping_magLinear) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		else								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLPICKY();
	switch (filterFlags & 0xff00)
	{	case texMapping_minNearest | texMapping_mipTopOnly:	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);					break;
		case texMapping_minLinear | texMapping_mipTopOnly:	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);					break;
		case texMapping_minNearest | texMapping_mipNearest: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);	break;
		case texMapping_minLinear | texMapping_mipNearest:	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);	break;
		case texMapping_minNearest | texMapping_mipBrilinear: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);	break;
		case texMapping_minLinear | texMapping_mipBrilinear:glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);		break;
		case texMapping_minNearest | texMapping_mipLinear:	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);	break;
		case texMapping_minLinear | texMapping_mipLinear:	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);		break;
		default: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);		break;
	}
	GLPICKY();
	if (filterFlags & texmapping_clampU) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		else							 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	GLPICKY();
	if (filterFlags & texmapping_clampV) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		else							 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	GLPICKY();

	// todo: Add in Anisotropic Filter support
	// #define texmapping_anisoLevelMask	0x0000000f	// Mask used to select the anisotropic mipmapping level.  When selecting anisotropic filterring, the level you choose is always between 0 (disabled) and 15 (the maximum allowed by the user's perferences).
	// glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
	tex->mapping = filterFlags;
}

void ogl_downloadbitmapTex(Texture *tex, bitmap *bm, uintf miplevel)
{	uintf dataType = bm->flags & bitmap_DataTypeMask;

	if (bm->width > videoinfo.MaxTextureWidth) msg("Texture too large",buildstr("Texture %s has a width of %i, maximum is %i",tex->name, bm->width, bm->width));
	if (bm->height > videoinfo.MaxTextureHeight) msg("Texture too large",buildstr("Texture %s has a height of %i, maximum is %i",tex->name, bm->height, bm->height));
#ifndef UsingGLES
	glEnable(GL_TEXTURE_2D);
	GLPICKY();
#endif
	glActiveTexture(GL_TEXTURE0);
	GLPICKY();
	tex->width = bm->width;
	tex->height = bm->height;
	if (tex->oemreference==0)
	{	ogl_initTexture(tex);
		ogl_setTextureFilter(tex,tex->mapping);
	}	else
	{	ogl_FlushQueue(ResourceTexture, tex);
		lastBindedTexture[0]=tex;
		glBindTexture(GL_TEXTURE_2D, tex->oemreference);
		GLPICKY();
	}

	switch (dataType)
	{	case bitmap_DataTypeRGB:
		{
#ifdef UsingGLES
			GLint	components = GL_BGRA_EXT;//GL_RGBA8;
#else
			GLint	components = GL_RGBA8;
#endif
			GLenum	format = GL_BGRA_EXT;

			uintf mipmappingFormula = tex->mapping & texmapping_mipMethodMask;
			if (mipmappingFormula == texmapping_mipNormalize)
			{	// TODO: normalize bitmap
			}	else
			if (mipmappingFormula == texmapping_mipMax1)
			{	// Max out each pixel so that r+g+b <= 1.0
				uintf bmNumPixels = bm->width * bm->height;
				uint32 *pix = (uint32 *)bm->pixel;
				for (uintf i=0; i<bmNumPixels; i++)
				{	uint32 r = (pix[i]&0x00ff0000)>>16;
					uint32 g = (pix[i]&0x0000ff00)>>8;
					uint32 b = (pix[i]&0x000000ff);
					uint32 len = r+g+b;
					if (len>255)
					{	r = (r<<8) / len;
						g = (g<<8) / len;
						b = (b<<8) / len;
						if (r>255) r=255;
						if (g>255) g=255;
						if (b>255) b=255;
						pix[i] = (pix[i]&0xff000000) + (r<<16) + (g<<8) + (b);
					}
				}
			}

#ifdef UsingGLES
			glTexImage2D(GL_TEXTURE_2D, 0, components, bm->width, bm->height, 0, components, GL_UNSIGNED_BYTE, bm->pixel);
#else
			glTexImage2D(GL_TEXTURE_2D, 0, components, bm->width, bm->height, 0, format, GL_UNSIGNED_BYTE, bm->pixel);
#endif
			GLPICKY();

/*			// --- Start of PBO version ---
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, TexturePBO[lastTexturePBO++]);
			// ogl_TestForError(__LINE__);
			if (lastTexturePBO>=numTexturePBOs) lastTexturePBO=0;
			glBufferData(GL_PIXEL_UNPACK_BUFFER, bm->width * bm->height * 4, bm->pixel, GL_STREAM_DRAW);
			// ogl_TestForError(__LINE__);
			glTexImage2D(GL_TEXTURE_2D, 0, components, bm->width, bm->height, 0, format, GL_UNSIGNED_BYTE, NULL);
			// ogl_TestForError(__LINE__);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bm->width, bm->height, format, GL_UNSIGNED_BYTE, 0);
			// ogl_TestForError(__LINE__);
*/
			if (mipmappingFormula == 0)
				glGenerateMipmap(GL_TEXTURE_2D);
			else if (mipmappingFormula == texmapping_mipNormalize)
				glGenerateMipmap(GL_TEXTURE_2D);
			else if (mipmappingFormula == texmapping_mipMax1)
				glGenerateMipmap(GL_TEXTURE_2D);
			GLPICKY();

/*			// ogl_TestForError(__LINE__);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			// ogl_TestForError(__LINE__);
*/
			tex->texmemused = bm->width * bm->height * 4;

			// glTexImage2D(GL_TEXTURE_2D, 0, components, bm->width, bm->height, 0, format, GL_UNSIGNED_BYTE, bm->pixel);	// Non-PBO version
			break;
		}
		case bitmap_DataTypeCompressed:
		{	//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP,GL_TRUE);
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, bm->width, bm->height, 0, (bm->width*bm->height)/2, bm->pixel);
			GLPICKY();
			tex->texmemused = bm->width * bm->height / 2;
			glGenerateMipmap(GL_TEXTURE_2D);
			GLPICKY();
			//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP,GL_FALSE);
			break;
		}
		default:
		{	msg("OpenGL Driver Error",buildstr("Unsupported Texture Type: %i",dataType));
			break;
		}
	}
}

void ogl_cleartexture(Texture *tex)
{	if (tex->oemreference>0)
	{	ogl_FlushQueue(ResourceTexture, tex);
	}
	GLuint ref = tex->oemreference;
	glDeleteTextures(1, &ref);
	tex->oemreference = 0;
	GLPICKY();
}

// ----------------------------------------------------------- Render Target -----------------------------------------------------
struct sOgl_RenderTarget3D
{	GLuint FrameBuffer;
	GLuint RenderBuffer;
};

void ogl_create3DRenderTarget(sRenderTarget3D *rt, intf width, intf height, uintf flags) //(Texture *tex, uintf width, uintf height, uintf flags)
{	// Sanity Check - sOgl_RenderTarget3D must fit inside sRenderTarget3D::OEMData
	if (sizeof(sOgl_RenderTarget3D)>sizeof(rt->OEMData))
		msg("Error in OpenGL Driver","3D RenderTarget data structure is too large (Program needs to be recompiled)");

	// Make sure the feature is supported
	sOgl_RenderTarget3D *glrt = (sOgl_RenderTarget3D *)&rt->OEMData;
	if (glGenFramebuffers==NULL)
		msg("OpenGL Missing Feature","A required feature 'glGenFrameBuffers' is not supported by your hardware or OpenGL driver");

	rt->tex = newTexture("Render Target Texture",0, texture_2D | texture_rendertarget | texture_nonmipped);
	rt->width = width;
	rt->height = height;

	// Create the texture
	bitmap *bm = newbitmap("RenderTextureBuffer",width,height,bitmap_RGB32);
	ogl_downloadbitmapTex(rt->tex, bm, 0);
	deleteBitmap(bm);

	if (lastBindedTexture[0]!=rt->tex)	// This should always fail, but it's here just to be safe
	{	lastBindedTexture[0]=rt->tex;
#ifndef UsingGLES
		glEnable(GL_TEXTURE_2D);
#endif
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rt->tex->oemreference);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glGenFramebuffers(1,&glrt->FrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, glrt->FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->tex->oemreference, 0 );
	glGenRenderbuffers(1, &glrt->RenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, glrt->RenderBuffer);
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, glrt->RenderBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void ogl_select3DRenderTarget(sRenderTarget3D *rt)
{	sOgl_RenderTarget3D *glrt = (sOgl_RenderTarget3D *)&rt->OEMData;
	if (rt)
	{	glBindFramebuffer(GL_FRAMEBUFFER,glrt->FrameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->tex->oemreference, 0 );
		glBindRenderbuffer(GL_RENDERBUFFER, glrt->RenderBuffer);
		//glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, glrt->RenderBuffer);
		glViewport(0, 0, rt->width, rt->height);
	}	else
	{	// Reset 3D render buffer back to screen display
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, screenWidth, screenHeight);
	}
}

// ----------------------------------------------------------- Shader Code -------------------------------------------------------
void ogl_shaderCompileError(const char *type, int shader, const char *shaderSource)
{	char buffer[512+2048];
	char text[768];
	GLsizei buflen;
	glGetShaderInfoLog(shader, sizeof(buffer), &buflen, buffer);
	tprintf(text,sizeof(text),"Error compiling %s shader:\n%s\n",type,buffer);

	sFileHandle *handle=fileCreate("logs/shaderError.log");
	fileWrite(handle,text,txtlen(text));
	buffer[0]=0;
	textfile *txt = newtextfile(shaderSource,txtlen(shaderSource),false);
	while (char *line=txt->readln())
	{	tprintf(buffer,sizeof(buffer),"%i\t%s\r\n",txt->linenum, line);
		fileWrite(handle,(void *)buffer, txtlen(buffer));
	}
	fileClose(handle);
	msg("OpenGL Error",text);
}

#ifdef UsingGLES
const char *GLESPrecisionStr = "precision mediump float;\n";
char GLES_Shader_Buffer[4096];

void ogl_compileShaderComponent(sShaderComponent *result, uintf componentType, const char *source)
{	// GLES requires precision specifier on float types!
	GLuint id;
	GLint status;
	switch (componentType)
	{	case shaderComponent_Vertex:	id = glCreateShaderObject(GL_VERTEX_SHADER);	break;
		case shaderComponent_Fragment:	id = glCreateShaderObject(GL_FRAGMENT_SHADER);	break;
		default: msg("Error calling 'newShaderComponent'",buildstr("invalid 'componentType' value %i",componentType)); break;
	}
	GLPICKY();

	if (componentType==shaderComponent_Fragment)
	{	int len = txtlen(GLESPrecisionStr);
		txtcpy(GLES_Shader_Buffer, sizeof(GLES_Shader_Buffer), GLESPrecisionStr);
		txtcpy(GLES_Shader_Buffer+len,sizeof(GLES_Shader_Buffer)-len,source);
		source=GLES_Shader_Buffer;
	}
	glShaderSource(id, 1, &source, NULL);
	GLPICKY();
	glCompileShader(id);
	GLPICKY();
	glGetShader(id, GL_COMPILE_STATUS, &status);
	if (status!=GL_TRUE)
	{	switch (componentType)
		{	case shaderComponent_Vertex:	ogl_shaderCompileError("vertex", id, source);	break;
			case shaderComponent_Fragment:	ogl_shaderCompileError("fragment", id, source); break;
			default: ogl_shaderCompileError("unknown", id, source); break;
		}
	}
	result->componentID = id;
}
#else
void ogl_compileShaderComponent(sShaderComponent *result, uintf componentType, const char *source)
{	GLuint id;
	GLint status;
	switch (componentType)
	{	case shaderComponent_Vertex:	id = glCreateShaderObject(GL_VERTEX_SHADER);	break;
		case shaderComponent_Fragment:	id = glCreateShaderObject(GL_FRAGMENT_SHADER);	break;
		default: msg("Error calling 'newShaderComponent'",buildstr("invalid 'componentType' value %i",componentType)); break;
	}
	glShaderSource(id, 1, &source, NULL);
	glCompileShader(id);
	glGetShader(id, GL_COMPILE_STATUS, &status);
	if (status!=GL_TRUE)
	{	switch (componentType)
		{	case shaderComponent_Vertex:	ogl_shaderCompileError("vertex", id, source);	break;
			case shaderComponent_Fragment:	ogl_shaderCompileError("fragment", id, source); break;
			default: ogl_shaderCompileError("unknown", id, source); break;
		}
	}
	result->componentID = id;
}
#endif

void ogl_deleteShaderComponent(sShaderComponent *component)
{	glDeleteShader(component->componentID);
}

void ogl_linkShaders(const char *name, sShaderComponent **shaders, uintf numShaders, sShaderProgram *result)
{	char infoLog[256];
	GLsizei infoLength;
	GLint	status;

	int shaderProgram = glCreateProgram();
	GLPICKY();
	for (uintf i=0; i<numShaders; i++)
	{	glAttachShader(shaderProgram, shaders[i]->componentID);
		GLPICKY();
	}
	glLinkProgram(shaderProgram);
	GLPICKY();
	glGetProgramiv(shaderProgram,GL_LINK_STATUS,&status);
	GLPICKY();
	if (status!=GL_TRUE)
	{	glGetProgramInfoLog(shaderProgram, sizeof(infoLog), &infoLength, infoLog);
		msg("Shader Error",buildstr("Error linking shader %s\n%s",name,infoLog));
	}
	result->programID = shaderProgram;
}

void ogl_deleteShaderProgram(sShaderProgram *shaderProgram)
{	glDeleteProgram(shaderProgram->programID);
}

void ogl_findAttribute(sShaderProgram *program, uintf elementID, const char *name)
{	int id = glGetAttribLocation(program->programID, name);
	if (id<0)
		msg("Shader Error",buildstr("Attribute '%s' not found within shader",name));
	program->elementID[elementID] = id;
}

void ogl_findUniform(sShaderProgram *program, uintf elementID, const char *name)
{	int id = glGetUniformLocation(program->programID, name);
	if (id<0)
		msg("Shader Error",buildstr("Uniform '%s' not found within shader",name));
	program->elementID[elementID] = id;
}

void ogl_shaderUniformConstantInt32(sShaderProgram *program, const char *name, int32 value)
{	int id = glGetUniformLocation(program->programID, name);
	GLPICKY();
	if (id<0)
		msg("Shader Error",buildstr("Uniform '%s' not found within shader",name));
	selectShaderProgram(program->programID);
	GLPICKY();
	glUniform1i(id, value);
	GLPICKY();
}

byte lastMtlFlags = 128;
intf lastShader = 0;

// ---------------------------------------------------------- Material Code ------------------------------------------------------
void ogl_PrepMaterial(Material *mtl)
{	byte flags = mtl->flags&0x7f;
	if (flags!=lastMtlFlags)
	{	byte flagDiff = flags ^ lastMtlFlags;
		if (flagDiff & mtl_2sided)
		{	if (flags & mtl_2sided)
				glDisable(GL_CULL_FACE);
			else
				glEnable(GL_CULL_FACE);
		}
		if (flagDiff & mtl_noZWrite)
		{	if (flags & mtl_noZWrite) glDepthMask(GL_FALSE); else glDepthMask(GL_TRUE);
		}
		lastMtlFlags = flags;
	}

	// Sets up the shader and loads the textures, and if necessary, loads in material values into the uniform variables
	if (mtl->flags & mtl_StandardLighting)
	{	msg("OpenGL::PrepMaterial function is incomplete","using StandardLighting when it is not yet completed");
		//glMaterialfv(GL_FRONT, GL_DIFFUSE, (GLfloat *)&mtl->diffuse);	// vec4 diffuse
		//glMaterialf(GL_FRONT, GL_SHININESS, mtl->shininess*128.0f);		// vec2 shininess
		//glMaterialf(GL_FRONT, GL_SPECULAR, mtl->shininessStr);
	}

	intf shader = mtl->shader->programID;
	if (shader!=lastShader)
	{	selectShaderProgram(shader);
		GLPICKY();
		lastShader = shader;
	}
	for (int channel = 0; channel<maxtexchannels; channel++)
	{	Texture *tex = mtl->texture[channel];
		if (!tex) break;
		if (lastBindedTexture[channel]!=tex)
		{	glActiveTexture(GL_TEXTURE0+channel);
			GLPICKY();
		    glBindTexture(GL_TEXTURE_2D, tex->oemreference);
		    GLPICKY();
			lastBindedTexture[channel] = tex;
		}
	}
	glBlendEquation(ogl_FinalBlendOp[mtl->finalBlendOp]);
	GLPICKY();
	glBlendFunc(ogl_FinalBlendLUT[mtl->finalBlendSrcScale], ogl_FinalBlendLUT[mtl->finalBlendDstScale]);
	GLPICKY();

/*
	float4 specVars;
	specVars.r = mtl->shininessStr;
	specVars.g = interpolation;											// ### MUST always manage interpolation!
	glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat *)&specVars);
	GLPICKY();
*/
}

// ------------------------------------------------------ Lighting Code ----------------------------------------------------------
void ogl_setAmbientLight(uint32 ambient)
{	float4 amb;
	ogl_colorI32toFloat(ambient, &amb);
	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (GLfloat *)&amb);
	msg("OpenGL::setAmbientLight function is incomplete","using setAmbientLight when it is not yet completed");
}

void ogl_changeLight(light3d *thislight)
{	uintf index = thislight->index;
	if (index<0 || index>ogl_maxLights) return;

//	glLightfv(GL_LIGHT0 + index, GL_DIFFUSE, &thislight->color.r);					// Color
//	glLightfv(GL_LIGHT0 + index, GL_SPECULAR, &thislight->specular.r);				// Specular Color (currently uses 'R' channel for specular level, no other values used

//	glLightf(GL_LIGHT0 + index, GL_CONSTANT_ATTENUATION,  thislight->attenuation0);
//	glLightf(GL_LIGHT0 + index, GL_LINEAR_ATTENUATION,    thislight->attenuation1);
//	glLightf(GL_LIGHT0 + index, GL_QUADRATIC_ATTENUATION, thislight->attenuation2);

	//float angle = thislight->outerangle * 180 / 3.14159f;
	//float4 tmp;
//	tmp.a = cos(thislight->outerangle * 180.0f / 3.14159f);
	//tmp.a = cos(thislight->outerangle * 3.14159f / 180.0f);		// This seems to be the correct value
//	glLightfv(GL_LIGHT0 + index, GL_AMBIENT, &tmp.r);							// Write additional light partameters into ambient channel  (alpha = cone outer angle)
//	glLightf(GL_LIGHT0 + index, GL_SPOT_CUTOFF, thislight->innerangle);
//	glLightf(GL_LIGHT0 + index, GL_SPOT_EXPONENT, thislight->spotExponent); //256.0f-angle*4.0f);//thislight->falloff);
}

GLint vertexDataGLConvert[] =
{	GL_FLOAT,			// vertex_float		0x00	// 32 bit float
	GL_HALF_FLOAT,		// vertex_half		0x01	// 16 bit float (Not yet supported - potential patent issue for implementing this!)
	GL_INT,				// vertex_int32		0x02	// 32 bit integer
	0,					// vertex_uint32	0x03	// 32 bit unsigned integer
    GL_SHORT,			// vertex_int16		0x04	// 16 bit integer
	GL_UNSIGNED_SHORT,	// vertex_uint16	0x05	// 16 bit unsigned integer
	GL_BYTE,			// vertex_int8		0x06	// 8 bit integer
	GL_UNSIGNED_BYTE,	// vertex_uint8		0x07	// 8 bit unsigned integer
	0,					// Matrix - 4x4 array of float32 (numElements represents the NUMBER OF MATRICIES)
	GL_BYTE,			// vertex_int8F		0x09	// 8 bit integer scaled to the float range -1.0 to 1.0
	GL_UNSIGNED_BYTE,	// vertex_uint8F	0x0A	// 8 bit integer scaled to the float range 0.0 to 1.0
};

// ---------------------------------------------------------- Drawing Code -------------------------------------------------------
extern float modelViewMatrix[];
void ogl_drawObj3D(newobj *o, float *matrix, void *uniforms)
{	float *fUniform;
	float **MtxUniform;

	Material *mtl = o->mtl;
	prepMatrix(matrix);
	ogl_PrepMaterial(mtl);

	// Cache variables locally
	sShaderProgram *shader = mtl->shader;
	uintf numAttributes = shader->numAttributes;
	uintf numUniforms = shader->numUniforms;

	// Point to the Vertex array
	glBindBuffer(GL_ARRAY_BUFFER, ((sOglVertexStream *)(o->verts))->VBO);

	// Set up vertex attributes
	for (uintf i=0; i<numAttributes; i++)
	{	glEnableVertexAttribArray(shader->elementID[i]);		// Enable Vertex processing
		glVertexAttribPointer(shader->elementID[i], shader->elementSize[i], vertexDataGLConvert[shader->elementType[i]], false, shader->totalAttributeSize, (void *)shader->elementOffset[i]);		// Specify where to find vertices and in what format
	}

	// Set up Uniforms
	uintf elementIndex = numAttributes;
	byte *uniformData = (byte *)uniforms;
	for (uintf i=0; i<numUniforms; i++)
	{	byte *elementData = uniformData + shader->elementOffset[elementIndex];
		switch (shader->elementType[elementIndex])
		{	case vertex_float:
				fUniform = (float *)elementData;
				switch(shader->elementSize[elementIndex])
				{
					case 1:	glUniform1fv(shader->elementID[elementIndex],1,fUniform); break;
					case 2: glUniform2fv(shader->elementID[elementIndex],1,fUniform); break;
					case 3: glUniform3fv(shader->elementID[elementIndex],1,fUniform); break;
					case 4: glUniform4fv(shader->elementID[elementIndex],1,fUniform); break;
				}
				break;
			case vertex_mat4x4Ptr:
				MtxUniform = (float **)elementData;
				glUniformMatrix4fv(shader->elementID[elementIndex], 1, false, *MtxUniform);
				break;
		}
		elementIndex++;
	}

	// Draw Geometry
	if (o->faces)
	{	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ((sOglFaceStream *)(o->faces))->FBO);
		glDrawElements(GL_TRIANGLES, o->faces->numFaces*3,GL_UNSIGNED_SHORT,0);
	}	else
	{	glDrawArrays(GL_TRIANGLES, 0, o->verts->numVerts);
	}
	ogl_TestForError(__LINE__);
}

void ogl_disableZBuffer(void)
{	glDisable(GL_DEPTH_TEST);
}

void ogl_enableZBuffer(void)
{	glEnable(GL_DEPTH_TEST);
}

// ------------------------------------------------------ Function Finding Code --------------------------------------------------
void ogl_bogusFunc(void)
{	msg("OpenGL Bogus Function","OpenGL Bogus Function");
}

struct functype
{	const char name[28];
	void *fnptr;
} oglVidFuncs[]={
	{"bogusFunc",			(void *)ogl_bogusFunc},
	// Initialisation
	{"getvideoinfo",		(void *)ogl_getvideoinfo},
	{"shutDownVideo",		(void *)OSGL_shutDownVideo},

	// Matricies
	{"updatecammatrix",		(void *)ogl_setcammatrix},

	// Lighting
	{"setambientlight",		(void *)ogl_setAmbientLight},
	{"changelight",			(void *)ogl_changeLight},

	// Materials
	{"prepMaterial",		(void *)ogl_PrepMaterial},

	// Shader Components
	{"compileShaderComponent", (void *)ogl_compileShaderComponent},
	{"deleteShaderComponent",  (void *)ogl_deleteShaderComponent},

	// Shader Programs
	{"linkShaders",			(void *)ogl_linkShaders},
	{"deleteShaderProgram",	(void *)ogl_deleteShaderProgram},
	{"findAttribute",		(void *)ogl_findAttribute},
	{"findUniform",			(void *)ogl_findUniform},
	// Shader Uniform Constants
	{"shaderUniformConstantInt32",(void *)ogl_shaderUniformConstantInt32},

	// Depricated Shaders
//	{"compileVertexShader",	(void *)ogl_compileVertexShader},
//	{"compileFragmentShader",(void *)ogl_compileFragmentShader},
//	{"linkShadersDepricated",(void *)ogl_linkShadersDepricated},

	// Renderring
	{"cls",					(void *)ogl_cls},
	{"update",				(void *)OSGL_update},
	{"drawObj3D",			(void *)ogl_drawObj3D},
	{"flushRenderQueue",	(void *)ogl_FlushQueue},

	// Render States
	{"enableZBuffer",		(void *)ogl_enableZBuffer},
	{"disableZBuffer",		(void *)ogl_disableZBuffer},

	// Textures
	{"downloadBitmapTex",	(void *)ogl_downloadbitmapTex},
	{"cleartexture",		(void *)ogl_cleartexture},
	{"create3DRenderTarget",(void *)ogl_create3DRenderTarget},
	{"select3DRenderTarget",(void *)ogl_select3DRenderTarget},
	{"setTextureFilter",	(void *)ogl_setTextureFilter},

	// Geometry
	{"createVertexStream",	(void *)ogl_createVertexStream},
	{"createFaceStream",	(void *)ogl_createFaceStream},
	{"uploadVertexStream",	(void *)ogl_uploadVertexStream},
	{"uploadFaceStream",	(void *)ogl_uploadFaceStream},

//	{"createVertexStreamDepricat",	(void *)ogl_createVertexStreamDepricated},
};

bool ogl_getfunc(char *fname, void **func)
{	// Given a text description of a function, return a pointer to it
	uintf numfuncs = sizeof(oglVidFuncs)/sizeof(functype);
	for (uintf i=0; i<numfuncs; i++)
	{	if (txticmp(oglVidFuncs[i].name,fname)==0)
		{	*func = oglVidFuncs[i].fnptr;
			return true;
		}
	}
	msg("OpenGL Missing Function",fname);
	return false;
}

void ogl_shutdown(void)
{	killbucket(sOglVertexStream, oglFlags, Generic_MemoryFlag32);
	killbucket(sOglFaceStream, oglFlags, Generic_MemoryFlag32);
}

void *vd_init3D(void *classfinder,uintf width, uintf height, uintf depth, uintf flags)
{	OSGL_preInit(flags);

	// Initialize variables
	current3DRenderTarget = NULL;
	ogl_horizon = (float)depth;
	ogl_Pi = 3.1415926535897932384626433832795f;
	ogl_PiDiv8 = ogl_Pi / 8.0f;
	ogl_Zoom = 1.0f;
	ogl_ViewPortWidth = (float)width;
	ogl_ViewPortHeight = (float)height;
	ogl_DisplayWidth = width;
	ogl_DisplayHeight = height;

	OSGL_createOpenGLWindow(width, height);

	// Clear the OpenGL Error state
	glGetError();

	// Optain OpenGL version number
	ogl_GetVersion();
	GLPICKY();
	oglFetchFunctions();
	GLPICKY();

	// Fill Driver Info structure
	videoinfo.structsize = sizeof(videoinfo);
	tprintf(videoinfo.DriverName,sizeof(videoinfo.DriverName),"OpenGL %i.%i (shader:%i.%i)",ogl_VersionMajor,ogl_VersionMinor,ogl_ShaderVersionMajor,ogl_ShaderVersionMinor);			// Name of the Device Driver (eg: Direct3D Display Driver)
	tprintf(videoinfo.DriverNameShort,sizeof(videoinfo.DriverNameShort),"OpenGL");							// Short Name of the Device Driver (eg: Direct3D)
	const char *szVendorString = (const char *)glGetString(GL_VENDOR);
	if(szVendorString != NULL)
		tprintf(videoinfo.AuthorName,sizeof(videoinfo.AuthorName),szVendorString);
	else
		tprintf(videoinfo.AuthorName,sizeof(videoinfo.AuthorName),"Not Specified");
	tprintf(videoinfo.LoadingMethod,sizeof(videoinfo.LoadingMethod),"Static");								// Either Static (device driver is loaded with the executable), or Dynamic (loaded as a seperate file)
	const char *szRendererString = (const char *)glGetString(GL_RENDERER);
	if(szRendererString != NULL)
		tprintf(videoinfo.DeviceDescription,sizeof(videoinfo.DeviceDescription),szRendererString);				// What the video hardware calls itself (eg: Nvidia GeForce3 Ti 500)
	else
		tprintf(videoinfo.DeviceDescription,sizeof(videoinfo.DeviceDescription),"Not Specified");

	GLint tmp;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tmp);				videoinfo.MaxTextureWidth	= tmp;
															videoinfo.MaxTextureHeight	= tmp;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &tmp);		videoinfo.NumTextureUnits	= tmp;
															videoinfo.TextureMemory		= 0;
	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &tmp);	videoinfo.MaxAnisotropy		= tmp;
//	glGetIntegerv(GL_MAX_VARYING_FLOATS, &tmp);				videoinfo.MaxShaderVaryings = tmp;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &tmp);				videoinfo.MaxVertexAttribs	= tmp;

	videoinfo.Features = videodriver_hwAccel | videodriver_trilinear | videodriver_anisotropic	| videodriver_nonP2Tex;	// ### Need to properly construct this value
	GLPICKY();

    // ***************************************** Set up default OpenGL State ********************************************

	// Set up Depth Buffer
    glEnable(GL_DEPTH_TEST);
	GLPICKY();
	glDepthFunc(GL_LEQUAL);
	GLPICKY();

    // Set up texture maps
#ifndef UsingGLES
    glEnable(GL_TEXTURE_2D);
	GLPICKY();
#endif

    // set up polygon culling
    glCullFace(GL_BACK);
	GLPICKY();
    glFrontFace(GL_CCW);
	GLPICKY();
    glEnable(GL_CULL_FACE);
	GLPICKY();

	// Set up Stencil Buffer
    glStencilOp(GL_INCR, GL_INCR, GL_INCR);
	GLPICKY();
    glClearStencil(0);
	GLPICKY();
    glStencilFunc(GL_EQUAL, 0x0, 0x01);
	GLPICKY();

	// Set up Miscellaneous Render States
	glDisable(GL_DITHER);
	GLPICKY();
	glEnable(GL_BLEND);
	GLPICKY();

	// --- Depricated features ---
	// Enable discarding of pixels with alpha of pure 0  (Depricated - Use Discard within fragment shader instead)
	// glEnable(GL_ALPHA_TEST);			// Don't think this is needed
	// glAlphaFunc(GL_GREATER, 0);

	// Set up default materials		(Depricated - all materials handled through shaders now)
	//	glMaterialfv(GL_FRONT, GL_AMBIENT, fLowLight);
	//	glMaterialfv(GL_FRONT, GL_SPECULAR, fBrightLight);
	//	glMateriali(GL_FRONT, GL_SHININESS, 128);

	// Setup light parameters		(Depricated - all lighting handled through shaders now)
    // glEnable(GL_LIGHTING);
	// if(oglIsExtSupported("GL_EXT_separate_specular_color"))						// If sepearate specular color is available ...
	//	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);	// Enable itmake torus shiney

/*	###
    // If multisampling was available and was selected, enable
    if(OSGL_FSAA == TRUE && startupOptions.nPixelFormatMS != 0)
        glEnable(GL_MULTISAMPLE_ARB);

*/

	OSGL_postInit(flags);
	GLPICKY();
	ogl_GenerateProjectionMatrix(width,height);
	GLPICKY();

	// Create PBOs for texture upload
//	lastTexturePBO = 0;
//	glGenBuffers(numTexturePBOs,TexturePBO);

//	printf("Display Driver: %s\n",videoinfo.DriverName);
//	printf("Author: %s\n",videoinfo.AuthorName);
//	printf("Device Description: %s\n",videoinfo.DeviceDescription);
	return (void *)&ogl_getfunc;
}
