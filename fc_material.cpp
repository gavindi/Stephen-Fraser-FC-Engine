/************************************************************************************/
/*								FC_Material file									*/
/*	    						----------------									*/
/*  					   (c) 1998-2004 by Stephen Fraser							*/
/*																					*/
/* Contains the code for handling materials (not the drawing code - that's in the   */
/* video driver																		*/
/*																					*/
/* Initialization Dependencies:														*/
/*   Initgraphics																	*/
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include "fc_h.h"
#include "bucketSystem.h"

#define maxGLSLLights 8
#define maxBuiltShaderLength 2048

enum eLightSpace			// In which co-ordinate space does lighting occur in?
{	lightSpace_Eye=0,		// Eye-space (OpenGL Default)
	lightSpace_Tangent		// Tangent Space (Normal Mapping, Parallax Mapping)
};

struct sGenVertShaderLibrary
{	byte					name[32];
	sShaderComponent		*shaderID;
	uintf					attribSize;
	uintf					flags;
	sGenVertShaderLibrary	*prev,*next;
} *usedsGenVertShaderLibrary, *freesGenVertShaderLibrary;

Material *usedMaterial = NULL, *freeMaterial = NULL;

const char *mtlBlendParam[]={"Zero","One","SrcCol","InvSrcCol","SrcAlpha","InvSrcAlpha","DstAlpha","InvDstAlpha","DstCol","InvDstCol","AlphaSaturation"};
const char *mtlBlendOp[]={"add","subtract","revsubtract","min","max"};

struct sStandardLightingUniforms		// Need to investigate 'Uniform Buffer Object' to optimise this
{	float3	sceneAmbient;
	float3	spotPosition[8];
	float3	spotDirection[8];
	float3	spotDiffuse[8];
	float3	spotSpecular[8];
	float	spotCosCutOff[8];
	float	spotOutterAngle[8];
};

/*
struct sTangentLightingAttributes
{	float3	position;
	float3	normal;
	float4	tangent;			// Only for tangent lighting
	float2	UV[numchannels];	// numchannels is dynamic
}
*/

//	float	*modelViewProjectionMatrix;
//	float	*modelViewMatrix;

struct sMaterialUniforms
{	float4	mtlDiffuse;
	float	mtlSpecularLevel;
	float	mtlShininess;
	float3	spotPosition[maxGLSLLights];
};

struct sShaderBuilderParameters
{	sShaderComponent *eyeSpaceVtx, *eyeSpaceFrag;
	sShaderComponent *TngSpaceVtx, *tngSpaceFrag;
	uintf	numLights;
} shaderBuilderParameters;

	// ### Start of GLESMerge
#ifdef UsingGLES
char tangentSpaceLightingFragSource[maxBuiltShaderLength];
char eyeSpaceLightingFragSource[maxBuiltShaderLength];
#else
sShaderComponent *tangentSpaceLightingFragShader, *eyeSpaceLightingFragShader;
#endif

sShaderComponent *createTangentSpaceLightingFragShader(uintf numLights)
{
#ifdef UsingGLES
	char *shaderSource = tangentSpaceLightingFragSource;
#else
	char shaderSource[maxBuiltShaderLength];
#endif
	// ### End of GLESMerge
	tprintf(shaderSource, sizeof(shaderSource),
	"// tangentSpaceLightingFragment (%i lights)\r\n"		// param1
	"uniform vec4 mtlDiffuse;\r\n"
	"uniform float mtlSpecularLevel;\r\n"
	"uniform float mtlShininess;\r\n"
	"uniform vec3 sceneAmbient;\r\n"
	"uniform vec3  spotDiffuse[%i];\r\n"						// param2
	"uniform vec3  spotSpecular[%i];\r\n"						// param3
	"uniform float spotCosCutOff[%i];\r\n"						// param4
	"uniform float spotOutterAngle[%i];\r\n"					// param5
	"varying vec3 lightDir[%i], spotDir[%i];\r\n"				// param6, param7
	"varying vec3 viewDir;\r\n"
	"vec3 normalMapTexel,normalizedViewDir;\r\n"
	"vec4 lightSpecular, lightDiffuse;\r\n"
	"vec3 lightEmissive,lightAmbient;\r\n"
	"void applyLighting(int lightID)\r\n"
	"{	vec3 D = normalize(spotDir[lightID]);\r\n"
	"	vec3 normalizedLightDir = normalize(lightDir[lightID]);\r\n"
	"	float cos_cur_angle = dot(-normalizedLightDir, D);\r\n"
	"	float cos_inner_cone_angle = spotCosCutOff[lightID];\r\n"
	"	float spot = smoothstep(spotOutterAngle[lightID], cos_inner_cone_angle, cos_cur_angle);\r\n"
	"	vec3 h = normalize(normalizedLightDir + normalizedViewDir);\r\n"
	"	float nDotL = max(0.0, dot(normalMapTexel, normalizedLightDir));\r\n"
	"	float nDotH = max(0.0, dot(normalMapTexel, h));\r\n"
	"	float power = (nDotL == 0.0) ? 0.0 : pow(nDotH, mtlShininess);\r\n"
	"	lightDiffuse.rgb += spotDiffuse[lightID] * nDotL * spot;\r\n"
	"	lightSpecular.rgb += spotSpecular[lightID] * power * spot;\r\n"
	"}\r\n"
	"void initTSLighting(vec3 normal)\r\n"
	"{	lightSpecular = vec4(0,0,0,mtlSpecularLevel);\r\n"
	"	lightDiffuse = vec4(0,0,0,1);\r\n"
	"	lightEmissive = vec3(0,0,0);\r\n"
	"	lightAmbient = sceneAmbient * mtlDiffuse.rgb;\r\n"
	"	normalMapTexel = normalize(normal * 2.0 - 1.0);\r\n"
	"	normalizedViewDir = normalize(viewDir);\r\n"
	,numLights,numLights,numLights,numLights,numLights,numLights,numLights);
	for (uintf i=0; i<numLights; i++)
	{	txtcatf(shaderSource, sizeof(shaderSource), "	applyLighting(%i);\r\n",i);
	}
	txtcat(shaderSource,sizeof(shaderSource), "}\r\n");
	// ### Start of GLESMerge
#ifndef UsingGLES
	tangentSpaceLightingFragShader = newShaderComponent(shaderComponent_Fragment,shaderSource);
	return tangentSpaceLightingFragShader;
#else
	return NULL;
#endif
	// ### End of GLESMerge
}

sShaderComponent *createEyeSpaceLightingFragShader(uintf numLights)
{
	// ### Start of GLESMerge
#ifdef UsingGLES
	char *shaderSource = eyeSpaceLightingFragSource;
#else
	char shaderSource[maxBuiltShaderLength];
#endif
	// ### End of GLESMerge
	tprintf(shaderSource, sizeof(shaderSource),
	"// eyeSpaceLightingFragment (%i lights)\r\n"		// param1
	"uniform vec4  mtlDiffuse;\r\n"
	"uniform float mtlSpecularLevel;\r\n"
	"uniform float mtlShininess;\r\n"
	"uniform vec3  sceneAmbient;\r\n"
	"uniform vec3  spotDirection[%i];\r\n"				// param2
	"uniform vec3  spotDiffuse[%i];\r\n"				// param3
	"uniform vec3  spotSpecular[%i];\r\n"				// param4
	"uniform float spotCosCutOff[%i];\r\n"				// param5
	"uniform float spotOutterAngle[%i];\r\n"			// param6

	"varying vec3 lightDir[%i];\r\n"					// param7
	"varying vec3 varyingNormal, eyeVec;\r\n"

	"vec3 normalizedNormal, normalizedEyeVec;\r\n"
	"vec4 lightSpecular, lightDiffuse;\r\n"
	"vec3 lightEmissive,lightAmbient;\r\n"

	"void applyLighting(int lightID)\r\n"
	"{	vec3 D = normalize(spotDirection[lightID]);\r\n"
	"	vec3 normalizedLightDir = normalize(lightDir[lightID]);\r\n"
	"	float cos_cur_angle = dot(-normalizedLightDir, D);\r\n"
	"	float cos_inner_minus_outer_angle = spotCosCutOff[lightID] - spotOutterAngle[lightID];\r\n"
	"	float spot = clamp((cos_cur_angle - spotOutterAngle[lightID]) / cos_inner_minus_outer_angle, 0.0, 1.0);\r\n"
	"	float lambertTerm = dot(normalizedNormal,normalizedLightDir);\r\n"	// If lambertTerm<0.0 then pixel is not lit by this light
	"	lightDiffuse.rgb += spotDiffuse[lightID] * max(lambertTerm,0.0) * spot;\r\n"
	"	vec3 R = reflect(-normalizedLightDir, normalizedNormal);\r\n"
	"	lightSpecular.rgb += spotSpecular[lightID] * pow( max(dot(R, normalizedEyeVec), 0.0), mtlShininess ) * spot * step(lambertTerm,0.0);\r\n"  // step(x,value) returns 1.0 if x>=value, or 0 if x<value
	"}\r\n"

	"void initESLighting(void)\r\n"
	"{	lightSpecular = vec4(0,0,0,mtlSpecularLevel);\r\n"					// Specular Level (Multiplied against lightSpecular.rgb)
	"	lightDiffuse = vec4(0,0,0,1);\r\n"
	"	lightEmissive = vec3(0,0,0);\r\n"
	"	lightAmbient = sceneAmbient * mtlDiffuse.rgb;\r\n"
	"	normalizedNormal = normalize(varyingNormal);\r\n"
	"	normalizedEyeVec = normalize(eyeVec);\r\n"
	,numLights,numLights,numLights,numLights,numLights,numLights,numLights);
	for (uintf i=0; i<numLights; i++)
	{	txtcatf(shaderSource, sizeof(shaderSource), "	applyLighting(%i);\r\n",i);
	}
	txtcat(shaderSource,sizeof(shaderSource), "}\r\n");
	// ### Start of GLESMerge
#ifndef UsingGLES
	eyeSpaceLightingFragShader=newShaderComponent(shaderComponent_Fragment,shaderSource);
	return eyeSpaceLightingFragShader;
#else
	return NULL;
#endif
}

bool materialSystemInitialized = false;

void initmaterial(void)
{	usedsGenVertShaderLibrary = NULL;
	freesGenVertShaderLibrary = NULL;
	createTangentSpaceLightingFragShader(maxGLSLLights);	//newShaderComponent(shaderComponent_Fragment, tangentSpaceLightingFragSrc);
	createEyeSpaceLightingFragShader(maxGLSLLights);			//newShaderComponent(shaderComponent_Fragment,eyeSpaceLightingFragSrc);
	// ### End of GLESMerge
	materialSystemInitialized = true;
}

void killmaterial(void)
{	if (!materialSystemInitialized) return;
	killbucket(sGenVertShaderLibrary,flags,Generic_MemoryFlag32);
	killbucket(Material, flags, Generic_MemoryFlag8);
}

Material *newmaterial(const char *name)	// Allocate a material
{	uintf i;
	Material *mtl;
	allocbucket(Material, mtl, flags, Generic_MemoryFlag8, 256, "Material Cache");
	mtl->diffuse.a = 1.00f;	mtl->diffuse.r = 1.00f;	mtl->diffuse.g = 1.00f;	mtl->diffuse.b = 1.00f;
	mtl->shininess = 0.4f; mtl->shininessStr = 1.0f;
	// mtl->bias = 0;
	txtcpy((char *)mtl->name, sizeof(mtl->name), name);
	mtl->finalBlendOp = finalBlend_add;
	mtl->finalBlendSrcScale = blendParam_SrcAlpha;
	mtl->finalBlendDstScale = blendParam_InvSrcAlpha;
	for (i=0; i<maxtexchannels; i++)
	{	mtl->texture[i]=NULL;
	}
	return mtl;
}

Material *copyMaterial(Material *src)
{	Material *mtl = newmaterial(src->name);
	uint8 flags = mtl->flags & Generic_MemoryFlag8;
	Material *prev = mtl->prev;
	Material *next = mtl->next;
	memcopy(mtl,src,sizeof(Material));
	mtl->flags &= ~Generic_MemoryFlag8;
	mtl->flags |= flags;
	mtl->prev = prev;
	mtl->next = next;
	return mtl;
}

void _deletematerial(Material *mtl)
{	if (!mtl) return;

	for (long i=0; i<8; i++)
		if (mtl->texture[i]) deletetexture(mtl->texture[i]);

	deletebucket(Material, mtl);
}

Material *findMaterial(const char *materialName, bool vital)
{	// Finds a material called materialName.
	// If no matching material is found and vital is true, it exits with an error.
	// If no matching material is found and vital is false, it returns NULL.
	Material *m = usedMaterial;
	while (m)
	{	if (txticmp(m->name,materialName)==0) return m;
		m = m->next;
	}
	if (vital)
		msg("Missing Material",buildstr("Unable to find material %s",materialName));
	return NULL;
}

void vShaderCreateName(char *vertexShaderName, const char *filename, byte *shaderName)
{	tprintf(vertexShaderName, 128, "%s::v_",filename);
	char xx[3]={0,0,0};
	for (uintf i=0; shaderName[i]!=255; i++)
	{	tprintf(xx,3,"%2X",shaderName[i]);
		txtcat(vertexShaderName,128,xx);
	}
	txtcat(vertexShaderName,128,"FF");
}

#define shaderFeature(condition,text) \
	if (!condition) \
	{	txtcat(shaderSource,sizeof(shaderSource),text); \
		condition = true; \
	}
/*
// ------------------------------------------------------ Material Translation ------------------------------------------------------------
sShaderProgram *buildStandardShaders(sFileHandle *handle, const char *filename, sImportMtl *mtl)
{	uintf i,bumptex;
	char shaderSource[maxBuiltShaderLength];
	char tmp[256];
	byte shaderName[32];
	sShaderComponent *vshader,*fshader;
	char vertexShaderName[128];		// Human Readable version of vertex shader name (eg: scene::v_0020FF)
	uintf attribSize;

	// Check what features we are using so we know what values we will need
	bool featureSet[32];
	for (i=0; i<32; i++)
	{	featureSet[i]=false;
	}
	for (i=0; i<mtl->numTextures; i++)
	{	byte slotType = mtl->slotType[i];
		if (slotType<32)
			featureSet[slotType] = true;
		if (slotType==slotType_Bump)
			bumptex = i;
	}

	// ************************ Code to try to reuse vertex shaders *********************
	// Construct Shader Caching Parameters
	eLightSpace lightSpace;
	if (featureSet[slotType_Bump])
	{	lightSpace = lightSpace_Tangent;
		shaderName[0]=0;
	}
	else
	{	lightSpace = lightSpace_Eye;
		shaderName[0]=1;
	}
	for (i=0; i<mtl->numTextures; i++)
	{	Texture *tex = mtl->texture[i];
		if (mtl->mapping[i] == 1)
		{	// Environmental Sphere Mapped
			shaderName[1]|=7;				// normalizedVertexEye | normalReflection | EnvMapScale
			shaderName[2+i]=128;			// Generated co-ord set 0 (Environmental Sphere Mapped) (bit 7=1 for Generated Co-ords)
		}	else
		{	// UV Co-ord Channel
			shaderName[2+i]=mtl->channel[i] | 32;		// Bits 0..3 = UV Channel Number, Bits 4..6 = (Number of axis-1), Bit 7=0 for Co-ord Set or 1 for Generated Co-ords
		}
	}
	shaderName[2+i]=255;				// End of texture Data
	uintf cacheSize = i+3;

	// needMUTEX
	// Check the library and see if we have a matching shader already
	sGenVertShaderLibrary *found = NULL;
	sGenVertShaderLibrary *search = usedsGenVertShaderLibrary;
	while (search)
	{	bool match = true;
		for (i=0; i<sizeof(shaderName); i++)
		{	if (search->name[i]!=shaderName[i])
			{	match = false;
				break;
			}
		}
		if (match)
		{	found = search;
			break;
		}
		search=search->next;
	}
	if (!found)
	{	allocbucket(sGenVertShaderLibrary, found, flags, Generic_MemoryFlag32, 1024, "Generated Vertex Shader Cache");
		// MUTEXRelease
		// ***************** End of code to try to reuse vertex shaders *********************

		// Find the number of Attribute UV channels
		uintf maxUV = 0;
		for (i=0; i<mtl->numTextures; i++)
		{	if ((mtl->mapping[i]*0x0f) == 0)
				if (mtl->channel[i]>=maxUV) maxUV=mtl->channel[i]+1;
		}

		if (lightSpace==lightSpace_Tangent)
		{	//						Tangent Light Space
			tprintf(shaderSource,sizeof(shaderSource),
				"// Tangent Space Vertex Shader\r\n"
				"varying   vec3   lightDir[%i], spotDir[%i];\r\n"	// param1, param2
				"varying   vec3   viewDir;\r\n"
				"uniform   mat4   modelViewProjectionMatrix;\r\n"
				"uniform   mat4   modelViewMatrix;\r\n"
				//"uniform   mat3x3 normalMatrix;\r\n"		// normalMatrix == modelViewMatrix if there is only uniform scaling
				"uniform   vec3   spotPosition[%i];\r\n"			// param3
				"uniform   vec3   spotDirection[%i];\r\n"			// param4
				"attribute vec3   position;\r\n"
				"attribute vec3   normal;\r\n"
				"attribute vec4   tangent;\r\n",maxGLSLLights,maxGLSLLights,maxGLSLLights,maxGLSLLights);

			if (mtl->numTextures>0)
			{	txtcatf(shaderSource,sizeof(shaderSource),"varying   vec2   varyingUV[%i];\r\n",mtl->numTextures);
			}
			for (i=0; i<maxUV; i++)
			{	txtcatf(shaderSource,sizeof(shaderSource),
					"attribute vec2   UV%i;\r\n",i);
			}
			attribSize = 40+maxUV*8;
			txtcatf(shaderSource,sizeof(shaderSource),
				"void main()\r\n"															// Common
				"{	vec3 vertexEye = vec3(modelViewMatrix * vec4(position,1.0));\r\n"		// Common
				"	vec3 varyingNormal = mat3(modelViewMatrix) * normal;\r\n"				// substituted 'normalMatrix' for modelViewMatrix.  Must have eyeSpaceNormal no matter what! May have to normalize this, but I don't think so.
				"	vec3 tangent_ = normalize(mat3(modelViewMatrix) * tangent.xyz);"		// substituted 'normalMatrix' for modelViewMatrix.
				"	vec3 binormal = cross(varyingNormal, tangent) * tangent.w;\r\n"

				"	mat3x3 tbnMatrix = mat3(	tangent_.x, binormal.x, varyingNormal.x,\r\n"
				"								tangent_.y, binormal.y, varyingNormal.y,\r\n"
				"								tangent_.z, binormal.z, varyingNormal.z);\r\n"
				"	viewDir = tbnMatrix * (-vertexEye);\r\n"
				);
			for (uintf lightNum = 0; lightNum<maxGLSLLights; lightNum++)
			{	txtcatf(shaderSource,sizeof(shaderSource),
					"	lightDir[%i] = tbnMatrix * (spotPosition[%i] - vertexEye);\r\n"	//lightDir;\r\n"
					"	spotDir[%i] = tbnMatrix * spotDirection[%i];\r\n"
					,lightNum,lightNum,lightNum,lightNum);
			}
		}	else
		{	//						Eye Light Space
			tprintf(shaderSource,sizeof(shaderSource),
				"// Eye Space Vertex Shader\r\n"
				"varying   vec3   lightDir[%i];\r\n"			// param1
				"varying   vec3   varyingNormal, eyeVec;\r\n"
				"uniform   mat4   modelViewProjectionMatrix;\r\n"
				"uniform   mat4   modelViewMatrix;\r\n"
				// "uniform   mat3x3 normalMatrix;\r\n"		// normalMatrix == modelViewMatrix if there is only uniform scaling
				"uniform   vec3   spotPosition[%i];\r\n"		// param2
				"attribute vec3   position;\r\n"
				"attribute vec3   normal;\r\n",maxGLSLLights,maxGLSLLights);
			if (mtl->numTextures>0)
			{	txtcatf(shaderSource,sizeof(shaderSource),"varying   vec2   varyingUV[%i];\r\n",mtl->numTextures);
			}
			for (i=0; i<maxUV; i++)
			{	txtcatf(shaderSource,sizeof(shaderSource),
					"attribute vec2   UV%i;\r\n",i);
			}
			attribSize = 24+maxUV*8;
			txtcatf(shaderSource,sizeof(shaderSource),
				"void main()\r\n"															// Common
				"{	vec3 vertexEye = vec3(modelViewMatrix * vec4(position,1.0));\r\n"	// Common
				"	varyingNormal = mat3(modelViewMatrix) * normal;\r\n"							// substituted 'normalMatrix' for modelViewMatrix.  Must have eyeSpaceNormal no matter what! May have to normalize this, but I don't think so.
				"	eyeVec = -vertexEye;\r\n"
				,maxUV);
			for (uintf lightNum = 0; lightNum<maxGLSLLights; lightNum++)
			{	txtcatf(shaderSource,sizeof(shaderSource),
					"	lightDir[%i] = spotPosition[%i] - vertexEye;\r\n"	//lightDir;\r\n"
					,lightNum,lightNum);
			}
		}

		bool haveNormalizedVertexEye = false;
		bool haveNormalReflection = false;
		bool haveEnvMapScale = false;
		for (i=0; i<mtl->numTextures; i++)
		{	Texture *tex = mtl->texture[i];
			if ((mtl->mapping[i]*0x0f) == 1)
			{	// This texture is EnvSphere Mapped
				shaderFeature(haveNormalizedVertexEye,	"	vec3 normalizedVertexEye = normalize( vertexEye );\r\n");
				shaderFeature(haveNormalReflection,		"	vec3 normalReflection = reflect( normalizedVertexEye, varyingNormal )+vec3(0,0,1);\r\n");
				shaderFeature(haveEnvMapScale,			"	float EnvMapScale = 2.0 * sqrt(dot( normalReflection, normalReflection));\r\n");			//sqrt( normalReflection.x*normalReflection.x + normalReflection.y*normalReflection.y + (normalReflection.z+1.0)*(normalReflection.z+1.0) );\r\n");
				txtcatf(shaderSource,sizeof(shaderSource),
					"\tvaryingUV[%i].x = normalReflection.x/EnvMapScale + 0.5;\r\n"
					"\tvaryingUV[%i].y = -(normalReflection.y/EnvMapScale + 0.5);\r\n",i,i);
			}	else
			{	// Standard UV mapped texture
				txtcatf(shaderSource,sizeof(shaderSource),"\tvaryingUV[%i].xy = UV%i.xy;\r\n",i,mtl->channel[i]);
			}
		}
		txtcat(shaderSource, sizeof(shaderSource),"\tgl_Position = modelViewProjectionMatrix * vec4(position,1.0);\r\n}\r\n");
		if (txtlen(shaderSource)>=maxBuiltShaderLength-1)
			msg("Shader Writer Overflow",buildstr("The automatic shader writer created a source file that was too long\r\nwhile writing a VERTEX shader for material %s",mtl->name));

		vshader = newShaderComponent(shaderComponent_Vertex,shaderSource);

		if (handle)
		{	vShaderCreateName(vertexShaderName, filename, shaderName);
			tprintf(tmp,sizeof(tmp),"VertexShader: %s\r\n",vertexShaderName);//%s\r\n",mtl->name);

			fileWrite(handle,tmp, txtlen(tmp));
			fileWrite(handle,shaderSource, txtlen(shaderSource));
		}

		found->shaderID = vshader;
		found->attribSize = attribSize;
		memcopy(found->name, shaderName, sizeof(shaderName));
	}	else	// End of caching system
	{	// MUTEXRelease (from searching for a match in the shader library)
		vshader = found->shaderID;
		attribSize = found->attribSize;
		vShaderCreateName(vertexShaderName, filename, found->name);
	}
	// ---------------------------------------------------- Fragment Shader ----------------------------------------------------
	tprintf(shaderSource, sizeof(shaderSource),			 // Create new shader source for fragment shader
		"// Fragment Shader\r\n"
		"uniform vec4 mtlDiffuse;\r\n");

	if (mtl->numTextures>0)
		txtcatf(shaderSource, sizeof(shaderSource),"varying vec2 varyingUV[%i];\r\n",mtl->numTextures);


	// Generate shader name
	// ###

	// Declare the texture maps
	for (i=0; i<mtl->numTextures; i++)
	{	txtcatf(shaderSource,sizeof(shaderSource),
			"uniform sampler2D tex%i;\r\n",i);
	}
	txtcat(shaderSource,sizeof(shaderSource),
		"vec4 lightSpecular, lightDiffuse;\r\n"
		"vec3 lightEmissive,lightAmbient, lighting;\r\n");

	if (lightSpace==lightSpace_Eye)
	{	txtcat(shaderSource,sizeof(shaderSource),"void initESLighting(void);\r\n");
	}
	else
	{	txtcat(shaderSource,sizeof(shaderSource),"void initTSLighting(vec3 normal);\r\n");
	}


/ *	// Direction Lighting
	txtcat(shaderSource,sizeof(shaderSource),
		"varying vec3 normal,lightDir,eyeVec;\r\n"
		"void main(void)\r\n{"
		"\tvec4 col;\r\n"
		"\tvec3 lighting = vec3(0,0,0);\r\n"
		"\tvec3 specular = vec3(0,0,0);\r\n"
		"\tvec3 ambient = sceneAmbient * gl_FrontMaterial.ambient.rgb;\r\n"
		"\tvec3 N = normalize(normal.rgb);\r\n"
		"\tvec3 L = normalize(lightDir.rgb);\r\n"
		"\tfloat lambertTerm = dot(N,L);\r\n"
		"\tif(lambertTerm > 0.0)\r\n"
		"\t{\tlighting += spotDiffuse[0] * lambertTerm;\r\n"
		"\t\tvec3 E = normalize(eyeVec);\r\n"
		"\t\tvec3 R = reflect(-L, N);\r\n"
		"\t\tspecular += spotSpecular[0] * pow( max(dot(R, E), 0.0), mtlShininess ) * mtlSpecularLevel;\r\n"
		"\t}\r\n"
		"\tcol.rgb = lighting * mtlDiffuse.rgb;\r\n"
		"\tcol.a = mtlDiffuse.a;\r\n");
* /
/ *	// Point Lighting
	txtcat(shaderSource,sizeof(shaderSource),
		"varying vec3 normal,lightDir,eyeVec;\r\n"
		"void main(void)\r\n{"
		"	vec3 specular = vec3(0,0,0);\r\n"
		"	vec4 lightDiffuse = vec4(0,0,0,1);\r\n"
		"	vec3 ambient = sceneAmbient * gl_FrontMaterial.ambient.rgb;\r\n"
		"	vec3 L = normalize(lightDir);\r\n"
		"	vec3 D = normalize(spotDirection[0]);\r\n"
		"	if (dot(-L, D) > spotCosCutoff[0])\r\n"
		"	{	vec3 N = normalize(normal);\r\n"
		"		float lambertTerm = max( dot(N,L), 0.0);\r\n"
		"		if(lambertTerm > 0.0)\r\n"
		"		{	lightDiffuse.rgb += spotDiffuse[0] * lambertTerm;\r\n"
		"			vec3 E = normalize(eyeVec);\r\n"
		"			vec3 R = reflect(-L, N);\r\n"
		"			specular += spotSpecular[0].specular.rgb * pow( max(dot(R, E), 0.0), mtlShininess );\r\n"
		"		}\r\n"
		"	}\r\n"
		"lightDiffuse.a = gl_FrontMaterial.ambient.a;\r\n"
"vec3 lighting = lightDiffuse.rgb;\r\n"
"vec4 col = lightDiffuse * mtlDiffuse;\r\n");
* /

	if (lightSpace==lightSpace_Eye)
	{	txtcat(shaderSource,sizeof(shaderSource),
			"void main (void)\r\n"
			"{	initESLighting();\r\n"
		);
	}	else
	{	uintf len = txtlen(shaderSource);
		txtcatf(shaderSource,sizeof(shaderSource),
			"void main (void)\r\n"
			"{	initTSLighting(texture2D(tex%i, varyingUV[%i].st).xyz);\r\n"
			,bumptex,bumptex);
	}

	// Need to process ambient lighting first
	if (featureSet[slotType_Ambient])
	{	for (i=0; i<mtl->numTextures; i++)
		{	if (mtl->slotType[i]==slotType_Ambient)
			{	if (mtl->texAmount[i]==100)
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightAmbient = mtlDiffuse.rgb * texture2D(tex%i, varyingUV[%i].xy).rgb;\r\n",i,i);		// mtlDiffuse should be mtlAmbient except it was removed from the engine assuming most materials have ambient==diffuse
				else
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightAmbient = mix(lightAmbient, mtlDiffuse.rgb * texture2D(tex%i, varyingUV[%i].xy).rgb, %f);\r\n",i,i,((double)mtl->texAmount[i])/100.0);
			}
		}
	}
	txtcat(shaderSource,sizeof(shaderSource),
		"	lighting = lightDiffuse.rgb + lightAmbient.rgb;\r\n"
		"	lightDiffuse.rgb = lighting * mtlDiffuse.rgb;\r\n"
		"	lightDiffuse.a = mtlDiffuse.a;\r\n"
		);

	bool haveDiffuse = false;

	for (i=0; i<mtl->numTextures; i++)
	{	int texAmount = mtl->texAmount[i];
		if ((mtl->slotType[i] & 64) && mtl->slotType[i]!=255)				// Composite
		{	txtcatf(shaderSource,sizeof(shaderSource),
				"\tvec4 mask = texture2D(tex%i, varyingUV[%i].xy);\r\n"
				"\tvec4 composite = texture2D(tex%i, varyingUV[%i].xy) * (1-(mask.r+mask.g+mask.b));\r\n",i,i,i+1,i+1);
			int numChannels = mtl->slotType[i] & 63;
			for (int j=1; j<numChannels; j++)
			{	char *scale = "mask.r";
				if (j==2) scale = "mask.g";
				if (j==3) scale = "mask.b";
				if (j==4) scale = "mask.a";
				txtcatf(shaderSource,sizeof(shaderSource),"\tcomposite += texture2D(tex%i, varyingUV[%i].xy) * %s;\r\n",i+j+1,i+j+1,scale);
			}
			i += numChannels;
			{	if (texAmount==100)
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightDiffuse.rgb = lighting * composite.rgb;\r\n");
//											"\tlightSpecular.a = composite.a;\r\n",i,i);
				else
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightDiffuse.rgb = mix(lightDiffuse.rgb, lighting * composite.rgb, %f);\r\n",((double)texAmount)/100.0);
//											"\tlightSpecular.a = mix(lightSpecular.a, composite.a, %f);\r\n",((double)texAmount)/100.0,((double)texAmount)/100.0);
				haveDiffuse = true;
			}
		}	else
		switch(mtl->slotType[i])
		{	case slotType_Diffuse:					// Diffuse
			{	if (haveDiffuse)
				{	txtcatf(shaderSource,sizeof(shaderSource),"\tlightDiffuse.rgb *= texture2D(tex%i, varyingUV[%i].xy).rgb;\r\n",i,i);
				}	else
				{	if (texAmount==100)
						txtcatf(shaderSource,sizeof(shaderSource),"\tlightDiffuse.rgb = lighting * texture2D(tex%i, varyingUV[%i].xy).rgb;\r\n",i,i);
					else
						txtcatf(shaderSource,sizeof(shaderSource),"\tlightDiffuse.rgb = mix(lightDiffuse.rgb, lighting * texture2D(tex%i, varyingUV[%i].xy).rgb, %f);\r\n",i,i,((double)texAmount)/100.0);
					haveDiffuse = true;
				}
				break;
			}
			case slotType_Specular:					// Specular
			{	if (texAmount==100)
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightSpecular.rgb *= texture2D(tex%i, varyingUV[%i].xy).rgb;\r\n",i,i);
				else
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightSpecular.rgb = mix(lightSpecular.rgb, texture2D(tex%i, varyingUV[%i].xy).rgb, %f);\r\n",i,i,((double)texAmount)/100.0);
				break;
			}
			case slotType_SpecularLevel:			// Specular Level
			{	if (texAmount==100)
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightSpecular.a = texture2D(tex%i, varyingUV[%i].xy).a;\r\n",i,i);
				else
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightSpecular.a = mix(lightSpecular.a,texture2D(tex%i, varyingUV[%i].xy).a, %f);\r\n",i,i,((double)texAmount)/100.0);
				break;
			}
			case slotType_Reflection:				// Reflection
			{	if (texAmount==100)
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightEmissive.rgb += texture2D(tex%i, varyingUV[%i].xy).rgb * lighting;\r\n",i,i);
				else
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightEmissive.rgb += texture2D(tex%i, varyingUV[%i].xy).rgb * lighting * %f;\r\n",i,i,((double)texAmount)/100.0);
				break;
			}
			case slotType_Emissive:					// Emissive
			{	if (texAmount==100)
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightEmissive.rgb = texture2D(tex%i, varyingUV[%i].xy).rgb;\r\n",i,i);
				else
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightEmissive.rgb = mix(lightEmissive.rgb,texture2D(tex%i, varyingUV[%i].xy).rgb, %f).rgb;\r\n",i,i,((double)texAmount)/100.0);
				break;
			}
			case slotType_Opacity:					// Opacity
			{	if (texAmount==100)
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightDiffuse.a = texture2D(tex%i, varyingUV[%i].xy).a;\r\n",i,i);
				else
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightDiffuse.a = mix(lightDiffuse.a,texture2D(tex%i, varyingUV[%i].xy).a, %f).a;\r\n",i,i,((double)texAmount)/100.0);
				break;
			}
			case slotType_DiffuseAndOpacity:		// Diffuse and Opacity
			{	if (texAmount==100)
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightDiffuse *= texture2D(tex%i, varyingUV[%i].xy);\r\n",i,i);
				else
					txtcatf(shaderSource,sizeof(shaderSource),"\tlightDiffuse = mix(lightDiffuse, texture2D(tex%i, varyingUV[%i], %f);\r\n",i,i,((double)texAmount)/100.0);
				break;
			}

			// These slot types have already been processed.
			case slotType_Ambient:
			case slotType_Bump:
				break;

			// These slot types aren't handled yet, the best thing to do is ignore them for now
			case slotType_Glossiness:
			case slotType_FilterColor:
			case slotType_Refraction:
			case slotType_Displacement:
				break;

			default:
				break;
		}
	}
	txtcat(shaderSource,sizeof(shaderSource),"\tgl_FragColor.rgb = lightDiffuse.rgb + lightSpecular.rgb*lightSpecular.a + lightEmissive;\r\n");
	txtcat(shaderSource,sizeof(shaderSource),"\tgl_FragColor.a = lightDiffuse.a;\r\n}\r\n");

	if (txtlen(shaderSource)>=maxBuiltShaderLength-1)
		msg("Shader Writer Overflow",buildstr("The automatic shader writer created a source file that was too long\r\nwhile writing a FRAGMENT shader for material %s",mtl->name));

	fshader = newShaderComponent(shaderComponent_Fragment,shaderSource);

	sShaderComponent *shaders[3];
	shaders[0] = vshader;
	shaders[1] = fshader;

	// ### Start of GLESMerge
	msg("Unfinished Code","buildStandardShaders has not been upgraded to GL 2+");
//	if (lightSpace==lightSpace_Eye)
//	{	shaders[2] = eyeSpaceLightingFragShader;
//	}	else
//	{	shaders[2] = tangentSpaceLightingFragShader;
//	}
	// ### End of GLESMerge

//	sShaderProgram *result = newShaderProgram(shaders, 3, attribSize);

	if (handle)
	{	tprintf(tmp,sizeof(tmp),"FragmentShader: %s::f_%s\r\n",filename,mtl->name);
		fileWrite(handle,tmp, txtlen(tmp));
		fileWrite(handle,shaderSource, txtlen(shaderSource));

		tprintf(tmp,sizeof(tmp),"ShaderProgram: %s::shader_%s = %s, %s::f_%s\r\n",filename,mtl->name,vertexShaderName,filename,mtl->name );
		fileWrite(handle,tmp, txtlen(tmp));
	}
//	return result;
	return NULL;
}
#undef shaderFeature

Material *translateMaterial(sImportMtl *importMtl, sFileHandle *genShaders, const char *className, uintf flags)
{	// Todo - Add support for minimum alpha
	char genShad[256];
	if (genShaders)
	{	tprintf(genShad,sizeof(genShad),"\r\n\r\n#----------------------------------------------------------------------------- %s\r\n",importMtl->name);
		fileWrite(genShaders, genShad, txtlen(genShad));
	}

	Material *mtl = newmaterial(importMtl->name);
	mtl->flags		= importMtl->flags | mtl_StandardLighting;

	// Textures
	for (byte j=0; j<importMtl->numTextures; j++)
	{	mtl->texture[j] = importMtl->texture[j];
	}

	// Read in final blending
	mtl->finalBlendDstScale = importMtl->targetParam;
	mtl->finalBlendSrcScale = importMtl->sourceParam;
	mtl->finalBlendOp  = importMtl->finalBlend;

	mtl->shader = buildStandardShaders(genShaders,className,importMtl);
	if (genShaders)
	{	tprintf(genShad,sizeof(genShad),"Material: %s::%s\r\n",className,importMtl->name);
		fileWrite(genShaders, genShad, txtlen(genShad));
		tprintf(genShad,sizeof(genShad),"Flags: %x\r\n",importMtl->flags);
		fileWrite(genShaders, genShad, txtlen(genShad));
		for (byte j=0; j<importMtl->numTextures; j++)
		{	tprintf(genShad,sizeof(genShad),"Texture: %s\r\n",importMtl->texture[j]->name);
			fileWrite(genShaders, genShad, txtlen(genShad));
		}
		tprintf(genShad,sizeof(genShad),"Shader: %s::shader_%s\r\n",className,importMtl->name);
		fileWrite(genShaders, genShad, txtlen(genShad));

		tprintf(genShad,sizeof(genShad),"Diffuse: %f, %f, %f, %f\r\n",importMtl->diffuse.r, importMtl->diffuse.g, importMtl->diffuse.b, importMtl->diffuse.a);
		fileWrite(genShaders,genShad,txtlen(genShad));

		tprintf(genShad,sizeof(genShad),"Blend: %s %s %s\r\n",mtlBlendParam[importMtl->targetParam], mtlBlendOp[importMtl->finalBlend], mtlBlendParam[importMtl->sourceParam]);
		fileWrite(genShaders,genShad,txtlen(genShad));
	}

	// ### This block disabled for now - we need the shader first
//	mtl->diffuse.a	= importMtl->diffuse.a;
//	mtl->diffuse.r	= importMtl->diffuse.r;
//	mtl->diffuse.g	= importMtl->diffuse.g;
//	mtl->diffuse.b	= importMtl->diffuse.b;
//	mtl->shininess	= importMtl->shininess;
//	mtl->shininessStr	= importMtl->shininessStr;

	// The following parameters are not needed in the material structure
	// mtl->numtex		= importMtl->numTextures;		// A NULL pointer in texture channel represents the end of the textures
	// mtl->specPower	= importMtl->spec_power;		// Common default of 4, anything else can be hard coded into shaders
	// mtl->bias		= importMtl->bias;				// Common default of 0
	// mtl->mapping[j]  = importMtl->mapping[j];		// Mapping sources are compiled into shaders
	// mtl->channel[j]	= importMtl->channel[j];		// Mapping co-ordinate channels are compiled into shaders
	// mtl->slotType[j]	= importMtl->slotType[j];		// Texture blending is compiled into the fragment shader
	// mtl->texAmount[j]= importMtl->texAmount[j];		// Texture amount is compiled into the fragment shader

	return mtl;
}
*/

/*
// ********************************************************************
// ****						Shader Parameter List					***
// ********************************************************************
ShaderParameter *newShaderParameter(void)
{	ShaderParameter *sp;
	allocbucket(ShaderParameter, sp, flags, Generic_MemoryFlag32, 128, "Shader Parameters");
	for (uintf i=0; i<16; i++)
	{	sp->sourcedata[i] = NULL;
		sp->manipulator[i] = NULL;
		sp->link = NULL;
	}
	return sp;
}

void SPM_CopyVector(float *sourcedata, float *destptr)	// This manipulator just copies the vector as-is
{	destptr[0] = sourcedata[0];
	destptr[1] = sourcedata[1];
	destptr[2] = sourcedata[2];
	destptr[3] = sourcedata[3];
}

void SPM_ToObjectSpace(float *sourcedata, float *destptr) // This manipulator transforms an point from world-object co-ordinates into object-space co-ordinates
{//	transformPointByMatrix((float3 *)sourcedata, InvObjMat, (float3 *)destptr);
 	destptr[0] = (sourcedata[0] * InvObjMat[mat_xvecx] + sourcedata[1] * InvObjMat[mat_yvecx] + sourcedata[2] * InvObjMat[mat_zvecx] + InvObjMat[mat_xpos]);
 	destptr[1] = (sourcedata[0] * InvObjMat[mat_xvecy] + sourcedata[1] * InvObjMat[mat_yvecy] + sourcedata[2] * InvObjMat[mat_zvecy] + InvObjMat[mat_ypos]);
 	destptr[2] = (sourcedata[0] * InvObjMat[mat_xvecz] + sourcedata[1] * InvObjMat[mat_yvecz] + sourcedata[2] * InvObjMat[mat_zvecz] + InvObjMat[mat_zpos]);
	destptr[3] = 1.0f;
}

void addShaderParam(ShaderParameter *sp, uintf *numparams, const char *line, intf *ofs, void *(*tokenfinder)(const char *))
{	char sep;
	char *token;

	// Obtain Manipulator
	void (*manipulator)(float*,float*) = NULL;
	token = gettoken((char *)line, ofs, &sep);
	if (txticmp(token, "ToObjectSpace")==0)  manipulator = SPM_ToObjectSpace;
	else if (txticmp(token, "CopyVector")==0)  manipulator = SPM_CopyVector;
	else if (!manipulator) manipulator = (void (*)(float*,float*))tokenfinder(token);
	else if (!manipulator) msg("Shader Error",buildstr("Unknown Shader Manipulator '%s'",token));
	sp->manipulator[*numparams] = manipulator;

	if (sep!='(') token = gettoken((char *)line, ofs, &sep);

	// Obtain Parameter
	float4 *offset = NULL;
	token = gettoken((char *)line, ofs, &sep);
	for (uintf tokennum = 0; tokennum<numGSVs; tokennum++)
	{	if (txticmp(token, ShaderVarNames[tokennum])==0)
		{	offset = (float4 *)&GlobalShaderVars + tokennum;
			break;
		}
	}
	if (!offset) offset = (float4 *)tokenfinder(token);
	if (!offset)
		msg("Shader Error",buildstr("Unknown Shader Parameter %s",token));
	sp->sourcedata[*numparams] = (float *)offset;
	token = gettoken((char *)line,ofs,&sep);
	stripquotes(token);
	txtcpy(sp->paramname[*numparams],sizeof(sp->paramname[0]),token);

	(*numparams)++;
	if (*numparams==16)
	{	*numparams = 0;
		sp->link = newShaderParameter();
	}
}
*/
/*
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

// ********************************************************************
// ****						Scripted Vertex Shader					***
// ********************************************************************
ScriptedVertexShader *findScriptedVertexShader(const char *name)
{	ScriptedVertexShader *vs;
	char buf[sizeof(vs->name)];
	txtcpy(buf, sizeof(buf), name);
	vs = usedScriptedVertexShader;
	while (vs)
	{	if (txticmp((char *)vs->name, buf)==0)	return vs;
		vs = vs->next;
	}
	return NULL;
}

ScriptedVertexShader *newScriptedVertexShader(const char *name)
{	ScriptedVertexShader *vs;
	if (findScriptedVertexShader(name))
	{	msg("newScriptedVertexShader Error",buildstr("A Scripted Vertex Shader with the name %s already exists",name));
	}
	allocbucket(ScriptedVertexShader, vs, flags, Generic_MemoryFlag32, 128, "Scripted Vertex Shaders");
	txtcpy((char *)vs->name, sizeof(vs->name), name);
	vs->BuildMatrix = StandardVSMatrixBuilder;
	vs->NumChannels = 0;
	vs->CreateFunc =  NULL;
	vs->CreateParameters[0] = 0;
	vs->CreateChannels = 0;
	vs->params = NULL;
	vs->script = NULL;
	return vs;
}

void deleteScriptedVertexShader(ScriptedVertexShader *vs)
{	deletebucket(ScriptedVertexShader, vs);
}

void deleteScriptedVertexShader(const char *name)
{	ScriptedVertexShader *vs = findScriptedVertexShader(name);
	if (!vs)
	{	msg("deleteScriptedVertexShader Error",buildstr("Could not find a Scripted Vertex Shader with the name %s",name));
	}
	deletebucket(ScriptedVertexShader, vs);
}
*/
/*
// ********************************************************************
// ****						Scripted Pixel Shader					***
// ********************************************************************
ScriptedPixelShader *findScriptedPixelShader(const char *name)
{	ScriptedPixelShader *ps;
	char buf[sizeof(ps->name)];
	txtcpy(buf, sizeof(buf), name);
	ps = usedScriptedPixelShader;
	while (ps)
	{	if (txticmp((char *)ps->name, buf)==0)	return ps;
		ps = ps->next;
	}
	return NULL;
}

ScriptedPixelShader *newScriptedPixelShader(const char *name)
{	ScriptedPixelShader *ps;
	if (findScriptedPixelShader(name))
	{	msg("newScriptedPixelShader Error",buildstr("A Scripted Pixel Shader with the name %s already exists",name));
	}
	allocbucket(ScriptedPixelShader, ps, flags, Generic_MemoryFlag32, 128, "Scripted Pixel Shaders");
	txtcpy((char *)ps->name, sizeof(ps->name), name);
	ps->params = NULL;
	ps->script = NULL;
	return ps;
}

void deleteScriptedPixelShader(ScriptedPixelShader *vs)
{	deletebucket(ScriptedPixelShader, vs);
}

void deleteScriptedPixelShader(const char *name)
{	ScriptedPixelShader *ps = findScriptedPixelShader(name);
	if (!ps)
	{	msg("deleteScriptedPixelShader Error",buildstr("Could not find a Scripted Pixel Shader with the name %s",name));
	}
	deletebucket(ScriptedPixelShader, ps);
}
*/
/*
// ********************************************************************
// ***						Material Override                       ***
// ********************************************************************
MtlOverride *findMtlOverride(const char *name)
{	MtlOverride *ps;
	char buf[sizeof(ps->name)];
	txtcpy(buf, name, sizeof(buf));
	ps = usedMtlOverride;
	while (ps)
	{	if (txticmp((char *)ps->name, buf)==0)	return ps;
		ps = ps->next;
	}
	return NULL;
}

MtlOverride *newMtlOverride(const char *name)
{	MtlOverride *ps;
	if (findMtlOverride(name))
	{	msg("newMtlOverride Error",buildstr("A Material Override with the name %s already exists",name));
	}
	allocbucket(MtlOverride, ps, flags, Generic_MemoryFlag32, 128, "Material Override");
	txtcpy((char *)ps->name, name, sizeof(ps->name));
	ps->ps = NULL;
	ps->vs = NULL;
	return ps;
}

void deleteMtlOverride(MtlOverride *vs)
{	deletebucket(MtlOverride, vs);
}

void deleteMtlOverride(const char *name)
{	MtlOverride *ps = findMtlOverride(name);
	if (!ps)
	{	msg("deleteMtlOverride Error",buildstr("Could not find a Material Override with the name %s",name));
	}
	deletebucket(MtlOverride, ps);
}
*/
/*
// ********************************************************************
// ****						Fixed/Scripted Shader					***
// ********************************************************************
FixedVertexShader *newFixedVertexShader(void)
{	FixedVertexShader *vs;
	allocbucket(FixedVertexShader, vs, flags, Generic_MemoryFlag8, 256, "Fixed Vertex Shader Cache");
	return vs;
}

FixedPixelShader *newFixedPixelShader(void)
{	FixedPixelShader *ps;
	allocbucket(FixedPixelShader, ps, flags, Generic_MemoryFlag32, 256, "Fixed Pixel Shader Cache");
	return ps;
}

void deleteFixedPixelShader(FixedPixelShader *ps)
{	deletebucket(FixedPixelShader, ps);
}

void deleteFixedVertexShader(FixedVertexShader *vs)
{	deletebucket(FixedVertexShader, vs);
}
*/
/*
// ***********************************************************************
// ***                         Shader Library                          ***
// ***********************************************************************
uintf autoSPgen = 0;
char shaderscript[8096];
char asmname[10];

class linkedtxtfile
{	private:
		uintf depth;
		textfile *txt[64];
		char	 *filename[64][256];
	public:
		linkedtxtfile(const char *filename);
		~linkedtxtfile(void);
		char *readln(void);
		char *getFileName(void);
		uintf getLineNum(void);
};

linkedtxtfile::linkedtxtfile(const char *flname)
{	depth = 0;
	txt[depth] = newtextfile(flname);
	txtcpy((char *)filename[depth],sizeof(filename[0]),flname);
}

linkedtxtfile::~linkedtxtfile(void)
{	while (depth)
		delete txt[depth--];
	delete txt[0];
}

char *linkedtxtfile::readln(void)
{	return txt[depth]->readln();
}

char *linkedtxtfile::getFileName(void)
{	return (char *)filename[depth];
}

uintf linkedtxtfile::getLineNum(void)
{	return txt[depth]->linenum;
}

void SLError(const char *errormsg, const char *regarding, linkedtxtfile *txt)
{	msg("Shader Library Error",buildstr("%s regarding [%s] in line %i of %s",errormsg,regarding,txt->getLineNum(),txt->getFileName()));
}

void *NULLtokenfinder(const char *token)
{	return NULL;
}

intf getInputTypeSize(uintf token)
{	switch(token)
	{	case ShaderInput_float1: return 1;
		case ShaderInput_float2: return 2;
		case ShaderInput_float3: return 3;
		case ShaderInput_float4: return 4;
		case ShaderInput_color32:return 5;
		default: msg("Function out of date","The FC Engine Function 'getInputTypeSize' needs to be updated");
	}
	return -1;
}

intf findSIoffset(ScriptedVertexShader *vs, const char *token)
{	uintf i=0;
	uintf result = 0;
	while (i<vs->NumChannels)
	{	if (txticmp(vs->InputName[i],token)==0)	return result;
		result += getInputTypeSize(vs->InputType[i]);
		i++;
	}
	msg("Unknown Identifier",buildstr("Cannot find Input Parameter %s in Vertex Shader %s",token,vs->name));
	return -1;
}

intf findSIoffset(ScriptedVertexShader *vs, uintf tokennum)
{	intf result = 0;
	for (uintf i=0; i<tokennum; i++)
		result += getInputTypeSize(vs->InputType[i]);
	return result;
}

intf getSIFloatSize(ScriptedVertexShader *vs)
{	uintf i=0;
	intf result = 0;
	while (i<vs->NumChannels)
	{	result += getInputTypeSize(vs->InputType[i++]);
	}
	return result;
}

void GenerateTangentBinormals(obj3d *obj)				// ### TODO: Fix this function
{//	Material *mtl = obj->material;
//	ScriptedVertexShader *vs = (ScriptedVertexShader *)mtl->VertexShader;
//	char *line = vs->CreateParameters;
//	intf ofs = 0;
//	char sep;
//	char *token = gettoken(line,&ofs,&sep);
//	stripquotes(token);
//	intf normaloffset = findSIoffset(vs,token);
//	token = gettoken(line,&ofs,&sep);
//	stripquotes(token);
//	intf UVoffset = findSIoffset(vs,token);
//	intf TanOffset = findSIoffset(vs, vs->NumChannels-2);
//	intf BinOffset = TanOffset+3;
//	intf floatsize = getSIFloatSize(vs);
	calcTangentBinormals(obj, 1, 2);
}

char *converttoken(const char *token, ScriptedVertexShader *vs, ShaderParameter *sp)
{	uintf index=0;
	bool found = false;
	// Test against entries in ShaderInput table
	if (vs)
	while (index<vs->NumChannels)
	{	if (txticmp(token,vs->InputName[index])==0)
		{	// Match found replace with input vector number
			asmname[0]='v';
			asmname[1]='0'+(char)index;
			asmname[2]=0;
			token = (char *)&asmname;
			found = true;
			break;
		}
		index++;
	}

	if (!found)
	{	if (vs) index = 4;
		else	index = 0;
		if (sp)
		{	ShaderParameter *_sp = sp;
			while (_sp)
			{	bool found = false;
				uintf spi = 0;
				while (_sp->paramname[spi][0])
				{	if (txticmp(_sp->paramname[spi],token)==0)
					{	// Match found, replace with constant number
						tprintf(asmname, sizeof(asmname), "c%i",index);
						token = asmname;
						found = true;
						break;
					}
					index++;
					spi++;
				}
				if (found) break;
				_sp = _sp->link;
			}
		}
	}

	if (!found)
	{	// Final Step - Check for FC Specific constants
		if (!vs)
		{	// Pixel Shader
			if (txticmp(token,"Diffuse")==0)			token = "v0";
			else if (txticmp(token,"Specular")==0)		token = "v1";
		}	else
		{	// Vertex Shader
			if (txticmp(token,"RenderMatrix")==0)		token = "c0";
			else if (txticmp(token,"outDiffuse")==0)	token = "oD0";
			else if (txticmp(token,"outSpecular")==0)	token = "oD1";
			else if (txticmp(token,"outPosition")==0)	token = "oPos";
			else if (txticmp(token,"outTexCoord0")==0)	token = "oT0";
			else if (txticmp(token,"outTexCoord1")==0)	token = "oT1";
			else if (txticmp(token,"outTexCoord2")==0)	token = "oT2";
			else if (txticmp(token,"outTexCoord3")==0)	token = "oT3";
			else if (txticmp(token,"outFog")==0)		token = "oFog";
			else if (txticmp(token,"outPointSize")==0)  token = "oPts";
		}
	}
	return (char *)token;
}

char *readShaderScript(linkedtxtfile *txt, ScriptedVertexShader *vs, ShaderParameter *sp)
{	char *line,*token;
	uintf scriptofs = 0;
	uintf linelen;
	char tokenised[256];
	intf ofs;
	char sep;

	while (true)
	{	line = txt->readln();
		while (line[0]==' ' || line[0]==9) line++;
		if (txticmp(line,"End")==0) break;
		uintf i=0;
		while (line[i]!=0 && line[i]!='#') i++;
		while ((i>0) && (line[i-1]==' ' || line[i-1]==9)) i--;
		line[i]=0;
		if (txtlen(line))
		{	// Process the tokens to replace variable/constant names with assembly opcodes
			tokenised[0]=0;
			ofs = 0;
			linelen = 0;
			while (true)
			{	token = gettokencustom(line,&ofs,&sep,", .()=[]*+/-\n\r\t_"); // Only need custom data for _BX2 override
				if (!token) break;
				//if (!token[0]) break;

				token = converttoken(token,vs,sp);
				if (sep=='_')						// Handle D3D '_BX2' override
				{	char tmp[256];
					txtcpy(tmp,256,token);
					token = gettoken(line,&ofs,&sep);
					txtcat(tmp,sizeof(tmp),"_");
					txtcat(tmp,sizeof(tmp),token);
					txtcpy(token,sizeof(tmp),tmp);
				}

				txtcpy(tokenised+linelen,sizeof(tokenised)-linelen,token);
				linelen += txtlen(token);
				if (sep)
				{	tokenised[linelen++]=sep;
				}	else
				{	break;
				}
			}
			tokenised[linelen]=0;
			if ((scriptofs + linelen +1 ) > sizeof(shaderscript))
				SLError("Shader Script Too Long","StartScript",txt);
			txtcpy(shaderscript+scriptofs,sizeof(shaderscript)-scriptofs, tokenised);
			scriptofs+=linelen;
			txtcpy(shaderscript+scriptofs,sizeof(shaderscript)-scriptofs,"\n");
			scriptofs+=1;
		}
	}
	return shaderscript;
}

void LoadShaderLibrary(const char *filename, void *(*tokenfinder)(const char *))
{	if (!tokenfinder) tokenfinder = NULLtokenfinder;
	linkedtxtfile *txt = new linkedtxtfile(filename);
	char *line = txt->readln();
	char *token;
	char sep;
	intf ofs;

	while (line)
	{	ofs = 0;

		// Skip blank lines
		skipblanks(line, &ofs);
		if (txtlen(line)==0)
		{	line = txt->readln();
			continue;
		}

		token = gettoken(line,&ofs,&sep);
		bool handled = false;

		// --- Vertex Shader ---
		if (txticmp(token, "VertexShader")==0)
		{	handled = true;
			token = gettoken(line,&ofs,&sep);
			stripquotes(token);
			ScriptedVertexShader *vs = newScriptedVertexShader(token);
			ShaderParameter *sp = NULL;
			uintf numparams = 0;

			while (true)
			{	line = txt->readln();
				if (!line)
					SLError("Unexpected End of File","VertexShader block",txt);
				ofs = 0;
				token = gettoken(line,&ofs,&sep);
				if (txticmp(token,"StartScript")==0) break;

				if (txticmp(token,"Share")==0)
				{	if (!sp)
					{	sp = newShaderParameter();
						vs->params = sp;
					}
					addShaderParam(sp, &numparams, line, &ofs, tokenfinder);
					if (sp->link) sp = sp->link;
				}

				if (txticmp(token,"CreateFunc")==0)
				{	handled = true;
					token = gettoken(line,&ofs,&sep);
					if (txticmp(token,"GenerateTangentBinormals")==0) vs->CreateFunc = GenerateTangentBinormals;
					else vs->CreateFunc = (void (*)(obj3d *))tokenfinder(token);
					if (!vs->CreateFunc) SLError("Unknown CreateFunction",token,txt);
					while (sep!='(')
						token = gettoken(line,&ofs,&sep);
					token = gettoken(line,&ofs,&sep);
					stripquotes(token);
					txtcpy(vs->CreateParameters, token, 256);
					while (sep!=',') token = gettoken(line,&ofs,&sep);
					vs->CreateChannels = (byte)getuinttoken(line,&ofs,&sep);
				}

				if (txticmp(token,"Input")==0)
				{	handled = true;
					token = gettoken(line,&ofs,&sep);
					if (txticmp(token,"float1")==0) vs->InputType[vs->NumChannels]=ShaderInput_float1;
					else if (txticmp(token,"float1")==0) vs->InputType[vs->NumChannels]=ShaderInput_float1;
					else if (txticmp(token,"float2")==0) vs->InputType[vs->NumChannels]=ShaderInput_float2;
					else if (txticmp(token,"float3")==0) vs->InputType[vs->NumChannels]=ShaderInput_float3;
					else if (txticmp(token,"float4")==0) vs->InputType[vs->NumChannels]=ShaderInput_float4;
					else if (txticmp(token,"dword")==0) vs->InputType[vs->NumChannels]=ShaderInput_color32;
					token = gettoken(line,&ofs,&sep);
					txtcpy(vs->InputName[vs->NumChannels++], token, 16);
				}
			}

			vs->script = readShaderScript(txt,vs,vs->params);
			compileVertexShader(vs);
		}

		// --- Pixel Shader ---
		if (txticmp(token, "PixelShader")==0)
		{	handled = true;
			token = gettoken(line,&ofs,&sep);
			stripquotes(token);
			ScriptedPixelShader *ps = newScriptedPixelShader(token);
			ShaderParameter *sp = NULL;
			uintf numparams = 0;

			while (true)
			{	line = txt->readln();
				if (!line)
					SLError("Unexpected End of File","PixelShader block",txt);
				ofs = 0;
				token = gettoken(line,&ofs,&sep);
				if (txticmp(token,"StartScript")==0) break;

				if (txticmp(token,"Share")==0)
				{	if (!sp)
					{	sp = newShaderParameter();
						ps->params = sp;
					}
					addShaderParam(sp, &numparams, line, &ofs, tokenfinder);
					if (sp->link) sp = sp->link;
				}
			}

			ps->script = readShaderScript(txt,NULL,ps->params);
			compilePixelShader(ps);
		}

		// --- Material Override  ---
		if (txticmp(token, "MaterialOverride")==0)
		{	handled = true;
			token = gettoken(line,&ofs,&sep);
			stripquotes(token);
			MtlOverride *mo = newMtlOverride(token);
			while (true)
			{	line = txt->readln();
				if (!line)
					SLError("Unexpected End of File","MtlOverride block",txt);
				ofs = 0;
				token = gettoken(line,&ofs,&sep);
				if (txticmp(token,"End")==0) break;
				if (txticmp(token,"PixelShader")==0)
				{	if (sep!='=') gettoken(line,&ofs,&sep);
					if (sep!='=') SLError("Syntax Error",token,txt);
					token = gettoken(line,&ofs,&sep);
					stripquotes(token);
					mo->ps = findScriptedPixelShader(token);
					continue;
				}
				if (txticmp(token,"VertexShader")==0)
				{	if (sep!='=') gettoken(line,&ofs,&sep);
					if (sep!='=') SLError("Syntax Error",token,txt);
					token = gettoken(line,&ofs,&sep);
					stripquotes(token);
					mo->vs = findScriptedVertexShader(token);
					continue;
				}
				SLError("Syntax Error",token,txt);
			}
		}

		if (!handled) SLError("Unknown Block Type",token,txt);
		line = txt->readln();
	}
	delete txt;
}
*/
