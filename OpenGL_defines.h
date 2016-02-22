
typedef ptrdiff_t GLsizeiptr;

#ifndef GL_ARB_shader_objects
	typedef int GLhandleARB;
	typedef char GLcharARB;
#endif
#ifndef GL_VERSION_2_0
	typedef char GLchar; 
#endif

#define GL_FUNC_ADD											0x8006
#define GL_MIN												0x8007
#define GL_MAX												0x8008
#define GL_FUNC_SUBTRACT									0x800A
#define GL_FUNC_REVERSE_SUBTRACT							0x800B
#define GL_GENERATE_MIPMAP									0x8191
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT						0x83F0
#define	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT					0x83F1
#define GL_SECONDARY_COLOR_ARRAY                            0x845E
#define GL_TEXTURE0											0x84C0
#define GL_TEXTURE_MAX_ANISOTROPY_EXT						0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT					0x84FF
#define GL_MAX_VERTEX_ATTRIBS								0x8869
#define GL_MAX_TEXTURE_COORDS								0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS							0x8872
#define GL_ARRAY_BUFFER										0x8892
#define GL_ELEMENT_ARRAY_BUFFER								0x8893
#define GL_STREAM_DRAW										0x88E0
#define GL_STATIC_DRAW										0x88E4
#define GL_DYNAMIC_DRAW										0x88E8
#define GL_PIXEL_UNPACK_BUFFER								0x88EC
#define GL_FRAGMENT_SHADER									0x8B30
#define	GL_VERTEX_SHADER									0x8B31
#define GL_MAX_VARYING_FLOATS								0x8B4B
#define GL_COMPILE_STATUS									0x8B81
#define GL_VALIDATE_STATUS									0x8B83
#define GL_SHADING_LANGUAGE_VERSION							0x8B8C

// ----------------------------------------------------------------------- OpenGL Prototypes ---------------------------------------------------------------


/*
typedef void (APIENTRY* PFNGLGENBUFFERSPROC) (GLsizei n, GLuint * buffers);
typedef void (APIENTRY* PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRY* PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage);

typedef void (APIENTRY* PFNGLACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRY* PFNGLCLIENTACTIVETEXTUREPROC) (GLenum texture);
typedef void (APIENTRY* PFNGLGENERATEMIPMAPPROC) (GLenum target);
typedef void (APIENTRY* PFNGLCOMPRESSEDTEXIMAGE2DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data);

typedef GLhandleARB (APIENTRY* PFNGLCREATESHADEROBJECTARBPROC) (GLenum shaderType);
typedef void (APIENTRY* PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar* * string, const GLint * length);
typedef void (APIENTRY* PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRY* PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint * params);
typedef void (APIENTRY* PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog);

typedef GLuint (APIENTRY* PFNGLCREATEPROGRAMPROC) ();
typedef void (APIENTRY* PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY* PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRY* PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY* PFNGLVALIDATEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY* PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint * params);
typedef void (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog);

typedef GLint (APIENTRY* PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar * name);
typedef void (APIENTRY* PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRY* PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);

typedef GLint (APIENTRY* PFNGLGETATTRIBLOCATIONPROC) (GLuint program, const GLchar * name);
typedef void (APIENTRY* PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);

typedef void (APIENTRY* PFNGLBLENDEQUATIONEXTPROC) (GLenum mode);

typedef void (APIENTRY* PFNGLSECONDARYCOLORPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);

extern PFNGLGENBUFFERSPROC					glGenBuffers;
extern PFNGLBINDBUFFERPROC					glBindBuffer;
extern PFNGLBUFFERDATAPROC					glBufferData;

extern PFNGLACTIVETEXTUREARBPROC			glActiveTexture;
extern PFNGLCLIENTACTIVETEXTUREPROC			glClientActiveTexture;
extern PFNGLGENERATEMIPMAPPROC				glGenerateMipmap;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC		glCompressedTexImage2D;

extern PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObject;
extern PFNGLSHADERSOURCEPROC				glShaderSource;
extern PFNGLCOMPILESHADERPROC				glCompileShader;
extern PFNGLGETSHADERIVPROC					glGetShader;
extern PFNGLGETSHADERINFOLOGPROC			glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC				glCreateProgram;
extern PFNGLATTACHSHADERPROC				glAttachShader;
extern PFNGLLINKPROGRAMPROC					glLinkProgram;
extern PFNGLUSEPROGRAMPROC					glUseProgram;
extern PFNGLBLENDEQUATIONEXTPROC			glBlendEquation;
extern PFNGLGETUNIFORMLOCATIONPROC			glGetUniformLocation;
extern PFNGLUNIFORM1IPROC					glUniform1i;
extern PFNGLVALIDATEPROGRAMPROC				glValidateProgram;
extern PFNGLGETPROGRAMIVPROC				glGetProgramiv;
extern PFNGLGETATTRIBLOCATIONPROC			glGetAttribLocation;
extern PFNGLUNIFORMMATRIX4FVPROC			glUniformMatrix4fv;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC	glDisableVertexAttribArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC		glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC			glVertexAttribPointer;
extern PFNGLGETPROGRAMINFOLOGPROC			glGetProgramInfoLog;
extern PFNGLSECONDARYCOLORPOINTEREXTPROC	glSecondaryColorPointer;
*/
