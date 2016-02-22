#include <windows.h>                // Win32 Framework (No MFC)
#include <stdio.h>                  // Standard IO (sprintf)
#include <math.h>
#include <gl\gl.h>					// not needed, as it's included from GLee, Microsoft OpenGL headers (version 1.1 by themselves)	(Path is OpenGL for OSX and GL for linux)
//#include "glee\glee.h"
#include <gl\glu.h>					// OpenGL Utilities
#include "OpenGL_defines.h"			// The defines and function prototypes for OpenGL
#include "fc_h.h"
#include <bucketsystem.h>

#define ogl_maxLights 3				// We need to limit this to 3 for old-generation shader cards, 8 for current gen

#define GLPICKY() //{ GLenum err = glGetError(); if (GL_NO_ERROR != err) msg("GL Picky Error",buildstr("*** GL error: 0x%X at %s:%i\n", err, __FILE__, __LINE__)); }

// ------------------------------------------------------------------ OpenGL Error / Warning Text ----------------------------------------------------------
#define oglTextDef \
txtCode(ogltxt_noText,"No Error")													\
txtCode(ogltxt_noVersion,"Failed to obtain OpenGL version number")					\
txtCode(ogltxt_noShaderVersion,"Failed to obtain OpenGL Shader version")			\
txtCode(ogltxt_noExtensions,"Failed to obtain OpenGL extensions list")				\
txtCode(ogltxt_badVersion,"You need OpenGL 2.0 or better to run this program")		\
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

// ----------------------------------------------------------------------- OpenGL Prototypes ---------------------------------------------------------------
#define oglprotos \
glproto(PFNGLGENBUFFERSPROC,			glGenBuffers,			"glGenBuffers",	true)	\
glproto(PFNGLBINDBUFFERPROC,			glBindBuffer,			"glBindBuffer", true)	\
glproto(PFNGLBUFFERDATAPROC,			glBufferData,			"glBufferData", true)	\
glproto(PFNGLACTIVETEXTUREARBPROC,		glActiveTexture,		"glActiveTexture", true) \
glproto(PFNGLCLIENTACTIVETEXTUREPROC,	glClientActiveTexture,	"glClientActiveTexture", true) \
glproto(PFNGLCREATESHADEROBJECTARBPROC,	glCreateShaderObject,	"glCreateShaderObjectARB", true) \
glproto(PFNGLSHADERSOURCEPROC,			glShaderSource,			"glShaderSource", true) \
glproto(PFNGLCOMPILESHADERPROC,			glCompileShader,		"glCompileShader",true) \
glproto(PFNGLGETSHADERIVPROC,			glGetShader,			"glGetShaderiv",true) \
glproto(PFNGLGETSHADERINFOLOGPROC,		glGetShaderInfoLog,		"glGetShaderInfoLog",true) \
glproto(PFNGLCREATEPROGRAMPROC,			glCreateProgram,		"glCreateProgram",true)\
glproto(PFNGLATTACHSHADERPROC,			glAttachShader,			"glAttachShader",true) \
glproto(PFNGLLINKPROGRAMPROC,			glLinkProgram,			"glLinkProgram",true) \
glproto(PFNGLUSEPROGRAMPROC,			glUseProgram,			"glUseProgram",true) \
glproto(PFNGLBLENDEQUATIONEXTPROC,		glBlendEquation,		"glBlendEquation",true) \
glproto(PFNGLGETUNIFORMLOCATIONPROC,	glGetUniformLocation,	"glGetUniformLocation",true) \
glproto(PFNGLUNIFORM1IPROC,				glUniform1i,			"glUniform1i",true) \
glproto(PFNGLVALIDATEPROGRAMPROC,		glValidateProgram,		"glValidateProgram",true) \
glproto(PFNGLGETPROGRAMIVPROC,			glGetProgramiv,			"glGetProgramiv",true) \
glproto(PFNGLDISABLEVERTEXATTRIBARRAYPROC,	glDisableVertexAttribArray,	"glDisableVertexAttribArray",true) \
glproto(PFNGLENABLEVERTEXATTRIBARRAYPROC,	glEnableVertexAttribArray,	"glEnableVertexAttribArray",true) \
glproto(PFNGLVERTEXATTRIBPOINTERPROC,		glVertexAttribPointer,		"glVertexAttribPointer",true) \

// Declare function pointers
#define glproto(dataType, funcName, txtName, critical) dataType funcName;
oglprotos
#undef glproto

#define glproto(dataType, funcName, txtName, critical) {&funcName, txtName, critical},
struct sOglProtoList
{	void		*func;
	const char	*txtName;
	bool		critical;
} oglProtoList[] = {
oglprotos
};
#undef glproto

void oglFetchFunctions(void)
{	for (uintf i=0; i<sizeof(oglProtoList)/sizeof(sOglProtoList); i++)
	{	void **x = (void **)oglProtoList[i].func;
		*x = wglGetProcAddress(oglProtoList[i].txtName);
		if ((!*x) && (oglProtoList[i].critical)) ogl_Error(ogltxt_missingExt,oglProtoList[i].txtName);
	}
}

// --------------------------------------------------------------------- Internal OpenGL Utilities ----------------------------------------------------------
void ogl_colorI32toFloat(uint32 col, float4 *flt)
{	flt->r = (float)((col & 0x00ff0000) >> 16) * oo255;
	flt->g = (float)((col & 0x0000ff00) >>  8) * oo255;
	flt->b = (float)((col & 0x000000ff)      ) * oo255;
	flt->a = (float)((col & 0xff000000) >> 24) * oo255;
}

// ------------------------------------------------------------------------- OpenGL Structures --------------------------------------------------------------
enum eOgl_ResourceType
{	ogl_Resource_NULL = 0,			// Specifying resource_NULL means you intend to force an operation to occur, this can slow down the system dramaticly
	ogl_ResourceTexture,
	ogl_ResourceVertexStream,
	ogl_ResourceFaceStream,
};

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
};

struct sOglFaceStream
{	sFaceStream	genericStream;
	GLuint	FBO;
	GLsizei	numVerts;								// Precalculated number of verticies (numfaces * 3)
};
	
// ------------------------------------------------------------------------- OpenGL Variables --------------------------------------------------------------
// Cachine Variables
float		ogl_Pi, ogl_PiDiv8;							// Precalculated values for pi and pi/8
Texture*	lastBindedTexture[32]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

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
{	const char *szVersionString = (const char *)glGetString(GL_VERSION);
	if(szVersionString == NULL)	ogl_Error(ogltxt_noVersion);			
	ogl_VersionMajor = atoi(szVersionString);
	ogl_VersionMinor = atoi(strstr(szVersionString, ".")+1);

	const char *szShaderString = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
	if(szShaderString == NULL)	ogl_Error(ogltxt_noShaderVersion);			
	ogl_ShaderVersionMajor = atoi(szVersionString);
	ogl_ShaderVersionMinor = atoi(strstr(szVersionString, ".")+1);

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
{	float tmpcammat[16];
	float invZmat[16]={1,0,0,0, 0,1,0,0, 0,0,-1,0, 0,0,0,1};
	matmult(tmpcammat,cammat,invZmat);
    matrixinvert( ogl_invcam, tmpcammat );

	// Reposition all lights
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float tmpmtx[16];
	matrixinvert( tmpmtx, ogl_invcam );			
	uintf i=0;
	light3d *light;
	while ((light = activelight[i]) && (i<ogl_maxLights))
	{
		float4 lightPos, mtxDir,lightDir;
		mtxDir.x = light->matrix[mat_zvecx];
		mtxDir.y = light->matrix[mat_zvecy];
		mtxDir.z = light->matrix[mat_zvecz];
		transformPointByMatrix((float3*)&lightPos, ogl_invcam, (float3 *)&light->matrix[mat_xpos]);
		rotateVectorByMatrix((float3 *)&lightDir, tmpmtx, (float3*)&mtxDir);
		lightPos.w = 1.0f;
		glLightfv(GL_LIGHT0 + light->index, GL_POSITION, (GLfloat *)&lightPos);				// Position
		glLightfv(GL_LIGHT0 + light->index, GL_SPOT_DIRECTION, (GLfloat *)&lightDir);		// Direction
		i++;
	}
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
void ogl_PrepMatrix(float *mtx)
{	matmult(ogl_ModelView,ogl_invcam,mtx);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(ogl_ModelView);
	GLPICKY();
}

void ogl_GenerateProjectionMatrix(int width, int height)
{	float fFOV = ogl_PiDiv8/ogl_Zoom;				// pi / 8 = 22.5 degrees ... This can be considdered a ZOOM factor
	float aspect = (float)ogl_ViewPortHeight / (float)ogl_ViewPortWidth;
	float w = aspect * (float)( cos(fFOV)/sin(fFOV) );
	float h =   1.0f * (float)( cos(fFOV)/sin(fFOV) );
	float Q = ogl_horizon / ( ogl_horizon - 1 );
	float fFOVdeg = (fFOV/(ogl_Pi*2))*3600;
	ogl_hfov = (int)(fFOVdeg / aspect);		// (180/8 = 22.5.  aspect = 0.75.  22.5/aspect = 30).
	ogl_vfov = (int)(fFOVdeg);				// (180/8 = 22.5).  
	makeidentity(ogl_ProjectionMat);
	ogl_ProjectionMat[ 0] = w;				
	ogl_ProjectionMat[ 5] = h;
	ogl_ProjectionMat[10] = -Q;
	ogl_ProjectionMat[11] = -Q;				// item 11 and 14 are swapped around from D3D version
	ogl_ProjectionMat[14] = -1.0f;
	ogl_ProjectionMat[15] = 1.0f;
	glMatrixMode( GL_PROJECTION);
	glFrustum(0,w, 0,h, 1,ogl_horizon);		// Just in case the OGL implementation sucks
	glLoadMatrixf( ogl_ProjectionMat );		// Now load in OUR projection matrix
	glMatrixMode(GL_MODELVIEW);
//	ogl_matrix->matmult(oglInvCamProjMat, oglProjectionMat, oglInvCamMat);
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

void ogl_getvideoinfo(void *data)
{	videoinfostruct *dest = (videoinfostruct *)data;
	uintf size = dest->structsize;
	if (size>sizeof(videoinfostruct)) size = sizeof(videoinfostruct);
	memcpy(dest,&videoinfo,size);
	dest->structsize = size;
}

void ogl_cls(void)
{    glClearColor(	(float)((backgroundrgb & 0x00ff0000)>>16)/255.0f,	// Red
					(float)((backgroundrgb & 0x0000ff00)>> 8)/255.0f,	// Green
					(float)((backgroundrgb & 0x000000ff)    )/255.0f,	// Blue
					(float)((backgroundrgb & 0xff000000)>>24)/255.0f);	// Alpha
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void ogl_TestForError(void)
{	GLenum glerr = glGetError();
	if (glerr==GL_NO_ERROR) return;
	const char *errText;
	switch (glerr)
	{	case GL_INVALID_ENUM: 
			errText = "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_INVALID_VALUE:
			errText = "GL_INVALID_VALUE: A numeric argument is out of range. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_INVALID_OPERATION:
			errText = "GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_STACK_OVERFLOW:
			errText = "GL_STACK_OVERFLOW: This function would cause a stack overflow. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_STACK_UNDERFLOW:
			errText = "GL_STACK_UNDERFLOW: This function would cause a stack underflow. The offending function is ignored, having no side effect other than to set the error flag.";
			break;
		case GL_OUT_OF_MEMORY:
			errText = "GL_OUT_OF_MEMORY: There is not enough memory left to execute the function. The state of OpenGL is undefined, except for the state of the error flags, after this error is recorded.";
			break;
		default:
			errText = "Unknown Error: An error occurred, but the documentation used to create this error report did not explain this scenario";
			break;
	}
	msg("OpenGL Error",errText);
}

/*
void msgmatrix(float *mtx)	// For debugging purposes ... displays a matrix using textout function
{	char buffer[1024];
	char line[4][256];
	for (intf y=0; y<4; y++)
		{	sprintf(line[y],"%.2f  %.2f  %.2f  %.2f",mtx[(y<<2)+0],mtx[(y<<2)+1],mtx[(y<<2)+2],mtx[(y<<2)+3]);
		}
	tprintf(buffer, 1024, "%s\n%s\n%s\n%s\n",line[0],line[1],line[2],line[3]);
	msg("Matrix",buffer);
}
*/

void renderFromStreams(sVertexStream *verts, sFaceStream *faces)
{	
	// To use vertex attributes, you need to use glBindAttribLocation or glGetAttribLocation - Future expansion possibility for extra speed
//	glEnableVertexAttribArray(0);
//	glEnableVertexAttribArray(1);
//	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, sizeof(sSimpleVertex), (GLvoid *)0);//offset);	offset += sizeof(float)*3;
//	glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, sizeof(sSimpleVertex), (GLvoid *)(3*sizeof(GLfloat)));//offset);	offset += sizeof(float)*3;

	glEnableClientState(GL_VERTEX_ARRAY);
	GLPICKY();
	glEnableClientState(GL_NORMAL_ARRAY);
	GLPICKY();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	GLPICKY();
	glEnableClientState(GL_COLOR_ARRAY);
	GLPICKY();

	glBindBuffer(GL_ARRAY_BUFFER, ((sOglVertexStream *)verts)->VBO);
	GLPICKY();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ((sOglFaceStream *)faces)->FBO);
	GLPICKY();

	uintf offset = 0;
	// Set up our vertex format - this is the code for standard vertices
	glVertexPointer		(3, GL_FLOAT, sizeof(sSimpleVertex), (GLvoid *)0);
	GLPICKY();
	glNormalPointer		(   GL_FLOAT, sizeof(sSimpleVertex), (GLvoid *)(3*sizeof(GLfloat)));
	GLPICKY();
	glTexCoordPointer	(2, GL_FLOAT, sizeof(sSimpleVertex), (GLvoid *)(6*sizeof(GLfloat)));
	GLPICKY();
	glColorPointer		(4, GL_FLOAT, sizeof(sSimpleVertex), (GLvoid *)(8*sizeof(GLfloat)));
	GLPICKY();
	glDrawElements(GL_TRIANGLES,((sOglFaceStream *)faces)->numVerts,GL_UNSIGNED_SHORT,0);
	ogl_TestForError();
}

sVertexStream *ogl_createVertexStream(sVertexStreamDescription *description, int numVerts, void *vertexData, int flags)
{	uintf memNeeded = sizeof(sOglVertexStream);
	uintf genStreamSize = sizeof(sVertexStream);
	sOglVertexStream *oglStream = (sOglVertexStream *)fcalloc(sizeof(sOglVertexStream),"createVertexStream");
	glGenBuffers(1,&oglStream->VBO);
	sVertexStream *genStream = &(oglStream->genericStream);
	if (!vertexData) vertexData = malloc(numVerts * description->size);
	genStream->desc = description;
	genStream->numVerts = numVerts;
	genStream->flags = flags;
	genStream->vertices = vertexData;
	genStream->link = NULL;
	return (sVertexStream *)oglStream;
}

void ogl_uploadVertexStream(sVertexStream *stream)
{	glBindBuffer(GL_ARRAY_BUFFER, ((sOglVertexStream *)stream)->VBO);
	glBufferData(GL_ARRAY_BUFFER, stream->desc->size*stream->numVerts, stream->vertices, GL_STATIC_DRAW);
}

sFaceStream *ogl_createFaceStream(int description, int numFaces, void *faceData, int flags)
{	sOglFaceStream *oglStream = (sOglFaceStream *)fcalloc(sizeof(sOglFaceStream),"CreateFaceStream");
	sFaceStream *genStream = &oglStream->genericStream;
	if (!faceData) faceData = malloc(numFaces * (description+1) * 6);
	glGenBuffers(1,&oglStream->FBO);
	genStream->desc = description;
	genStream->numFaces = numFaces;
	oglStream->numVerts = numFaces * 3;
	genStream->flags = flags;
	genStream->faces = faceData;
	return (sFaceStream *)oglStream;
}

void ogl_uploadFaceStream(sFaceStream *stream)
{	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ((sOglFaceStream *)stream)->FBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*stream->numFaces, stream->faces, GL_STATIC_DRAW);
}

// ------------------------------------------------------- Render Queue Code ----------------------------------------------------
void ogl_FlushQueue(eOgl_ResourceType rt, GLuint reference)
{	// This function flushes the render queue so that synchronous jobs may be performed.  Where reasonable, the render queue will be
	// scanned to see if the specified resource is used in the queue, and if not, it doesn't flush the queue.
	// %%% empty for now
}

// ----------------------------------------------------- Texture mapping Code ---------------------------------------------------
void ogl_initTexture(Texture *tex)
{	// Allocate an OpenGL reference number to this texture
	GLuint ref;
	glGenTextures(1, &ref);
	tex->oemreference = ref;
}

void ogl_downloadbitmapTex(Texture *tex, bitmap *bm, uintf miplevel)
{	GLint	components = GL_RGBA8;
	GLenum	format = GL_BGRA_EXT;

	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	tex->width = bm->width;
	tex->height = bm->height;
	if (tex->oemreference==0) 
	{	ogl_initTexture(tex);
	}	else
	{	ogl_FlushQueue(ogl_ResourceTexture, tex->oemreference);
	}
	lastBindedTexture[0]=tex;
	glBindTexture(GL_TEXTURE_2D, tex->oemreference);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);								// %%% I think this mipmap generater assumes RGB, may need to be hand-coded for DOT3 maps
	glTexImage2D(GL_TEXTURE_2D, 0, components, bm->width, bm->height, 0, format, GL_UNSIGNED_BYTE, bm->pixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);					// %%% unknown if this is correct
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void ogl_cleartexture(Texture *tex)
{	if (tex->oemreference>0) 
	{	ogl_FlushQueue(ogl_ResourceTexture, tex->oemreference);
	}
	GLuint ref = tex->oemreference;
	glDeleteTextures(1, &ref);
	tex->oemreference = 0;
	GLPICKY();
}

// ----------------------------------------------------------- Shader Code -------------------------------------------------------
void ogl_shaderCompileError(const char *type, int shader, const char *shaderSource)
{	char buffer[512+2048];
	char text[768];
	GLsizei buflen;
	glGetShaderInfoLog(shader, sizeof(buffer), &buflen, buffer);
	tprintf(text,sizeof(text),"Error compiling %s shader:\n%s\n",type,buffer);
	
	sFileHandle *handle=fileCreate("shaderError.log");
	fileWrite(handle,text,strlen(text));
	fileWrite(handle,(void *)shaderSource,strlen(shaderSource));
	fileClose(handle);
	msg("OpenGL Error",text);
}

int ogl_compileVertexShader(int vshader, const char *shaderSource)
{	if (vshader==0)
	{	vshader = glCreateShaderObject(GL_VERTEX_SHADER);
	}
	glShaderSource(vshader, 1, &shaderSource, NULL);
	glCompileShader(vshader);
	GLint status;
	glGetShader(vshader, GL_COMPILE_STATUS, &status);
	if (status!=GL_TRUE)
	{	ogl_shaderCompileError("vertex", vshader, shaderSource);
	}
	return vshader;
}

int ogl_compileFragmentShader(int fshader, const char *shaderSource)
{	if (fshader==0)
	{	fshader = glCreateShaderObject(GL_FRAGMENT_SHADER);
	}
	glShaderSource(fshader, 1, &shaderSource, NULL);
	glCompileShader(fshader);
	GLint status;
	glGetShader(fshader, GL_COMPILE_STATUS, &status);
	if (status!=GL_TRUE)
	{	ogl_shaderCompileError("fragment", fshader, shaderSource);
	}
	return fshader;
}

int ogl_linkShaders(int shaderProgram, int vshader, int fshader, int numTextures)
{	if (shaderProgram==0)
	{	shaderProgram = glCreateProgram();
	}
	glAttachShader(shaderProgram, vshader);
	glAttachShader(shaderProgram, fshader);
	glLinkProgram(shaderProgram);

	int isValid;
	glValidateProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &isValid);
	if (!isValid) msg("Invalid Shader","The shader linker reports one of your shaders is invalid");

	glUseProgram(shaderProgram);
	for (int i=0; i<numTextures; i++)
	{	int tex = glGetUniformLocation(shaderProgram, buildstr("tex%i",i));
		if (tex>=0) 
			glUniform1i(tex,i);
	}
	GLPICKY();
	return shaderProgram;
}

// ---------------------------------------------------------- Material Code ------------------------------------------------------
GLfloat gl_White[4]={1.0f, 1.0f, 1.0f, 1.0f};
void ogl_PrepMaterial(Material *mtl)
{	if (!mtl)
	{	glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, gl_White);
		glUseProgram(0);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ZERO,GL_ONE);
	}	else
	{	glMaterialfv(GL_FRONT, GL_DIFFUSE, (GLfloat *)&mtl->diffuse);
		glMaterialf(GL_FRONT, GL_SHININESS, mtl->shininess*128.0f);
		float4 specVars;
		specVars.r = mtl->shininessStr;
		glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat *)&specVars);
		glUseProgram(mtl->shader);
		for (int channel = 0; channel<maxtexchannels; channel++)
		{	Texture *tex = mtl->texture[channel];
			if (!tex) break;
			//if (lastBindedTexture[channel]!=tex)
			{	glActiveTexture(GL_TEXTURE0+channel);
			    glBindTexture(GL_TEXTURE_2D, tex->oemreference);
				lastBindedTexture[channel] = tex;
			}
		}
		glBlendEquation(ogl_FinalBlendOp[mtl->finalBlendOp]);
		glBlendFunc(ogl_FinalBlendLUT[mtl->finalBlendSrcScale], ogl_FinalBlendLUT[mtl->finalBlendDstScale]);
	}
	GLPICKY();
}

// ------------------------------------------------------ Lighting Code ----------------------------------------------------------
void ogl_setAmbientLight(uint32 ambient)
{	float4 amb;
	ogl_colorI32toFloat(ambient, &amb);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (GLfloat *)&amb);
}

void ogl_changeLight(light3d *thislight)
{	uintf index = thislight->index;
	if (index<0 || index>ogl_maxLights) return;

	glLightfv(GL_LIGHT0 + index, GL_DIFFUSE, &thislight->color.r);					// Color
	glLightfv(GL_LIGHT0 + index, GL_SPECULAR, &thislight->specular.r);				// Specular Color

	glLightf(GL_LIGHT0 + index, GL_CONSTANT_ATTENUATION,  thislight->attenuation0);
	glLightf(GL_LIGHT0 + index, GL_LINEAR_ATTENUATION,    thislight->attenuation1);
	glLightf(GL_LIGHT0 + index, GL_QUADRATIC_ATTENUATION, thislight->attenuation2);

	//float angle = thislight->outerangle * 180 / 3.14159f;
	glLightf(GL_LIGHT0 + index, GL_SPOT_CUTOFF, thislight->outerangle);
	glLightf(GL_LIGHT0 + index, GL_SPOT_EXPONENT, thislight->spotExponent); //256.0f-angle*4.0f);//thislight->falloff);
}

// ------------------------------------------------------ Function Finding Code --------------------------------------------------
void ogl_bogusFunc(void)
{	msg("OpenGL Bogus Function","OpenGL Bogus Function");
}

struct functype
{	const char name[25];
	void *fnptr;
} oglVidFuncs[]={	
	{"bogusFunc",			ogl_bogusFunc},
	// Initialisation
	{"getvideoinfo",		ogl_getvideoinfo},
	{"shutDownVideo",		OSGL_shutDownVideo},
	// Matricies
	{"updatecammatrix",		ogl_setcammatrix},
	// Lighting
	{"setambientlight",		ogl_setAmbientLight},
	{"changelight",			ogl_changeLight},
	// Shaders
	{"compileVertexShader",	ogl_compileVertexShader},
	{"compileFragmentShader",ogl_compileFragmentShader},
	{"linkShaders",			ogl_linkShaders},
	// Renderring
	{"cls",					ogl_cls},
	{"update",				OSGL_update},
	// Textures
	{"downloadBitmapTex",	ogl_downloadbitmapTex},
	{"cleartexture",		ogl_cleartexture},
	// Geometry
	{"createVertexStream",	ogl_createVertexStream},
	{"createFaceStream",	ogl_createFaceStream},
	{"uploadVertexStream",	ogl_uploadVertexStream},
	{"uploadFaceStream",	ogl_uploadFaceStream},
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

void *vd_init3D(void *classfinder,uintf width, uintf height, uintf depth, uintf flags)
{	OSGL_preInit(flags);

	// Initialize variables
	ogl_horizon = (float)depth;
	ogl_Pi = 3.1415926535897932384626433832795f;
	ogl_PiDiv8 = ogl_Pi / 8.0f;
	ogl_Zoom = 1.0f;
	ogl_ViewPortWidth = (float)width;
	ogl_ViewPortHeight = (float)height;
	ogl_DisplayWidth = width;
	ogl_DisplayHeight = height;

	OSGL_createOpenGLWindow(width, height);

	// Optain OpenGL version number
	ogl_GetVersion();
	oglFetchFunctions();
	
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
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tmp);
	videoinfo.MaxTextureWidth	= tmp;
	videoinfo.MaxTextureHeight	= tmp;
	videoinfo.NumTextureUnits	= 32;	// ### Need to calculate this
	videoinfo.TextureMemory	= 0;
	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &tmp);
	videoinfo.MaxAnisotropy	= tmp;
	videoinfo.Features			= videodriver_hwAccel | videodriver_trilinear | videodriver_anisotropic	| videodriver_nonP2Tex;	// ### Need to properly construct this value

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, (GLint *)&videoinfo.NumTextureUnits);

    // Clear stencil buffer with zero, increment by one whenever anybody
    // draws into it. When stencil function is enabled, only write where
    // stencil value is zero. This prevents the transparent shadow from drawing
    // over itself
    glStencilOp(GL_INCR, GL_INCR, GL_INCR);
    glClearStencil(0);
    glStencilFunc(GL_EQUAL, 0x0, 0x01);

    // Cull backs of polygons
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // Setup light parameters
    glEnable(GL_LIGHTING);

	// Mostly use material tracking
//	glMaterialfv(GL_FRONT, GL_AMBIENT, fLowLight);
//	glMaterialfv(GL_FRONT, GL_SPECULAR, fBrightLight);
//    glMateriali(GL_FRONT, GL_SHININESS, 128);

    // Set up texture maps
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Enable blending
	
	glEnable(GL_ALPHA_TEST);			// Don't think this is needed
	glEnable(GL_BLEND);
/*	###
    // If multisampling was available and was selected, enable
    if(OSGL_FSAA == TRUE && startupOptions.nPixelFormatMS != 0)
        glEnable(GL_MULTISAMPLE_ARB);

    // If sepearate specular color is available, make torus shiney
    if(oglIsExtSupported("GL_EXT_separate_specular_color"))
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
*/

	OSGL_postInit(flags);

    glMatrixMode(GL_MODELVIEW);

//	printf("Display Driver: %s\n",videoinfo.DriverName);
//	printf("Author: %s\n",videoinfo.AuthorName);
//	printf("Device Description: %s\n",videoinfo.DeviceDescription);
	return &ogl_getfunc;
}

// ### This func does not belong here
void StandardVSMatrixBuilder(float *result, float *InverseCameraProjectionMatrix, float *ObjectMatrix)
{	// Multiply matrix a * b and store in result, but transpose result along the way
	long element = 0;
	for( long i=0; i<4; i++ ) 
    {	long ii=i<<2;
		for( long j=0; j<4; j++ ) 
		{	result[element] =	InverseCameraProjectionMatrix[j   ] * ObjectMatrix[ii  ] +
								InverseCameraProjectionMatrix[j+ 4] * ObjectMatrix[ii+1] +
								InverseCameraProjectionMatrix[j+ 8] * ObjectMatrix[ii+2] +
								InverseCameraProjectionMatrix[j+12] * ObjectMatrix[ii+3];
			element+=4;
		}
		element -=15;
	}
}

