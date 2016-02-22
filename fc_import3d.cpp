/************************************************************************************/
/*									FC-Import3D										*/
/*	    							-----------										*/
/*  					   (c) 1998-2002 by Stephen Fraser							*/
/*																					*/
/* Contains 3D geometry importers.													*/
/*																					*/
/*	* OSF - Optimised Scene File - Native to the FC Engine							*/
/*	* MD2 - Quake 2 character Files - Not officially supported						*/
/*																					*/
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#define max3SFLights 8

extern logfile *pluginlog;

sImport3dCallbacks InternalImport3DCallbacks;

// ************************************************************************************
// ***								Import 3D Support Functions						***
// ************************************************************************************
bool import3DtrueSceneHeader(sImport3DHeader *hdr)
{	return true;
}
bool import3DtrueTextPoolPreLoad(void)
{	return true;
}
bool import3DtrueMaterialPreLoad(const char *mtlName)
{	return true;
}
bool import3DtrueMaterialPostLoad(sImportMtl *mtl)
{	return true;
}
bool import3DtrueObjectPreLoad(const char *objName, sImportObjHeader *hdr)
{	return true;
}
bool import3DtrueObjectPostLoad(newobj *obj, uint16 *faces, uintf numFaces, void *vertices, uintf numVerts, void *vsd)	// vsd should be sVertexStreamDescription but it is depricated
{	return true;
}

bool import3DfalseSceneHeader(sImport3DHeader *hdr)
{	return false;
}
bool import3DfalseTextPoolPreLoad(void)
{	return false;
}
bool import3DfalseMaterialPreLoad(const char *mtlName)
{	return false;
}
bool import3DfalseMaterialPostLoad(sImportMtl *mtl)
{	return false;
}
bool import3DfalseObjectPreLoad(const char *objName, sImportObjHeader *hdr)
{	return false;
}
bool import3DfalseObjectPostLoad(newobj *obj, uint16 *faces, uintf numFaces, void *vertices, uintf numVerts, void *vsd)	// vsd should be sVertexStreamDescription but it is depricated
{	return false;
}

sImport3dCallbacks import3DTrueCallbacks =
{	&import3DtrueSceneHeader, import3DtrueTextPoolPreLoad, &import3DtrueMaterialPreLoad, &import3DtrueMaterialPostLoad, &import3DtrueObjectPreLoad, &import3DtrueObjectPostLoad, NULL
};

sImport3dCallbacks import3DFalseCallbacks =
{	&import3DfalseSceneHeader, import3DfalseTextPoolPreLoad, &import3DfalseMaterialPreLoad, &import3DfalseMaterialPostLoad, &import3DfalseObjectPreLoad, &import3DfalseObjectPostLoad, NULL
};

void initImport3DCallbacks(sImport3dCallbacks *cb)
{	if (cb==NULL)
	{	memcopy(&InternalImport3DCallbacks, &import3DTrueCallbacks, sizeof(sImport3dCallbacks));
		return;
	}
	memcopy(&InternalImport3DCallbacks, cb, sizeof(sImport3dCallbacks));
	if (InternalImport3DCallbacks.cbSceneHeader == NULL)		InternalImport3DCallbacks.cbSceneHeader			= import3DtrueSceneHeader;
	if (InternalImport3DCallbacks.cbTextPoolPreLoad == NULL)	InternalImport3DCallbacks.cbTextPoolPreLoad		= import3DtrueTextPoolPreLoad;
	if (InternalImport3DCallbacks.cbMaterialPreLoad == NULL)	InternalImport3DCallbacks.cbMaterialPreLoad		= import3DtrueMaterialPreLoad;
	if (InternalImport3DCallbacks.cbMaterialPostLoad == NULL)	InternalImport3DCallbacks.cbMaterialPostLoad	= import3DtrueMaterialPostLoad;
	if (InternalImport3DCallbacks.cbObjectPreLoad == NULL)		InternalImport3DCallbacks.cbObjectPreLoad		= import3DtrueObjectPreLoad;
	if (InternalImport3DCallbacks.cbObjectPostLoad == NULL)		InternalImport3DCallbacks.cbObjectPostLoad		= import3DtrueObjectPostLoad;
}

// ************************************************************************************
// ***							Import 3SF (Replaces OSF)							***
// ************************************************************************************
#define maxHeaderSize 100

struct OSFobjhdr					// This is the header of OBJECTS within the OSF file, not the OSF File Header
{	uint32	nameofs;
	char	compression_[4];		// FourCC of mesh compression codec
	byte	channelsinobj;			// Number of texture channels in object
	byte	numbones;				// Number of bones in object
	byte	sceneStackIndex;		// Index into the scene's global matrix stack
	byte	reserved1;				// Reserved for future expansion (and to keep things aligned)
	uint32	wirecolor;				// Wireframe color
	int32	mtlnumber;				// Material Index number
	uint32	extraVertexData;		// A set of flags to indicate which additional vertex data elements are included
	uint32	numfaces;				// Number of faces in object
	uint32	numverts;				// Number of vertices in object
	uint32	numkeyframes;			// Number of keyframes of animation
	uint32	animsize;				// Size in bytes of raw animation data (format is specific to mesh compression codec)
	float3	pivot;					// co-ordinates of object's pivot point (relative to the group)
};

void fetch3SFBlock(byte **start, char *blockType, byte **ptrCopy, uintf *blockSize)
{	char tmp[4];
	memcopy(blockType, *start, 4);
	*start += 4;
	uint32 size = GetLittleEndianINT32(*start);
	*blockSize = (uintf)size;
	*start += 4;
	*ptrCopy = *start;
	*start += size;
	memcopy(tmp, *start, 4);
	if (txtncmp(tmp,"endb",4)!=0)
		msg("3SF Error",buildstr("The 3SF File is corrupt, Failed to correctly close a %s block",blockType));// called %s",OSFlastblock,lastblockname));
	*start += 4;
}

struct sImportMod
{	char	*objName;
	uintf	command;
};

const char *import3SFCommands[]={"","sortLowY","sortHighY"};
#define numImport3SFCommands 3

sScene3D *load3sfmem(const char *filename, sImport3dCallbacks *callbacks, byte *_3sf, intf bytesRemain, uintf import3DFlags)
{	msg("Incomplete Transition","load3sfmem needs to be updated to using OpenGL 3.2 style rendering");
	return NULL;
/*	sScene3D *scene;					// The scene we're building
	sImport3DHeader sceneHeader;

	logfile *log = newlogfile("3sf.txt");

	initImport3DCallbacks(callbacks);
	uintf memNeeded;
	uintf flags = 0;					// This is the group loading flags
	char blockType[5]={0,0,0,0,0};
	byte *block;
	uintf blockSize;
	bool haveHeader = false;
	uintf j;
	uintf	version;					// Version is Major*100 + Minor, so 4.01 would be 401

	uintf numobjs = 0;
	uintf numMtls = 0;
	uintf numLights = 0;

	// Declarations for variables added to the header on newer files
	uint32 maxAnimMem = 0;
	uint32 totalAnimMem = 0;

	// These variables are for scripted mods specified with the 3SF file
	sImportMod imports[1024];
	uintf numImports=0;
	void *importModMem = NULL;
	uintf importModMemSize = 0;

	bool allocedHeader = false;			// Indicates whether or not we allocated the header

	// The following variables are used as stand-ins for when you don't allocate something
//	void	*vertexImportBuffer=NULL;	// Temporary storage container to hold vertex arrays prior to them being sent to the video card
	void	*faceImportBuffer;			// Temporary storage container to hold face arrays prior to them being sent to the video card
	newobj	tmpObj;

	uint32	maxObjMem, maxObjFaces;

	char	*textPool = NULL;
	uintf	textPoolSize;
	bool	usingTempTextPool = false;	// If we haven't allocated a textpool, use the one in the raw input stream

	uintf num3SFmaterials,num3SFtextures,num3SFlights;
	sImportMtl importMtl;

	fileNameInfo(filename);
	char genShad[256];
	tprintf(genShad,sizeof(genShad),"%s/%s_generatedShaders.txt",filepath, fileactualname);
	char filename3SF[256];
	txtcpy(filename3SF,sizeof(filename3SF),fileactualname);
	sFileHandle *genShaders = fileCreate(genShad);

	byte *buf = _3sf;
	if (log) log->log("Loaded file %s (%i bytes)",filename,bytesRemain);

	while (bytesRemain>0)
	{	fetch3SFBlock(&buf, blockType, &block, &blockSize);
		bytesRemain -= (blockSize+12);
		if (txtncmp(blockType,"HDR:",4)==0)
		{	if (blockSize>maxHeaderSize) msg("3SF Error",buildstr("Error loading file: %s\r\nThe header is unexpectedly long",filename));
			if (log) log->log("Received HEADER block %s (%i bytes,  %i Remain)",blockType,blockSize,bytesRemain);
			version	= block[0]*256+block[1]; block+=2;												//  2
			if (version<0x401) msg("Old 3SF File",buildstr("Expecting version 402, received version %x in %s",version,filename));
			byte	mtxStackSize	= *block++;			// Number of elements in the matrix stack	//  3
			block++;									// reserved1;								//  4
			float	horizon			= GetLittleEndianFLOAT32(block);	block+=4;					//  8
			uint32  ambientLight	= GetLittleEndianUINT32(block);		block+=4;					// 12
			uint32	backgroundColor	= GetLittleEndianUINT32(block);		block+=4;					// 16
			uint32	fogColor		= GetLittleEndianUINT32(block);		block+=4;					// 20
			float	fogstart		= GetLittleEndianFLOAT32(block);	block+=4;					// 24
			float	fogend			= GetLittleEndianFLOAT32(block);	block+=4;					// 28
			num3SFlights			= GetLittleEndianUINT32(block);		block+=4;					// 32
			num3SFtextures			= GetLittleEndianUINT32(block);		block+=4;					// 36
			num3SFmaterials			= GetLittleEndianUINT32(block);		block+=4;					// 40
			uint32	numobjects		= GetLittleEndianUINT32(block);		block+=4;					// 44
			uint32	numfaces		= GetLittleEndianUINT32(block);		block+=4;					// 48
			uint32	numverts		= GetLittleEndianUINT32(block);		block+=4;					// 52
			maxObjMem				= GetLittleEndianUINT32(block);		block+=4;					// 56
			maxObjFaces				= GetLittleEndianUINT32(block);		block+=4;					// 60
			if (version>=0x402)
			{	maxAnimMem			= GetLittleEndianUINT32(block);		block+=4;					// 64
				totalAnimMem		= GetLittleEndianUINT32(block);		block+=4;					// 68
			}
			uint32	numportals		= GetLittleEndianUINT32(block);		block+=4;					// 72
			block+=4;				// numAnimSequence												// 76
			textPoolSize			= GetLittleEndianUINT32(block);		block+=4;					// 80
			block+=4;				// Flags - Currently not used, will be 0						// 84
			uint32 numUVVerts[4];
			numUVVerts[0]			= GetLittleEndianUINT32(block);		block+=4;					// 88
			numUVVerts[1]			= GetLittleEndianUINT32(block);		block+=4;					// 92
			numUVVerts[2]			= GetLittleEndianUINT32(block);		block+=4;					// 96
			numUVVerts[3]			= GetLittleEndianUINT32(block);		block+=4;					// 100

			sceneHeader.numMaterials = num3SFmaterials;
			sceneHeader.numobjects = numobjects;
			sceneHeader.numTextures = num3SFtextures;
			sceneHeader.textPoolSize = textPoolSize;
			sceneHeader.scenePtr = NULL;
			if (InternalImport3DCallbacks.cbSceneHeader(&sceneHeader))
			{	memNeeded = sizeof(sScene3D) + sizeof(newobj)*numobjects + num3SFtextures*ptrsize + num3SFmaterials*ptrsize + num3SFlights*ptrsize + textPoolSize;
				byte *buf = fcalloc(memNeeded,buildstr("Import3sf: %s.3sf",fileactualname));
				scene = (sScene3D *)buf;			buf+=sizeof(sScene3D);
				txtcpy(scene->name, sizeof(scene->name), fileactualname);
				scene->ambient = ambientLight;
				scene->numobjs = numobjects;
				scene->numMtls = num3SFmaterials;
				scene->numLights = num3SFlights;
				scene->numTexextures = num3SFtextures;
				scene->objs = (newobj *)buf;		buf+=sizeof(newobj)*numobjects;
				scene->texArray = (Texture **)buf;	buf+=num3SFtextures * ptrsize;
				scene->mtlArray = (Material **)buf; buf+=num3SFmaterials * ptrsize;
				scene->lightArray = (light3d **)buf;buf+=num3SFlights * ptrsize;
				textPool = (char *)buf;				buf+=textPoolSize;
				scene->viewDistance = 5000.0f;
				allocedHeader = true;
			}
			memNeeded = / *maxObjMem +* / maxObjFaces*6;
			byte *buf = fcalloc(memNeeded,"Import3SF Temp Buffers");
			faceImportBuffer = (void *)buf;		buf += maxObjFaces*6;

			haveHeader = true;
			if (InternalImport3DCallbacks.cbProgress) InternalImport3DCallbacks.cbProgress(&sceneHeader,0,0,0);

		}	else
		if (!haveHeader)
			msg("3SF Error",buildstr("Error loading file: %s\r\nThe file contains no header, or the header is not at the beginning of the file (the file is corrupt)",filename));
		else


		if (txtncmp(blockType,"TXT:",4)==0)
		{	if (InternalImport3DCallbacks.cbTextPoolPreLoad())
			{	if (blockSize!=textPoolSize)
					msg("3SF Error",buildstr("Error loading file: %s\r\nThe Text Pool does not match the size specified in the file header (the file is corrupt)",filename));
				if (log) log->log("Received TEXTPOOL block %s (%i bytes,  %i Remain)",blockType,blockSize,bytesRemain);
				memcopy(textPool, block, blockSize);

				// Process the .import file (if it exists)
				char *importFileName = buildstr("%s/%s.import",filepath,fileactualname);
				if (fileExists(importFileName))
				{	textfile *txt = newtextfile(importFileName);
					while (char *line=txt->readln())
					{	char *token, sep;
						long ofs=0;
						token = gettokencustom(line,&ofs,&sep,"\t ");
						// Search text pool for this text
						bool found = false;
						uintf txtofs = 0;
						while (txtofs<textPoolSize)
						{	if (txticmp(token,textPool+txtofs)==0)
							{	found=true;
								break;
							}
							txtofs += txtlen(textPool+txtofs)+1;
						}
						if (!found) continue;
						if (numImports>=1024) break;
						imports[numImports].objName = textPool+txtofs;
						imports[numImports].command = 0;
						token = gettokencustom(line,&ofs,&sep,"\t ");
						for (uintf i=1; i<numImport3SFCommands; i++)
						{	if (txticmp(token,(const char *)import3SFCommands[i])==0)
							{	imports[numImports].command = i;
								break;
							}
						}
						numImports++;
					}	// For each line of the .import file
				}	// If the .import file exists
			}	else
			{	// We do actually need the textpool, even if we stated that we didn't want it
				usingTempTextPool = true;
				textPool = (char *)block;
			}
		}	else

		if (txtncmp(blockType,"TEX:",4)==0)
		{	uint32 numtex = GetLittleEndianUINT32((byte *)block);	block+=4;
			if (numtex!=num3SFtextures)
				msg("3SF Error",buildstr("Error loading file: %s\r\nThe number of textures in the textures block does not match the number in the header",filename));
			for (uintf i=0; i<numtex; i++)
			{	char *texName = textPool+GetLittleEndianUINT32((byte *)block);	block+=4;
				scene->texArray[i] = newTexture(texName,0, texture_currentImporter);
				scene->texArray[i]->mapping = GetLittleEndianUINT32((byte *)block);	block+=4;
			}
		}	else

		if (txtncmp(blockType,"MTL:",4)==0)
		{	char tmpName[92];
			char *namePtr = textPool+GetLittleEndianUINT32((byte *)block);	block+=4;
			if (InternalImport3DCallbacks.cbMaterialPreLoad(namePtr))
			{	tprintf((char *)tmpName,sizeof(tmpName),"%s::%s",filename3SF,namePtr);

				Material *mtl = findMaterial(tmpName,false);
				if (!mtl)
				{	mtl = findMaterial(namePtr,false);
				}
				if (!mtl)
				{	memfill(&importMtl,0,sizeof(importMtl));
					importMtl.name = namePtr;

					importMtl.flags		|= *block++;
					importMtl.minalpha	 = *block++;
					importMtl.numTextures= *block++;

					if (log) log->log("-------------------------------------------------------------");
					if (log) log->log("Material %s",tmpName);

					// Read base colors
					importMtl.ambient.a = 1.0f;
					importMtl.ambient.r = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.ambient.g = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.ambient.b = GetLittleEndianFLOAT32((byte *)block);	block+=4;

					importMtl.diffuse.r = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.diffuse.g = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.diffuse.b = GetLittleEndianFLOAT32((byte *)block);	block+=4;

					importMtl.specular.a = 1.0f;
					importMtl.specular.r = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.specular.g = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.specular.b = GetLittleEndianFLOAT32((byte *)block);	block+=4;

					importMtl.emissive.a = 1.0f;
					importMtl.emissive.r = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.emissive.g = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.emissive.b = GetLittleEndianFLOAT32((byte *)block);	block+=4;

					importMtl.diffuse.a	= GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.spec_power= GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.shininess = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.shininessStr = GetLittleEndianFLOAT32((byte *)block);	block+=4;
					importMtl.bias		= *block++;
					if (log) log->log("Alpha: %i, flags=%x, minalpha=%i, Shininess=%f, shinyStrength=%f",(int)(importMtl.diffuse.a*255.0f),importMtl.flags, importMtl.minalpha,importMtl.shininess,importMtl.shininessStr);

					// Textures
					for (j=0; j<importMtl.numTextures; j++)
					{	uint32 texID = GetLittleEndianUINT32((byte *)block);	block+=4;	// Texture Filename
//						dword texflag = 0;
//						if (flags & newgroup_notextures)
//						{	texflag |= texture_dontload;
//						}
						importMtl.texture[j]	= scene->texArray[texID];//newTexture(texName, 0, texture_currentImporter);//FromFile(texName,0); // texflag);
						byte mapping = *block++;
//						importMtl.texture[j]->mapping = mapping>>4;			// 0x0f 1 = UVMAP_SPHERE_ENV, 2 =
						importMtl.mapping[j]	= mapping>>4;				// 0x0f 1 = UVMAP_SPHERE_ENV, 2 =
						importMtl.channel[j]	= mapping&0x0f;				// Mapping parameters (eg: for mapping 0 (explicit UV co-ords, this is the UV co-ord channel number)
						importMtl.slotType[j]	= *block++;
						importMtl.texAmount[j]	= *block++;
						if (importMtl.slotType[j]==slotType_Reflection) importMtl.mapping[j] = 1;
						if (log) log->log("texture %s, mapping %x, slotType %i, texAmount %i, channel %i",importMtl.texture[j]->name, importMtl.texture[j]->mapping, importMtl.slotType[j], importMtl.texAmount[j], importMtl.channel[j]);
					}

					// Read in final blending
					importMtl.targetParam = *block++;
					importMtl.sourceParam = *block++;
					importMtl.finalBlend  = *block++;

					if (InternalImport3DCallbacks.cbMaterialPostLoad(&importMtl))
						mtl = translateMaterial(&importMtl,genShaders,filename3SF,0);
					//log->log("Received MATERIAL block %s with %i textures (%i bytes,  %i Remain)",mtl->name,numtex,blockSize,bytesRemain);
				}	// if we didn't find an existing material with that name
				scene->mtlArray[numMtls++]=mtl;
				if (InternalImport3DCallbacks.cbProgress) InternalImport3DCallbacks.cbProgress(&sceneHeader,numobjs,numMtls,0);
			}	// If preload material callback
		}	else

		if (txtncmp(blockType,"LGT:",4)==0 && numLights==0)
		{/ *	light3d *l = newlight(light_freerange);
			l->name = textPool+GetLittleEndianUINT32((byte *)block);	block+=4;
			l->flags |= GetLittleEndianUINT32((byte *)block);			block+=4;
			l->color.r = 1.0f;//GetLittleEndianFLOAT32((byte *)block);			block+=4;
			l->color.g = 1.0f;//GetLittleEndianFLOAT32((byte *)block);			block+=4;
			l->color.b = 1.0f;//GetLittleEndianFLOAT32((byte *)block);			block+=4;
			l->specular.r = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			l->specular.g = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			l->specular.b = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			float posX = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			float posY = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			float posZ = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			float dirX = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			float dirY = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			float dirZ = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			makeidentity(l->matrix);
			l->matrix[12] = posX;
			l->matrix[13] = posY;
			l->matrix[14] = posZ;
//			matlookalong(l->matrix,posX,posY,posZ, dirX,dirY,dirZ);
//			fixrotations(l->matrix);
			l->range = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			l->falloff = GetLittleEndianFLOAT32((byte *)block);		block+=4;		// This is always 1.0 for now
			l->attenuation0 = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			l->attenuation1 = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			l->attenuation2 = GetLittleEndianFLOAT32((byte *)block);		block+=4;
			l->innerangle= GetLittleEndianFLOAT32((byte *)block);			block+=4;
			l->outerangle= GetLittleEndianFLOAT32((byte *)block);			block+=4;
			l->flags |= light_enabled;
			changelight(l);
			scene->lightArray[numLights++] = l;
			* /
/ *
	long  index;				// Don't touch - it's used by the engine, modifying this variable (even restoring it back to a previous value) can have disasterous results
	Matrix matrix;				// Matrix for the light (defining position and target)
	float3 color;				// Color components of the light (can be negative)
	float3 specular;			// Specular component of the light
	float attenuation0,attenuation1,attenuation2; // Light attenuation over distance ... Intensity = 1.0 / (a0 + d * a1 + d*d * a2) where d = distance
	float innerangle,outerangle;// Angle (in radians) of a spotlight's inner (specular) and outer (diffuse) circles
	float spotExponent;			// Higher numbers result in a more focused specular.
	float range;				// Range of the light in world units (usually does not affect directional lights)
	float falloff;				// Sets the falloff value of the outer circle of the light (typically set this to 1.0)
	float manager;				// A temp variable used by the light manager
	dword flags;				// Type of the light and various other flags
	light3d *prev,*next;		// links to next and previous lights (internal engine use only)
* /

/ *
		blockwritefloat((ls.hotsize/360.0f)*2*3.1415926535897932384626433832795f);
		blockwritefloat((ls.fallsize/360.0f)*2*3.1415926535897932384626433832795f);
		if (ls.type==AMBIENT_LGT)
		{	header.ambient = 0xff000000 +
							 ((uint32)(ls.color.r*ls.intens * 255.0f) << 16) +
							 ((uint32)(ls.color.g*ls.intens * 255.0f) <<  8) +
							 ((uint32)(ls.color.b*ls.intens * 255.0f)      );
		}
* /
		} else

		if (txtncmp(blockType,"OBJ:",4)==0)
		{	float3 pivot;
			OSFobjhdr *hdr = (OSFobjhdr *)block;
			block += sizeof(OSFobjhdr);
			sImportObjHeader importObjHdr;
			importObjHdr.numBones = 0;
			importObjHdr.numFaces = GetLittleEndianUINT32((byte *)&hdr->numfaces);
			importObjHdr.numKeyFrames = GetLittleEndianUINT32((byte *)&hdr->numkeyframes);
			if (importObjHdr.numKeyFrames!=1) msg("Import 3SF Error","Invalid numKeyFrames - keyframes are not implemented yet");
			importObjHdr.numVerts = GetLittleEndianUINT32((byte *)&hdr->numverts);
			importObjHdr.pivot = hdr->pivot;
			importObjHdr.reserved1 = 0;
			importObjHdr.sceneStackIndex = 0;

			uintf channelsinobj = GetLittleEndianUINT32((byte *)&hdr->channelsinobj);
			char *objName = textPool+GetLittleEndianUINT32((byte *)&hdr->nameofs);
			intf mtlnum = GetLittleEndianINT32((byte *)&hdr->mtlnumber);
			pivot.x = GetLittleEndianFLOAT32((byte *)&hdr->pivot.x);
			pivot.y = GetLittleEndianFLOAT32((byte *)&hdr->pivot.y);
			pivot.z = GetLittleEndianFLOAT32((byte *)&hdr->pivot.z);

			uintf numfaces = importObjHdr.numFaces;
			uintf numverts = importObjHdr.numVerts;

			byte *channelinfo = block;
			block += channelsinobj;

//			if (importObjHdr.numKeyFrames==1)
//			{	if (hdr->extraVertexData & 1)
//					importObjHdr.vsd = vertexDescription_PNTUV+channelsinobj;		// Single Frame, with tangent
//				else
//					importObjHdr.vsd = vertexDescription_PNUV+channelsinobj;		// Single Frame, no tangent
//			}	else
//			{	if (hdr->extraVertexData & 1)
//					importObjHdr.vsd = vertexDescription_APNTUV+channelsinobj;		// Multiple Frames, with tangent
//				else
//					importObjHdr.vsd = vertexDescription_APNUV+channelsinobj;		// Multiple Frames, no tangent
//			}

			if (InternalImport3DCallbacks.cbObjectPreLoad(objName, &importObjHdr))
			{	newobj *obj;
				if (allocedHeader)
					obj = &scene->objs[numobjs++];
				else
					obj = &tmpObj;
				if (numverts>65535) msg("3SF Error",buildstr("Error loading file: %s\r\nObject %i (%s) has too many verticies (%i, maximum is 65535)",filename,objName,numverts));
				if (mtlnum>=(intf)num3SFmaterials) msg("3SF Error",buildstr("Error loading file: %s\r\nObject %i (%s) requested a material number that is out of range",filename,numobjs-1,obj->name));
				obj->lighting = NULL;
				obj->maxlights = max3SFLights;
				obj->name = objName;
				if ((mtlnum>=0) && allocedHeader)
					obj->mtl = scene->mtlArray[mtlnum];
				else
				{	// obj->mtl = NULL;//NULLMtl;
					msg("3SF Error",buildstr("Error loading file: %s\r\nObject %s has no material assigned to it",filename,obj->name));
				}
				makeidentity(obj->matrix);
				obj->matrix[mat_xpos] = pivot.x;
				obj->matrix[mat_ypos] = pivot.y;
				obj->matrix[mat_zpos] = pivot.z;
				if (numobjs==1)
				{	// This is our first object, set up initial scene bounding box
					scene->minX = scene->maxX = pivot.x;
					scene->minY = scene->maxY = pivot.y;
					scene->minZ = scene->maxZ = pivot.z;
				}
				uint16 *faces;
				if (numfaces>maxObjFaces)
					msg("3SF Error",buildstr("Error loading file: %s\r\nnumFaces (%i) > maxObjFaces (%i) in object %s",filename,numfaces,maxObjFaces,objName));
				if (numverts * importObjHdr.vsd->size > maxObjMem)
					msg("3SF Error",buildstr("Error loading file: %s\r\nVertexMemUsage (%i) > maxObjMem (%i) in object %s",filename,numverts * importObjHdr.vsd->size, maxObjMem, objName));
				faces = (uint16*)faceImportBuffer;

				// Copy faces to buffer
				if (numverts<65536)
				{	uintf faceIndex = 0;
					for (uintf i=0; i<numfaces; i++)
					{	faces[faceIndex++] = (uint16)(GetLittleEndianUINT32((byte *)block)); block+=4;
						faces[faceIndex++] = (uint16)(GetLittleEndianUINT32((byte *)block)); block+=4;
						faces[faceIndex++] = (uint16)(GetLittleEndianUINT32((byte *)block)); block+=4;
					}
				}	else
				{	memcopy(faces, block, numfaces*12);
				}

				// Calculate pointer to vertex data, it is already in the exact format needed by the video card (Assuming Little Endian CPU ### write code for Big Endian)
				uintf verticesSize = numverts * importObjHdr.vsd->size + (numverts * importObjHdr.numKeyFrames * importObjHdr.vsd->animSize);		// Number of bytes in the vertex data
				void *vertsPtr = block;
				block += verticesSize;

				// Calculate dimensions
				sVertexStream tmpVS;		// sVertexStreamDepricated tmpVS;
				// tmpVS.desc = importObjHdr.vsd;
				tmpVS.vertexSize = importObjHdr.vsd->size;
				tmpVS.flags = vertexStreamFlags_modifyOnce;
				tmpVS.numKeyFrames = importObjHdr.numKeyFrames;
				tmpVS.numVerts = numverts;
				tmpVS.vertices = vertsPtr;
				dimensions3D *d = &obj->dimensions;
				bool haveDimensions = false;	// ### calcDimensions(&tmpVS, d, 0);
				if (!haveDimensions)
				{	d->center.x=0.0f;	d->center.y=0.0f;	d->center.z=0.0f;
					d->max.x = 0.5f;	d->max.y = 0.5f;	d->max.z = 0.5f;
					d->min.x =-0.5f;	d->min.y =-0.5f;	d->min.z =-0.5f;
					d->depth = 1.0f;	d->height = 1.0f;	d->width = 1.0f;
					d->radius = 1.0f;	d->radiusSquared = 1.0f;
				}

				float radius = d->radius;
				if (allocedHeader && haveDimensions)
				{	if ((pivot.x-d->min.x) < scene->minX) scene->minX = pivot.x-d->min.x;
					if ((pivot.y-d->min.y) < scene->minY) scene->minY = pivot.y-d->min.y;
					if ((pivot.z-d->min.z) < scene->minZ) scene->minZ = pivot.z-d->min.z;
					if ((pivot.x+d->max.x) > scene->maxX) scene->maxX = pivot.x+d->max.x;
					if ((pivot.y+d->max.y) > scene->maxY) scene->maxY = pivot.y+d->max.y;
					if ((pivot.z+d->max.z) > scene->maxZ) scene->maxZ = pivot.z+d->max.z;
				}
				obj->preDrawCallback = NULL;
				obj->userPtr = NULL;
				obj->stackLevel = 0;

				if (InternalImport3DCallbacks.cbObjectPostLoad(obj,faces,numfaces,vertsPtr,numverts,importObjHdr.vsd))
				{	// Scan the .import data to see if we need to manipulate this object in any way
					for (uintf i=0; i<numImports; i++)
					{	if (obj->name==imports[i].objName)
						{	switch(imports[i].command)
							{	case 1:		// sortLowY
									if (importModMemSize<numfaces*sizeof(float))
									{	if (importModMem) fcfree(importModMem);
										char buf[64];
										tprintf(buf,sizeof(buf),"%s.import",obj->name);
										importModMem = fcalloc(numfaces*sizeof(float),buf);
//										float3 *verts;
//										for (uintf i=0; i<numfaces; i++)
//										{
//										}
									}
							}
							break;
						}
					}
/ *
	sImportMod imports[1024];
	uintf numImports=0;
	void *importModMem = NULL;
	uintf importModMemSize = 0;
* /
					// Now send the object data to the video driver
					obj->faces = createFaceStream(0,numfaces,faces,0);
					uploadFaceStream(obj->faces);
					obj->verts = createVertexStream(vertsPtr, numverts, tmpVS.vertexSize, tmpVS.flags);	// ### Update for animation createVertexStreamDepricated(importObjHdr.vsd, numverts, importObjHdr.numKeyFrames, vertsPtr, 0);	// ### Update for animation
					uploadVertexStream(obj->verts);
					obj->verts->vertices = NULL;
				}
				if (log) log->log("Received OBJECT block %s using material %s with %i faces, %i vertices (%i bytes,  %i Remain)",obj->name, obj->mtl->name, numfaces, numverts,blockSize,bytesRemain);
				if (InternalImport3DCallbacks.cbProgress) InternalImport3DCallbacks.cbProgress(&sceneHeader,numobjs,numMtls,0);
			}	// if preload object
		}	else
		{	// Unrecognised block
			if (log) log->log("Received Unrecognised block %s (%i bytes,  %i Remain)",blockType,blockSize,bytesRemain);
		}
	}
	if (faceImportBuffer) fcfree(faceImportBuffer);
	if (log) delete log;
	fileClose(genShaders);

	// Count number of textures
	uintf numTextures = 0;
	Texture *tex = getfirsttexture();
	while (tex)
	{	if (tex->flags & texture_currentImporter) numTextures++;
		tex = tex->next;
	}

	uintf texturesLoaded = 0;
	tex = getfirsttexture();
	while (tex)
	{	if (tex->flags & texture_currentImporter)
		{	textureLoad(tex);
			tex->flags &= ~texture_currentImporter;
			texturesLoaded++;
			if (InternalImport3DCallbacks.cbProgress) InternalImport3DCallbacks.cbProgress(&sceneHeader,numobjs,numMtls,texturesLoaded);
		}
		tex = tex->next;
	}

// textureLoad(tex);

	// Clean up
	if (importModMem) fcfree(importModMem);

	return scene;
*/
}

sScene3D *load3sf(const char *filename, sImport3dCallbacks *callbacks, uintf import3DFlags)
{	byte *buf = fileLoad(filename);
	intf size = filesize;
	sScene3D *result = load3sfmem(filename,callbacks,buf,size, import3DFlags);
	fcfree(buf);
	return result;
}

// ************************************************************************************
// ***									Import OSF									***
// ************************************************************************************
/*
#define osfbyte	 *(byte *)buf; buf+=1
#define osfword  GetLittleEndianUINT16(buf);	buf += 2	// *(word *)buf; buf+=2
#define osfdword GetLittleEndianUINT32(buf);	buf += 4	// *(dword *)buf; buf+=4
#define osffloat GetLittleEndianFLOAT32(buf); 	buf += 4
#define osfstring textpool+GetLittleEndianUINT32(buf); buf+=4; if (chr<textpool || chr>textpool+textpoolsize) msg("OSFImport Error","Corrupted Text Pool");
#define osfSkipI32 buf += 4;
#define maxheadersize 0x50 // VITAL!!! This must be updated with every new version of the OSF file format

struct lightheader
{	uint32	name;
	uint32	flags;
	float3	col;
	float3	spec;
	float3	pos;
	float3	dir;
	float	range;
	float	falloff;
	float3	attenuation;
	float	innerangle;
	float	outerangle;
};

char OSFlastblock[5];
const char *lastblockname = NULL;

uint32 openOSFblock(sFileHandle *handle, char *blockname)
{	byte tmp[8];
	fileRead(handle, tmp, 8);
	OSFlastblock[0] = blockname[0] = tmp[0];
	OSFlastblock[1] = blockname[1] = tmp[1];
	OSFlastblock[2] = blockname[2] = tmp[2];
	OSFlastblock[3] = blockname[3] = tmp[3];
	OSFlastblock[4] = 0;
	return GetLittleEndianUINT32(&tmp[4]);
}

void closeOSFblock(sFileHandle *handle)
{	char tmp[5];
	tmp[4] = 0;
	fileRead(handle, tmp, 4);
	if (txticmp(tmp,"endb")) msg("OSF Error",buildstr("The OSF File is corrupt, Failed to correctly close a %s block called %s",OSFlastblock,lastblockname));
	lastblockname = "NULL";
}

group3d *importosf(char *filename,dword flags)		// Old Version
{	group3d		*g1;
	long		memneeded;
	byte		*mem;
	char		*textpool;
	uintf		j,k;
	char		*chr;
	byte		osfheader[maxheadersize];
	byte		*animbuffer;

//	char		animcode[5] = {'s','m','p','l',0};						// ### Unused
	logfile		*osflog = newlogfile(NULL);
//	uintf		osffilesize = fileGetSize(filename);					// ### Unused
	sFileHandle *handle = fileOpen(filename);
	byte		*buf = osfheader;
	char		_blockname[5] = {0,0,0,0,0};
	char		*blockname = &_blockname[0];

	byte		mtxStackSize;

	// Read in Header
	uint32 headersize = openOSFblock(handle,blockname);
	lastblockname = "Header";
	if (headersize>maxheadersize)
	{	msg("Import OSF Error",buildstr("The OSF File %s has an unexpectedly long header (%x), and cannot be read\r\n",filename,headersize));
	}
	if (txticmp(blockname,"HDR:"))
	{	msg("Import OSF Error",buildstr("The OSF File %s is invalid, it does not begin with a header block\r\n",filename));
	}
	fileRead(handle,buf,headersize);	// Read in remainder of the header
	dword		version			= ((dword)buf[0])*100+buf[1]; buf+=2;

	if (version<400)
	{	msg("Import OSF Error",buildstr("The OSF File %s is version %.2f.  The OSF file format was completely changed from version 4.0\r\nSo I'm afraid that you cannot load this file.  Sorry",filename,((float)version)/100.0f));
	}

	// We should genrate a warning if version>401
	if (version>401)
	{	msg("Import OSF Error",buildstr("The OSF File %s is version %.2f.  The OSF importer in this program can only read up to version 4.01",filename,((float)version)/100.0f));
	}

	mtxStackSize = osfbyte;				// Number of elements in the Matrix Stack

	buf++;								//byte	reserved1;
	float	horizon			= osffloat;
	uint32  ambientLight	= osfdword;
	uint32	backgroundColor	= osfdword;
	uint32	fogColor		= osfdword;
	float	fogstart		= osffloat;
	float	fogend			= osffloat;
	uint32	numlights		= osfdword;
	uint32	nummaterials	= osfdword;
	uint32	numobjects		= osfdword;
	uint32	numfaces		= osfdword;
	uint32	numverts		= osfdword;
	uint32	animmem			= osfdword;
	osfSkipI32;							// num2Dtverts
	osfSkipI32;							// num3Dtverts
	osfSkipI32;							// num4Dtverts
	uint32	numportals		= osfdword;
	osfSkipI32;							// numanimseq
	uint32	textpoolsize	= osfdword;
	osfSkipI32;							// osfflags
	closeOSFblock(handle);

	if (numlights>0)		ambientLight		= 0xff000000;

	osflog->log("Importing File: %s",filename);
	osflog->log("");
	osflog->log("File Version: %.2f",(float)version/100.0f);
//	osflog->log("Animation Compression: %i",animcompress);
	osflog->log("Number of Materials: %i",nummaterials);
	osflog->log("Number of Objects: %i",numobjects);
	osflog->log("Number of Faces: %i",numfaces);
	osflog->log("Number of Verticies: %i",numverts);
	osflog->log("Animation Memory Usage: %i bytes",animmem);
	osflog->log("Text Pool Size: %i",textpoolsize);
	osflog->log("");

	// Allocate static data
	memneeded = numobjects	* sizeof(obj3d*)	+
				numlights	* sizeof(light3d*)	+
				nummaterials* sizeof(Material*) +
				textpoolsize + animmem + sizeof(group3d);

	mem = fcalloc(memneeded,buildstr("3D Model %s static data",filename));

	g1			= (group3d *)mem;	mem+=sizeof(group3d);
	g1->obj		= (obj3d **)mem;	mem+=numobjects*sizeof(obj3d*);
	textpool	= (char *)mem;		mem+=textpoolsize;
	g1->lights	= (light3d **)mem;	mem+=numlights*sizeof(light3d*);
	g1->material= (Material **)mem;	mem+=nummaterials*sizeof(Material **);
	animbuffer	= mem;				mem+=animmem;
	synctest("importosf",(byte *)g1,memneeded,mem);

	// Fill in group3d structure
	g1->numobjs		= numobjects;
	g1->numlights	= numlights;
	g1->nummaterials= nummaterials;
	g1->flags		= group_memstruct;// | pref_flags;
	g1->uset		= 0;
	g1->numframes	= 1;
	g1->numportals	= numportals;
	g1->other		= NULL;
	g1->horizon		= horizon;
	g1->backgroundColor = backgroundColor;
	g1->ambientLight = ambientLight;
	g1->fogColor	= fogColor;
	g1->fogstart	= fogstart;
	g1->fogend		= fogend;

	bool havetextblock = false;
	uintf nextmtl = 0;
	uintf nextobj = 0;
	uintf nextlight = 0;

	while(1)
	{	dword blocksize = openOSFblock(handle,blockname);

		// --- TEXT POOL ---
		if (txticmp(blockname,"TXT:")==0)
		{	lastblockname = "Text Pool";
			fileRead(handle,textpool,textpoolsize);
			havetextblock = true;
			closeOSFblock(handle);
		}	else

		// --- Material ---
		if (txticmp(blockname,"MTL:")==0)
		{	byte mtlbuf[256];
			fileRead(handle,&mtlbuf[0],blocksize);
			buf = &mtlbuf[0];

			chr					= osfstring;
			lastblockname		= chr;
			Material *mtl		= newmaterial(chr);
			g1->material[nextmtl++] = mtl;
			mtl->parent = g1;
			mtl->flags		|= osfbyte;
			mtl->minalpha	 = osfbyte;
			dword numtex	 = osfbyte;

			// Read in textures
			for (j=0; j<numtex; j++)
			{	chr			 = osfstring;
				dword texflag = 0;
				if (flags & newgroup_notextures)
				{	//texflag |= texture_dontload;
				}
				mtl->texture[j]	= newTextureFromFile(chr,texflag);
				mtl->texture[j]->mapping = osfdword;
			}

			// Read in old-style vertex shader
//			if (mtl->flags & mtl_vertexshader)
//			{	// We have an overriden vertex shader ... skip the VS data in the file
//				buf += sizeof(int32)*14 + 1 + numtex;
//			}	else
			{	FixedVertexShader   *fs = (FixedVertexShader *)mtl->VertexShader;
				fs->ambient.a = 1.0f;
				fs->ambient.r = osffloat;
				fs->ambient.g = osffloat;
				fs->ambient.b = osffloat;

				fs->diffuse.r = osffloat;
				fs->diffuse.g = osffloat;
				fs->diffuse.b = osffloat;

				fs->specular.a = 1.0f;
				fs->specular.r = osffloat;
				fs->specular.g = osffloat;
				fs->specular.b = osffloat;

				fs->emissive.a = 1.0f;
				fs->emissive.r = osffloat;
				fs->emissive.g = osffloat;
				fs->emissive.b = osffloat;

				fs->diffuse.a	= osffloat;
				fs->spec_power	= osffloat;
				fs->bias		= osfbyte;

				for (j=0; j<numtex; j++)
				{	fs->modifier[j] = osfbyte;
				}
			}

			// Read in pixel shader
//			if (mtl->flags & mtl_pixelshader)
//			{	// We have an overriden pixel shader ... skip the PS data in the file
//				buf += numtex * 3;
//			}	else
			{	FixedPixelShader	*fps = (FixedPixelShader *)mtl->PixelShader;
				for (j=0; j<numtex; j++)
				{	fps->colorop[j]	= osfbyte;
					fps->alphaop[j]	= osfbyte;
					fps->channel[j]	= osfbyte;
				}
			}

			// Read in final blending
			mtl->targetParam = osfbyte;
			mtl->sourceParam = osfbyte;
			mtl->finalblend  = osfbyte;
			closeOSFblock(handle);

			osflog->log("Material %i: %s, flags %x",nextmtl-1,mtl->name,mtl->flags);
			osflog->log("");
		}	else

		// --- Light ---
		if (txticmp(blockname,"Lgt:")==0)
		{	lightheader lgt;
			if (blocksize!=sizeof(lightheader)) msg("Import OSF Error","Light block size mismatch");
			fileRead(handle,&lgt,blocksize);
			light3d *l = newlight(light_managed);
			g1->lights[nextlight++] = l;
			l->name = textpool+GetLittleEndianUINT32((byte *)&lgt.name);
			lastblockname = l->name;
			l->flags |= GetLittleEndianUINT32((byte *)&lgt.flags);
			l->color.r = GetLittleEndianFLOAT32((byte *)&lgt.col.r);
			l->color.g = GetLittleEndianFLOAT32((byte *)&lgt.col.g);
			l->color.b = GetLittleEndianFLOAT32((byte *)&lgt.col.b);
			l->specular.r = GetLittleEndianFLOAT32((byte *)&lgt.spec.r);
			l->specular.g = GetLittleEndianFLOAT32((byte *)&lgt.spec.g);
			l->specular.b = GetLittleEndianFLOAT32((byte *)&lgt.spec.b);
			float posx,posy,posz,dirx,diry,dirz;
			posx = GetLittleEndianFLOAT32((byte *)&lgt.pos.x);
			posy = GetLittleEndianFLOAT32((byte *)&lgt.pos.y);
			posz = GetLittleEndianFLOAT32((byte *)&lgt.pos.z);
			dirx = GetLittleEndianFLOAT32((byte *)&lgt.dir.x);
			diry = GetLittleEndianFLOAT32((byte *)&lgt.dir.y);
			dirz = GetLittleEndianFLOAT32((byte *)&lgt.dir.z);
			matlookalong(l->matrix,posx,posy,posz, dirx,diry,dirz);
			fixrotations(l->matrix);
			l->range= GetLittleEndianFLOAT32((byte *)&lgt.range);
			l->falloff=GetLittleEndianFLOAT32((byte *)&lgt.falloff);
			l->attenuation0=GetLittleEndianFLOAT32((byte *)&lgt.attenuation.x);
			l->attenuation1=GetLittleEndianFLOAT32((byte *)&lgt.attenuation.y);
			l->attenuation2=GetLittleEndianFLOAT32((byte *)&lgt.attenuation.z);
			l->innerangle= GetLittleEndianFLOAT32((byte *)&lgt.innerangle);
			l->outerangle= GetLittleEndianFLOAT32((byte *)&lgt.outerangle);
			l->flags |= light_enabled;
			changelight(l);
			osflog->log("");
			osflog->log("Light %i (%s): r=%.2f, g=%.2f, b=%.2f, range = %.2f",nextlight,l->name,l->color.r,l->color.g,l->color.b,l->range);
			osflog->log("  x=%.2f, y=%.2f, z=%.2f,  dx=%.2f, dy = %.2f, dz = %.2f",posx,posy,posz,dirx,diry,dirz);
			closeOSFblock(handle);
		}	else

		// --- Object ---
		if (txticmp(blockname,"Obj:")==0)
		{	OSFobjhdr hdr;
			fileRead(handle,&hdr,sizeof(OSFobjhdr));
			chr = textpool+GetLittleEndianUINT32((byte *)&hdr.nameofs);
			byte channelinfo[8];
			uintf channelsinobj = GetLittleEndianUINT32((byte *)&hdr.channelsinobj);
			fileRead(handle,&channelinfo[0],channelsinobj);	// read in texture channel sizes
			_sObjCreateInfo	objinfo;
			memfill(&objinfo,0,sizeof(objinfo));
			objinfo.numfaces = GetLittleEndianUINT32((byte *)&hdr.numfaces);
			objinfo.numverts = GetLittleEndianUINT32((byte *)&hdr.numverts);
			objinfo.flags = GetLittleEndianUINT32((byte *)&hdr.flags);

			// Obtain Material Pointer
			int32 matnum = GetLittleEndianINT32((byte *)&hdr.mtlnumber);
			if (matnum>=(int32)nummaterials) msg("OSFImport Error",buildstr("Object %i (%s) requested a material number that is out of range",nextobj,chr));
			Material *mtl = NULLmtl;
			if (matnum>=0) mtl = g1->material[matnum];

			// Obtain Texture Co-ordinate Info
			uintf vertsize = 0;
			uintf uvcount = 0;
			for(j=0; j<channelsinobj; j++)
			{	objinfo.NumTexComponents[j] = channelinfo[j];
				vertsize = channelinfo[j] * sizeof(float);
				uvcount += channelinfo[j];
			}

			// Add in any additional texture channels required by the vertex shader
//			if (mtl->flags & mtl_vertexshader)
//			{	ScriptedVertexShader *vs = (ScriptedVertexShader *)mtl->VertexShader;
//				for (k=0; k<vs->CreateChannels; k++)
//				{	objinfo.NumTexComponents[k+channelsinobj]=3;
//				}
//			}

			lastblockname = chr;
			char compression[5];
			compression[0] = hdr.compression[0];
			compression[1] = hdr.compression[1];
			compression[2] = hdr.compression[2];
			compression[3] = hdr.compression[3];
			compression[4] = 0;
			obj3d *o = g1->obj[nextobj++] = newobj3d(chr, &objinfo, GetLittleEndianUINT32((byte *)&hdr.numkeyframes), compression, mtl);
			o->matrix = NULL;
			o->wirecolor = GetLittleEndianUINT32((byte *)&hdr.wirecolor);
			o->worldpos.x = GetLittleEndianFLOAT32((byte *)&hdr.pivot.x);
			o->worldpos.y = GetLittleEndianFLOAT32((byte *)&hdr.pivot.y);
			o->worldpos.z = GetLittleEndianFLOAT32((byte *)&hdr.pivot.z);
			lockmesh(o,lockflag_everything);

			// Read faces
			uint8 facedata[12];
			if (version<401)
			{	for (j=0; j<o->numfaces; j++)
				{	fileRead(handle,facedata,12);
					o->face[j].v1 = GetLittleEndianUINT32(&facedata[0]);
					o->face[j].v2 = GetLittleEndianUINT32(&facedata[4]);
					o->face[j].v3 = GetLittleEndianUINT32(&facedata[8]);
				}
			}	else
			{	if (o->numverts<256)
				{	for (j=0; j<o->numfaces; j++)
					{	fileRead(handle,facedata,3);
						o->face[j].v1 = facedata[0];
						o->face[j].v2 = facedata[1];
						o->face[j].v3 = facedata[2];
					}
				}	else
				if (o->numverts<65536)
				{	for (j=0; j<o->numfaces; j++)
					{	fileRead(handle,facedata,6);
						o->face[j].v1 = GetLittleEndianUINT16(&facedata[0]);
						o->face[j].v2 = GetLittleEndianUINT16(&facedata[2]);
						o->face[j].v3 = GetLittleEndianUINT16(&facedata[4]);
					}
				}	else
				{	for (j=0; j<o->numfaces; j++)
					{	fileRead(handle,facedata,12);
						o->face[j].v1 = GetLittleEndianUINT32(&facedata[0]);
						o->face[j].v2 = GetLittleEndianUINT32(&facedata[4]);
						o->face[j].v3 = GetLittleEndianUINT32(&facedata[8]);
					}
				}
			}

			// Read Vertices
			struct OSFvert
			{	float x,y,z, nx,ny,nz, uv[8*12];
			} osfvert;
			if (o->numkeyframes==1)
			{	vertsize += 6*sizeof(float);
			}

			float3 *pos = o->vertpos;
			float3 *nrm = o->vertnorm;
			float  *uvw = o->texcoord[0];

			for (j=0; j<o->numverts; j++)
			{	if (o->numkeyframes==1)
				{	fileRead(handle,&osfvert, vertsize);
					pos->x = GetLittleEndianFLOAT32((byte *)&osfvert.x);
					pos->y = GetLittleEndianFLOAT32((byte *)&osfvert.y);
					pos->z = GetLittleEndianFLOAT32((byte *)&osfvert.z);
					nrm->x = GetLittleEndianFLOAT32((byte *)&osfvert.nx);
					nrm->y = GetLittleEndianFLOAT32((byte *)&osfvert.ny);
					nrm->z = GetLittleEndianFLOAT32((byte *)&osfvert.nz);
				}	else
				{	fileRead(handle,&osfvert.uv, vertsize);
				}
				for (uintf k=0; k<uvcount; k++)
				{	uvw[k] = GetLittleEndianFLOAT32((byte *)&osfvert.uv[k]);
				}
				pos = (float3 *)incVertexPtr(pos,o);
				nrm = (float3 *)incVertexPtr(nrm,o);
				uvw = (float  *)incVertexPtr(uvw,o);
			}
			closeOSFblock(handle);

			calcRadius(o);
//			if (matnum>=0)
//			{	if (g1->material[matnum]->flags & mtl_vertexshader)
//				{	ScriptedVertexShader *vs = (ScriptedVertexShader *)g1->material[matnum]->VertexShader;
//					if (vs->CreateFunc) vs->CreateFunc(o);
//				}
//			}
			unlockmesh(o);
		}	else

		if (txticmp(blockname,"END:")==0)
		{	closeOSFblock(handle);
			break;
		}	else
		{	msg("ImportOSF Error",buildstr("Error in file %s, unrecognised block type %s",filename,blockname));
		}
	}

	groupdimensions(g1);
	if (version<400)
	{	// Prior to version 4.00, so we're missing valid horizon information.  Generate a usable value
		g1->horizon	= g1->radius * 0.6f;
	}
	delete osflog;
//	fcfree(osf);
	return g1;
}
*/

// ************************************************************************************
// ***								Import MD2										***
// ***								----------										***
// ***																				***
// ***	This file is provided for educational purposes only.  MD2 is not an			***
// ***  officially supported file format in the FC engine.  This code is based on   ***
// ***  code published by id Software.												***
// ***																				***
// ***	initplugin - Initializes the plugin and connects it to the engine			***
// ***	readmd2 - Reads in a single MD2 file (character and weapon are seperate		***
// ***            files)															***
// ***	importmd2 - Called by the engine when the application requests an MD2 file  ***
// ***				to be loaded (set up within initplugin).						***
// ***																				***
// ************************************************************************************

// These variables are needed because the engine is not linked into the plugins, so pointers
// to their services must be aquired
/*
fc_memory	*md2mem = NULL;

long readmd2(char *filename, Material *mtl, obj3d **obj)
{	msg("Incomplete Code Error","MD2 importer requires a basic set of shaders");
	// pseudocode:
	// if (!MD2VShader) MD2VShader = ...;	// Should take input from three vertex streams (prev frame, next frame, UV co-ords)
	// if (!MD2FShader) MD2FShader = ...;
	// if (!MD2FShaderBump) MD2FShaderBump = ...;	// Should be used only if a bump map is detected
	// if (!MD2ShaderP) MD2ShaderP = ...;
	struct md2verttype
	{	byte z,x,y;				// Values are re-ordered to convert to FC axis
		byte lightnormalindex;
	};

	struct compressvert
	{	dword vnum,tnum;
	};

	long count,i;
	compressvert *cvert;

	byte *md2	= fileLoad(filename);
	byte *dmd2	= md2;
	dmd2+=4;							// * Skip magic number
	dmd2+=4;							// * Skip version number
	int32 skinwidth	= GetLittleEndianUINT32(dmd2);	dmd2+=4;		// Width  in pixels of the skin texture.  MD2 texture co-ordinates are provided in texel space, FC needs them as 0.0 to 1.0 notation
	int32 skinheight= GetLittleEndianUINT32(dmd2);	dmd2+=4;		// Height in pixels of the skin texture.  MD2 texture co-ordinates are provided in texel space, FC needs them as 0.0 to 1.0 notation
	int32 framesize	= GetLittleEndianUINT32(dmd2);	dmd2+=4;		// Size in bytes of each frame within the file
	dmd2+=4;							// * Skip number of skins
	dmd2+=4;							// * Skip number of vertices - they're not needed in this importer //long numverts	= *dmd2++;		// Number of vertices in mesh
	int32 numtex	= GetLittleEndianUINT32(dmd2);	dmd2+=4;		// Number of texture vertices (MD2 does not require a direct correlation between vertices and texture co-ordinates)
	int32 numfaces	= GetLittleEndianUINT32(dmd2);	dmd2+=4;		// Number of faces in the mesh
	dmd2+=4;							// * Skip number of OpenGL commands - FC does not need these
	int32 numframes	= GetLittleEndianUINT32(dmd2);	dmd2+=4;		// Number of frames in the animation
	dmd2+=4;							// * Skip offset in bytes of the skin information
	int32 texofs	= GetLittleEndianUINT32(dmd2);	dmd2+=4;		// Offset in bytes of the texture information
	int32 faceofs	= GetLittleEndianUINT32(dmd2);	dmd2+=4;		// Offset in bytes of the face indicies
	int32 frameofs	= GetLittleEndianUINT32(dmd2);	dmd2+=4;		// Offset in bytes of the keyframe animation data
	dmd2+=4;							// * Skip Offset in bytes of the OpenGL commands
	dmd2+=4;							// * Skip Offset in bytes to the end of the file

	// Read in face index data
	// MD2 files have seperate arrays for vertices and texture co-ordinates.  FC does not
	// permit this.  We must convert the MD2 structure to something FC can manage
	word nextvert = 0;			// Index to next free vertex
	word tv[3],t[3];			// Temporary storage for vertex data
	cvert = (compressvert *)md2mem->fcalloc(sizeof(compressvert)*numfaces*3,"MD2 Vertex matching buffer");
	byte *faceadr = (byte *)(md2+faceofs);
	for (count=0; count<numfaces; count++)
	{	tv[0] = GetLittleEndianUINT16(faceadr+0);	// Index of Texture Vertex 0
		tv[1] = GetLittleEndianUINT16(faceadr+2);	// Index of Texture Vertex 1
		tv[2] = GetLittleEndianUINT16(faceadr+4);	// Index of Texture Vertex 2
		 t[0] = GetLittleEndianUINT16(faceadr+6);	// Index of Vertex 0
		 t[1] = GetLittleEndianUINT16(faceadr+8);	// Index of Vertex 1
		 t[2] = GetLittleEndianUINT16(faceadr+10); 	// Index of Vertex 2
		for (i=0; i<3; i++)
		{	word selectedvert = nextvert;	// If no match is found, default to using the next free vertex
			// Try to find an existing vertex which matches texture and vertex indicies
			for (long j=0; j<nextvert; j++)
			{	if (tv[i]==cvert[j].vnum && t[i]==cvert[j].tnum)
					selectedvert = (word)j;
			}
			// No match was found, place new data into the next free vertex position
			if (selectedvert==nextvert)
			{	cvert[nextvert].vnum=tv[i];
				cvert[nextvert].tnum= t[i];
				nextvert++;
			}
			PutLittleEndianUINT16(faceadr+(i<<1), selectedvert);	// Change face data to point to new vertex index
		}
		faceadr+=12;						// Advance to the next face
	}

	newobjcompression = "md2";
	*obj = newobj3d(filename, "pos:f3,nrm:f3,tex:f2", numfaces, nextvert, numframes, mtl,0);
	newobjcompression = "smpl";

	obj3d *o = *obj;
	lockmesh(o, lockflag_everything);		// readmd2
	Face *f = o->face;

	// Set up material
	mtl->diffuse.a = 1.0f;	mtl->diffuse.r = 1.0f;	mtl->diffuse.g = 1.0f;	mtl->diffuse.b = 1.0f;
	mtl->finalBlendOp = finalBlend_add;
	mtl->finalBlendSrcScale = blendParam_SrcAlpha;
	mtl->finalBlendDstScale = blendParam_InvSrcAlpha;
	fileNameInfo(filename);
	mtl->texture[0]=newTextureFromFile(buildstr("%s.tga",fileactualname),0);
//	ps->colorop[0]=blendop_modulate;
//	ps->alphaop[0]=blendop_modulate;
//	ps->channel[0]=0;
//	mat->mapping[0]=mtl_mipmap | mtl_bilinear;

	// Set up faces
	faceadr = md2+faceofs;
	for (count=0; count<numfaces; count++)
	{	f[count].v1 = GetLittleEndianUINT16(faceadr+2);
		f[count].v2 = GetLittleEndianUINT16(faceadr+0);
		f[count].v3 = GetLittleEndianUINT16(faceadr+4);
		faceadr+=12;
	}

	// Texture Co-ordinates
	byte *texadr = (byte *)(md2 + texofs);
	for (count = 0; count<numtex; count++)
	{	word ts = GetLittleEndianUINT16(texadr);	texadr+=2;
		word tt = GetLittleEndianUINT16(texadr);	texadr+=2;
		float rs = (float)(ts)/skinwidth;
		float rt = (float)(tt)/skinheight;
		for (i=0; i<nextvert; i++)
		{	if (cvert[i].tnum==(dword)count)
			{	float *tvert = (float *)getVertexPtr(o,texcoord[0],i);
				tvert[0]=rs;
				tvert[1]=rt;
			}
		}
	}

	// Frames
	for (long framenum = 0; framenum<numframes; framenum++)
	{	byte  *framestart = (byte *)(md2 + frameofs + framenum * framesize);

		byte *dest = (byte *)o->animdata + framenum * (24 + nextvert*4);
		float3 *destf3 = (float3 *)dest;

		// Obtain frame scale and convert to FC axis
		destf3[0].z = GetLittleEndianFLOAT32(framestart+0);
		destf3[0].x =-GetLittleEndianFLOAT32(framestart+4);
		destf3[0].y = GetLittleEndianFLOAT32(framestart+8);

		// Obtain frame translate and convert to FC axis
		destf3[1].z = GetLittleEndianFLOAT32(framestart+12);
		destf3[1].x =-GetLittleEndianFLOAT32(framestart+16);
		destf3[1].y = GetLittleEndianFLOAT32(framestart+20);

		dest += 24;
		md2verttype *md2vert = (md2verttype *)(framestart+40);
		for (long vnum=0; vnum<nextvert; vnum++)
		{	long usevert = cvert[vnum].vnum;
			// The following code is needed for converting co-ordinates from MD2 axis to FC axis (FC Axis is X increases to the right, Y increases upwards, and Z increases into the distance)
			dest[0] = md2vert[usevert].x;
			dest[1] = md2vert[usevert].y;
			dest[2] = md2vert[usevert].z;
			dest[3] = md2vert[usevert].lightnormalindex;
			dest+=4;
		}
	}

	unlockmesh(o);
	o->frame1 = 0;
	o->frame2 = 0;
	o->decodeframe(o);
	lockmesh(o, lockflag_everything);
	calcRadius(o);						// Without a radius, the FC renderer is unable to draw an object
	o->wirecolor = 0xffff00ff;			// This value is really only needed for programs that display a wireframe version of the object (like OSFTools)
	o->name="MD2";						// We need a string that will be static.  Using 'actualfilename' would cause the objects name to change at runtime.  It is possible to allocate memory for the string, and copy 'actualfilename' to it, but why bother, it's not that important.
	// Clean up temporary memory
	md2mem->free(cvert);
	md2mem->free(md2);
	unlockmesh(o);
	return numframes;
}

group3d *importmd2(char *filename,dword flags)
{	char		weaponflname[256];
	Material	*mat[2];
	bool		haveweapon = false;

	if (!md2mem)
	{	// md2 plugin has not yet been initialized ... do it now
		md2mem = (fc_memory*)classfinder("memory");
	}

	// Reconstruct the filename from filenameinfo components
	txtcpy(weaponflname,sizeof(weaponflname),filepath);
	txtcat(weaponflname,sizeof(weaponflname),fileactualname);
	txtcat(weaponflname,sizeof(weaponflname),"w.md2");
	haveweapon = (fileExists(weaponflname)!=NULL);

	// Allocate memory for materials
	if (haveweapon)
	{	mat[0] = newmaterial("MD2_Skin");
		mat[1] = newmaterial("MD2_Weapon");
	}	else
	{	mat[0] = newmaterial("MD2_Skin");
		mat[1] = NULLmtl;
	}
	// Allocate memory for group structures (don't yet know polygon / vertex counts)
	dword memneeded = sizeof(obj3d*) + sizeof(group3d);;
	if (haveweapon) memneeded += sizeof(obj3d*);

	byte *md2buffer = md2mem->fcalloc(memneeded,"Import MD2 header data");

	// Distribute memory to group3d structure and the two obj3d structures
	byte *buf = md2buffer;
	group3d *g1 = (group3d *)buf;		buf+=sizeof(group3d);
	g1->obj=(obj3d **)buf;				buf+=sizeof(obj3d*);
	if (haveweapon)						buf+=sizeof(obj3d*);
	md2mem->synctest("MD2Alloc",md2buffer,memneeded,buf);

	// Set up group3d structure
	g1->material = mat;
	g1->numlights = 0;
	g1->flags	 = group_memstruct;
	g1->uset	 = 0;

	// Load main character mesh (eg: dude.md2)
	fileNameInfo(filename);

	dword numframes1 = readmd2(filename, mat[0], &g1->obj[0]);
	if (haveweapon)
	{	g1->numobjs = 2;
		g1->nummaterials = 2;
		dword numframes2 = readmd2(weaponflname, mat[1], &g1->obj[1]);
		// The group3d structure must hold the maximum number of frames for all objects within it
		if (numframes1>numframes2)
			g1->numframes=numframes1;
		else
			g1->numframes=numframes2;
	}	else
	{	g1->nummaterials = 1;
		g1->numobjs = 1;
		g1->numframes = numframes1;
	}
	g1->horizon			= 5000.0f;
	g1->backgroundColor = 0xff000000;
	g1->ambientLight	= 0xffffffff;
	g1->fogColor		= 0xff000000;
	g1->fogstart		= 0.5f;
	g1->fogend			= 1.0f;
	groupdimensions(g1);
	return g1;
}
*/
void init3dplugins(void)
{	//addGenericPlugin((void *)importosf, PluginType_GeometryReader, ".osf");				// Register the OSF loader
	//addGenericPlugin((void *)importmd2, PluginType_GeometryReader, ".md2");				// Register the MD2 loader
}
