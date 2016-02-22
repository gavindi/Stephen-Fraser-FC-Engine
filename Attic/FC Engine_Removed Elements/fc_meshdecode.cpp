// ************************************************************************************
// ***																				***
// ***								Mesh Compression								***
// ***																				***
// ************************************************************************************
/*
enum AnimMeshCodecAction
{	AnimMesh_GetInfo = 0,				// Obtain a series of strings (textpool) describing the codec.
	AnimMesh_GetDecoder,				// Obtain a pointer to the decoder function.  Param1 points to a 'AnimMesh_Decoder' variable to receive the pointer to the function.
	AnimMesh_GetMemRequirements,		// Obtain the number of bytes needed to store all frames (param1 = pointer to obj3d structure with valid 'numverts' and 'numkeyframes' membervariables, param2 = pointer to uintf variable to hold result, param3 = unused)
	AnimMesh_CompressFrame,				// Compress the current object data to a particular frame number (param1 = obj3d structure locked for vertex reading, param2 = unused, param3 = frame number)
	AnimMesh_ForceDWORD = 0x7fffffff
};
typedef bool (*AnimMeshCodecType)(AnimMeshCodecAction action, void *param1, void *param2, uintf param3);
typedef void (*AnimMesh_Decoder)(obj3d *obj);
*/
// Example Uses ...
//		// Obtain pointer to codec (using codec "smpl")
//		AnimMeshCodecType codec = (AnimMeshCodecType)getPluginHandler("smpl",PluginType_AnimMeshCodec);
//
//		// Retrieve information about codec
//		char *textpool;
//		uintf textpoolsize;
//		codec(AnimMesh_GetInfo, &textpool, &textpoolsize, 0);
// 
//		// Retrieve decoder from codec (For storage in the obj3d structure)
//		AnimMesh_Decoder decoder;
//		codec(AnimMesh_GetDecoder, &decoder, NULL, 0);
//
//		// Obtain memory requirements for a particuar obj3d object (obj3d only needs numverts and numframes member-variables)
//		uintf memneeded;
//		codec(AnimMesh_GetMemRequirements, myobject, &memneeded, 0);
//
//		// Compress the current data in the mesh to an animation frame
//		// Note: This assumes the buffer pointed to by obj3d::animdata has been properly allocated as memneeded * numframes bytes in size
//		codec(AnimMesh_CompressFrame, myobject, NULL, frame_number);
//		

/************************************************************************************/
/*						3D Mesh Animation Decompressors								*/
/*	    				-------------------------------								*/
/*	 					(c) 1998-2005 by Stephen Fraser								*/
/*																					*/
/* Contains the functions for decompressing 3D mesh animations						*/
/*																					*/
/* Init Dependancies:	Plugins														*/
/* Init Function:		void initMeshDecoders(void)									*/
/* Term Function:		<none>														*/
/*																					*/
/*------------------------------  W A R N I N G  -----------------------------------*/
/*  This source file is only for the use of Stephen Fraser.  Any other use without	*/
/* my express permission may result in legal action being taken!					*/
/************************************************************************************/

#include "fc_h.h"

bool TestMeshLock(obj3d *obj,uintf validflags)
{	bool lockedbyme = false;
	if (obj->flags & obj_meshlocked)
	{	// We could have a problem here - the current lock may be invalid
		if ((obj->flags & validflags)!=validflags)
			msg("Mesh Locking Error",buildstr("AnimatingMesh Codec called on an object (%s) locked in an incompatible way",obj->name));
	}	else
	{	lockedbyme = true;
		lockmesh(obj, validflags);
	}
	return lockedbyme;
}

// Flags used in Mesh Decoders:
// 1st Char:	R/S = Vertex Access: Random / Sequential-Only
// 2nd Char:	S/D = Single / Dual Pass required to calculate memory requirements
// 3rd Char:	I/J = Interpolate / Jump between frames

// ----------------------------------------------------------------------------------------------
//										Simple (SMPL)
// ----------------------------------------------------------------------------------------------
// Designed by Stephen Fraser
// Write All   : No
// Header	   : None
// Frame Header: None
// Vertex      : 12 bytes (float3 position)
// Basic Info  : Stores a float3 value for every vertex for every frame.  Normals are kept
//				 the same between frames.  
// ----------------------------------------------------------------------------------------------
char AnimMesh_SimpleInfo[] =
{	"smpl\0"						// code
	"Simple Mesh Decompression\0"	// Name
	"Stephen Fraser\0"				// Author
	"RSI"							// Flags
};

char *AnimMesh_SimpleDecoder(obj3d *obj)
{	float ratio = obj->interpolation;
	float3 *pv = (float3 *)obj->animdata;
	dword index1 = obj->numverts*obj->frame1;
	dword index2 = obj->numverts*obj->frame2;
	float3 *p1 = &pv[index1];
	float3 *p2 = &pv[index2];
						
	lockmesh(obj, lockflag_write | lockflag_verts);
	float3 *v = (float3 *)obj->vertpos;
	for (uintf i=0; i<obj->numverts; i++)
	{	v->x = (p2->x - p1->x) * ratio + p1->x;
		v->y = (p2->y - p1->y) * ratio + p1->y;
		v->z = (p2->z - p1->z) * ratio + p1->z;
		v = (float3 *)incVertexPtr(v, obj);
		p1++;
		p2++;
	}
	unlockmesh(obj);
	return NULL;
}

bool AnimMesh_SimpleCodec(AnimMeshCodecAction action, void *param1, void *param2, uintf param3)
{	switch (action)
	{	case AnimMesh_GetInfo:
		{	// param1: points to pointer to hold information string
			void **info = (void **)param1;
			*info = (void *)AnimMesh_SimpleInfo;
			uintf *infosize = (uintf *)param2;
			*infosize = sizeof(AnimMesh_SimpleInfo);
			return true;
		}
		case AnimMesh_GetDecoder:
		{	// param1: points to pointer to hold decoder function
			void **decoder = (void **)param1;
			*decoder = (void *)AnimMesh_SimpleDecoder;
			return true;
		}
		case AnimMesh_GetMemRequirements:
		{	// param1: points to obj3d structure
			// param2: points to uintf variable to hold size in bytes
			uintf *result = (uintf *)param2;
			obj3d *obj = (obj3d *)param1;
			*result = obj->numkeyframes * (obj->numverts*sizeof(float3));
			return true;
			break;
		}
		case AnimMesh_CompressFrame:
		{	// param1: points to obj3d structure
			// param3: frame number to store animation in
			obj3d *obj = (obj3d *)param1;
			bool lockedbyme = TestMeshLock(obj,obj_meshlockread | lockflag_verts);
			float3 *pos = obj->vertpos;
			float3 *dst = &((float3 *)obj->animdata)[param3 * obj->numverts];
			for (uintf i=0; i<obj->numverts; i++)
			{	dst[i] = *pos;
				pos = (float3 *)incVertexPtr(pos, obj);
			}
			if (lockedbyme) unlockmesh(obj);
			return true;
		}
		default:
			return false;
		
	}
}

// ----------------------------------------------------------------------------------------------
//									Quake2 Model (MD2)
// ----------------------------------------------------------------------------------------------
// Designed by id Software
// Write All   : Yes
// Header	   : None
// Frame Header: 24 bytes (float3 scale, float3 translate)
// Vertex      : 4 bytes (byte x, byte y, byte z, byte normalIndex)
// Basic Info  : Scales byte-sized vertices by a scale, and translates them into position,
//               and then obtains a normal vector using a lookup value
// ----------------------------------------------------------------------------------------------
char AnimMesh_MD2Info[] =
{	"md2\0"							// code
	"Quake2 Model\0"				// Name
	"Stephen Fraser\0"				// Author
	"RSI"							// Flags
};

struct md2float3 {float x,y,z;};
md2float3 md2normals_[] =
{	{ 0.000000f,  0.850651f, -0.525731f},
	{-0.238856f,  0.864188f, -0.442863f},
	{ 0.000000f,  0.955423f, -0.295242f},
	{-0.500000f,  0.809017f, -0.309017f},
	{-0.262866f,  0.951056f, -0.162460f},
	{ 0.000000f,  1.000000f,  0.000000f},
	{-0.850651f,  0.525731f,  0.000000f},
	{-0.716567f,  0.681718f, -0.147621f},
	{-0.716567f,  0.681718f,  0.147621f},
	{-0.525731f,  0.850651f,  0.000000f},
	{-0.500000f,  0.809017f,  0.309017f},
	{ 0.000000f,  0.850651f,  0.525731f},
	{ 0.000000f,  0.955423f,  0.295242f},
	{-0.238856f,  0.864188f,  0.442863f},
	{-0.262866f,  0.951056f,  0.162460f},
	{-0.147621f,  0.716567f, -0.681718f},
	{-0.309017f,  0.500000f, -0.809017f},
	{-0.425325f,  0.688191f, -0.587785f},
	{-0.525731f,  0.000000f, -0.850651f},
	{-0.442863f,  0.238856f, -0.864188f},
	{-0.681718f,  0.147621f, -0.716567f},
	{-0.587785f,  0.425325f, -0.688191f},
	{-0.809017f,  0.309017f, -0.500000f},
	{-0.864188f,  0.442863f, -0.238856f},
	{-0.688191f,  0.587785f, -0.425325f},
	{-0.681718f, -0.147621f, -0.716567f},
	{-0.809017f, -0.309017f, -0.500000f},
	{-0.850651f,  0.000000f, -0.525731f},
	{-0.850651f, -0.525731f,  0.000000f},
	{-0.864188f, -0.442863f, -0.238856f},
	{-0.955423f, -0.295242f,  0.000000f},
	{-0.951056f, -0.162460f, -0.262866f},
	{-1.000000f,  0.000000f,  0.000000f},
	{-0.955423f,  0.295242f,  0.000000f},
	{-0.951056f,  0.162460f, -0.262866f},
	{-0.864188f,  0.442863f,  0.238856f},
	{-0.951056f,  0.162460f,  0.262866f},
	{-0.809017f,  0.309017f,  0.500000f},
	{-0.864188f, -0.442863f,  0.238856f},
	{-0.951056f, -0.162460f,  0.262866f},
	{-0.809017f, -0.309017f,  0.500000f},
	{-0.525731f,  0.000000f,  0.850651f},
	{-0.681718f,  0.147621f,  0.716567f},
	{-0.681718f, -0.147621f,  0.716567f},
	{-0.850651f,  0.000000f,  0.525731f},
	{-0.688191f,  0.587785f,  0.425325f},
	{-0.442863f,  0.238856f,  0.864188f},
	{-0.587785f,  0.425325f,  0.688191f},
	{-0.309017f,  0.500000f,  0.809017f},
	{-0.147621f,  0.716567f,  0.681718f},
	{-0.425325f,  0.688191f,  0.587785f},
	{-0.295242f,  0.000000f,  0.955423f},
	{ 0.000000f,  0.000000f,  1.000000f},
	{-0.162460f,  0.262866f,  0.951056f},
	{ 0.525731f,  0.000000f,  0.850651f},
	{ 0.295242f,  0.000000f,  0.955423f},
	{ 0.442863f,  0.238856f,  0.864188f},
	{ 0.162460f,  0.262866f,  0.951056f},
	{ 0.309017f,  0.500000f,  0.809017f},
	{ 0.147621f,  0.716567f,  0.681718f},
	{ 0.000000f,  0.525731f,  0.850651f},
	{-0.442863f, -0.238856f,  0.864188f},
	{-0.309017f, -0.500000f,  0.809017f},
	{-0.162460f, -0.262866f,  0.951056f},
	{ 0.000000f, -0.850651f,  0.525731f},
	{-0.147621f, -0.716567f,  0.681718f},
	{ 0.147621f, -0.716567f,  0.681718f},
	{ 0.000000f, -0.525731f,  0.850651f},
	{ 0.309017f, -0.500000f,  0.809017f},
	{ 0.442863f, -0.238856f,  0.864188f},
	{ 0.162460f, -0.262866f,  0.951056f},
	{-0.716567f, -0.681718f,  0.147621f},
	{-0.500000f, -0.809017f,  0.309017f},
	{-0.688191f, -0.587785f,  0.425325f},
	{-0.238856f, -0.864188f,  0.442863f},
	{-0.425325f, -0.688191f,  0.587785f},
	{-0.587785f, -0.425325f,  0.688191f},
	{-0.716567f, -0.681718f, -0.147621f},
	{-0.500000f, -0.809017f, -0.309017f},
	{-0.525731f, -0.850651f,  0.000000f},
	{ 0.000000f, -0.850651f, -0.525731f},
	{-0.238856f, -0.864188f, -0.442863f},
	{ 0.000000f, -0.955423f, -0.295242f},
	{-0.262866f, -0.951056f, -0.162460f},
	{ 0.000000f, -1.000000f,  0.000000f},
	{ 0.000000f, -0.955423f,  0.295242f},
	{-0.262866f, -0.951056f,  0.162460f},
	{ 0.238856f, -0.864188f, -0.442863f},
	{ 0.500000f, -0.809017f, -0.309017f},
	{ 0.262866f, -0.951056f, -0.162460f},
	{ 0.850651f, -0.525731f,  0.000000f},
	{ 0.716567f, -0.681718f, -0.147621f},
	{ 0.716567f, -0.681718f,  0.147621f},
	{ 0.525731f, -0.850651f,  0.000000f},
	{ 0.500000f, -0.809017f,  0.309017f},
	{ 0.238856f, -0.864188f,  0.442863f},
	{ 0.262866f, -0.951056f,  0.162460f},
	{ 0.864188f, -0.442863f,  0.238856f},
	{ 0.809017f, -0.309017f,  0.500000f},
	{ 0.688191f, -0.587785f,  0.425325f},
	{ 0.681718f, -0.147621f,  0.716567f},
	{ 0.587785f, -0.425325f,  0.688191f},
	{ 0.425325f, -0.688191f,  0.587785f},
	{ 0.955423f, -0.295242f,  0.000000f},
	{ 1.000000f,  0.000000f,  0.000000f},
	{ 0.951056f, -0.162460f,  0.262866f},
	{ 0.850651f,  0.525731f,  0.000000f},
	{ 0.955423f,  0.295242f,  0.000000f},
	{ 0.864188f,  0.442863f,  0.238856f},
	{ 0.951056f,  0.162460f,  0.262866f},
	{ 0.809017f,  0.309017f,  0.500000f},
	{ 0.681718f,  0.147621f,  0.716567f},
	{ 0.850651f,  0.000000f,  0.525731f},
	{ 0.864188f, -0.442863f, -0.238856f},
	{ 0.809017f, -0.309017f, -0.500000f},
	{ 0.951056f, -0.162460f, -0.262866f},
	{ 0.525731f,  0.000000f, -0.850651f},
	{ 0.681718f, -0.147621f, -0.716567f},
	{ 0.681718f,  0.147621f, -0.716567f},
	{ 0.850651f,  0.000000f, -0.525731f},
	{ 0.809017f,  0.309017f, -0.500000f},
	{ 0.864188f,  0.442863f, -0.238856f},
	{ 0.951056f,  0.162460f, -0.262866f},
	{ 0.442863f,  0.238856f, -0.864188f},
	{ 0.309017f,  0.500000f, -0.809017f},
	{ 0.587785f,  0.425325f, -0.688191f},
	{ 0.147621f,  0.716567f, -0.681718f},
	{ 0.238856f,  0.864188f, -0.442863f},
	{ 0.425325f,  0.688191f, -0.587785f},
	{ 0.500000f,  0.809017f, -0.309017f},
	{ 0.716567f,  0.681718f, -0.147621f},
	{ 0.688191f,  0.587785f, -0.425325f},
	{ 0.262866f,  0.951056f, -0.162460f},
	{ 0.238856f,  0.864188f,  0.442863f},
	{ 0.262866f,  0.951056f,  0.162460f},
	{ 0.500000f,  0.809017f,  0.309017f},
	{ 0.716567f,  0.681718f,  0.147621f},
	{ 0.525731f,  0.850651f,  0.000000f},
	{ 0.688191f,  0.587785f,  0.425325f},
	{ 0.425325f,  0.688191f,  0.587785f},
	{ 0.587785f,  0.425325f,  0.688191f},
	{-0.295242f,  0.000000f, -0.955423f},
	{-0.162460f,  0.262866f, -0.951056f},
	{ 0.000000f,  0.000000f, -1.000000f},
	{ 0.000000f,  0.525731f, -0.850651f},
	{ 0.295242f,  0.000000f, -0.955423f},
	{ 0.162460f,  0.262866f, -0.951056f},
	{-0.442863f, -0.238856f, -0.864188f},
	{-0.162460f, -0.262866f, -0.951056f},
	{-0.309017f, -0.500000f, -0.809017f},
	{ 0.442863f, -0.238856f, -0.864188f},
	{ 0.162460f, -0.262866f, -0.951056f},
	{ 0.309017f, -0.500000f, -0.809017f},
	{-0.147621f, -0.716567f, -0.681718f},
	{ 0.147621f, -0.716567f, -0.681718f},
	{ 0.000000f, -0.525731f, -0.850651f},
	{-0.587785f, -0.425325f, -0.688191f},
	{-0.425325f, -0.688191f, -0.587785f},
	{-0.688191f, -0.587785f, -0.425325f},
	{ 0.688191f, -0.587785f, -0.425325f},
	{ 0.425325f, -0.688191f, -0.587785f},
	{ 0.587785f, -0.425325f, -0.688191f}
};
float3 *md2normals = (float3 *)&md2normals[0];

char *AnimMesh_MD2Decoder(obj3d *obj)
{	float ratio2 = obj->interpolation;
	float ratio1 = 1-ratio2;

	uintf index1 = (obj->numverts*4+24)*obj->frame1;
	uintf index2 = (obj->numverts*4+24)*obj->frame2;

	float3 *scale1 = (float3 *)((byte *)obj->animdata+index1);
	float3 *scale2 = (float3 *)((byte *)obj->animdata+index2);
	float3 *translate1 = (float3 *)((byte *)obj->animdata+index1+12);
	float3 *translate2 = (float3 *)((byte *)obj->animdata+index2+12);
	byte *vertdata1 = (byte *)obj->animdata+index1+24;
	byte *vertdata2 = (byte *)obj->animdata+index2+24;

	lockmesh(obj, lockflag_write | lockflag_verts);
	float3 *v = obj->vertpos;
	float3 *n = obj->vertnorm;
	
	for (uintf i=0; i<obj->numverts; i++)	
	{
		v->x =	((intToFloat[vertdata1[0]]*scale1->x+translate1->x)*ratio1) +
				((intToFloat[vertdata2[0]]*scale2->x+translate2->x)*ratio2) ;
		v->y =	((intToFloat[vertdata1[1]]*scale1->y+translate1->y)*ratio1) +
			    ((intToFloat[vertdata2[1]]*scale2->y+translate2->y)*ratio2) ;
		v->z =	((intToFloat[vertdata1[2]]*scale1->z+translate1->z)*ratio1) +
				((intToFloat[vertdata2[2]]*scale2->z+translate2->z)*ratio2) ;
//		v->x =	(((float)vertdata1[0]*scale1->x+translate1->x)*ratio1) +
//				(((float)vertdata2[0]*scale2->x+translate2->x)*ratio2) ;
//		v->y =	(((float)vertdata1[1]*scale1->y+translate1->y)*ratio1) +
//			    (((float)vertdata2[1]*scale2->y+translate2->y)*ratio2) ;
//		v->z =	(((float)vertdata1[2]*scale1->z+translate1->z)*ratio1) +
//				(((float)vertdata2[2]*scale2->z+translate2->z)*ratio2) ;
		float3 *norm1 = &md2normals[vertdata1[3]];
		float3 *norm2 = &md2normals[vertdata2[3]];
		n->x = norm1->x * ratio1 + norm2->x * ratio2;
		n->y = norm1->y * ratio1 + norm2->y * ratio2;
		n->z = norm1->z * ratio1 + norm2->z * ratio2;

		v = (float3 *)incVertexPtr(v, obj);
		n = (float3 *)incVertexPtr(n, obj);
		vertdata1 += 4;
		vertdata2 += 4;
	}
	unlockmesh(obj);
	return NULL;
}

bool AnimMesh_MD2Codec(AnimMeshCodecAction action, void *param1, void *param2, uintf param3)
{	switch (action)
	{	case AnimMesh_GetInfo:
		{	// param1: points to pointer to hold information string
			void **info = (void **)param1;
			*info = (void *)AnimMesh_MD2Info;
			return true;
		}
		case AnimMesh_GetDecoder:
		{	// param1: points to pointer to hold decoder function
			void **decoder = (void **)param1;
			*decoder = (void *)AnimMesh_MD2Decoder;
			return true;
		}
		case AnimMesh_GetMemRequirements:
		{	// param1: points to obj3d structure
			// param2: points to uintf variable to hold size in bytes
			uintf *result = (uintf *)param2;
			obj3d *obj = (obj3d *)param1;
			*result = obj->numkeyframes * (24 + obj->numverts*4);
			return true;
			break;
		}
		case AnimMesh_CompressFrame:
		{	// param1: points to obj3d structure
			// param3: frame number to store animation in
			return false;			// not yet supported
		}
		default:
			return false;
		
	}
}

void initMeshDecoders(void)
{	addGenericPlugin((void *)AnimMesh_SimpleCodec,	PluginType_AnimMeshCodec, "smpl");	// Simple Codec
	addGenericPlugin((void *)AnimMesh_MD2Codec,		PluginType_AnimMeshCodec, "md2");	// Simple Codec
}
