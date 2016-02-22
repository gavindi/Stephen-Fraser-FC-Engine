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
/*------------------------------  W A R N I N G  -----------------------------------*/
/*  This source file is only for the use of Stephen Fraser.  Any other use without	*/
/* my express permission may result in legal action being taken!					*/
/************************************************************************************/

#include "fc_h.h"
#include "bucketsystem.h"

Material			*usedMaterial			= NULL, *freeMaterial			= NULL;
Material *NULLmtl = NULL;

class fcmaterial : public fc_material
{	public:
	Material *newmaterial(const char *name)							{return ::newmaterial(name);};
	void	_deletematerial(Material *mtl)							{		::_deletematerial(mtl);};
} *OEMmaterial = NULL;

fc_material *initmaterial(void)
{	OEMmaterial = new fcmaterial;
	NULLmtl = newmaterial("Null Material");
	NULLmtl->minalpha = 1;
	NULLmtl->finalBlendOp = finalBlend_add;
	NULLmtl->finalBlendSrcScale = blendParam_One;
	NULLmtl->finalBlendDstScale = blendParam_Zero;
	return OEMmaterial;
}

void killmaterial(void)
{	if (NULLmtl) deletematerial(NULLmtl);
	if (OEMmaterial) delete OEMmaterial;
	killbucket(Material, reserved2, Generic_MemoryFlag8);
}

Material *newmaterial(const char *name)	// Allocate a material
{	uintf i;
	Material *mtl;
	allocbucket(Material, mtl, reserved2, Generic_MemoryFlag8, 256, "Material Cache");
	mtl->diffuse.a = 1.00f;	mtl->diffuse.r = 1.00f;	mtl->diffuse.g = 1.00f;	mtl->diffuse.b = 1.00f;
	mtl->shininess = 4.0f; mtl->shininessStr = 1.0f;
	// mtl->bias = 0;
	txtcpy((char *)mtl->name, sizeof(mtl->name), name);
	mtl->finalBlendOp = finalBlend_add;
	mtl->finalBlendSrcScale = blendParam_SrcAlpha;
	mtl->finalBlendDstScale = blendParam_InvSrcAlpha;
	mtl->minalpha = 1;
	mtl->parent = NULL;
	for (i=0; i<maxtexchannels; i++)
	{	mtl->texture[i]=NULL;
	}
	return mtl;
}

void _deletematerial(Material *mtl)
{	if (!mtl) return;

	for (long i=0; i<8; i++)
		if (mtl->texture[i]) deletetexture(mtl->texture[i]);

	deletebucket(Material, mtl);
}

const char *mtlBlendParam[]={"Zero","One","SrcCol","InvSrcCol","SrcAlpha","InvSrcAlpha","DstAlpha","InvDstAlpha","DstCol","InvDstCol","AlphaSaturation"};
const char *mtlBlendOp[]={"add","subtract","revsubtract","min","max"};

#define shaderFeature(condition,text) \
	if (!condition) \
	{	txtcat(shaderSource,sizeof(shaderSource),text); \
		condition = true; \
	}

// ------------------------------------------------------ Material Translation ------------------------------------------------------------
int buildStandardShaders(sFileHandle *handle, const char *filename, sImportMtl *mtl)
{	uintf i;
	char shaderSource[2048];
	char tmp[2048];

	txtcpy(shaderSource, sizeof(shaderSource),
		"const float lightRadius = 5.0;"
		"varying vec3 lightDir;"
		"varying vec3 spotDir;"
		"varying vec3 viewDir;"

		"void main()"
		"{	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
		"	gl_TexCoord[0] = gl_MultiTexCoord0;"
		"	vec3 normal = normalize(gl_NormalMatrix * gl_Normal);"
		"	vec3 vertexEye = vec3(gl_ModelViewMatrix * gl_Vertex);"
		"	lightDir = (gl_LightSource[0].position.xyz - vertexEye);"

		"	vec3 tangent = normalize(gl_NormalMatrix * gl_Color.xyz);"
		"	vec3 binormal = cross(normal, tangent) * gl_Color.w;"
	
		"	mat3 tbnMatrix = mat3(	tangent.x, binormal.x, normal.x, "
		"							tangent.y, binormal.y, normal.y, "
		"							tangent.z, binormal.z, normal.z);"
    
		"	lightDir = tbnMatrix * (lightDir  / 10.0);"
		"	spotDir = tbnMatrix * gl_LightSource[0].spotDirection;"
		"	viewDir = tbnMatrix * (-vertexEye);"
		"}");

/*	// Check what features we are using so we know what values we will need
	bool featureSet[32];
	for (i=0; i<32; i++)
	{	featureSet[i]=false;
	}
	for (i=0; i<mtl->numTextures; i++)
	{	byte slotType = mtl->slotType[i];
		if (slotType<32)
			featureSet[slotType] = true;
	}

	// Declare any varying variables (ones sent from the vertex shader to the fragment shader
//	if (featureSet[slotType_Bump])
//		txtcpy(shaderSource,sizeof(shaderSource),"varying vec3 lightDir, spotDir, viewDir;\r\n");
//	else
		txtcpy(shaderSource,sizeof(shaderSource),"varying vec3 normal, lightDir, eyeVec;\r\n");
//	txtcat(shaderSource,sizeof(shaderSource),
//		"in vec4 attribPosition;\r\n"
//		);

	bool haveViewVertVector = false;
	bool haveNormalReflection = false;
	bool haveEnvMapScale = false;
	bool haveTangent = false;
	bool haveBinormal = false;

	txtcat(shaderSource,sizeof(shaderSource),"void main(void)\r\n{");

//	if (featureSet[slotType_Bump])
//		txtcpy(shaderSource,sizeof(shaderSource),"	vec3 normal, eyeVec;\r\n");

	txtcat(shaderSource,sizeof(shaderSource),
		"	normal = gl_NormalMatrix * gl_Normal;\r\n"			// must have eyeSpaceNormal no matter what!
		"	vec3 vertexEye = vec3(gl_ModelViewMatrix * gl_Vertex);\r\n" //gl_Vertex);\r\n"
		"	lightDir = vec3(gl_LightSource[0].position.xyz - vertexEye);\r\n"
		"	eyeVec = -vertexEye;\r\n");

/*    
    vec3 tangent = normalize(attribNormalMatrix * gl_MultiTexCoord1.xyz);
    vec3 binormal = cross(normal, tangent) * gl_MultiTexCoord1.w;

	mat3 tbnMatrix = mat3(tangent.x, binormal.x, normal.x,
                          tangent.y, binormal.y, normal.y,
                          tangent.z, binormal.z, normal.z);
    
    lightDir = tbnMatrix * (lightDir / lightRadius);

    spotDir = gl_LightSource[0].spotDirection;
    spotDir = tbnMatrix * spotDir;

    viewDir = -vertexEye;
    viewDir = tbnMatrix * viewDir;
* /

	
	for (i=0; i<mtl->numTextures; i++)
	{	Texture *tex = mtl->texture[i];
		if (mtl->mapping[i] == 1)
		{	// This texture is EnvSphere Mapped
			shaderFeature(haveViewVertVector,	"	vec3 viewVertVector = normalize( vec3(gl_ModelViewMatrix * gl_Vertex) );\r\n");
			shaderFeature(haveNormalReflection,	"	vec3 normalReflection = reflect( viewVertVector, normal )+vec3(0,0,1);\r\n");
			shaderFeature(haveEnvMapScale,		"	float EnvMapScale = 2.0 * sqrt(dot( normalReflection, normalReflection));\r\n");			//sqrt( normalReflection.x*normalReflection.x + normalReflection.y*normalReflection.y + (normalReflection.z+1.0)*(normalReflection.z+1.0) );\r\n");
			tprintf(tmp,sizeof(tmp),
				"\tgl_TexCoord[%i].x = normalReflection.x/EnvMapScale + 0.5;\r\n"
				"\tgl_TexCoord[%i].y = -(normalReflection.y/EnvMapScale + 0.5);\r\n",i,i);
			txtcat(shaderSource,sizeof(shaderSource),tmp);
		}	else
		{	// Standard UV mapped texture
			tprintf(tmp,sizeof(tmp),"\tgl_TexCoord[%i].xy = gl_MultiTexCoord%i.xy;\r\n",i,mtl->channel[i]);
			txtcat(shaderSource,sizeof(shaderSource),tmp);
		}
	}
	txtcat(shaderSource,sizeof(shaderSource),"\tgl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\r\n}\r\n");
*/
	int vshader = compileVertexShader(0,shaderSource);
	
	if (handle)
	{	tprintf(tmp,sizeof(tmp),"VertexShader: %s::v_%s\r\n",filename,mtl->name);
		fileWrite(handle,tmp, txtlen(tmp));
		fileWrite(handle,shaderSource, txtlen(shaderSource));
	}

	// ---------------------------------------------------- Fragment Shader ----------------------------------------------------
	txtcpy(shaderSource, sizeof(shaderSource),
		//"#define USE_INNER_AND_OUTER_CONES\r\n"

		"uniform sampler2D tex0;\r\n"
		"uniform sampler2D tex1;\r\n"

		"#if defined(USE_INNER_AND_OUTER_CONES)\r\n"
		"	uniform float cosOuterCone;\r\n"
		"	uniform float cosInnerCone;\r\n"
		"#endif\r\n"

		"varying vec3 lightDir;\r\n"
		"varying vec3 spotDir;\r\n"
		"varying vec3 viewDir;\r\n"
//"varying vec4 vtxTangent;\r\n"

		"void main()\r\n"
		"{	vec3 l = lightDir;\r\n"
		"	float atten = 1.0;" //max(0.0, 1.0 - dot(l, l));"

		"	l = normalize(l);\r\n"
		"	float spotDot = dot(-l, normalize(spotDir));\r\n"

		"#if defined(USE_INNER_AND_OUTER_CONES)\r\n"
		"	float spotEffect = smoothstep(cosOuterCone, cosInnerCone, spotDot);\r\n"
		"#else\r\n"
		"	float spotEffect = (spotDot < gl_LightSource[0].spotCosCutoff) ? 0.0 : pow(spotDot, gl_LightSource[0].spotExponent);\r\n"
		"#endif\r\n"

		"	atten *= spotEffect;"
		"	vec3 n = normalize(texture2D(tex0, gl_TexCoord[0].st).xyz * 2.0 - 1.0);"
		"	vec3 v = normalize(viewDir);"
		"	vec3 h = normalize(l + v);"

		"	float nDotL = max(0.0, dot(n, l));"
		"	float nDotH = max(0.0, dot(n, h));"
		"	float power = (nDotL == 0.0) ? 0.0 : pow(nDotH, gl_FrontMaterial.shininess);"

//		"	vec4 ambient = gl_FrontLightProduct[0].ambient;"
	    "	vec3 diffuse = gl_LightSource[0].diffuse.rgb * nDotL * atten;"
		"	vec3 specular = gl_LightSource[0].specular.rgb * power * atten;"
		"	vec3 color = diffuse + specular;"

//		"	gl_FragColor.rgb = vtxTangent.rgb;\r\n" 
		"	gl_FragColor.rgb = color;\r\n"
//		"	gl_FragColor.rgb = color.rgb;\r\n" // * texture2D(tex1, gl_TexCoord[0].st);"
		"	gl_FragColor.a = 1.0;\r\n"
		"}");

/*	shaderSource[0]=0;
	// Declare the texture maps
	for (i=0; i<mtl->numTextures; i++)
	{	tprintf(tmp,sizeof(tmp),"uniform sampler2D tex%i;\r\n",i);
		txtcat(shaderSource,sizeof(shaderSource),tmp);
	}
/*	// Direction Lighting
	txtcat(shaderSource,sizeof(shaderSource),
		"varying vec3 normal,lightDir,eyeVec;\r\n"
		"void main(void)\r\n{"
		"\tvec4 col;\r\n"
		"\tvec3 lighting = vec3(0,0,0);\r\n" 
		"\tvec3 specular = vec3(0,0,0);\r\n"
		"\tvec3 ambient = gl_LightModel.ambient.rgb * gl_FrontMaterial.ambient.rgb;\r\n"
		"\tvec3 N = normalize(normal.rgb);\r\n"
		"\tvec3 L = normalize(lightDir.rgb);\r\n"
		"\tfloat lambertTerm = dot(N,L);\r\n"
		"\tif(lambertTerm > 0.0)\r\n"
		"\t{\tlighting += gl_LightSource[0].diffuse.rgb * lambertTerm;\r\n"
		"\t\tvec3 E = normalize(eyeVec);\r\n"
		"\t\tvec3 R = reflect(-L, N);\r\n"
		"\t\tspecular += gl_LightSource[0].specular.rgb * pow( max(dot(R, E), 0.0), gl_FrontMaterial.shininess ) * gl_FrontMaterial.specular.r;\r\n"
		"\t}\r\n"
		"\tcol.rgb = lighting * gl_FrontMaterial.diffuse.rgb;\r\n"
		"\tcol.a = gl_FrontMaterial.diffuse.a;\r\n");
* /
/*	// Point Lighting
	txtcat(shaderSource,sizeof(shaderSource),
		"varying vec3 normal,lightDir,eyeVec;\r\n"
		"void main(void)\r\n{"
		"	vec3 specular = vec3(0,0,0);\r\n"
		"	vec4 lightDiffuse = vec4(0,0,0,1);\r\n"
		"	vec3 ambient = gl_LightModel.ambient.rgb * gl_FrontMaterial.ambient.rgb;\r\n"
		"	vec3 L = normalize(lightDir);\r\n"
		"	vec3 D = normalize(gl_LightSource[0].spotDirection);\r\n"
		"	if (dot(-L, D) > gl_LightSource[0].spotCosCutoff)\r\n" 
		"	{	vec3 N = normalize(normal);\r\n"
		"		float lambertTerm = max( dot(N,L), 0.0);\r\n"
		"		if(lambertTerm > 0.0)\r\n"
		"		{	lightDiffuse.rgb += gl_LightSource[0].diffuse.rgb * lambertTerm;\r\n"
		"			vec3 E = normalize(eyeVec);\r\n"
		"			vec3 R = reflect(-L, N);\r\n"
		"			specular += gl_LightSource[0].specular.rgb * pow( max(dot(R, E), 0.0), gl_FrontMaterial.shininess );\r\n"
		"		}\r\n"
		"	}\r\n"
		"lightDiffuse.a = gl_FrontMaterial.ambient.a;\r\n"
"vec3 lighting = lightDiffuse.rgb;\r\n"
"vec4 col = lightDiffuse * gl_FrontMaterial.diffuse;\r\n"); 
* /
	// Spot Lighting
	txtcat(shaderSource,sizeof(shaderSource),
"varying vec3 normal, lightDir, eyeVec;\r\n"
"const float cos_outer_cone_angle = 0.707;\r\n" // 36 degrees

"vec4 lightSpecular, lightDiffuse;\r\n"
"vec3 lightEmissive,lightAmbient, lighting;\r\n"
"void eyeSpaceLighting(void)\r\n"
"{	lightSpecular = vec4(0,0,0,gl_FrontMaterial.specular.r);\r\n"
"	lightDiffuse = vec4(0,0,0,1);\r\n"
"	lightEmissive = vec3(0,0,0);\r\n"
"	lightAmbient = gl_LightModel.ambient.rgb * gl_FrontMaterial.ambient.rgb;\r\n"
"	vec3 L = normalize(lightDir);\r\n"
"	vec3 D = normalize(gl_LightSource[0].spotDirection);\r\n"
"	float cos_cur_angle = dot(-L, D);\r\n"
"	float cos_inner_cone_angle = gl_LightSource[0].spotCosCutoff;\r\n"
"	float cos_inner_minus_outer_angle = cos_inner_cone_angle - cos_outer_cone_angle;\r\n"
"	float spot = clamp((cos_cur_angle - cos_outer_cone_angle) / cos_inner_minus_outer_angle, 0.0, 1.0);\r\n"
"	vec3 N = normalize(normal);\r\n"
"	float lambertTerm = dot(N,L);\r\n"
"	if(lambertTerm > 0.0)\r\n"
"	{	lightDiffuse.rgb += gl_LightSource[0].diffuse.rgb * lambertTerm * spot;\r\n"
"		vec3 E = normalize(eyeVec);\r\n"
"		vec3 R = reflect(-L, N);\r\n"
"		lightSpecular.rgb += gl_LightSource[0].specular.rgb * pow( max(dot(R, E), 0.0), gl_FrontMaterial.shininess ) * spot;\r\n"
"	}\r\n"
"	lighting = lightDiffuse.rgb;\r\n"
"	lightDiffuse *= gl_FrontMaterial.diffuse;\r\n"
"}\r\n\r\n"

"void main (void)\r\n"
"{	eyeSpaceLighting();\r\n"
);

	for (i=0; i<mtl->numTextures; i++)
	{	int texAmount = mtl->texAmount[i];
		switch(mtl->slotType[i])
		{	case slotType_Ambient:
			{	if (texAmount==100)
					tprintf(tmp,sizeof(tmp),"\tlightAmbient = gl_FrontMaterial.ambient.rgb * texture2D(tex%i, gl_TexCoord[%i].xy).rgb;\r\n",i,i);
				else
					tprintf(tmp,sizeof(tmp),"\tlightAmbient = mix(lightAmbient, gl_FrontMaterial.ambient.rgb * texture2D(tex%i, gl_TexCoord[%i].xy).rgb, %f;\r\n",i,i,((double)texAmount)/100.0);
				txtcat(shaderSource,sizeof(shaderSource),tmp);
				break;
			}
			case slotType_Diffuse:
			{	if (texAmount==100)
					tprintf(tmp,sizeof(tmp),"\tlightDiffuse.rgb = lighting * texture2D(tex%i, gl_TexCoord[%i].xy).rgb;\r\n",i,i);
				else
					tprintf(tmp,sizeof(tmp),"\tlightDiffuse.rgb = mix(lightDiffuse.rgb, lighting * texture2D(tex%i, gl_TexCoord[%i].xy).rgb, %f;\r\n",i,i,((double)texAmount)/100.0);
				txtcat(shaderSource,sizeof(shaderSource),tmp);
				break;
			}
			case slotType_Specular:
			{	if (texAmount==100)
					tprintf(tmp,sizeof(tmp),"\tlightSpecular.rgb *= texture2D(tex%i, gl_TexCoord[%i].xy).rgb;\r\n",i,i);
				else
					tprintf(tmp,sizeof(tmp),"\tlightSpecular.rgb = mix(lightSpecular.rgb, texture2D(tex%i, gl_TexCoord[%i].xy).rgb, %f;\r\n",i,i,((double)texAmount)/100.0);
				txtcat(shaderSource,sizeof(shaderSource),tmp);
				break;
			}
			case slotType_SpecularLevel:
			{	if (texAmount==100)
					tprintf(tmp,sizeof(tmp),"\tlightSpecular.a = texture2D(tex%i, gl_TexCoord[%i].xy).a;\r\n",i,i);
				else
					tprintf(tmp,sizeof(tmp),"\tlightSpecular.a = mix(lightSpecular.a,texture2D(tex%i, gl_TexCoord[%i].xy).a, %f);\r\n",i,i,((double)texAmount)/100.0);
				txtcat(shaderSource,sizeof(shaderSource),tmp);
				break;
			}
			case slotType_Reflection:
			{	if (texAmount==100)
					tprintf(tmp,sizeof(tmp),"\tlightEmissive.rgb += texture2D(tex%i, gl_TexCoord[%i].xy).rgb * lighting;\r\n",i,i);
				else
					tprintf(tmp,sizeof(tmp),"\tlightEmissive.rgb += texture2D(tex%i, gl_TexCoord[%i].xy).rgb * lighting * %f;\r\n",i,i,((double)texAmount)/100.0);
				txtcat(shaderSource,sizeof(shaderSource),tmp);
				break;
			}
			case slotType_Emissive:
			{	if (texAmount==100)
					tprintf(tmp,sizeof(tmp),"\tlightEmissive.rgb = max(col.rgb,texture2D(tex%i, gl_TexCoord[%i].xy).rgb).rgb;\r\n",i,i);
				else
					tprintf(tmp,sizeof(tmp),"\tlightEmissive.rgb = max(col.rgb,texture2D(tex%i, gl_TexCoord[%i].xy).rgb * %f).rgb;\r\n",i,i,((double)texAmount)/100.0);
				txtcat(shaderSource,sizeof(shaderSource),tmp);
				break;
			}
			case slotType_Opacity:
			{	if (texAmount==100)
					tprintf(tmp,sizeof(tmp),"\tlightDiffuse.a = texture2D(tex%i, gl_TexCoord[%i].xy).a;\r\n",i,i);
				else
					tprintf(tmp,sizeof(tmp),"\tlightDiffuse.a = mix(lightDiffuse.a,texture2D(tex%i, gl_TexCoord[%i].xy).a, %f).a;\r\n",i,i,((double)texAmount)/100.0);
				txtcat(shaderSource,sizeof(shaderSource),tmp);
				break;
			}
			case slotType_DiffuseAndOpacity:
			{	if (texAmount==100)
					tprintf(tmp,sizeof(tmp),"\tlightDiffuse *= texture2D(tex%i, gl_TexCoord[%i].xy);\r\n",i,i);
				else
					tprintf(tmp,sizeof(tmp),"\tlightDiffuse = mix(lightDiffuse, texture2D(tex%i, gl_TexCoord[%i], %f);\r\n",i,i,((double)texAmount)/100.0);
				txtcat(shaderSource,sizeof(shaderSource),tmp);
				break;
			}

			// These slot types aren't handled yet, the best thing to do is ignore them for now
//			case slotType_Bump:
//			case slotType_Glossiness:
//			case slotType_FilterColor:
//			case slotType_Refraction:
//			case slotType_Displacement:
//				break;

			default:
				if (texAmount==100)
					tprintf(tmp,sizeof(tmp),"\tlightDiffuse.rgb *= texture2D(tex%i, gl_TexCoord[%i].xy).rgb;\r\n",i,i);
				else
					tprintf(tmp,sizeof(tmp),"\tlightDiffuse.rgb = mix(lightDiffuse.rgb, texture2D(tex%i, gl_TexCoord[%i].xy).rgb, %f);\r\n",i,i,((double)texAmount)/100.0);
				txtcat(shaderSource,sizeof(shaderSource),tmp);
				break;
		}
	}
	txtcat(shaderSource,sizeof(shaderSource),"\tgl_FragColor.rgb = max(lightDiffuse.rgb + lightSpecular.rgb*lightSpecular.a + lightEmissive, lightAmbient);\r\n");
	txtcat(shaderSource,sizeof(shaderSource),"\tgl_FragColor.a = lightDiffuse.a;\r\n}\r\n");
*/
	int fshader = compileFragmentShader(0,shaderSource);
	int shaderProgram = linkShaders(0,vshader, fshader,mtl->numTextures);

	if (handle)
	{	tprintf(tmp,sizeof(tmp),"FragmentShader: %s::f_%s\r\n",filename,mtl->name);
		fileWrite(handle,tmp, txtlen(tmp));
		fileWrite(handle,shaderSource, txtlen(shaderSource));

		tprintf(tmp,sizeof(tmp),"ShaderProgram: %s::shader_%s = %s::v_%s, %s::f_%s\r\n",filename,mtl->name,filename,mtl->name,filename,mtl->name );
		fileWrite(handle,tmp, txtlen(tmp));
	}
	return shaderProgram;
}
#undef shaderFeature

Material *translateMaterial(sImportMtl *importMtl, sFileHandle *genShaders, const char *className, uintf flags)
{	char genShad[256];
	if (genShaders)
	{	tprintf(genShad,sizeof(genShad),"\r\n\r\n#----------------------------------------------------------------------------- %s\r\n",importMtl->name);
		fileWrite(genShaders, genShad, txtlen(genShad));
	}

	Material *mtl = newmaterial(importMtl->name);
	mtl->flags		= importMtl->flags;
	mtl->minalpha	= importMtl->minalpha;	
	mtl->diffuse.a	= importMtl->diffuse.a;
	mtl->diffuse.r	= importMtl->diffuse.r;
	mtl->diffuse.g	= importMtl->diffuse.g;
	mtl->diffuse.b	= importMtl->diffuse.b;
	mtl->shininess	= importMtl->shininess;
	mtl->shininessStr	= importMtl->shininessStr;

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
