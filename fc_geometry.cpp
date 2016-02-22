/************************************************************************************/
/*								FC_GEOMETRY file									*/
/*	    						----------------									*/
/*  					   (c) 1998-2011 by Stephen Fraser							*/
/*																					*/
/* Contains the code for handling 3D geometry, and high-level drawing code			*/
/*																					*/
/*------------------------------  W A R N I N G  -----------------------------------*/
/*  This source file is only for the use of Stephen Fraser.  Any other use without	*/
/* my express permission may result in legal action being taken!					*/
/************************************************************************************/

#include <stdlib.h>
#include "fc_h.h"
#include <bucketSystem.h>

//void renderFromStreamsDepricated(sVertexStreamDepricated *verts, sFaceStream *faces, uintf frame0, uintf frame1);
void ogl_PrepMatrixDepricated(float *mtx);

aSyncDrawBuffer *freeaSyncDrawBuffer = NULL, *usedaSyncDrawBuffer = NULL;
aSyncDrawBuffer *zSortHead = NULL;

Material *lastMtl = NULL;

void updateMaterial(Material *m)
{	if (lastMtl==m) lastMtl=NULL;
}

inline void drawASyncObj(aSyncDrawBuffer *dbuf)
{	msg("geometry::drawASyncObj needs updating","drawASyncObj uses depricated function 'renderFromStreams'");
	newobj *obj = dbuf->obj;
	if (dbuf->preDrawCallback)
		dbuf->preDrawCallback(dbuf);

	if (obj->mtl!=lastMtl)
	{	prepMaterial(obj->mtl);
		lastMtl=obj->mtl;
	}
	ogl_PrepMatrixDepricated(dbuf->matrix);

	//renderFromStreamsDeprticated(obj->verts,obj->faces, dbuf->startFrame, dbuf->endFrame);
	deletebucket(aSyncDrawBuffer,dbuf);
}

void flushZNode(aSyncDrawBuffer *node)
{	aSyncDrawBuffer *nodeRight = node->right;	// Cache right node pointer because
	if (node->left) flushZNode(node->left);
	drawASyncObj(node);							// The node is deleted here
	if (nodeRight) flushZNode(nodeRight);		// Flush right node based on cached pointer
}

void flushZSort(void)
{	if (zSortHead)
		flushZNode(zSortHead);
	zSortHead = NULL;
}

void drawNewObj(newobj *obj, float *matrix)
{	aSyncDrawBuffer *dbuf;
	allocbucket(aSyncDrawBuffer,dbuf,flags,Generic_MemoryFlag32,1024,"Async Draw Buffers");
	memcopy(dbuf->matrix,matrix,sizeof(dbuf->matrix));
	dbuf->obj = obj;
	dbuf->userPtr = obj->userPtr;
	dbuf->preDrawCallback = obj->preDrawCallback;
	dbuf->startFrame=0;
	dbuf->endFrame=0;
	Material *m = obj->mtl;
	if (m)
	{	if (m->flags & mtl_zsort)
		{	// Insert into ZSort array
			if (zSortHead)
			{	aSyncDrawBuffer *node = zSortHead;
				aSyncDrawBuffer *lastnode;
				while (node!=NULL)
				{	lastnode = node;
					if (dbuf->matrix[mat_zpos]>node->matrix[mat_zpos])
						node = node->right;
					else
						node=node->left;
				}
				if (dbuf->matrix[mat_zpos]>lastnode->matrix[mat_zpos])
					lastnode->right = dbuf;
				else
					lastnode->left = dbuf;
				dbuf->left = NULL;
				dbuf->right = NULL;
			}	else
			{	zSortHead = dbuf;
				dbuf->left = NULL;
				dbuf->right = NULL;
			}
		}	else
		{	// Don't need to Z-Sort
			drawASyncObj(dbuf);
		}
	}	else
	{	// No material - so won't need to Z-Sort
		drawASyncObj(dbuf);
	}
}

void drawNewAnimObj(newobj *obj, float *matrix, uintf startFrame, uintf endFrame, float interpolation)
{	aSyncDrawBuffer *dbuf;
	allocbucket(aSyncDrawBuffer,dbuf,flags,Generic_MemoryFlag32,1024,"Async Draw Buffers");
	memcopy(dbuf->matrix,matrix,sizeof(dbuf->matrix));
	dbuf->obj = obj;
	dbuf->userPtr = obj->userPtr;
	dbuf->preDrawCallback = obj->preDrawCallback;
	dbuf->interpolation = interpolation;
	dbuf->startFrame = startFrame;
	dbuf->endFrame = endFrame;
	Material *m = obj->mtl;
	if (m)
	{	if (m->flags & mtl_zsort)
		{	// Insert into ZSort array
			if (zSortHead)
			{	aSyncDrawBuffer *node = zSortHead;
				aSyncDrawBuffer *lastnode;
				while (node!=NULL)
				{	lastnode = node;
					if (dbuf->matrix[mat_zpos]>node->matrix[mat_zpos])
						node = node->right;
					else
						node=node->left;
				}
				if (dbuf->matrix[mat_zpos]>lastnode->matrix[mat_zpos])
					lastnode->right = dbuf;
				else
					lastnode->left = dbuf;
				dbuf->left = NULL;
				dbuf->right = NULL;
			}	else
			{	zSortHead = dbuf;
				dbuf->left = NULL;
				dbuf->right = NULL;
			}
		}	else
		{	// Don't need to Z-Sort
			drawASyncObj(dbuf);
		}
	}	else
	{	// No material - so won't need to Z-Sort
		drawASyncObj(dbuf);
	}
}

/*
obj3d *newobj3d(const char *name, vertexFormat vertdef, uintf numfaces, uintf numverts, uintf numframes, Material *mtl,uintf flags)
void createQuad(obj3d *obj, uintf startVert, uintf startFace,
				float x, float y, float z,
				uintf rotX, uintf rotY, uintf rotZ,
				float width, float height,
				float u1, float u2, float v1, float v2)
//void centerobj(obj3d *obj)
void calcRadius(obj3d *obj)
//void calcnormals(obj3d *o)
//void calcnormals2(obj3d *o)
//void calcnormals3(obj3d *o)
//void tangent( float3 *v0, float3 *v1, float3 *v2, float2 *t0, float2 *t1, float2 *t2, float3 &tg)
//void calcTangentBinormals(obj3d *o, uintf TangentChannel, uintf BinormalChannel)
//void getfacenormal(obj3d *obj, long facenum, float3 *n)
//void reorientobj(obj3d *obj,float *curmat)
//void deleteobj3d(obj3d *obj)
//obj3d *createplane(float width, float depth, uintf wseg, uintf dseg, Material *mtl)
//obj3d *createbox(float width, float height, float depth,uintf wseg,uintf hseg, uintf dseg,Material *mtl)
//obj3d *createcylinder(float height, float radius, uintf segments, Material *mtl, dword flags)
//obj3d *createsphere(float radius, uintf segments, Material *mtl)
//bool collide(collideData *data)
cHeightMap *newheightmap(const char *filename,dword cellsize, float heightscale, float worldscale, Material *mtl, uintf numtexchans)
//void groupdimensions(group3d *scene)
//group3d *newgroup3d(char *filename,dword flags)
//cBlur *newBlur(uintf size, float brightness)
//sPlane PlaneFromMatrixX(float *mat)
//sPlane PlaneFromMatrixY(float *mat)
//sPlane PlaneFromMatrixZ(float *mat)
*/

#ifdef fcdebug
#define debugTestLock(obj,testflags)	\
	if ((obj->flags & (testflags))!=(testflags)) msg("Debug Error","Mesh is not adequately locked");
#else
#define debugTestLock(obj,flags)
#endif

const float EPSILON  =  1.0e-5f;		// Tolerance for FLOATs

char			defaultobjname[]="Unnamed";
videoinfostruct	videoinfodata;
//obj3d			*usedobj3d, *freeobj3d;

void initgeometry(void)
{	if (!_fcplugin) msg("Engine Initialization Failure","InitGeometry Called before Plugin System is Initialized");
}

void killgeometry(void)
{	killbucket(aSyncDrawBuffer, flags, Generic_MemoryFlag32);
}

// ****************************************************************************************
// ***							3D Object Creation										***
// ****************************************************************************************
/*
obj3d *newobj3d(const char *name, vertexFormat vertdef, uintf numfaces, uintf numverts, uintf numframes, Material *mtl,uintf flags)
{
msg("newobj3d","This function is depricated and removed");
return NULL;

	const char *framecompression = newobjcompression;
	obj3d *obj;
	allocbucket(obj3d, obj, flags, obj_memstruct, 128, "Object Cache");

	// Allocate Material
	if (mtl==NULL)
	{	mtl = newmaterial(name);
		mtl->parent = obj;
		obj->flags|=obj_memmaterial;
	}

	obj->name = name;

	// Allocate memory if necessary for vertex and face data
	uintf memneeded = 0;

	uintf staticVertBufSize = getVertexSize(vertdef);
	memneeded = numverts * staticVertBufSize;

	memneeded += sizeof(Face) * numfaces;
	byte *buf = fcalloc(memneeded,"Geometry Buffer");
	memfill(buf, 0, memneeded);
	obj->staticVertices = newVertexBuffer(vertdef, numverts, 1, buf, 0);	buf += numverts * staticVertBufSize;
	obj->face = (Face *)buf;												buf += numfaces * sizeof(Face);

	vd_newobj3d(obj, vertdef, numfaces, numverts, flags);

	// initialize animation data
	obj->numkeyframes = numframes;
	obj->frame1		= 0;
	obj->frame2		= 1;
	obj->interpolation = 0.0f;
	obj->decodeframe = NULL;
	obj->animcachecontrol = 0;
	if (numframes>1)
	{	msg("incomplete code","animating mesh not yet supported in flameVM render core");
		/ *
		if (!framecompression) msg("Invalid Object Data",buildstr("An attempt was made to create a new animating 3D Object (%s),\n but no mesh codec was specified\n(Call to newobj3d, framecompression parameter = NULL)",name));
		AnimMeshCodecType codec = (AnimMeshCodecType) getPluginHandler(framecompression,PluginType_AnimMeshCodec);
		obj->MeshCodec = codec;
		if (!codec) msg("Missing Animmesh Codec",buildstr("Can not create an animating object (%s) using the %s codec, as it either does not exist or was not initialized",name,framecompression));
		uintf memneeded;
		if (!codec(AnimMesh_GetMemRequirements, obj, &memneeded, 0))
		{	msg("Error in AnimMesh Codec",buildstr("Error retrieving memory requirements for object %s using the %s codec",name,framecompression));
		}
		obj->animdata = fcalloc(memneeded,buildstr("%s animation",name));
		if (!codec(AnimMesh_GetDecoder,&obj->decodeframe,NULL,0))
		{	msg("Error in AnimMesh Codec",buildstr("Error retrieving decoder function for object %s using the %s codec",name,framecompression));
		}
		obj->flags|=obj_mempverts;
		* /
	}

	// initialize drawing style and position
	obj->wirecolor = 0xffff00ff;
	if (mtl==NULLmtl)
		obj->material = NULL;
	else
		obj->material = mtl;
	obj->worldpos.x=0;
	obj->worldpos.y=0;
	obj->worldpos.z=0;
	obj->matrix = NULL;
	obj->width = 1;
	obj->height = 1;
	obj->depth = 1;
	obj->radius = 1;
	obj->radius_squared = 1;

	// Initialize Lighting
	obj->lighting = NULL;
	obj->maxlights = 8; // ###

	// User Settings
	obj->animsize = 0;
	obj->flags |= flags;
	obj->zsDrawFunc = NULL;
	obj->uset = 0;
	obj->parent = NULL;
	return obj;
}
*/

/*
obj3d *newobj3d(const char *name, const char *vertdef, uintf numfaces, uintf numverts, uintf numframes, Material *mtl, uintf flags)
{	//vertexFormat cvf = compileVertexFormat(vertdef);
//	return newobj3d(name, cvf, numfaces, numverts, numframes, mtl, flags);
msg("newobj3d","This function is depricated and removed");
return NULL;
}
*/
/*
obj3d *newobj3d(const char *name, _sObjCreateInfo *objinfo,long numframes, char *framecompression, Material *mtl)
{	// ### This function is depricated!!!
	uint8 vertdef[64];
	vertdef[0]=vertexUsage_position;
	vertdef[1]=vertexData_float + 3;
	vertdef[2]=vertexUsage_normal;
	vertdef[3]=vertexData_float + 3;
	uintf ofs = 4;
	for (uintf i=0; objinfo->NumTexComponents[i]>0; i++)
	{	vertdef[ofs++]=vertexUsage_texture;
		vertdef[ofs++]=vertexData_float + (uint8)objinfo->NumTexComponents[i];
	}
	vertdef[ofs++]=0;
	return newobj3d(name, (uint8 *)vertdef, objinfo->numfaces, objinfo->numverts, numframes, mtl, objinfo->flags);
}
*/

// ****************************************************************************************
// ***							Geometry Building Functions								***
// ****************************************************************************************
/*
void createQuad(obj3d *obj, uintf startVert, uintf startFace,	// Object pointer, and where to start with vertices and faces
				float x, float y, float z,						// Center of the quad
				uintf rotX, uintf rotY, uintf rotZ,				// Rotation in 1/10ths of a degree, Rotation order is Y,X,Z (FPS style)
				float width, float height,						// Dimensions of the quad
				float u1, float u2, float v1, float v2)			// Range for the texture map co-ordinates
{	sVertexElement position,normal,texture;
	float3	pos,transPos;
	uint8	*v,*n,*t;

	// Obtain position pointer
	findVertexElement(obj->staticVertices, vertexUsage_position, 0, &position);
	v = (uint8 *)position.pointer + startVert * position.bufferSize;

	// Obtain normal pointer
	findVertexElement(obj->staticVertices, vertexUsage_normal, 0, &normal);
	n = (uint8 *)normal.pointer + startVert * normal.bufferSize;

	// Obtain texture pointer
	findVertexElement(obj->staticVertices, vertexUsage_texture, 0, &texture);
	t = (uint8 *)texture.pointer + startVert * texture.bufferSize;

	Matrix m;
	makeidentity(m);
	rotatearoundy(m,rotY);
	rotatearoundx(m,rotX);
	rotatearoundz(m,rotZ);
	fixrotations(m);
	translatematrix(m,x,y,z);

	pos.z = 0;

	width *= 0.5f;
	height *= 0.5f;

	// Vertex 0
	// Vertex Position
	pos.x = -width;
	pos.y = height;
	transformPointByMatrix(&transPos, m, &pos);
	writeVertexElement(v, &position, transPos.x, transPos.y, transPos.z, 0);
	v += position.bufferSize;
	// Vertex Normal
	writeVertexElement(n, &normal, m[mat_zvecy], m[mat_zvecx], m[mat_zvecz], 0);
	n += normal.bufferSize;
	// Texture Co-ordinate
	writeVertexElement(t, &texture, u2, v1, 0, 0);
	t += texture.bufferSize;

	// Vertex 1
	pos.x = width;
	pos.y = -height;
	transformPointByMatrix(&transPos, m, &pos);
	writeVertexElement(v, &position, transPos.x, transPos.y, transPos.z, 0);
	v += position.bufferSize;
	writeVertexElement(n, &normal, m[mat_zvecy], m[mat_zvecx], m[mat_zvecz], 0);
	n += normal.bufferSize;
	writeVertexElement(t, &texture, u1, v2, 0, 0);
	t += texture.bufferSize;

	// Vertex 2
	pos.x = -width;
	pos.y = -height;
	transformPointByMatrix(&transPos, m, &pos);
	writeVertexElement(v, &position, transPos.x, transPos.y, transPos.z, 0);
	v += position.bufferSize;
	writeVertexElement(n, &normal, m[mat_zvecy], m[mat_zvecx], m[mat_zvecz], 0);
	n += normal.bufferSize;
	writeVertexElement(t, &texture, u2, v2, 0, 0);
	t += texture.bufferSize;

	// Vertex 3
	pos.x = width;
	pos.y = height;
	transformPointByMatrix(&transPos, m, &pos);
	writeVertexElement(v, &position, transPos.x, transPos.y, transPos.z, 0);
	v += position.bufferSize;
	writeVertexElement(n, &normal, m[mat_zvecy], m[mat_zvecx], m[mat_zvecz], 0);
	n += normal.bufferSize;
	writeVertexElement(t, &texture, u1, v1, 0, 0);
	t += texture.bufferSize;

	Face *face = obj->face;
	face[startFace+0].v1 = startVert+0;
	face[startFace+0].v2 = startVert+1;
	face[startFace+0].v3 = startVert+2;

	face[startFace+1].v1 = startVert+0;
	face[startFace+1].v2 = startVert+3;
	face[startFace+1].v3 = startVert+1;
}
*/

// ****************************************************************************************
// ***								Geometry Cleanup									***
// ****************************************************************************************
/*
void centerobj(obj3d *obj)
{	// Centers an object, makes it easier to cull from the scene (improves overall speed)
	float3 *vert;
	dword count;
	float minx,maxx,miny,maxy,minz,maxz,cx,cy,cz;
	float3 *pvert = (float3 *)obj->animdata;

	if (obj->numkeyframes==1)
	{	vert = (float3 *)obj->vertpos;
		if (!vert) msg("Object Locking Error","'centerobj' called on an object which was not locked");
		minx = vert->x;
		maxx = vert->x;
		miny = vert->y;
		maxy = vert->y;
		minz = vert->z;
		maxz = vert->z;
		for (count=1; count<obj->numverts; count++)
		{	vert = (float3 *)incVertexPtr(vert, obj);
			if (minx>vert->x) minx = vert->x;
			if (maxx<vert->x) maxx = vert->x;
			if (miny>vert->y) miny = vert->y;
			if (maxy<vert->y) maxy = vert->y;
			if (minz>vert->z) minz = vert->z;
			if (maxz<vert->z) maxz = vert->z;
		}
	}	else
	{	msg("Imcomplete Engine Function","'centerobj' does not support animated models as yet");
		minx = pvert[0].x;
		maxx = pvert[0].x;
		miny = pvert[0].y;
		maxy = pvert[0].y;
		minz = pvert[0].z;
		maxz = pvert[0].z;
		for (count=1; count<obj->numverts*obj->numkeyframes; count++)
		{	if (minx>pvert[count].x) minx = pvert[count].x;
			if (maxx<pvert[count].x) maxx = pvert[count].x;
			if (miny>pvert[count].y) miny = pvert[count].y;
			if (maxy<pvert[count].y) maxy = pvert[count].y;
			if (minz>pvert[count].z) minz = pvert[count].z;
			if (maxz<pvert[count].z) maxz = pvert[count].z;
		}
	}

	cx = (minx+maxx)/2;
	cy = (miny+maxy)/2;
	cz = (minz+maxz)/2;
	obj->worldpos.x += cx;
	obj->worldpos.y += cy;
	obj->worldpos.z += cz;

	if (obj->numkeyframes==1)
	{	vert = obj->vertpos;
		for (count=0; count<obj->numverts; count++)
		{	vert->x-=cx;
			vert->y-=cy;
			vert->z-=cz;
			vert = (float3 *)incVertexPtr(vert, obj);
		}
	}	else
	{	for (count=0; count<obj->numverts*obj->numkeyframes; count++)
		{	pvert[count].x-=cx;
			pvert[count].y-=cy;
			pvert[count].z-=cz;
		}
	}
}
*/

/*
bool calcDimensions(sVertexStreamDepricated *verts, dimensions3D *dimensions, uintf flags)
{	uintf numFrames;
	if (flags&1) numFrames = verts->numKeyFrames; else numFrames=1;

	// Step 1 - Determine vertex position in stream
	bool failed = false;
	sVertexStreamDescription *vsd = verts->desc;
	sVertexStreamElement element;
	if (!getVertexStreamOffset(vsd, vertexUsage_position, 0, &element)) failed=true;
	if ((element.dataType & 0x0f)!=3) failed = true;		// Must have 3 elements!
	if ((element.dataType & 0xf0)!= vertexData_float) failed = true;
	if (verts->numVerts==0) failed=true;
	if (failed)
	{	return false;
	}

	uintf offset = element.offset;
	uintf vertexStride = element.vertexStride;

	// Step 2 - Set up initial minimum and maximum values
	byte *ptr8 = ((byte *)(verts->vertices))+offset;
	float3 *pos = (float3 *)ptr8;
	dimensions->min.x = dimensions->max.x = pos->x;
	dimensions->min.y = dimensions->max.y = pos->y;
	dimensions->min.z = dimensions->max.z = pos->z;
	ptr8 += vertexStride;

	// Step 3 - Check each vertex for max position
	for (uintf i=1; i<verts->numVerts; i++)
	{	pos = (float3 *)ptr8;
		if (pos->x > dimensions->max.x) dimensions->max.x = pos->x;
		if (pos->y > dimensions->max.y) dimensions->max.y = pos->y;
		if (pos->z > dimensions->max.z) dimensions->max.z = pos->z;
		if (pos->x < dimensions->min.x) dimensions->min.x = pos->x;
		if (pos->y < dimensions->min.y) dimensions->min.y = pos->y;
		if (pos->z < dimensions->min.z) dimensions->min.z = pos->z;
		ptr8 += vertexStride;
	}
	dimensions->width = dimensions->max.x - dimensions->min.x;
	dimensions->height= dimensions->max.y - dimensions->min.y;
	dimensions->depth = dimensions->max.z - dimensions->min.z;
	dimensions->center.x = dimensions->min.x - (dimensions->min.x/2.0f);
	dimensions->center.y = dimensions->min.y - (dimensions->min.y/2.0f);
	dimensions->center.z = dimensions->min.z - (dimensions->min.z/2.0f);
	float rx = dimensions->width/2;
	float ry = dimensions->height/2;
	float rz = dimensions->depth/2;
	dimensions->radiusSquared = rx*rx + ry*ry + rz*rz;
	dimensions->radius = sqrt(dimensions->radiusSquared);
	return true;
}
*/

// ****************************************************************************************
// ***								Normal Vector Generation							***
// ****************************************************************************************
/*
void calcNormals(newobj *o)		// // Fast normal calculation, smooths all vertices, only works with simpleVertexDescription
{	sVertexStream *v = o->verts;
	sFaceStream *f = o->faces;
	uint16 *face16 = (uint16 *)(f->faces);
	sSimpleVertex *vertSmp = (sSimpleVertex *)(v->vertices);
	uintf faceIndex = 0;
	for (intf i=0; i<v->numVerts; i++)
	{	vertSmp[i].nx=0;
		vertSmp[i].ny=0;
		vertSmp[i].nz=0;
	}
	for (intf i=0; i<f->numFaces; i++)
	{	sSimpleVertex *vtx0 = &vertSmp[face16[faceIndex++]];
		sSimpleVertex *vtx1 = &vertSmp[face16[faceIndex++]];
		sSimpleVertex *vtx2 = &vertSmp[face16[faceIndex++]];

		// Generate Vectors
		float3 vec1,vec2;
		vec1.x = vtx1->x - vtx0->x;
		vec1.y = vtx1->y - vtx0->y;
		vec1.z = vtx1->z - vtx0->z;
		vec2.x = vtx2->x - vtx0->x;
		vec2.y = vtx2->y - vtx0->y;
		vec2.z = vtx2->z - vtx0->z;

		// Cross Product the two vectors
		float3 n;
		crossProduct(&n,&vec1,&vec2);

		// Normalize the vector
		normalize(&n);

		// Assign this normal to each vertex it was calculated from
		vtx0->nx -= n.x;
		vtx0->ny -= n.y;
		vtx0->nz -= n.z;

		vtx1->nx -= n.x;
		vtx1->ny -= n.y;
		vtx1->nz -= n.z;

		vtx2->nx -= n.x;
		vtx2->ny -= n.y;
		vtx2->nz -= n.z;
	}
	for (intf i=0; i<v->numVerts; i++)
	{	normalize((float3*)&vertSmp[i].nx);
	}
}
*/
/*
void calcnormals(obj3d *o)
{	// Fast normal calculation, smooths all vertices
	float	v1x,v1y,v1z, v2x,v2y,v2z, nx,ny,nz;
	float3	*v0,*v1,*v2,*n0,*n1,*n2;
	Face	*f;
	uintf i;

	if (!o->vertpos)
		msg("Object Locking Error","'calcnormals' called on an object which was not locked");

	for (i=0; i<o->numverts; i++)
	{	float3 *n = (float3 *)getVertexPtr(o, vertnorm, i);//&o->vert[i];
		n->x=0;
		n->y=0;
		n->z=0;
	}

	for (i=0; i<o->numfaces; i++)
	{	f  = o->face;

		v0 = (float3 *)getVertexPtr(o, vertpos, f[i].v1);
		v1 = (float3 *)getVertexPtr(o, vertpos, f[i].v2);
		v2 = (float3 *)getVertexPtr(o, vertpos, f[i].v3);
		n0 = (float3 *)getVertexPtr(o, vertnorm, f[i].v1);
		n1 = (float3 *)getVertexPtr(o, vertnorm, f[i].v2);
		n2 = (float3 *)getVertexPtr(o, vertnorm, f[i].v3);

		// Calculate 2 vectors from the plane
		v1x = v1->x - v0->x;
		v1y = v1->y - v0->y;
		v1z = v1->z - v0->z;

		v2x = v2->x - v0->x;
		v2y = v2->y - v0->y;
		v2z = v2->z - v0->z;

		// Use those 2 vectors to calculate a normal
		_crossproduct(n,v1,v2);

		// Normalize the normal vector (make length = 1)
		_normalize(n);

		// Assign this normal to each vertex it was calculated from
		n0->x += nx;
		n0->y += ny;
		n0->z += nz;

		n1->x += nx;
		n1->y += ny;
		n1->z += nz;

		n2->x += nx;
		n2->y += ny;
		n2->z += nz;
	}

	for (i=0; i<o->numverts; i++)
	{	float3 *n = (float3 *)getVertexPtr(o, vertnorm, i);
		float d = -1.0f/(float)sqrt(n->x*n->x + n->y*n->y + n->z*n->z);
		n->x*=d;
		n->y*=d;
		n->z*=d;
	}
}
*/
/*
bool findUniquePoints(obj3d *obj, void *buffer, uintf bufsize, Face **faces)
{	// Given an obj3d, generate a series of unique positions.  That is, all vertices with different normals
	// and texture co-ordinates, which occupy the same point in 3D space, are welded together.  An appropriate
	// face list is also generated to help match up points with the original vertices.
	// This information can be useful during the calculation of shadow volumes, or cell shading
	uintf memneeded = sizeof(float3)*2*obj->numverts + sizeof(Face)*obj->numfaces;
	if (memneeded < bufsize) return false;
}
*/
/*
void calcnormals3(obj3d *o)
{	uintf faceIndex, vertIndex;

	if (!o->vertpos)
		msg("Object Locking Error","'calcnormals2' called on an object which was not locked");
	intf	*vertmatch = (intf *)alloctempbuffer(o->numverts * sizeof(intf),"CalcNormals2");

	// Clear normals and match up close vertices
	for (vertIndex=0; vertIndex<o->numverts; vertIndex++)
	{	float3	*norm = (float3 *)getVertexPtr(o, vertnorm, vertIndex);
		float3 *thisvert = (float3 *)getVertexPtr(o, vertpos, vertIndex);
		vertmatch[vertIndex] = -1;
		norm->x = 0.0f;
		norm->y = 0.0f;
		norm->z = 0.0f;

		for (uintf i=0; i<vertIndex; i++)
		{	float3 *testvert = (float3 *)getVertexPtr(o, vertpos, i);
			float3 dif;
			dif.x = testvert->x - thisvert->x;
			dif.y = testvert->y - thisvert->y;
			dif.z = testvert->z - thisvert->z;
			if ( (dif.x * dif.x + dif.y * dif.y + dif.z * dif.z) < 0.0001f)
			{	vertmatch[vertIndex] = i;
				break;
			}
		}
	}

	for (faceIndex=0; faceIndex<o->numfaces; faceIndex++)
	{	float3	*v1 = (float3 *)getVertexPtr(o, vertpos, o->face[faceIndex].v1);
		float3	*v2 = (float3 *)getVertexPtr(o, vertpos, o->face[faceIndex].v2);
		float3	*v3 = (float3 *)getVertexPtr(o, vertpos, o->face[faceIndex].v3);
		float3	vec1,vec2;
		vec1.x = v2->x - v1->x;
		vec1.y = v2->y - v1->y;
		vec1.z = v2->z - v1->z;
		vec2.x = v3->x - v1->x;
		vec2.y = v3->y - v1->y;
		vec2.z = v3->z - v1->z;
		float3 n;
		crossProduct(&n,&vec1,&vec2);
		normalize(&n);
		v1[1].x += n.x;
		v1[1].y += n.y;
		v1[1].z += n.z;
		v2[1].x += n.x;
		v2[1].y += n.y;
		v2[1].z += n.z;
		v3[1].x += n.x;
		v3[1].y += n.y;
		v3[1].z += n.z;
	}

	// Sum normals on close vertices, then normalize
	for (vertIndex=0; vertIndex<o->numverts; vertIndex++)
	{	float3 *thisvert = (float3 *)getVertexPtr(o, vertnorm, vertIndex);
		if (vertmatch[vertIndex]>=0)
		{	float3	*thatvert = (float3 *)getVertexPtr(o, vertnorm, vertmatch[vertIndex]);
			thisvert->x += thatvert->x;
			thisvert->y += thatvert->y;
			thisvert->z += thatvert->z;
		}
		normalize(thisvert);
	}

	freetempbuffer(vertmatch);
}
*/
/*
void calcnormals2(obj3d *o)
{	float	v1x,v1y,v1z, v2x,v2y,v2z, nx,ny,nz;
	float3	*v,*v0p,*v0n,*v1p,*v1n,*v2p,*v2n;
	Face	*f;
	dword	i,j,numpoints;
	if (!o->vertpos)
		msg("Object Locking Error","'calcnormals2' called on an object which was not locked");
	byte *buf = alloctempbuffer(o->numverts * (2+sizeof(float3)*2),"Calcnormals2");
	word *p2v = (word *)buf; buf+=o->numverts*2;
	float3 *pointpos = (float3 *)buf; buf+=o->numverts*sizeof(float3);
	float3 *pointnrm = (float3 *)buf; buf+=o->numverts*sizeof(float3);

	numpoints=0;
	for (i=0; i<o->numverts; i++)
	{	v = (float3 *)getVertexPtr(o, vertpos, i);
		bool found=false;
		for (j=0; j<numpoints; j++)
		{	float dx = v->x-pointpos[j].x;
			float dy = v->y-pointpos[j].y;
			float dz = v->z-pointpos[j].z;
			float diff = dx*dx + dy*dy + dz*dz;
			if (diff<0.0001f)
			{	// Dis point already exists
				p2v[i]=(word)j;
				found=true;
				break;
			}
		}
		if (!found)
		{	// Ah needs a new point in 3D space
			p2v[i]=(word)numpoints;
			pointpos[numpoints++] = *v;
		}
	}

	for (i=0; i<numpoints; i++)
	{	pointnrm[i].x=0;
		pointnrm[i].y=0;
		pointnrm[i].z=0;
	}

	uintf dodgeypolys = 0;

	for (i=0; i<o->numfaces; i++)
	{	f  = o->face;
		uint32 p1 = p2v[f[i].v1];
		uint32 p2 = p2v[f[i].v2];
		uint32 p3 = p2v[f[i].v3];

		if (p1==p2 || p1==p3 || p2==p3)
		{	dodgeypolys++;
			continue;	// The normal from this face will be of infinite size
		}

		v0p = &pointpos[p1];	v0n = &pointnrm[p1];
		v1p = &pointpos[p2];	v1n = &pointnrm[p2];
		v2p = &pointpos[p3];	v2n = &pointnrm[p3];

		// Calculate 2 vectors from the plane
		v1x = v0p->x - v1p->x;
		v1y = v0p->y - v1p->y;
		v1z = v0p->z - v1p->z;

		v2x = v0p->x - v2p->x;
		v2y = v0p->y - v2p->y;
		v2z = v0p->z - v2p->z;

		// Use those 2 vectors to calculate a normal
		_normalize(v1);
		_normalize(v2);
		_crossproduct(n,v1,v2);

		// Normalize the normal vector (make length = 1)
		_normalize(n);

		// Assign this normal to each vertex it was calculated from
		v0n->x += nx;
		v0n->y += ny;
		v0n->z += nz;

		v1n->x += nx;
		v1n->y += ny;
		v1n->z += nz;

		v2n->x += nx;
		v2n->y += ny;
		v2n->z += nz;
	}

	if (dodgeypolys)
		errorslog->log("Warning: Calcnormals2 detected %i non-displayable polygon(s) in object %s",dodgeypolys, o->name);
	uintf dodgeynorms = 0;

	for (i=0; i<numpoints; i++)
	{	float len = (float)sqrt(pointnrm[i].x*pointnrm[i].x + pointnrm[i].y*pointnrm[i].y + pointnrm[i].z*pointnrm[i].z);
		if (len<0.01f) dodgeynorms++;
		len = 1.0f / len;
		pointnrm[i].x*=len;
		pointnrm[i].y*=len;
		pointnrm[i].z*=len;
	}
	if (dodgeynorms)
		errorslog->log("Warning: Calcnormals2 was unable to calculate normals for %i point(s) in object %s",dodgeynorms, o->name);


	float3 *n = (float3 *)o->vertnorm;
	for (i=0; i<o->numverts; i++)
	{	dword index = p2v[i];
		n->x = -pointnrm[index].x;
		n->y = -pointnrm[index].y;
		n->z = -pointnrm[index].z;
		n = (float3 *)incVertexPtr(n, o);
	}
	freetempbuffer(p2v);
}
*/

void tangent( float3 *v0, float3 *v1, float3 *v2, float2 *t0, float2 *t1, float2 *t2, float3 &tg)
{
	float3 edge0,edge1;
	float2 cross;

	edge0.x = v1->x - v0->x;
	edge0.y = t1->x - t0->x;
	edge0.z = t1->y - t0->y;

	edge1.x = v2->x - v0->x;
	edge1.y = t2->x - t0->x;
	edge1.z = t2->y - t0->y;

	cross.x = edge0.y*edge1.z - edge0.z*edge1.y;
	cross.y = edge0.z*edge1.x - edge0.x*edge1.z;
	tg.x = - cross.y / cross.x;

	edge0.x = v1->y - v0->y;
	edge1.x = v2->y - v0->y;
	cross.x = edge0.y*edge1.z - edge0.z*edge1.y;
	cross.y = edge0.z*edge1.x - edge0.x*edge1.z;
	tg.y = - cross.y / cross.x;

	edge0.x = v1->z - v0->z;
	edge1.x = v2->z - v0->z;
	cross.x = edge0.y*edge1.z - edge0.z*edge1.y;
	cross.y = edge0.z*edge1.x - edge0.x*edge1.z;
	tg.z = - cross.y / cross.x;
}
/*
void calcTangentBinormals(obj3d *o, uintf TangentChannel, uintf BinormalChannel)
{	uintf i;
	if (!o->vertpos)
		msg("Object Locking Error","'calcTangentBinormals' called on an object which was not locked");

	float3 *tan = (float3*)o->texcoord[TangentChannel];
	float3 *bin = (float3*)o->texcoord[BinormalChannel];
	for (i=0; i<o->numverts; i++);
	{	tan->x = 0.0f;	tan->y = 0.0f;	tan->z = 0.0f;	tan = (float3 *)incVertexPtr(tan, o);
		bin->x = 0.0f;	bin->y = 0.0f;	tan->z = 0.0f;	bin = (float3 *)incVertexPtr(tan, o);
	}

	uintf posofs = o->posofs;
	uintf texofs = o->texofs[0];
	uintf tanofs = o->texofs[TangentChannel];
	uintf binofs = o->texofs[BinormalChannel];

	for (i=0; i<o->numfaces; i++)
	{	float *v0 = (float *)getVertexPtr(o, staticVertices->data, o->face[i].v1);
		float *v1 = (float *)getVertexPtr(o, staticVertices->data, o->face[i].v2);
		float *v2 = (float *)getVertexPtr(o, staticVertices->data, o->face[i].v3);
		if (v0==v1 || v0==v2 || v1==v2)
		{	continue;						// This is a degenerate triangle
		}
		float3 *v0Pos = (float3 *)&v0[posofs];
		float3 *v1Pos = (float3 *)&v1[posofs];
		float3 *v2Pos = (float3 *)&v2[posofs];

		float2 *v0UV  = (float2 *)&v0[texofs];
		float2 *v1UV  = (float2 *)&v1[texofs];
		float2 *v2UV  = (float2 *)&v2[texofs];

		float3 *v0Tan = (float3 *)&v0[tanofs];
		float3 *v1Tan = (float3 *)&v1[tanofs];
		float3 *v2Tan = (float3 *)&v2[tanofs];

		float3 *v0Bin = (float3 *)&v0[binofs];
		float3 *v1Bin = (float3 *)&v1[binofs];
		float3 *v2Bin = (float3 *)&v2[binofs];

		float3 FaceNormal;
		float3 vec1, vec2;
		vecSubtract(&vec1, v1Pos, v0Pos);
		vecSubtract(&vec2, v2Pos, v0Pos);
//		_normalize(vec1.);
//		_normalize(vec2.);
		crossProduct(&FaceNormal, &vec1, &vec2);
		normalize(&FaceNormal);

		float3 FaceTangent;
		tangent( v0Pos, v1Pos, v2Pos,
				 v0UV,  v1UV,  v2UV,
				 FaceTangent );
		normalize(&FaceTangent);

		float3 FaceBinormal;
		crossProduct(&FaceBinormal,&FaceNormal,&FaceTangent);
		normalize(&FaceBinormal);

		v0Tan->x += FaceTangent.x; v0Tan->y += FaceTangent.y; v0Tan->z += FaceTangent.z;
		v1Tan->x += FaceTangent.x; v1Tan->y += FaceTangent.y; v1Tan->z += FaceTangent.z;
		v2Tan->x += FaceTangent.x; v2Tan->y += FaceTangent.y; v2Tan->z += FaceTangent.z;

		v0Bin->x += FaceBinormal.x; v0Bin->y += FaceBinormal.y;	v0Bin->z += FaceBinormal.z;
		v1Bin->x += FaceBinormal.x; v1Bin->y += FaceBinormal.y;	v1Bin->z += FaceBinormal.z;
		v2Bin->x += FaceBinormal.x; v2Bin->y += FaceBinormal.y;	v2Bin->z += FaceBinormal.z;
	}

	tan = (float3*)o->texcoord[TangentChannel];
	bin = (float3*)o->texcoord[BinormalChannel];
	for (i=0; i<o->numverts; i++);
	{	normalize(tan);	tan = (float3 *)incVertexPtr(tan, o);
		normalize(bin);	bin = (float3 *)incVertexPtr(bin, o);
	}
}
*/
// Scale an object (FC does not appreciate scaling through matricies, so do it through verticies)
/*
void scaleobj3dnu(obj3d *obj, float factorx, float factory, float factorz)		// Scale an object non-uniformly
{	// ### Depricated - scale through vertex shaders instead
	dword count;
	msg("Imcomplete Function","scaleobj3dnu has not been updated to the V4 render core");
	if (obj->numkeyframes==1)
	{	for (count=0; count<obj->numverts; count++)
		{	obj->vertpos[count].x*=factorx;
			obj->vertpos[count].y*=factory;
			obj->vertpos[count].z*=factorz;
		}
	}	else
	{	for (dword i=0; i<obj->numkeyframes; i++)
		{	obj->frame1 = i;
			obj->frame2 = i;
			obj->interpolation = 0;
			obj->decodeframe(obj);
			for (count=0; count<obj->numverts; count++)
			{	obj->vertpos[count].x*=factorx;
				obj->vertpos[count].y*=factory;
				obj->vertpos[count].z*=factorz;
			}
			//compressobj3dframe(obj,i);
		}
	}
	obj->worldpos.x *= factorx;
	obj->worldpos.y *= factory;
	obj->worldpos.z *= factorz;
	calcRadius(obj);
}
*/

/*
void scaleobj3d(obj3d *obj, float factor)
{	// ### Depricated - scale through vertex shaders instead
	msg("Imcomplete Function","scaleobj3d has not been updated to the V4 render core");
	scaleobj3dnu(obj, factor, factor, factor);
}
*/
/*
void getfacenormal(obj3d *obj, long facenum, float3 *n)
{	float3 *v1, *v2, *v3;
	msg("Imcomplete Function","getfacenormal has not been updated to the V4 render core");
	Face *f = &obj->face[facenum];

	v1 = &obj->vertpos[f->v1];
	v2 = &obj->vertpos[f->v2];
	v3 = &obj->vertpos[f->v3];
	float vec1x = v2->x - v1->x;
	float vec1y = v2->y - v1->y;
	float vec1z = v2->z - v1->z;
	float vec2x = v3->x - v1->x;
	float vec2y = v3->y - v1->y;
	float vec2z = v3->z - v1->z;

	// Crossproduct
	n->x = vec1y * vec2z - vec1z * vec2y;
	n->y = vec1z * vec2x - vec1x * vec2z;
	n->z = vec1x * vec2y - vec1y * vec2x;

	// Normalize
	float len = -1.0f/(float)sqrt(n->x*n->x + n->y*n->y + n->z*n->z);
	n->x *= len;
	n->y *= len;
	n->z *= len;
}
*/
/*
void reorientobj(obj3d *obj,float *curmat)
{	// Reorient the object according to the current matrix
	if (obj->numkeyframes>1)
	{	// Multiple frames
		msg("Incomplete Engine Function","'reorientobj' does not support animated models as yet");
/ *		for (dword frame=0; frame<obj->numkeyframes; frame++)
		{	dword index = frame*obj->numverts;
			for (dword vert=0; vert<obj->numverts; vert++)
			{	vertex1 *pv = &obj->pvert[index+vert];
				float nx = pv->x * curmat[ 0] + pv->y * curmat[ 4] + pv->z * curmat[ 8] + curmat[12];
				float ny = pv->x * curmat[ 1] + pv->y * curmat[ 5] + pv->z * curmat[ 9] + curmat[13];
				float nz = pv->x * curmat[ 2] + pv->y * curmat[ 6] + pv->z * curmat[10] + curmat[14];
				pv->x = nx;
				pv->y = ny;
				pv->z = nz;
			}
		}
* /	}
	// Single frame	(Also need to do this for multiple frames so normal recalculation works later on
	for (uintf vert=0; vert<obj->numverts; vert++)
	{	float3 *pv = (float3 *)getVertexPtr(obj, vertpos, vert);//&obj->vert[vert];
		float nx = pv->x * curmat[ 0] + pv->y * curmat[ 4] + pv->z * curmat[ 8] + curmat[12];
		float ny = pv->x * curmat[ 1] + pv->y * curmat[ 5] + pv->z * curmat[ 9] + curmat[13];
		float nz = pv->x * curmat[ 2] + pv->y * curmat[ 6] + pv->z * curmat[10] + curmat[14];
		pv->x = nx;
		pv->y = ny;
		pv->z = nz;
	}
}
*/
/*
void deleteobj3d(obj3d *obj)
{	if (obj->flags & obj_meshlocked) unlockmesh(obj);
	vd_deleteobj3d(obj);
	if (obj->flags & obj_memmaterial)
		deletematerial(obj->material);
	if (obj->flags & obj_mempverts)
		fcfree(obj->animdata);
	deletebucket(obj3d, obj);
}
*/
// ********************************************************************
// ***						Object Creation Tools					***
// ********************************************************************

uintf addvertex(float x, float y, float z, float u, float v)
{	msg("Imcomplete Function","addvertex has not been updated to the V4 render core");
/*	tvertex *tex = &creationbuf->tvert[creationbuf->numverts];
	vertex *vtx = &creationbuf->vert[creationbuf->numverts];
	vtx->x = x;
	vtx->y = y;
	vtx->z = z;
	tex->u = u;
	tex->v = v;
	creationbuf->numverts++;
	return creationbuf->numverts-1;
*/
	return 0;
}

void addquad(uintf v1, uintf v2, uintf v3, uintf v4)
{	msg("Imcomplete Function","addquad has not been updated to the V4 render core");

/*	Face *f = &creationbuf->face[creationbuf->numfaces];
	f->v1 = v1;
	f->v2 = v2;
	f->v3 = v3;
	f++;
	f->v1 = v1;
	f->v2 = v3;
	f->v3 = v4;
	creationbuf->numfaces += 2;
*/
}

void addplane(float x1, float y1, float z1, float u, float v,
			  float uaxisx, float uaxisy, float uaxisz,
			  float vaxisx, float vaxisy, float vaxisz,
			  float texu,	float texv,	  float texdu, float texdv)
{	msg("Imcomplete Function","addplane has not been updated to the V4 render core");
/*	dword vofs = creationbuf->numverts;
	vertex *vtx = &creationbuf->vert[vofs];
	tvertex *tv = &creationbuf->tvert[vofs];
	Face *f	  = &creationbuf->face[creationbuf->numfaces];
	vtx[0].x = x1;
	vtx[1].x = x1+vaxisx*v;
	vtx[2].x = x1+uaxisx*u+vaxisx*v;
	vtx[3].x = x1+uaxisx*u;

	vtx[0].y = y1;
	vtx[1].y = y1+vaxisy*v;
	vtx[2].y = y1+uaxisy*u+vaxisy*v;
	vtx[3].y = y1+uaxisy*u;

	vtx[0].z = z1;
	vtx[1].z = z1+vaxisz*v;
	vtx[2].z = z1+uaxisz*u+vaxisz*v;
	vtx[3].z = z1+uaxisz*u;

	tv[0].u = texu;			tv[0].v = texv;
	tv[1].u = texu;			tv[1].v = texv+texdv;
	tv[2].u = texu+texdu;	tv[2].v = texv+texdv;
	tv[3].u = texu+texdu;	tv[3].v = texv;

	f[0].v1 = vofs+0; f[0].v2 = vofs+1; f[0].v3 = vofs+3;
	f[1].v1 = vofs+3; f[1].v2 = vofs+1; f[1].v3 = vofs+2;

	creationbuf->numverts+=4;
	creationbuf->numfaces+=2;
*/
}

/*
obj3d *createplane(float width, float depth, uintf wseg, uintf dseg, Material *mtl)
{	msg("Incomplete Function","createplane has not been updated to the V4 render core");
	dword u,v;

	initobjcreation(mtl);
	float wsf = 1.0f / (float)wseg;
	float dsf = 1.0f / (float)dseg;
	float w=(width*0.5f);
	float d=(depth*0.5f);
	width  *= wsf;
	depth  *= dsf;
	for (u=0; u<wseg; u++)
		for (v=0; v<dseg; v++)
			addplane(-w+u*width, 0, d-v*depth, width, depth,	1, 0, 0,  0, 0,-1,u*wsf,v*dsf,wsf,dsf);
	calcRadius(creationbuf);
	calcnormals(creationbuf);
	return creationbuf;
}
*/
/*
obj3d *createbox(float width, float height, float depth,uintf wseg,uintf hseg, uintf dseg,Material *mtl)
{	msg("Incomplete Function","createbox has not been updated to the V4 render core");
	initobjcreation(mtl);

	dword u,v;
	float wsf = 1.0f / (float)wseg;
	float hsf = 1.0f / (float)hseg;
	float dsf = 1.0f / (float)dseg;

	float w=(width*0.5f);
	float h=(height*0.5f);
	float d=(depth*0.5f);

	width  *= wsf;
	height *= hsf;
	depth  *= dsf;

	// Front
	for (u=0; u<wseg; u++)
		for (v=0; v<hseg; v++)
			addplane(-w+u*width, h-v*height,-d, width, height,  1, 0, 0,  0,-1, 0,u*wsf,v*hsf,wsf,hsf);
	// Right
	for (u=0; u<dseg; u++)
		for (v=0; v<hseg; v++)
			addplane( w, h-v*height,-d+u*depth, depth, height,  0, 0, 1,  0,-1, 0,u*dsf,v*hsf,dsf,hsf);
	// Rear
	for (u=0; u<wseg; u++)
		for (v=0; v<hseg; v++)
			addplane( w-u*width, h-v*height, d, width, height, -1, 0, 0,  0,-1, 0,u*wsf,v*hsf,wsf,hsf);
	// Left
	for (u=0; u<dseg; u++)
		for (v=0; v<hseg; v++)
			addplane(-w, h-v*height, d-u*depth, depth, height,  0, 0,-1,  0,-1, 0,u*dsf,v*hsf,dsf,hsf);
	// Top
	for (u=0; u<wseg; u++)
		for (v=0; v<dseg; v++)
			addplane(-w+u*width, h, d-v*depth, width, depth,	1, 0, 0,  0, 0,-1,u*wsf,v*dsf,wsf,dsf);
	// Bottom
	for (u=0; u<wseg; u++)
		for (v=0; v<dseg; v++)
			addplane( w-u*width,-h, d-v*depth,-width, depth,	1, 0, 0,  0, 0,-1,u*wsf,v*dsf,wsf,dsf);

	calcRadius(creationbuf);
	calcnormals(creationbuf);
	return creationbuf;
}
*/
/*
obj3d *createcylinder(float height, float radius, uintf segments, Material *mtl, dword flags)
{	msg("Incomplete Function","createcylinder has not been updated to the V4 render core");

//	uintf i;
//	float nx,ny,nz;

	initobjcreation(mtl);

	height *= 0.5f;
	vertex *vtx = creationbuf->vert;
	tvertex *tv = creationbuf->tvert;
	Face *f		= creationbuf->face;

	float angledelta = 3600 / (float)segments;
	float currentangle = 0;
	float texudelta = 1.0f / (float) segments;
	float currenttexu = 0;

	for (i=0; i<=segments; i++)
	{	if (i==segments)
			currentangle = 0;

		vtx->x = mycos[(uintf)currentangle]*radius;
		vtx->y = height;
		vtx->z = mysin[(uintf)currentangle]*radius;
		tv->u  = currenttexu;
		tv->v  = 0.0f;
		vtx++;
		tv++;
		vtx->x = mycos[(uintf)currentangle]*radius;
		vtx->y =-height;
		vtx->z = mysin[(uintf)currentangle]*radius;
		tv->u  = currenttexu;
		tv->v  = 1.0f;
		vtx++;
		tv++;
		currentangle += angledelta;
		currenttexu += texudelta;
	}
	for (i=0; i<segments; i++)
	{	f[(i<<1)  ].v1 = ((i  )<<1);	// Top left
		f[(i<<1)  ].v2 = ((i  )<<1)+1;	// Bottom left
		f[(i<<1)  ].v3 = ((i+1)<<1)+1;	// Bottom right
		f[(i<<1)+1].v1 = ((i  )<<1);	// Top left
		f[(i<<1)+1].v2 = ((i+1)<<1)+1;	// Bottom right
		f[(i<<1)+1].v3 = ((i+1)<<1);	// Top right
	}
	uintf nextvert = (segments+1)<<1;
	uintf nextface = segments<<1;

	if ((flags & objcreate_DontCapTop) == 0)
	{	// Cap the top
		currentangle = 0;
		uintf startvert = nextvert;
		vtx = creationbuf->vert;
		tv  = creationbuf->tvert;
		f	= creationbuf->face;
		vtx[nextvert].x = 0;
		vtx[nextvert].y = height;
		vtx[nextvert].z = 0;
		tv[nextvert].u = 0.5f;
		tv[nextvert].v = 0.5f;
		nextvert++;
		for (i=0; i<segments; i++)
		{	vtx[nextvert].x = mycos[(uintf)currentangle]*radius;
			vtx[nextvert].y = height;
			vtx[nextvert].z = mysin[(uintf)currentangle]*radius;
			tv[nextvert].u = mycos[(uintf)currentangle] * 0.5f + 0.5f;
			tv[nextvert].v = mysin[(uintf)currentangle] * 0.5f + 0.5f;
			if (i>0)
			{	f[nextface].v1 = startvert;
				f[nextface].v2 = nextvert-1;
				f[nextface].v3 = nextvert;
				nextface++;
			}
			nextvert++;
			currentangle += angledelta;
		}
		f[nextface].v1 = startvert;
		f[nextface].v2 = nextvert-1;
		f[nextface].v3 = startvert+1;
		nextface++;
	}

	if ((flags & objcreate_DontCapBottom) == 0)
	{	// Cap the bottom
		currentangle = 0;
		uintf startvert = nextvert;
		vtx = creationbuf->vert;
		tv  = creationbuf->tvert;
		f	= creationbuf->face;
		vtx[nextvert].x = 0;
		vtx[nextvert].y =-height;
		vtx[nextvert].z = 0;
		tv[nextvert].u = 0.5f;
		tv[nextvert].v = 0.5f;
		nextvert++;
		for (i=0; i<segments; i++)
		{	vtx[nextvert].x = mycos[(uintf)currentangle]*radius;
			vtx[nextvert].y =-height;
			vtx[nextvert].z = mysin[(uintf)currentangle]*radius;
			tv[nextvert].u = mycos[(uintf)currentangle] * 0.5f + 0.5f;
			tv[nextvert].v = mysin[(uintf)currentangle] * 0.5f + 0.5f;
			if (i>0)
			{	f[nextface].v1 = startvert;
				f[nextface].v2 = nextvert;
				f[nextface].v3 = nextvert-1;
				nextface++;
			}
			nextvert++;
			currentangle += angledelta;
		}
		f[nextface].v1 = startvert;
		f[nextface].v2 = startvert+1;
		f[nextface].v3 = nextvert-1;
		nextface++;
	}

	creationbuf->numverts = nextvert;
	creationbuf->numfaces = nextface;

	calcradius(creationbuf);
	calcnormals(creationbuf);

	// Eliminate the line between the first and last segment
	nx = creationbuf->vert[0].nx + creationbuf->vert[segments<<1].nx;
	ny = creationbuf->vert[0].ny + creationbuf->vert[segments<<1].ny;
	nz = creationbuf->vert[0].nz + creationbuf->vert[segments<<1].nz;
	normalize(n);
	creationbuf->vert[0].nx = nx;	creationbuf->vert[segments<<1].nx = nx;
	creationbuf->vert[0].ny = ny;	creationbuf->vert[segments<<1].ny = ny;
	creationbuf->vert[0].nz = nz;	creationbuf->vert[segments<<1].nz = nz;

	nx = creationbuf->vert[1].nx + creationbuf->vert[(segments<<1)+1].nx;
	ny = creationbuf->vert[1].ny + creationbuf->vert[(segments<<1)+1].ny;
	nz = creationbuf->vert[1].nz + creationbuf->vert[(segments<<1)+1].nz;
	normalize(n);
	creationbuf->vert[1].nx = nx;	creationbuf->vert[(segments<<1)+1].nx = nx;
	creationbuf->vert[1].ny = ny;	creationbuf->vert[(segments<<1)+1].ny = ny;
	creationbuf->vert[1].nz = nz;	creationbuf->vert[(segments<<1)+1].nz = nz;

	return creationbuf;
}
*/
/*
obj3d *createsphere(float radius, uintf segments, Material *mtl)
{	msg("Incomplete Function","createsphere has not been updated to the V4 render core");
	initobjcreation(mtl);

	float fsegments = (float)segments;
	if (segments<2) segments=2;
	float hangleinc = (3.14159f) / fsegments;
	float angleinc = (3.14159f * 2) / fsegments;
	float heightangle = 0;
	for (uintf heightsegs = 0; heightsegs<segments+1; heightsegs++)
	{	float y = (float)cos(heightangle);
		float r = (float)sin(heightangle)*radius;
		heightangle+= hangleinc;
		float angle = 0;
		for (uintf ringseg = 0; ringseg<segments+2; ringseg++)
		{	float x = (float)cos(angle);
			float z = (float)sin(angle);
			angle += angleinc;
			uintf v = addvertex(x*r,y*radius,z*r, (float)ringseg / fsegments, (float)heightsegs / fsegments);
			vertex *vtx = &creationbuf->vert[v];
			vtx->nx = x*r;
			vtx->ny = y*radius;
			vtx->nz = z*r;
			normalize(vtx->n);
			if (heightsegs>0)
			{	if (ringseg>1)
					addquad((v-(segments+2))-1, v-1, v, v-(segments+2));
			}
		}
	}

	calcradius(creationbuf);

	return creationbuf;
}
*/

// ****************** 3D Collision Detection ***********************
bool collide(sCollideData *data)
{	float collideu, collidev, collidew, collidedist;
	intf firstface = data->startFace;
	intf lastface = data->lastFace;
	intf vertexSize = data->vertexSize;

	byte *vertByte = (byte *)data->VertexPosPtr;

	// Detects a colision between an object, and a line which starts at x,y,z and has a
	// normalized direction vector nx,ny,nz.  Start scanning from face 'startface'
	uintf faceIndex = 0;
	for (intf i=firstface; i<lastface; i++)
	{	float3 *v0 = (float3 *)(vertByte + vertexSize * data->faceData[faceIndex++]);
		float3 *v1 = (float3 *)(vertByte + vertexSize * data->faceData[faceIndex++]);
		float3 *v2 = (float3 *)(vertByte + vertexSize * data->faceData[faceIndex++]);

		float3 UAxis, VAxis;

		// Calculate the U axis = UAxis
		UAxis.x = v1->x - v0->x;
		UAxis.y = v1->y - v0->y;
		UAxis.z = v1->z - v0->z;

		// Calculate the V axis = VAxis
		VAxis.x = v2->x - v0->x;
		VAxis.y = v2->y - v0->y;
		VAxis.z = v2->z - v0->z;

		// Take the cross product between the line direction vector and VAxis = P
 		float3 p;
		crossProduct(&p,&data->direction,&VAxis);

		// Take the dot product between UAxis and p = A    (cosine of angle of penetration 0=parallel, 1=perpendicular)
		float a = dotproduct(&UAxis,&p);
		if ((float)fabs(a)<EPSILON) continue;	// perpendicular ... will never collide
		float ooa = 1.0f / a;

		// Subtract v0 from line start = S
		float3 s;
		s.x = data->lineStart.x - v0->x;
		s.y = data->lineStart.y - v0->y;
		s.z = data->lineStart.z - v0->z;

		// take dot product of S and P divided by A = ** Barry U **
		collideu = ooa * (dotproduct(&s,&p) );
		if ((collideu<0.0f) || (collideu>1.0f)) continue;	// out of bounds

		// Take cross product of S and UAxis = Q
		float3 q;
		crossProduct(&q,&s,&UAxis);

		// take dot product of Line Vector and Q divided by A = ** Barry V **
		collidev = ooa * (dotproduct(&data->direction,&q) );
	    if ((collidev < 0.f) || ((collideu+collidev) > 1.0f)) continue; // out of bounds

	    //take the dot product of VAxis and Q divided by A = ** T **
		collidedist = ooa * (dotproduct(&VAxis,&q) );
		if (collidedist>data->distance || collidedist<0) continue; // Too far away / wrong direction
		collidew = 1.0f - (collideu + collidev);
//		if (data->FaceNormal)
//		{	float3 *normal = (float3 *)getVertexPtr(obj, vertnorm, f->v1);
//			*data->FaceNormal = *normal;
//		}

		data->baryCoords.x = collideu;
		data->baryCoords.y = collidev;
		data->baryCoords.z = collidew;
		data->collideDistance = collidedist;
		data->collideFace = i;
		return true;
	}
	return false;
}

/*
// ********************************************************************************
// ***								Heightmap									***
// ********************************************************************************
// Notes about heightmaps
// * If you specify a cellsize of 20, then the cell is made up of 20 * 20 * 2 polygons, and (20+1) * (20+1) vertices
// * if the heightmap's width or height is not evenly divisible by cellsize, the rightmost and bottommost pixels are ignored
class oemHeightMap : public cHeightMap
{	private:
		dword	xcells,ycells;		// numer of cells across and down
		dword	vertsincell;		// number of pixels in each cell (vertsincell * vertsincell)
		float	oomapscale;			// 1 / mapscale
		byte	*staticdata;		// Pointer to memory which must remain static
		bitmap  *heightBitmap;		// Pointer to bitmap containing heightmap, or NULL
	public:
		uintf	flags;
	public:
		oemHeightMap(bitmap *bm,uintf cellsize, float heightscale, float worldscale, Material *mtl, const char *vertdef);
		~oemHeightMap(void);
		float getheight(float x,float z,obj3d **obj, intf *facenum, float3 *faceNormal);
		bool lineOfSight(float3 *eye, float3 *target);
		void bitmapToWorld(float3 *pos);
		void worldToBitmap(float3 *pos);
		void draw(void);
};

#define heightmapmem_bitmap		1

oemHeightMap::~oemHeightMap(void)
{	deletegroup3d(terrain);
	fcfree(staticdata);
	if (flags & heightmapmem_bitmap) deletebitmap(heightBitmap);
}

cHeightMap *newheightmap(const char *filename,uintf cellsize, float heightscale, float worldscale, Material *mtl,const char *vertdef)
{	bitmap *heightbm = newbitmap(filename);
	oemHeightMap *h = new oemHeightMap(heightbm,cellsize,heightscale,worldscale, mtl, vertdef);
	h->flags |= heightmapmem_bitmap;
	return h;
}

cHeightMap *newheightmap(bitmap *bm,uintf cellsize, float heightscale, float worldscale, Material *mtl,const char *vertdef)
{	return new oemHeightMap(bm,cellsize,heightscale,worldscale, mtl, vertdef);
}

byte hmapvert[]={	vertexUsage_position,	vertexData_float + 3,
					vertexUsage_normal,		vertexData_float + 3,
					vertexUsage_texture,	vertexData_float + 2,
					vertexUsage_texture,	vertexData_float + 2,
					vertexUsage_end};

oemHeightMap::oemHeightMap(bitmap *bm,uintf cellsize, float heightscale, float worldscale, Material *mtl,const char *vertdef)
{	// Generate Terrain from heightmap
	uintf i;
	sVertexElement	position, normal, texture;
	uint8 *pos,*nrm,*tex;

	vertsincell = cellsize;
	mapscale = worldscale;
	oomapscale = 1.0f / worldscale;
	heightBitmap = bm;
	flags = 0;
	mapwidth = bm->width * mapscale;
	mapheight= heightscale * 256;
	mapdepth = bm->height* mapscale;
	float yscalelut[256];
	for (i=0; i<256; i++)
	{	yscalelut[i] = heightscale*i;// + yscale * (float)cos(3.14159*0.5f*(float)i);
	}
	// How many cells are needed?
	xcells = (bm->width ) / vertsincell;
	ycells = (bm->height) / vertsincell;
	dword numcells = xcells*ycells;

	// Allocate static data
	dword memneeded = sizeof(group3d) + (sizeof(obj3d*)+12)*numcells;
	byte *mem = fcalloc(memneeded,buildstr("Heightmap static data"));
	staticdata = mem;
	terrain		= (group3d *)mem;	mem+=sizeof(group3d);
	terrain->obj= (obj3d **)mem;	mem+=sizeof(obj3d*)*numcells;
	char *txt	= (char *)mem;		mem+=12*numcells;
	synctest("Heightmap::Alloc static buffers",(byte *)terrain,memneeded,mem);

	terrain->numobjs		= numcells;
	terrain->numlights		= 0;
	terrain->nummaterials	= 0;
	terrain->material		= NULL;		// No materials are allocated with this data structure!
	terrain->flags			= 0;
	terrain->uset			= 0;
	terrain->numframes		= 1;
	terrain->numportals		= 0;

	i=0;
	float ystart =-((float)bm->height)*worldscale*0.5f;
	long heighty = 0;
	for (uintf yc = 0; yc<ycells; yc++)
	{	float xstart =-((float)bm->width)*worldscale*0.5f;
		uintf heightx = 0;
		uintf cellheight = vertsincell;
		if (yc==(ycells-1)) cellheight--;			// Last row needs to be one extra vertex in height
		for (uintf xc = 0; xc<xcells; xc++)
		{	dword cellwidth = vertsincell;
			if (xc==(xcells-1)) cellwidth--;		// Last column needs to be one extra vertex in width

			char *name = txt+12*i;
			tprintf(name, 12, "Cell %2i-%2i",(int)xc,(int)yc);
			obj3d *o = newobj3d(name, hmapvert, cellwidth*cellheight*2, (cellwidth+1)*(cellheight+1), 1,mtl, flags);
			terrain->obj[i] = o;
			lockmesh(o, lockflag_everything);
			o->wirecolor = rand() | 0xff000000;
			//sVertexElement	position, normal, texture;
			findVertexElement(o->staticVertices, vertexUsage_position, 0, &position);
			findVertexElement(o->staticVertices, vertexUsage_normal, 0, &normal);
			findVertexElement(o->staticVertices, vertexUsage_texture, 0, &texture);
			pos = (uint8 *)position.pointer;
			nrm = (uint8 *)normal.pointer;
			tex = (uint8 *)texture.pointer;

			uintf x,y;
			for (y=0; y<(cellheight+1); y++)
			{	uintf hy = heighty+y;
				for (x=0; x<(cellwidth+1); x++)
				{	uintf vi = y*(cellwidth+1)+x;
					uintf hx = heightx+x;

					// This code generates the landscape normals
					uintf uy,dy,lx,rx;
					if (hy==0) uy=hy; else uy=hy-1;
					if (hx==0) lx=hy; else lx=hx-1;
					if (hy==(uintf)bm->height-1) dy=hy; else dy=hy+1;
					if (hx==(uintf)bm->width-1)  rx=hy; else rx=hx+1;

					uintf uylut = uy * bm->width;
					uintf hylut = hy * bm->width;
					uintf dylut = dy * bm->width;

					float heightbuf[9];

					heightbuf[0] = yscalelut[bm->pixel[lx+uylut]]; // Left  / Up
					heightbuf[1] = yscalelut[bm->pixel[hx+uylut]]; //         Up
					heightbuf[2] = yscalelut[bm->pixel[rx+uylut]]; // Right / Up

					heightbuf[3] = yscalelut[bm->pixel[lx+hylut]]; // Left
					heightbuf[4] = yscalelut[bm->pixel[hx+hylut]]; //
					heightbuf[5] = yscalelut[bm->pixel[rx+hylut]]; // Right

					heightbuf[6] = yscalelut[bm->pixel[lx+dylut]]; // Left  / Down
					heightbuf[7] = yscalelut[bm->pixel[hx+dylut]]; //         Down
					heightbuf[8] = yscalelut[bm->pixel[rx+dylut]]; // Right / Down

					float nx=0,ny=0,nz=0;
					float tnx, tny, tnz;
					float v1x,v1y,v1z,v2x,v2y,v2z;
					float xs = worldscale;
					float ys = worldscale;

					v1x =  0; v1y = heightbuf[1]-heightbuf[4]; v1z = ys;	// V1 = Up vector
					v2x =-xs; v2y = heightbuf[3]-heightbuf[4]; v2z =  0;	// V2 = Left vector
					_crossproduct(n,v2,v1);									// Normal Up / Left
					_normalize(n);

					v1x =-xs; v1y = heightbuf[3]-heightbuf[4]; v1z =  0;	// V1 = Left vector
					v2x =  0; v2y = heightbuf[7]-heightbuf[4]; v2z =-ys;	// V2 = Down vector
					_crossproduct(tn,v2,v1);									// Normal Down / Left
					_normalize(tn);
					nx += tnx; ny += tny; nz += tnz;

					v1x =  0; v1y = heightbuf[7]-heightbuf[4]; v1z =-ys;	// V1 = Down vector
					v2x = xs; v2y = heightbuf[5]-heightbuf[4]; v2z =  0;	// V2 = Right vector
					_crossproduct(tn,v2,v1);									// Normal Down / Right
					_normalize(tn);
					nx += tnx; ny += tny; nz += tnz;

					v1x = xs; v1y = heightbuf[5]-heightbuf[4]; v1z =  0;	// V1 = Right vector
					v2x =  0; v2y = heightbuf[1]-heightbuf[4]; v2z = ys;	// V2 = Up vector
					_crossproduct(tn,v2,v1);									// Normal Up / Right
					_normalize(tn);
					nx += tnx; ny += tny; nz += tnz;

					_normalize(n);
					float px = xstart+(float)x*worldscale;
					float py = yscalelut[bm->pixel[hx+hylut]];
					float pz = -(ystart+(float)y*worldscale);
					writeVertexElement(pos, &position, px, py, pz, 0);
					writeVertexElement(nrm, &normal, nx, ny, nz, 0);
					writeVertexElement(tex, &texture, (px / mapwidth) + 0.5f, -(pz / mapdepth) + 0.5f, 0, 0);
					pos += position.bufferSize;
					nrm += normal.bufferSize;
					tex += texture.bufferSize;
				}
			}
			xstart+=worldscale * (float)vertsincell;//1.0f/(float)xcells;
			heightx+=vertsincell;

			uintf fi = 0;
			Face *f = o->face;
			for (y=0; y<cellheight; y++)
			{	uintf ofs = y*(cellwidth+1);
				for (x=0; x<cellwidth; x++)
				{	f[fi].v1 = ofs+x;
					f[fi].v2 = ofs+x+cellwidth+1;
					f[fi].v3 = ofs+x+1;
					f[fi+1].v1 = ofs+x+1;
					f[fi+1].v2 = ofs+x+cellwidth+1;
					f[fi+1].v3 = ofs+x+cellwidth+2;
					fi+=2;
				}
			}

			centerobj(o);
			calcRadius(o);
			unlockmesh(o);
			i++;
		} // for (xc in xcells)
		ystart+=worldscale * (float)vertsincell;//1.0f/(float)ycells;
		heighty+=vertsincell;
	} // for (yc in ycells)
}

uintf getheight_itterations = 0;

float oemHeightMap::getheight(float x,float z,obj3d **obj, intf *facenum, float3 *faceNormal)
{	getheight_itterations++;
	dword gx = (dword)((x+mapwidth * 0.5f) * oomapscale);
	dword gz = (dword)(((mapdepth-z)-mapdepth * 0.5f) * oomapscale);
	dword cellx = gx/vertsincell;
	dword celly = gz/vertsincell;
	if (cellx<0) return -1;
	if (celly<0) return -1;
	if (cellx>=xcells) return -1;
	if (celly>=ycells) return -1;
	dword cellnum = celly*xcells+cellx;
	if (cellnum>terrain->numobjs) return -1;
	obj3d *o = terrain->obj[cellnum];
	if (obj) *obj = o;

	dword firstface = ((gz-(celly*vertsincell))*vertsincell + gx-(cellx*vertsincell))<<1;

	collideData cd;
	cd.target = o;
	makeFloat3(cd.lineStart, x,mapheight,z);
	makeFloat3(cd.direction, 0, -1, 0);
	cd.collideDistance = mapheight;
	cd.startFace = firstface;
	cd.flags = collide_only2faces;
	cd.FaceNormal = faceNormal;
	if (collide(&cd))
	{	if (facenum) *facenum = cd.collideFace;
		getheight_itterations--;
		return mapheight-cd.collideDistance;
	}	else
	{	// In very rare situations, you can fall between 2 cells.  (It does happen but very rarely)
		// To solve this, try moving 0.01 in X and Z and try again.
		if (getheight_itterations>5)
		{	getheight_itterations--;
			return -1;
		}
		float tmp = getheight(x+0.01f, z+0.01f, obj, facenum, faceNormal);
		getheight_itterations--;
		return tmp;
	}
}

bool oemHeightMap::lineOfSight(float3 *eye, float3 *target)
{	intf dxi,dyi,dhi, steps, bmoffs, addxval, addyval;

	if (!heightBitmap) return true;		// We can't test for line of sight without the bitmap
	float startx = ( eye->x + mapwidth*0.5f) * oomapscale;
	float starty = ((mapdepth-eye->z) - mapdepth*0.5f) * oomapscale;
	float starth = eye->y*256.0f / mapheight;

	float endx = ( target->x + mapwidth*0.5f) * oomapscale;
	float endy = ((mapdepth-target->z) - mapdepth*0.5f) * oomapscale;
	float endh = target->y*256.0f / mapheight;

	float dx = endx - startx;
	float dy = endy - starty;
	float dh = endh - starth;

	if (dx<0)
	{	addxval = -1;
		dx = -dx;
	}	else
	{	addxval = 1;
	}

	if (dy<0)
	{	addyval = -(intf)heightBitmap->width;
		dy = -dy;
	}	else
	{	addyval = heightBitmap->width;
	}

	intf ch = (intf)(starth * 65536.0f);

	bmoffs = (intf)starty*heightBitmap->width + (intf)startx;

	if (dx>dy)
	{	dyi = ((intf)((dy * 65536.0f) / dx)) & 0xffff;
		dhi = (intf)((dh * 65536.0f) / dx);
		steps = (intf)dx;
		intf ypart = 0;
		for (dxi=0; dxi<steps; dxi++)
		{	if (heightBitmap->pixel[bmoffs] > (ch>>16)) return false;
			bmoffs +=addxval;
			ypart += dyi;
			if (ypart>0xffff)
			{	bmoffs += addyval;
				ypart -= 0x10000;
			}
			ch += dhi;
		}
	}	else
	{	dxi = ((intf)((dx * 65536.0f) / dy)) & 0xffff;
		dhi = (intf)((dh * 65536.0f) / dy);
		steps = (intf)dy;
		intf xpart = 0;
		for (dyi=0; dyi<steps; dyi++)
		{	if (heightBitmap->pixel[bmoffs] > (ch>>16)) return false;
			bmoffs +=addyval;
			xpart += dxi;
			if (xpart>0xffff)
			{	bmoffs += addxval;
				xpart -= 0x10000;
			}
			ch += dhi;
		}
	}
	return true;
}

void oemHeightMap::bitmapToWorld(float3 *pos)
{	pos->x = (pos->x*mapscale)-(mapwidth*0.5f);
	pos->z =-((pos->z*mapscale)-(mapdepth*0.5f));
}

void oemHeightMap::worldToBitmap(float3 *pos)
{	pos->x = (pos->x+mapwidth * 0.5f) * oomapscale;
	pos->z = ((mapdepth-pos->z)-mapdepth * 0.5f) * oomapscale;
}

void oemHeightMap::draw(void)
{	for (dword i=0; i<terrain->numobjs; i++)
	{	drawobj3d(terrain->obj[i]);
	}
}
*/

sPlane PlaneFromMatrixX(float *mat)			// Create a plane who's normal points the same direction as the Matrix's X vector
{	sPlane p;
	p.nx = mat[mat_xvecx];
	p.ny = mat[mat_xvecy];
	p.nz = mat[mat_xvecz];
	p.d  = mat[mat_xpos] * p.nx + mat[mat_ypos] * p.ny + mat[mat_zpos] * p.nz;
	return p;
}

sPlane PlaneFromMatrixY(float *mat)			// Create a plane who's normal points the same direction as the Matrix's Y vector
{	sPlane p;
	p.nx = mat[mat_yvecx];
	p.ny = mat[mat_yvecy];
	p.nz = mat[mat_yvecz];
	p.d  = mat[mat_xpos] * p.nx + mat[mat_ypos] * p.ny + mat[mat_zpos] * p.nz;
	return p;
}

sPlane PlaneFromMatrixZ(float *mat)			// Create a plane who's normal points the same direction as the Matrix's Z vector
{	sPlane p;
	p.nx = mat[mat_zvecx];
	p.ny = mat[mat_zvecy];
	p.nz = mat[mat_zvecz];
	p.d  = mat[mat_xpos] * p.nx + mat[mat_ypos] * p.ny + mat[mat_zpos] * p.nz;
	return p;
}

/*
void groupdimensions(group3d *scene)
{	uintf i;
	obj3d *obj;
	float minx, miny, minz, maxx, maxy, maxz;
	if (scene->numobjs==0)
	{	minx=miny=minz=maxx=maxy=maxz=0;
	}	else
	{	minx = scene->obj[0]->worldpos.x - scene->obj[0]->radius;
		maxx = scene->obj[0]->worldpos.x + scene->obj[0]->radius;
		miny = scene->obj[0]->worldpos.y - scene->obj[0]->radius;
		maxy = scene->obj[0]->worldpos.y + scene->obj[0]->radius;
		minz = scene->obj[0]->worldpos.z - scene->obj[0]->radius;
		maxz = scene->obj[0]->worldpos.z + scene->obj[0]->radius;
	}
	for (i=1; i<scene->numobjs; i++)
	{	obj = scene->obj[i];
		if (obj->worldpos.x-obj->radius < minx) minx = obj->worldpos.x-obj->radius;
		if (obj->worldpos.x+obj->radius > maxx) maxx = obj->worldpos.x+obj->radius;
		if (obj->worldpos.y-obj->radius < miny) miny = obj->worldpos.y-obj->radius;
		if (obj->worldpos.y+obj->radius > maxy) maxy = obj->worldpos.y+obj->radius;
		if (obj->worldpos.z-obj->radius < minz) minz = obj->worldpos.z-obj->radius;
		if (obj->worldpos.z+obj->radius > maxz) maxz = obj->worldpos.z+obj->radius;
	}
	scene->center.x = (minx+maxx)/2;
	scene->center.y = (miny+maxy)/2;
	scene->center.z = (minz+maxz)/2;
	for (i=0; i<scene->numobjs; i++)
	{	obj = scene->obj[i];
		obj->worldpos.x -= scene->center.x;
		obj->worldpos.y -= scene->center.y;
		obj->worldpos.z -= scene->center.z;
	}

	scene->width = maxx-scene->center.x;
	scene->height = maxy-scene->center.y;
	scene->depth = maxz-scene->center.z;
	scene->radiussquared =	scene->width*scene->width +
							scene->height*scene->height +
							scene->depth*scene->depth;
	scene->radius = (float)sqrt(scene->radiussquared);
}
*/

//	----- Geometry Importers ----
char CurrentGeometryName[128];										// Filename of geometry currently being imported
char *opengeometrystring=pluginfilespecs[PluginType_GeometryReader];

extern char texpath[];
/*
group3d *newgroup3d(char *filename,dword flags)
{	char ext[32];
	group3d *(*importer)(char *filename,dword flags)=NULL;
	group3d *result = NULL;

	fileNameInfo(filename);
	tprintf(ext, sizeof(ext),".%s",fileextension);

	char tmptexpath[texpathsize];
	txtcpy(tmptexpath,texpathsize,texpath);
	txtcat(texpath,texpathsize,";");
	txtcat(texpath,texpathsize,filepath);

	importer = (group3d *(*)(char *,dword))getPluginHandler(ext, PluginType_GeometryReader);
	if (importer)
	{	result = importer(filename,flags);
	}

	if (importer==NULL)
		msg("3D Importer File Error",buildstr("%s is not a supported 3D Geometry file\n",filename));
	if (result==NULL)
		msg("3D Importer File Error",buildstr("While %s files are supported,\nthere is something wrong with\n%s\nwhich makes it unusable\n",ext,filename));
	txtcpy(texpath,texpathsize,tmptexpath);
	return result;
}
*/


// ************************************************************************
// ***						Prebuilt 3D Effect: Blur					***
// ************************************************************************
// Shader parameters
float4 cBlur_TexOfs[16];
float4 cBlur_ColScale[4];

void *cBlur_TokenFinder(const char *token)
{	if (txticmp(token,"BlurOfs0")==0)   return (void *)&cBlur_TexOfs[0];
	if (txticmp(token,"BlurOfs1")==0)   return (void *)&cBlur_TexOfs[1];
	if (txticmp(token,"BlurOfs2")==0)   return (void *)&cBlur_TexOfs[2];
	if (txticmp(token,"BlurOfs3")==0)   return (void *)&cBlur_TexOfs[3];
	if (txticmp(token,"BlurOfs4")==0)   return (void *)&cBlur_TexOfs[4];
	if (txticmp(token,"BlurOfs5")==0)   return (void *)&cBlur_TexOfs[5];
	if (txticmp(token,"BlurOfs6")==0)   return (void *)&cBlur_TexOfs[6];
	if (txticmp(token,"BlurOfs7")==0)   return (void *)&cBlur_TexOfs[7];
	if (txticmp(token,"BlurOfs8")==0)   return (void *)&cBlur_TexOfs[8];
	if (txticmp(token,"BlurOfs9")==0)   return (void *)&cBlur_TexOfs[9];
	if (txticmp(token,"BlurOfs10")==0)   return (void *)&cBlur_TexOfs[10];
	if (txticmp(token,"BlurOfs11")==0)   return (void *)&cBlur_TexOfs[11];
	if (txticmp(token,"BlurOfs12")==0)   return (void *)&cBlur_TexOfs[12];
	if (txticmp(token,"BlurOfs13")==0)   return (void *)&cBlur_TexOfs[13];
	if (txticmp(token,"BlurOfs14")==0)   return (void *)&cBlur_TexOfs[14];
	if (txticmp(token,"BlurOfs15")==0)   return (void *)&cBlur_TexOfs[15];

	if (txticmp(token,"ColorScale0")==0) return (void *)&cBlur_ColScale[0];
	if (txticmp(token,"ColorScale1")==0) return (void *)&cBlur_ColScale[1];
	if (txticmp(token,"ColorScale2")==0) return (void *)&cBlur_ColScale[2];
	if (txticmp(token,"ColorScale3")==0) return (void *)&cBlur_ColScale[3];
	return NULL;
}

class oemBlur : public cBlur
{
public:
	float		texscale[4],colorscale[4];
	Material	*BlurPass1,	*BlurPass2;
	Material	*BlurLeft,	*BlurRight,	*BlurUp, *BlurDown;
	Texture		*tempTex;
	uintf		blurSize;			// This is the size, in pixels of the blur texture

	oemBlur(uintf size, float brightness);
	~oemBlur(void);
	void blur(void);
};

cBlur *newBlur(uintf size, float brightness)
{	oemBlur *result = new oemBlur(size, brightness);
	return (cBlur *)result;
}

oemBlur::oemBlur(uintf size, float brightness)
{/*	blurSize = size;

	LoadShaderLibrary("BlurShader.txt",cBlur_TokenFinder);

	source = newRenderTexture(blurSize, blurSize, 0);
	source->mapping = texMapping_magLinear;
	tempTex = newRenderTexture(blurSize, blurSize, 0);
	source->mapping = texMapping_magLinear;

	texscale[0] = 1.0f / (float)blurSize;
	texscale[1] = 2.0f / (float)blurSize;
	texscale[3] = 3.0f / (float)blurSize;
	texscale[4] = 4.0f / (float)blurSize;
	memfill(&cBlur_TexOfs,0,sizeof(cBlur_TexOfs));

	colorscale[0] = brightness;
	colorscale[1] = brightness * 0.75f;
	colorscale[2] = brightness * 0.50f;
	colorscale[3] = brightness * 0.25f;
	cBlur_ColScale[0].a = 1.0f;
	cBlur_ColScale[1].a = 1.0f;
	cBlur_ColScale[2].a = 1.0f;
	cBlur_ColScale[3].a = 1.0f;

	FixedPixelShader *ps;
	BlurLeft = newmaterial("BlurLeft");
	BlurLeft->texture[0] = copytexture(source);
	BlurLeft->texture[1] = copytexture(source);
	BlurLeft->texture[2] = copytexture(source);
	BlurLeft->texture[3] = copytexture(source);
	BlurLeft->finalblend = finalBlend_add;
	BlurLeft->sourceParam = blendParam_One;
	BlurLeft->targetParam = blendParam_One;

	BlurRight = newmaterial("BlurRight");
	BlurRight->texture[0] = copytexture(source);
	BlurRight->texture[1] = copytexture(source);
	BlurRight->texture[2] = copytexture(source);
	BlurRight->texture[3] = copytexture(source);
	BlurRight->finalblend = finalBlend_add;
	BlurRight->sourceParam = blendParam_One;
	BlurRight->targetParam = blendParam_One;

	BlurPass2 = newmaterial("Blur_Pass2");
	BlurPass2->texture[0] = copytexture(tempTex);
	ps = (FixedPixelShader *)BlurPass2->PixelShader;
	ps->colorop[0] = blendop_selecttex;
	ps->alphaop[0] = blendop_selecttex;
	BlurPass2->finalblend = finalBlend_add;
	BlurPass2->sourceParam = blendParam_One;
	BlurPass2->targetParam = blendParam_Zero;

	BlurUp = newmaterial("BlurUp");
	BlurUp->texture[0] = copytexture(tempTex);
	BlurUp->texture[1] = copytexture(tempTex);
	BlurUp->texture[2] = copytexture(tempTex);
	BlurUp->texture[3] = copytexture(tempTex);
	BlurUp->finalblend = finalBlend_add;
	BlurUp->sourceParam = blendParam_One;
	BlurUp->targetParam = blendParam_One;

	BlurDown = newmaterial("BlurDown");
	BlurDown->texture[0] = copytexture(tempTex);
	BlurDown->texture[1] = copytexture(tempTex);
	BlurDown->texture[2] = copytexture(tempTex);
	BlurDown->texture[3] = copytexture(tempTex);
	BlurDown->finalblend = finalBlend_add;
	BlurDown->sourceParam = blendParam_One;
	BlurDown->targetParam = blendParam_One;

	material = newmaterial("Blur_Mtl");
	material->texture[0] = copytexture(source);
	ps = (FixedPixelShader *)material->PixelShader;
	ps->colorop[0] = blendop_selecttex;
	ps->alphaop[0] = blendop_selecttex;
	material->finalblend = finalBlend_add;
	material->sourceParam = blendParam_One;
	material->targetParam = blendParam_One;
*/
}

oemBlur::~oemBlur(void)
{/*	deletematerial(material);
	deletematerial(BlurDown);
	deletematerial(BlurUp);
	deletematerial(BlurPass2);
	deletematerial(BlurRight);
	deletematerial(BlurLeft);
	deletetexture(tempTex);
	deletetexture(source);
*/
}

void oemBlur::blur(void)
{/*	// Set up LEFT scaling
	cBlur_TexOfs[ 0].x = -texscale[0];
	cBlur_TexOfs[ 1].x = -texscale[1];
	cBlur_TexOfs[ 2].x = -texscale[2];
	cBlur_TexOfs[ 3].x = -texscale[3];

	// Set up RIGHT scaling
	cBlur_TexOfs[ 4].x = texscale[0];
	cBlur_TexOfs[ 5].x = texscale[1];
	cBlur_TexOfs[ 6].x = texscale[2];
	cBlur_TexOfs[ 7].x = texscale[3];

	// Set up UP scaling
	cBlur_TexOfs[ 8].y = -texscale[0];
	cBlur_TexOfs[ 9].y = -texscale[1];
	cBlur_TexOfs[10].y = -texscale[2];
	cBlur_TexOfs[11].y = -texscale[3];

	// Set up DOWN scaling
	cBlur_TexOfs[12].y = texscale[0];
	cBlur_TexOfs[13].y = texscale[1];
	cBlur_TexOfs[14].y = texscale[2];
	cBlur_TexOfs[15].y = texscale[3];

	// Set up color scaling
	cBlur_ColScale[0].r = cBlur_ColScale[0].g = cBlur_ColScale[0].b = colorscale[0];
	cBlur_ColScale[1].r = cBlur_ColScale[1].g = cBlur_ColScale[1].b = colorscale[1];
	cBlur_ColScale[2].r = cBlur_ColScale[2].g = cBlur_ColScale[2].b = colorscale[2];
	cBlur_ColScale[3].r = cBlur_ColScale[3].g = cBlur_ColScale[3].b = colorscale[3];

	// Construct Blur Surface
	disableZbuffer();
	// Pass 1 - Blur Left & Right  (Add in 4xBlurTex1 to BlurTex2), cost = 4 reads per texel
	setrendertarget3d(tempTex);
	cls3D(0xff000000);		// ### Should obtain a color from somewhere
//	drawMtlAsTexSprite(BlurLeft, -1.0f,-1.0f, 2.0f,2.0f);
//	drawMtlAsTexSprite(BlurRight, -1.0f,-1.0f, 2.0f,2.0f);

	// Pass 2 - Copy Existing data as new center pixel  (Copy from BlurTex2 to BlurTex1), cost = 1 read per texel
	setrendertarget3d(source);
//	drawMtlAsTexSprite(BlurPass2, -1.0f,-1.0f, 2.0f,2.0f);

	// Pass 4 - Blur Up & Down  (Add in 4xBlurTex2 to BlurTex1), cost = 4 reads per texel
//	drawMtlAsTexSprite(BlurUp, -1.0f,-1.0f, 2.0f,2.0f);
//	drawMtlAsTexSprite(BlurDown, -1.0f,-1.0f, 2.0f,2.0f);
	enableZbuffer();
*/
}
