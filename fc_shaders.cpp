/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <stddef.h>					// This is required for the 'offsetof' statement
#include "fc_h.h"
#include <bucketSystem.h>

uint16		quadFacesIndices[6] = { 0,1,2, 2,1,3 };
sFaceStream *quadFacesStream;

// Plain single-colored rectangle drawn at front of viewport (like a billboard)
const char *colorRectVSStr =
"uniform	vec4	rect;\n"
"attribute	vec4	scale;\n"
"void main(void)\n"
"{	vec4 tmp = scale*rect;\n"
"	gl_Position = vec4(tmp.x+tmp.z, tmp.y+tmp.w, 0.0, 1.0);\n"
"}";
const char *colorRectFSStr =
"uniform	vec4	color;\n"
"void main(void)\n"
"{	gl_FragColor = color;\n"
"}";
struct sColorRectShaderUniform
{	float4	coords;
	float4	color;
};
float4 colorRectVerts[4] = {
	{	1, 0,0, 1	},			// 0: Top Left
	{	0, 0,1, 1	},			// 1: Top Right
	{	1, 1,0, 0	},			// 2: Bottom Left
	{	0, 1,1, 0	}};			// 3: Bottom Right
sVertexStream	*colorRectVtxStream;
sShaderComponent*colorRectVtxShader;
sShaderComponent*colorRectFrgShader;
sShaderProgram	*colorRectShader;
Material		*colorRectMtl;
newobj			colorRectObj;
sColorRectShaderUniform colorRectShaderUniform;

// Simple Shader (rectangle drawn as unlit texture)
const char *simpleTexVSStr =
"uniform	mat4	matrix;\n"						// A Uniform comes from the CPU every time we draw, in this case, it is our matrix (position and orientation of 3D object)
"attribute	vec3	position;\n"					// An Attribute is an element within our vertex stream, in this case, the position of the vertex relative to the object's origin
"attribute	vec2	uv;\n"							// Texture Co-ordinates
"varying	vec2	uvPassthru;\n"					// A Varying is a variable passed from the vertex shader to the Fragment shader.
"void main(void)\n"									// Starting point of the vertex shader is the main function
"{	uvPassthru = uv;\n"								// Send the UV attributes through to the fragment shader
"	gl_Position = matrix * vec4(position, 1.0);\n"	// gl_Position is the output from the shader saying where in SCREEN space to put the vertex
"}";

const char *scaledTexVSStr =
"uniform	mat4	matrix;\n"						// A Uniform comes from the CPU every time we draw, in this case, it is our matrix (position and orientation of 3D object)
"uniform	vec2	UVScale;\n"						// UV Scale factor
"attribute	vec3	position;\n"					// An Attribute is an element within our vertex stream, in this case, the position of the vertex relative to the object's origin
"attribute	vec2	uv;\n"							// Texture Co-ordinates
"varying	vec2	uvPassthru;\n"					// A Varying is a variable passed from the vertex shader to the Fragment shader.
"void main(void)\n"									// Starting point of the vertex shader is the main function
"{	uvPassthru = uv * UVScale;\n"					// Send the UV attributes through to the fragment shader
"	gl_Position = matrix * vec4(position, 1.0);\n"	// gl_Position is the output from the shader saying where in SCREEN space to put the vertex
"}";

const char *simpleTexFSStr =
"uniform	sampler2D tex0;\n"						// A sampler2D is a texture map
"varying	vec2	uvPassthru;\n"					// A Varying is a variable passed from the vertex shader to the Fragment shader.
"void main(void)\n"									// Starting point of the fragment shader is also called main.
"{	gl_FragColor = texture2D(tex0, uvPassthru);\n"	// gl_FragColor is the output from the shader saying the color of the pixel / fragment.
"}";

sShaderComponent*simpleTexVtxShader;	// The Vertex shader - How the vertices are manipulated before they are drawn (rotated and projected onto a 2D screen)
sShaderComponent*scaledTexVtxShader;	// The Scaled Vertex shader - How the vertices are manipulated before they are drawn (rotated and projected onto a 2D screen)
sShaderComponent*simpleTexFrgShader;	// The Fragment shader - How each pixel / fragment is drawn
sShaderProgram	*simpleTexShader;		// The shader program is the combination of the vertex shader + fragment shader + attributes + uniforms + varyings into a single program
sShaderProgram	*scaledTexShader;		// The shader program is the combination of the vertex shader + fragment shader + attributes + uniforms + varyings into a single program

sSimpleTexUniform simpleTexUniform;


// --------------------------------------------------------------------------------------------
// ---										ShaderComponents								---
// --------------------------------------------------------------------------------------------
sShaderComponent *usedsShaderComponent=NULL;
sShaderComponent *freesShaderComponent=NULL;

sShaderComponent *newShaderComponent(uintf componentType, const char *source)
{	sShaderComponent *result;
	uintf type = componentType & shaderComponent_TypeMask;
	allocbucket(sShaderComponent, result, flags, Generic_MemoryFlag32, 1024, "Shader Components");
	vd_compileShaderComponent(result, type, source);
	result->flags |= type;
	return result;
}


// --------------------------------------------------------------------------------------------
// ---										ShaderPrograms									---
// --------------------------------------------------------------------------------------------
sShaderProgram *freesShaderProgram = NULL;
sShaderProgram *usedsShaderProgram = NULL;

/*
sShaderProgram *newShaderProgram(sShaderComponent **shaders, uintf numShaders, uintf totalAttribSize)
{	// Allocate a sShader structures to hold the specified shader.
	sShaderProgram *result;
	allocbucket(sShaderProgram, result, flags, Generic_MemoryFlag32, 128, "Shader Programs");
	result->totalAttributeSize = totalAttribSize;
	result->numAttributes = 0;
	result->numUniforms = 0;
	result->link = NULL;
	vd_linkShaders("newShader",shaders, numShaders, result);
	return result;
}
*/

sShaderProgram *newShaderProgram(sShaderComponent *VertexShader, sShaderComponent *PixelShader, uintf totalAttribSize)
{	// Allocate a sShader structures to hold the specified shader.
	sShaderComponent *shaders[2];
	sShaderProgram *result;
	allocbucket(sShaderProgram, result, flags, Generic_MemoryFlag32, 128, "Shader Programs");
	result->totalAttributeSize = totalAttribSize;
	result->numAttributes = 0;
	result->numUniforms = 0;
	result->link = NULL;
	result->vertexShader = VertexShader;
	result->fragmentShader = PixelShader;
	shaders[0] = VertexShader;
	shaders[1] = PixelShader;
	vd_linkShaders("newShader",shaders, 2, result);
	return result;
}

void copyShaderProgramElements(sShaderProgram *dst, sShaderProgram *src)
{	// Copy all attribute and uniform information from the source (src) shader program to destination (dst)
	// Example: Writing a replacement fragment shader for the HUD system, you would compile the new fragment shader component, compile the new program, copy the shader program elements from the original HUD shader program
	uintf flags = dst->flags & Generic_MemoryFlag32;
	sShaderProgram *prev = dst->prev;
	sShaderProgram *next = dst->next;
	intf programID = dst->programID;
	void *programPtr = dst->programPtr;
	memcopy(dst,src,sizeof(sShaderProgram));
	dst->programPtr = programPtr;
	dst->programID = programID;
	dst->flags &= ~Generic_MemoryFlag32;
	dst->flags |= flags;
	dst->prev = prev;
	dst->next = next;
}

void deleteShaderProgram(sShaderProgram *shaderProgram)
{	// Remove a shader from memory
	vd_deleteShaderProgram(shaderProgram);
	//while (shaderProgram)
	//{	sShaderProgram *tmp = shaderProgram->next;
		deletebucket(sShaderProgram, shaderProgram);
	//	shaderProgram = tmp;
	//}
}

void shaderAttribute(sShaderProgram *shader, const char *attribName, byte dataType, byte numElements, uintf offset)
{	if (shader->numUniforms>0)
		msg("Shader Error","All attributes must be defined before defining Uniforms");
	intf id = shader->numAttributes;
	if (id>32)
		msg("Shader Error","Shaders can currently only handle 32 independant data elements");			// ### Code Incomplete - should allocate another sShaderProgram
	vd_findAttribute(shader,id,attribName);
	shader->elementType[id] = dataType;
	shader->elementSize[id] = numElements;
	shader->elementOffset[id] = offset;
	shader->numAttributes++;
}

void shaderUniform(sShaderProgram *shader, const char *attribName, byte dataType, byte numElements, uintf offset)
{	intf id = shader->numAttributes + shader->numUniforms;
	if (id>32)
		msg("Shader Error","Shaders can currently only handle 32 independant data elements");			// ### Code Incomplete - should allocate another sShaderProgram
	vd_findUniform(shader,id,attribName);
	shader->elementType[id] = dataType;
	shader->elementSize[id] = numElements;
	shader->elementOffset[id] = offset;
	shader->numUniforms++;
}

void drawColorRect(uint32 color, float x1, float y1, float x2, float y2)
{	colorRectShaderUniform.coords.x = x1*2.0f-1.0f;
	colorRectShaderUniform.coords.y = 1.0f-y1*2.0f;
	colorRectShaderUniform.coords.z = x2*2.0f-1.0f;
	colorRectShaderUniform.coords.w = 1.0f-y2*2.0f;
	colorRectShaderUniform.color.a = (float)((color & 0xff000000)>>24)/256.0f;
	colorRectShaderUniform.color.r = (float)((color & 0xff0000)>>16)/256.0f;
	colorRectShaderUniform.color.g = (float)((color & 0xff00)>>8)/256.0f;
	colorRectShaderUniform.color.b = (float)(color & 0xff)/256.0f;
	drawObj3D(&colorRectObj, colorRectObj.matrix, &colorRectShaderUniform);
}

void fc_initShaders(void)
{	quadFacesStream = createFaceStream(2,quadFacesIndices,0);
	uploadFaceStream(quadFacesStream);

	// Compile the shaders
	colorRectVtxShader = newShaderComponent(shaderComponent_Vertex,colorRectVSStr);
	colorRectFrgShader = newShaderComponent(shaderComponent_Fragment,colorRectFSStr);
	colorRectShader	   = newShaderProgram(colorRectVtxShader,colorRectFrgShader, sizeof(float4));
	shaderAttribute(colorRectShader, "scale", vertex_float, 4, 0);
	shaderUniform(colorRectShader, "rect",  vertex_float, 4, offsetof(sColorRectShaderUniform,coords));
	shaderUniform(colorRectShader, "color", vertex_float, 4, offsetof(sColorRectShaderUniform,color));
	colorRectMtl = newmaterial("colorRect");
	colorRectMtl->finalBlendDstScale = blendParam_InvSrcAlpha;
	colorRectMtl->finalBlendSrcScale = blendParam_SrcAlpha;
	colorRectMtl->finalBlendOp = finalBlend_add;
	colorRectMtl->flags &= Generic_MemoryFlag8;
	colorRectMtl->shader = colorRectShader;
	colorRectVtxStream = createVertexStream(colorRectVerts,4,sizeof(float4), vertexStreamFlags_modifyOnce);
	uploadVertexStream(colorRectVtxStream);
	memfill(&colorRectObj,0,sizeof(colorRectObj));
	colorRectObj.faces = quadFacesStream;
	colorRectObj.verts = colorRectVtxStream;
	colorRectObj.mtl = colorRectMtl;

	simpleTexVtxShader = newShaderComponent(shaderComponent_Vertex, simpleTexVSStr);			// Compile vertex shader
	scaledTexVtxShader = newShaderComponent(shaderComponent_Vertex, scaledTexVSStr);			// Compile Scaled vertex shader
	simpleTexFrgShader = newShaderComponent(shaderComponent_Fragment,simpleTexFSStr);			// Compile fragment shader

	simpleTexShader = newShaderProgram(simpleTexVtxShader, simpleTexFrgShader, sizeof(sSimpleVertex));	// Link the vertex and fragment shaders together
	shaderAttribute(simpleTexShader, "position", vertex_float, 3, offsetof(sSimpleVertex,x));	// Tell the shader where to get the 'position' attribute from
	shaderAttribute(simpleTexShader, "uv", vertex_float, 2, offsetof(sSimpleVertex,u));			// Tell the shader where to get the 'UV' attribute from
	shaderUniformConstantInt32(simpleTexShader,"tex0", 0);										// Tell the shader where to get the 'tex0' uniform from (it's a constant value 0)
	shaderUniform(simpleTexShader, "matrix", vertex_mat4x4Ptr, 1, 0);							// Tell the shader where to get the 'matrix' uniform from (A pointer in the uniform structure points to the matrix data)

	scaledTexShader = newShaderProgram(scaledTexVtxShader, simpleTexFrgShader, sizeof(sSimpleVertex));	// Link the vertex and fragment shaders together
	shaderAttribute(scaledTexShader, "position", vertex_float, 3, offsetof(sSimpleVertex,x));	// Tell the shader where to get the 'position' attribute from
	shaderAttribute(scaledTexShader, "uv", vertex_float, 2, offsetof(sSimpleVertex,u));			// Tell the shader where to get the 'UV' attribute from
	shaderUniformConstantInt32(scaledTexShader,"tex0", 0);										// Tell the shader where to get the 'tex0' uniform from (it's a constant value 0)
	shaderUniform(scaledTexShader, "matrix", vertex_mat4x4Ptr, 1, 0);							// Tell the shader where to get the 'matrix' uniform from (A pointer in the uniform structure points to the matrix data)
	shaderUniform(scaledTexShader, "UVScale", vertex_float, 2, offsetof(sSimpleTexUniform,UVScale));	// Tell the shader where to get the 'UVScale' uniform from

	simpleTexUniform.shaderMVPMatrix = modelViewProjectionMatrix;
}

//								*** This file is temporarily disabled until the shader compiler is complete ***
//
// This file manages the shaders, including (future) compiling, and managing the shader library.
//

/*
--------------------------- Header section -----------------------
struct sShader
{	char		name[92];
	int			shaderID;
	byte		shaderType;
	byte		reserved1;			// Reserved for future expansion (used for data alignment right now)
	byte		reserved2;			// Reserved for future expansion (used for data alignment right now)
	sShader		*prev,*next;		// Linked List variables
	sShader		*left,*right;		// BSP variables
	byte		flags;
};

#define shaderType_Vertex	0x00	// Shader is a vertex shader
#define shaderType_Fragment	0x01	// Shader is a pixel / fragment shader (pixel and fragment shaders are the same thing, but fragment is a more accurate name)
#define shaderType_Pixel	0x01	// Shader is a pixel / fragment shader (pixel and fragment shaders are the same thing, but fragment is a more accurate name)
#define shaderType_Program	0xff

sShader *newShader(const char *name, byte type);	// Create a new shader
void deleteShader(sShader *shader);					// Delete a shader
sShader *findShader(const char *shaderName, byte shaderType, bool vital);	// Find a shader based in it's name and type, if vital is true and the shader is not found, the program exits with an error
void loadShaderLibrary(const char *filename);		// Load a shader library file, creating every shader found in that file
---------------------------------------------------------------------


sShader *usedsShader = NULL, *freesShader=NULL;

sShader *newShader(const char *name, byte type)
{	sShader *s;
	allocbucket(sShader, s, flags, Generic_MemoryFlag8, 1024, "Shaders");
	txtcpy(s->name, sizeof(s->name), name);
	s->shaderID = 0;
	s->shaderType = type;
	return s;
}

void freeShader(sShader *shader)
{	deletebucket(sShader, shader);
}

void cleanParsedShaderName(char *dest, intf destSize, const char *src)
{	intf start = 0;
	while ((src[start]==' ' || src[start]=='\t') && src[start]!=0) start++;
	intf stop = txtlen(src)-1;
	while ((src[stop]==' ' || src[stop]=='\t') && stop>=start) stop--;
	stop++;
	if (stop+start>=destSize) stop = start+destSize-1;
	char *dst = dest;
	while (start<stop)
		*dst++=src[start++];
	*dst=0;
}

sShader *aquireShaderFromLibrary(textfile *txt,char *name,byte shaderType)
{	uintf i,j,k;
	char nameCopy[92];
	char shaderSource[4096];
	bool haveMain = false;
	bool skipFirst = false;
	bool inMultilineComment = false;
	uintf bracketCount = 0;
	uintf shaderSourceOffset = 0;
	bool reprocessLine = false;
	char *line;
	while(name[0]==' ' || name[0]=='\t') name++;
	i = txtlen(name)-1;
	while ((name[i]==' ' || name[i]=='\t') && i>1) i--;
	name[i+1]=0;

	cleanParsedShaderName(nameCopy, sizeof(nameCopy), name);

	while(1)
	{	if (haveMain && (bracketCount==0) && (!skipFirst)) break;
		if (!reprocessLine)
			line = txt->readln();
		else
			reprocessLine = false;
		if (!line)
		{	msg("Shader Library Error",buildstr("Unexpected end of file, compiling shader %s",name));
		}
		// Tail end of multi-line comments
		if (inMultilineComment)
		{	for (i=0; line[i]!=0; i++)
			{	if (line[i]=='*' && line[i+1]=='/')
				{	inMultilineComment = false;
					for (j=0;line[i]!=0; i++)
						line[j++] = line[i];
					reprocessLine=true;
					break;
				}
			}
			continue;
		}

		// Clear out leading spaces
		while (line[0]==' ' || line[0]=='\t') line++;

		// Ignore commented lines and empty lines
		if (line[0]=='#' || line[0]==0) continue;

		// Strip out comments
		for (i=0; line[i]!=0; i++)
		{	if (line[i]=='#') {	line[i]=0;	break; }
			if (line[i]=='/' && line[i+1]=='/') {	line[i]=0; break; }
			if (line[i]=='/' && line[i+1]=='*')
			{	inMultilineComment = true;
				for (j=i+2; line[j]!=0; j++)
				{	if (line[j]=='*' && line[j+1]=='/')
					{	inMultilineComment = false;
						k = i;
						for (;line[j]!=0; j++)
							line[k++] = line[j];
						break;
					}
				}
			}
		}
		if (inMultilineComment) continue;

		// Count '{' and '}' characters
		for (i=0; line[i]!=0; i++)
		{	if (line[i]=='{')
			{	bracketCount++;
				skipFirst=false;
			}
			if (line[i]=='}')
			{	if (bracketCount==0) msg("Shader Library Error",buildstr("Unexpected '}' compiling shader %s",name));
				bracketCount--;
			}
		}

		if (txtnicmp(line,"void main",9)==0)
		{	skipFirst = true;
			haveMain = true;
		}

		txtcpy(shaderSource+shaderSourceOffset, sizeof(shaderSource)-shaderSourceOffset, line);
		shaderSourceOffset += txtlen(line);
	}
	sShader *s = newShader(nameCopy,shaderType);
	if (shaderType==shaderType_Vertex)   s->shaderID=compileVertexShader(0,shaderSource);
	if (shaderType==shaderType_Fragment) s->shaderID=compileFragmentShader(0,shaderSource,name);
	return s;
}

sShader *findShader(const char *shaderName, byte shaderType, bool vital)
{	// Finds a shader of type shaderType called shaderName.
	// If no matching shader is found and vital is true, it exits with an error.
	// If no matching shader is found and vital is false, it returns NULL.
	// WARNING: Trying to link with shader 0 will cause an exit with error!
	//			Renderring with shader program 0 creates plain white pixels
	sShader *s = usedsShader;
	while (s)
	{	if ((shaderType==s->shaderType) && (txticmp(s->name,shaderName)==0)) return s;
		s = s->next;
	}
	if (vital)
		msg("Missing Shader",buildstr("Unable to find shader %s matching the requested shader type",shaderName));
	return NULL;
}

void loadShaderLibrary(const char *filename)
{	char *line;
	intf ofs;
	char sep;
	char breaker;
	char shaderProgramName[92], shaderVertexName[92],shaderFragmentName[92],materialName[92];
	Material *currentMaterial = NULL;
	sShader *s;

	if (!fileExists(filename)) return;

	logfile *shaderLog = newlogfile("shaderLibraryLog.txt");

	textfile *txt = newtextfile(filename);
	while (line = txt->readln())
	{	if (line[0]=='#') continue;
		uintf linelen = txtlen(line);
		if (linelen==0) continue;
		ofs = 0;
		char *token = gettokencustom(line,&ofs,&breaker,":");
		if (token)
		{	skipblanks(line,&ofs);
			if (txticmp(token,"VertexShader")==0)
			{	s = aquireShaderFromLibrary(txt,line+ofs,shaderType_Vertex);
				shaderLog->log("Found VertexShader   %i: %s",s->shaderID, s->name);
			} else
			if (txticmp(token,"FragmentShader")==0)
			{	s = aquireShaderFromLibrary(txt,line+ofs,shaderType_Fragment);
				shaderLog->log("Found FragmentShader %i: %s",s->shaderID, s->name);
			} else
			if (txticmp(token,"ShaderProgram")==0)
			{	token = gettokencustom(line,&ofs,&breaker,"=");
				if (!token)
					msg("Shader Library Error",buildstr("Unexpected end of line processing Shader Program: %s",line));
				cleanParsedShaderName(shaderProgramName, sizeof(shaderProgramName), token);
				token = gettokencustom(line,&ofs,&breaker,",");
				if (!token)
					msg("Shader Library Error",buildstr("Unexpected end of line processing Shader Program: %s",line));
				cleanParsedShaderName(shaderVertexName, sizeof(shaderVertexName), token);
				cleanParsedShaderName(shaderFragmentName, sizeof(shaderFragmentName),line+ofs);

				sShader *shaderV = findShader(shaderVertexName, shaderType_Vertex, true);
				sShader *shaderF = findShader(shaderFragmentName, shaderType_Fragment, true);
				sShader *shaderP = newShader(shaderProgramName, shaderType_Program);
				int shaders[3];
				shaders[0] = shaderV->shaderID;
				shaders[1] = shaderF->shaderID;
				shaderP->shaderID = linkShaders(0, shaders, 2, 0, shaderProgramName);
				msg("Incomplete Code","LoadShaderLibrary needs to know numTextures before it links shaders");
				shaderLog->log("Found ShaderProgram %i: %s(%i,%i) : (%s,%s)",shaderP->shaderID, shaderProgramName,shaderV->shaderID,shaderF->shaderID,shaderVertexName,shaderFragmentName);
			}	else
			if (txticmp(token,"Material")==0)
			{	token = gettokencustom(line,&ofs,&breaker,"=");
				if (!token)
					msg("Shader Library Error",buildstr("Unexpected end of line processing material: %s",line));
				cleanParsedShaderName(materialName,sizeof(materialName),token);
				currentMaterial = newmaterial(materialName);
			}	else
			if (txticmp(token,"Flags")==0)
			{	if (!currentMaterial)
					msg("Shader Library Error",buildstr("Flags parameter found outside of Material description: %s",line));
				currentMaterial->flags = (byte)getuinttoken(line,&ofs,&sep);
			}	else
			if (txticmp(token,"Texture")==0)
			{	if (!currentMaterial)
					msg("Shader Library Error",buildstr("Texture parameter found outside of Material description: %s",line));
				uintf i=0;
				while (currentMaterial->texture[i] && i<maxtexchannels) i++;
				cleanParsedShaderName(shaderProgramName, sizeof(shaderProgramName),line+ofs);
				currentMaterial->texture[i]=newTextureFromFile(shaderProgramName,0);
			}	else
			if (txticmp(token,"Shader")==0)
			{	if (!currentMaterial)
					msg("Shader Library Error",buildstr("Shader parameter found outside of Material description: %s",line));
				uintf i=0;
				while (currentMaterial->texture[i] && i<maxtexchannels) i++;
				cleanParsedShaderName(shaderProgramName, sizeof(shaderProgramName),line+ofs);
				s = findShader(shaderProgramName, shaderType_Program, true);
				currentMaterial->shader = s->shaderID;
			}	else
			if (txticmp(token,"Diffuse")==0)
			{	if (!currentMaterial)
					msg("Shader Library Error",buildstr("Shader parameter found outside of Material description: %s",line));
				float colorElement[4];
				for (uintf i=0; i<4; i++)
					colorElement[i]=getfloattoken(line,&ofs,&sep);
				currentMaterial->diffuse.r = colorElement[0];
				currentMaterial->diffuse.g = colorElement[1];
				currentMaterial->diffuse.b = colorElement[2];
				currentMaterial->diffuse.a = colorElement[3];
			}	else
			if (txticmp(token,"Blend")==0)
			{	byte i;
				if (!currentMaterial)
					msg("Shader Library Error",buildstr("Blend parameter found outside of Material description: %s",line));
				token = gettoken(line,&ofs, &sep);
				bool recognisedToken = false;
				for (i=0; i<blendParam_lastBlendParam; i++)
				{	if (txticmp(token,mtlBlendParam[i])==0) currentMaterial->finalBlendDstScale = i;
					recognisedToken = true;
					break;
				}
				if (!recognisedToken)
					msg("Shader Library Error",buildstr("Unrecognised Blend parameter %s in line %s",token,line));

				recognisedToken = false;
				for (i=0; i<finalBlend_lastblendop; i++)
				{	if (txticmp(token,mtlBlendOp[i])==0) currentMaterial->finalBlendOp = i;
					recognisedToken = true;
					break;
				}
				if (!recognisedToken)
					msg("Shader Library Error",buildstr("Unrecognised Blend parameter %s in line %s",token,line));

				recognisedToken = false;
				for (i=0; i<finalBlend_lastblendop; i++)
				{	if (txticmp(token,mtlBlendParam[i])==0) currentMaterial->finalBlendDstScale = i;
					recognisedToken = true;
					break;
				}
				if (!recognisedToken)
					msg("Shader Library Error",buildstr("Unrecognised Blend operation %s in line %s",token,line));
				currentMaterial = NULL;
			}
		}
	}
	delete txt;
	delete shaderLog;
}
*/
