/************************************************************************************/
/*								FC Utilities file									*/
/*	    						-----------------									*/
/*  					   (c) 1998-2002 by Stephen Fraser							*/
/*																					*/
/* Contains lots of small modules that would be too cumbersome to have as           */
/* independant files																*/
/*																					*/
/*	* Global Plugin System															*/
/*  * Obj3d Assistant																*/
/*  * Group3d Assistant																*/
/*  * Collisions																	*/
/*  * Visibility Points																*/
/*																					*/
/*------------------------------  W A R N I N G  -----------------------------------*/
/*  This source file is only for the use of Stephen Fraser.  Any other use without	*/
/* my express permission may result in legal action being taken!					*/
/************************************************************************************/

/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#include <stdlib.h>
#include "fc_h.h"
#include <bucketSystem.h>

// ****************************************************************************************
// ***								Object Assist										***
// ****************************************************************************************
/*
void deleteface(obj3d *o1, long fnum)
{	// Deletes face [fnum] from an obj3d structure - move last entry to deleted one
	o1->face[fnum]=o1->face[o1->numfaces-1];
	o1->numfaces--;
}

void deletevert(obj3d *obj, dword i)
{	// Deletes vertex [i] from an obj3d structure - any face using that vert now uses last vert
	dword j;

	if (obj->numverts==0) return;
	dword lastvert = obj->numverts-1;
	obj->vert[i] = obj->vert[lastvert];
//	obj->tvert[i] = obj->tvert[lastvert];

	if (obj->numkeyframes>1)
	{	// Move the animation vertex data down a spot
		for (j=0; j<obj->numkeyframes; j++)
		{	dword index = j*obj->numverts;
			obj->pvert[i+index] = obj->pvert[lastvert+index];
		}
		// Slide all data down
		for (j=1; j<obj->numkeyframes; j++)
		{	dword writeindex = j * (obj->numverts-1);
			dword readindex  = j * (obj->numverts);
			for (dword k=0; k<(obj->numverts-1); k++)
				obj->pvert[writeindex+k] = obj->pvert[readindex+k];
		}
	}

	for (j=0; j<obj->numfaces; j++)
	{	if (obj->face[j].v1==lastvert) obj->face[j].v1=i;
		if (obj->face[j].v2==lastvert) obj->face[j].v2=i;
		if (obj->face[j].v3==lastvert) obj->face[j].v3=i;
	}
	obj->numverts--;
}
*/
/*
// *****************************************************************
// ***						 Group Assist						 ***
// *****************************************************************
void centergroup(group3d *g1, float3 *center)
{
	msg("Incomplete Code","centerGroup function not yet ported to flameVM");
	// Reposition each object within a group so each object's origin is 0,0,0.  The group itself is centered at 0,0,0 also
	float3 *v;
	dword i,j,k;
	float minx,miny,minz,maxx,maxy,maxz,x,y,z;
	minx = miny = minz = 99999999.9f;
	maxx = maxy = maxz =-99999999.9f;
	float cx, cy, cz;

	// Find object center
	for (i=0; i<g1->numobjs; i++)
	{	obj3d *o=g1->obj[i];
		if (o->numkeyframes==1)
			lockmesh(o, lockflag_read | lockflag_write | lockflag_verts);
		if (o->matrix)
			msg("Processing Error","A call was made to centergroup but matrices were in use!");
		for (k=0; k<o->numkeyframes; k++)
		{	if (o->numkeyframes>1)
			{	o->frame1 = k;
				o->frame2 = k;
				o->decodeframe(o);
			}
			v = (float3 *)o->vertpos;
			for (j=0; j<o->numverts; j++)
			{	x = v->x + o->worldpos.x;
				y = v->y + o->worldpos.y;
				z = v->z + o->worldpos.z;

				if (x<minx) minx = x;
				if (y<miny) miny = y;
				if (z<minz) minz = z;
				if (x>maxx) maxx = x;
				if (y>maxy) maxy = y;
				if (z>maxz) maxz = z;
				v = (float3 *)incVertexPtr(v, o);
			}
		}
	}
	cx = (minx+maxx)*0.5f;
	cy = (miny+maxy)*0.5f;
	cz = (minz+maxz)*0.5f;

	for (i=0; i<g1->numobjs; i++)
	{	obj3d *o=g1->obj[i];
		if (o->numkeyframes>1)
		{	for (k=0; k<o->numkeyframes; k++)
			{	o->frame1 = k;
				o->frame2 = k;
				o->decodeframe(o);
				v = (float3 *)o->vertpos;
				for (j=0; j<o->numverts; j++)
				{	v->x = v->x+o->worldpos.x-cx;
					v->y = v->y+o->worldpos.y-cy;
					v->z = v->z+o->worldpos.z-cz;
					v = (float3 *)incVertexPtr(v, o);
				}
				//compressobj3dframe(o,k);
			}
		} else
		{	v = (float3 *)o->vertpos;
			for (j=0; j<o->numverts; j++)
			{	v->x = v->x+o->worldpos.x-cx;
				v->y = v->y+o->worldpos.y-cy;
				v->z = v->z+o->worldpos.z-cz;
				v = (float3 *)incVertexPtr(v, o);
			}
		}
		o->worldpos.x = 0;
		o->worldpos.y = 0;
		o->worldpos.z = 0;
		if (o->numkeyframes==1)
			unlockmesh(o);
	}
	if (center)
	{	center->x = cx;
		center->y = cy;
		center->z = cz;
	}
}

void extractobj(group3d *g1, dword onum)
{	// Extracts object [onum] from a group3d structure - move all upper objects back 1 space
	for (dword i=onum; i<g1->numobjs-1; i++)
		g1->obj[i]=g1->obj[i+1];
	g1->numobjs--;
}

void deletegroup3d(group3d *g1)
{	long i;
	for (i=0; i<(long)g1->numobjs; i++)
		deleteobj3d(g1->obj[i]);
	if (g1->flags & group_memobj)
		fcfree(g1->obj);

	for (i=g1->nummaterials-1; i>=0; i--)
	{	Material *mtl = g1->material[i];
		deletematerial(mtl);
	}
	if (g1->flags & group_memmaterial)
		fcfree(g1->material);

	if (g1->flags & group_memother)
		fcfree(g1->other);

	if (g1->flags & group_memstruct)
		fcfree(g1);
}

// *****************************************************************
// ***						Group Export Functions				 ***
// *****************************************************************
dword osfblocksize;
dword osfblockseek;
sFileHandle *osfhandle;

void writeosfblock(sFileHandle *handle, const char *blockname, void *ptr, int size)
{	byte buf[4];
	fileWrite(handle, (void *)blockname, 4);
	PutLittleEndianUINT32(buf,size);
	fileWrite(handle, buf, 4);
	if (ptr) fileWrite(handle, ptr, size);
	fileWrite(handle, (void *)"endb",4);
}

void startosfblock(const char *blockname)
{	osfblocksize = 0;
	fileWrite(osfhandle, (void *)blockname, 4);
	osfblockseek = fileSeek(osfhandle,0,seek_fromcurrent);
	fileWrite(osfhandle, &osfblocksize, 4);
}

inline void osfblockwrite(void *data, dword size)
{	fileWrite(osfhandle,data,size);
	osfblocksize += size;
}

inline void osfblockwritechar4(char *txt)
{	osfblockwrite(txt,4);
}

inline void osfblockwritefloat(float data)
{	byte buf[4];
	PutLittleEndianFLOAT32(buf,data);
	osfblockwrite(buf,4);
}
inline void osfblockwritecolor(float3 col)
{	byte buf[4];
	PutLittleEndianFLOAT32(buf,col.r);
	osfblockwrite(buf,4);
	PutLittleEndianFLOAT32(buf,col.g);
	osfblockwrite(buf,4);
	PutLittleEndianFLOAT32(buf,col.b);
	osfblockwrite(buf,4);
}
inline void osfblockwritecolor(float4 col)
{	byte buf[4];
	PutLittleEndianFLOAT32(buf,col.r);
	osfblockwrite(buf,4);
	PutLittleEndianFLOAT32(buf,col.g);
	osfblockwrite(buf,4);
	PutLittleEndianFLOAT32(buf,col.b);
	osfblockwrite(buf,4);
}
inline void osfblockwritedword(uint32 data)
{	byte buf[4];
	PutLittleEndianUINT32((byte *)&buf,data);
	osfblockwrite((byte *)&buf,4);
}
inline void osfblockwritebyte(uint8 data)
{	osfblockwrite(&data,1);
}

uintf	getTextPoolOfs(const char *txt, const char *textpool, uintf textpoolsize)
{	uintf ofs = 0;
	while (ofs < textpoolsize)
	{	if (txtcmp(textpool+ofs, txt)==0)
		{	return ofs;
		}
		ofs += txtlen(textpool + ofs) + 1;
	}
	msg("OSF Export Failed",buildstr("The text string %s did not appear in the textpool",txt));
	return 0;
}

void osfblockwritestring(char *txt, char *textpool, uintf textpoolsize)
{	// Locate string in text pool
	osfblockwritedword(getTextPoolOfs(txt,textpool,textpoolsize));
/ *	uintf ofs = 0;
	while (ofs < textpoolsize)
	{	if (txtcmp(textpool+ofs, txt)==0)
		{	osfblockwritedword(ofs);
			return;
		}
		ofs += txtlen(textpool + ofs) + 1;
	}
	msg("OSF Export Failed",buildstr("The text string %s did not appear in the textpool",txt));
* /
}

void endosfblock()
{	fileWrite(osfhandle, (void *)"endb", 4);
	uintf currentseek = fileSeek(osfhandle,0,seek_fromcurrent);
	fileSeek(osfhandle,osfblockseek,seek_fromstart);
	fileWrite(osfhandle, &osfblocksize, 4);
	fileSeek(osfhandle,currentseek,seek_fromstart);
}

struct OSFheader
{	byte  versionmajor;		// Major version number of the file, example, 3 for version 3.22
	byte  versionminor;		// Minor version number of the file, example, 22 for version 3.22
	byte  stacksize;		// Number of elements in the matrix stack (set to 0 for no matrix stack)
	byte  reserved1;		// This is reserved (mainly to keep the 32 bit alignment)
	float horizon;			// The horizon level
	dword ambient;			// Ambient light used for viewing tools
	dword backcol;			// Background color used for viewing tools
	dword fogcolor;			// Color of the fog used in viewing tools
	float fogstart;			// Starting point of fog (typical range of 0.0 to 1.0)
	float fogend;			// Ending point of fog (typical range of 0.0 to 1.0)
	dword numlights;		// Number of lights in scene
	dword nummaterials;		// Number of materials in the file
	dword numobjects;		// Number of objects in the file
	dword numfaces;			// Total number of faces throughout scene
	dword numverts;			// Total number of verticies in scene
	dword animmem;			// Memory used (in bytes) of amimation data
	dword num2Dverts;		// Total number of 2D texture verticies throughout scene
	dword num3Dverts;		// Total number of 2D texture verticies throughout scene
	dword num4Dverts;		// Total number of 2D texture verticies throughout scene
	dword numportals;		// Number of portals in scene
	dword numanimseq;		// Number of animation sequences
	dword textpoolsize;		// Size of the scene's text pool (where all object / material names are stored)
	dword flags;			// Not yet used ... set to 0
};

void exportgroup(char *filename,group3d *g1)
{
	msg("Incomplete Code","exportGroup function has not been updated to flameVM compatibility");
/ *	uintf		nummtls,numtexs,numlights,numfaces,numverts,animmem,num2Dverts,num3Dverts,num4Dverts;
	uintf		objindex, mtlindex,texindex,vertindex,spareindex;
	uintf		textpoolsize;
	obj3d		*obj;
	Material	*mtl;
	Texture		*tex;
	OSFheader	osfheader;

	// ------------------------------------------------------------------------------------------------------
	// We don't know how many materals are really in use ... Allocate room 1 material per object
	nummtls = 0;
	Material **mtllist = (Material **)fcalloc(g1->numobjs * sizeof(Material *),"Export OSF Material Buffer");
	for (objindex = 0; objindex<g1->numobjs; objindex++)
	{	obj = g1->obj[objindex];
		mtl = obj->material;
		bool foundmtl = false;
		for (mtlindex=0; mtlindex<nummtls; mtlindex++)
		{	if (mtllist[mtlindex]==mtl)
			{	foundmtl = true;
				break;
			}
		}
		if (!foundmtl)
		{	mtllist[nummtls++]=mtl;
		}
	}

	// ------------------------------------------------------------------------------------------------------
	// We don't know how many textures are in use ... Allocate room for 1 texture per used texture slot	in each material
	numtexs = 0;
	for (mtlindex=0; mtlindex<nummtls; mtlindex++)
	{	mtl = mtllist[mtlindex];
		texindex=0;
		while (texindex<8 && mtl->texture[texindex]!=NULL)
		{	numtexs++;
			texindex++;
		}
	}
	Texture **texlist = (Texture **)fcalloc(numtexs * sizeof(Texture *),"Export OSF Texture Buffer");
	numtexs = 0;
	for (mtlindex=0; mtlindex<nummtls; mtlindex++)
	{	mtl = mtllist[mtlindex];
		texindex=0;
		while (texindex<8 && mtl->texture[texindex]!=NULL)
		{	tex = mtl->texture[texindex];
			bool foundtex = false;
			for (spareindex=0; spareindex<numtexs; spareindex++)
			{	if (txticmp(texlist[spareindex++]->name,tex->name)==0)
				{	break;
				}
			}
			if (!foundtex)
			{	texlist[numtexs++] = tex;
			}
			texindex++;
		}
	}

	// ------------------------------------------------------------------------------------------------------
	// Calculate size requrements of textpool
	textpoolsize = 0;
	for (texindex=0; texindex<numtexs; texindex++)
	{	textpoolsize += (uintf)txtlen(texlist[texindex]->name) + 1;
	}
	for (mtlindex=0; mtlindex<nummtls; mtlindex++)
	{	textpoolsize += (uintf)txtlen(mtllist[mtlindex]->name) + 1;
	}
	for (objindex=0; objindex<g1->numobjs; objindex++)
	{	textpoolsize += (uintf)txtlen(g1->obj[objindex]->name) + 1;
	}

	// ------------------------------------------------------------------------------------------------------
	// Generate the text pool
	char *textpool = (char *)fcalloc(textpoolsize,"Export OSF Text Pool");
	char *textpoolptr = textpool;
	uintf textpoolremain = textpoolsize;
	uintf textlen;

	for (texindex=0; texindex<numtexs; texindex++)
	{	txtcpy(textpoolptr,texlist[texindex]->name,textpoolremain);
		textlen = txtlen(texlist[texindex]->name) + 1;
		textpoolptr += textlen;
		textpoolremain -= textlen;
	}
	for (mtlindex=0; mtlindex<nummtls; mtlindex++)
	{	txtcpy(textpoolptr,mtllist[mtlindex]->name,textpoolremain);
		textlen = txtlen(mtllist[mtlindex]->name) + 1;
		textpoolptr +=  textlen;
		textpoolremain -= textlen;
	}
	for (objindex=0; objindex<g1->numobjs; objindex++)
	{	txtcpy(textpoolptr,g1->obj[objindex]->name,textpoolremain);
		textlen = txtlen(g1->obj[objindex]->name) + 1;
		textpoolptr +=  textlen;
		textpoolremain -= textlen;
	}

	// ------------------------------------------------------------------------------------------------------
	// Count and verify lights
	numlights = 0;
	if (g1->lights)
		while (numlights<g1->numlights)
		{	if (!g1->lights[numlights]) break;
			numlights++;
		}

	// ------------------------------------------------------------------------------------------------------
	// Add up number of faces, vertices and animation memory
	numfaces = 0;
	numverts = 0;
	animmem = 0;
	num2Dverts = 0;
	num3Dverts = 0;
	num4Dverts = 0;
	for (objindex=0; objindex<g1->numobjs; objindex++)
	{	obj = g1->obj[objindex];
		numfaces += obj->numfaces;
		numverts += obj->numverts;
		animmem  = 0;					// ### Need a way to calculate this!
		switch(obj->numtexchans)
		{	case 2: num2Dverts+=obj->numverts; break;
			case 3: num3Dverts+=obj->numverts; break;
			case 4: num4Dverts+=obj->numverts; break;
			default: break;
		}
	}

	// ------------------------------------------------------------------------------------------------------
	// Generate Header
	osfheader.versionmajor = 4;
	osfheader.versionminor = 0;
	osfheader.stacksize = 0;
	osfheader.reserved1 = 0;
	PutLittleEndianFLOAT32((byte *)&osfheader.horizon, g1->horizon);
	PutLittleEndianUINT32((byte *)&osfheader.ambient, g1->ambientLight);
	PutLittleEndianUINT32((byte *)&osfheader.backcol, g1->backgroundColor);
	PutLittleEndianUINT32((byte *)&osfheader.fogcolor, g1->fogColor);
	PutLittleEndianFLOAT32((byte *)&osfheader.fogstart, g1->fogstart);
	PutLittleEndianFLOAT32((byte *)&osfheader.fogend, g1->fogend);
	PutLittleEndianUINT32((byte *)&osfheader.numlights, 0);//numlights);		###
	PutLittleEndianUINT32((byte *)&osfheader.nummaterials, nummtls);
	PutLittleEndianUINT32((byte *)&osfheader.numobjects, g1->numobjs);
	PutLittleEndianUINT32((byte *)&osfheader.numfaces, numfaces);
	PutLittleEndianUINT32((byte *)&osfheader.numverts, numverts);
	PutLittleEndianUINT32((byte *)&osfheader.animmem, animmem);
	PutLittleEndianUINT32((byte *)&osfheader.num2Dverts, num2Dverts);
	PutLittleEndianUINT32((byte *)&osfheader.num3Dverts, num3Dverts);
	PutLittleEndianUINT32((byte *)&osfheader.num4Dverts, num4Dverts);
	PutLittleEndianUINT32((byte *)&osfheader.numportals, 0);
	PutLittleEndianUINT32((byte *)&osfheader.numanimseq, 0);
	PutLittleEndianUINT32((byte *)&osfheader.textpoolsize, textpoolsize);
	PutLittleEndianUINT32((byte *)&osfheader.flags, 0);

	// ------------------------------------------------------------------------------------------------------
	// Write File Header
	osfhandle = fileCreate(filename);
	writeosfblock(osfhandle, "HDR:", &osfheader, sizeof(osfheader));
	writeosfblock(osfhandle, "TXT:", textpool, textpoolsize);

	// ------------------------------------------------------------------------------------------------------
	// Write out all materials
	for (mtlindex=0; mtlindex<nummtls; mtlindex++)
	{	mtl = mtllist[mtlindex];
		uintf numtex = 0;
		while (mtl->texture[numtex])
		{	numtex++;
		}
		startosfblock("MTL:");
		osfblockwritestring(mtl->name, textpool, textpoolsize);
		osfblockwritebyte(mtl->flags & ~(mtl_pixelshader | mtl_vertexshader));
		osfblockwritebyte(1);
		osfblockwritebyte((byte)numtex);
		for (texindex=0; texindex<numtex; texindex++)
		{	osfblockwritestring(mtl->texture[texindex]->name, textpool, textpoolsize);
			osfblockwritedword(mtl->texture[texindex]->mapping);
		}
		if (mtl->flags & mtl_vertexshader)
		{	float3 shadercolor;
			shadercolor.r = 1.0f;
			shadercolor.g = 1.0f;
			shadercolor.b = 1.0f;
			osfblockwritecolor(shadercolor);	// ambient
			osfblockwritecolor(shadercolor);	// diffuse
			osfblockwritecolor(shadercolor);	// specular
			osfblockwritecolor(shadercolor);	// emissive
			osfblockwritefloat(1.0f);			// alpha
			osfblockwritefloat(4.0f);			// Specular Power
			osfblockwritebyte(0);				// Z-BIAS
			for (texindex=0; texindex<numtex; texindex++)
				osfblockwritebyte(0);
		}	else
		{	FixedVertexShader *vs = (FixedVertexShader *)mtl->VertexShader;
			osfblockwritecolor(vs->ambient);
			osfblockwritecolor(vs->diffuse);
			osfblockwritecolor(vs->specular);
			osfblockwritecolor(vs->emissive);
			osfblockwritefloat(vs->diffuse.a);
			osfblockwritefloat(vs->spec_power);
			osfblockwritebyte(vs->bias);
			for (texindex=0; texindex<numtex; texindex++)
			{	osfblockwritebyte(vs->modifier[texindex]);
			}
		}
		if (mtl->flags & mtl_pixelshader)
		{	for (texindex=0; texindex<numtex; texindex++)
			{	osfblockwritebyte(blendop_modulate);	// colorop
				osfblockwritebyte(blendop_modulate);	// alphaop
				osfblockwritebyte(0);					// Texture channel selector
			}
		}	else
		{	FixedPixelShader *ps = (FixedPixelShader *)mtl->PixelShader;
			for (texindex=0; texindex<numtex; texindex++)
			{	osfblockwritebyte(ps->colorop[texindex]);
				osfblockwritebyte(ps->alphaop[texindex]);
				osfblockwritebyte(ps->channel[texindex]);
			}
		}
		// Final Blending Op
		osfblockwritebyte(mtl->targetParam);
		osfblockwritebyte(mtl->sourceParam);
		osfblockwritebyte(mtl->finalblend);
		endosfblock();
	}

	// ------------------------------------------------------------------------------------------------------
	// Write out all objects
	for (objindex=0; objindex<g1->numobjs; objindex++)
	{	byte channelinfo[8];

		obj3d *obj = g1->obj[objindex];
		startosfblock("Obj:");
		OSFobjhdr hdr;
		memfill(&hdr,0,sizeof(hdr));
		for (spareindex=0; spareindex<obj->numtexchans; spareindex++)
			channelinfo[spareindex]=(byte)obj->texelements[spareindex];
		uintf nameofs = getTextPoolOfs(obj->name, textpool, textpoolsize);
		char *codecinfo;
		uintf codecinfosize;
		if (obj->MeshCodec)
		{	obj->MeshCodec(AnimMesh_GetInfo, &codecinfo, &codecinfosize, 0);
			hdr.compression[0] = codecinfo[0];
			hdr.compression[1] = codecinfo[1];
			hdr.compression[2] = codecinfo[2];
			hdr.compression[3] = codecinfo[3];
		}	else
		{	hdr.compression[0] = 's';
			hdr.compression[1] = 'm';
			hdr.compression[2] = 'p';
			hdr.compression[3] = 'l';
		}

		PutLittleEndianUINT32((byte *)&hdr.channelsinobj, obj->numtexchans);
		PutLittleEndianUINT32((byte *)&hdr.numfaces, obj->numfaces);
		PutLittleEndianUINT32((byte *)&hdr.numverts, obj->numverts);
		PutLittleEndianUINT32((byte *)&hdr.flags, obj->flags & obj_exportmask);
		PutLittleEndianINT32((byte *)&hdr.nameofs, nameofs);
		PutLittleEndianUINT32((byte *)&hdr.wirecolor, obj->wirecolor);
		PutLittleEndianFLOAT32((byte *)&hdr.pivot.x, obj->worldpos.x);
		PutLittleEndianFLOAT32((byte *)&hdr.pivot.y, obj->worldpos.y);
		PutLittleEndianFLOAT32((byte *)&hdr.pivot.z, obj->worldpos.z);
		PutLittleEndianUINT32((byte *)&hdr.numkeyframes, obj->numkeyframes);
		PutLittleEndianUINT32((byte *)&hdr.animsize, obj->animsize);

		if (obj->material)
		{	for (mtlindex=0; mtlindex<nummtls; mtlindex++)
			{	if (mtllist[mtlindex]==obj->material)
				{	PutLittleEndianINT32((byte *)&hdr.mtlnumber, mtlindex);
					break;
				}
			}
		}	else
		{	PutLittleEndianINT32((byte *)&hdr.mtlnumber, -1);
		}
		osfblockwrite(&hdr,sizeof(OSFobjhdr));
		osfblockwrite(&channelinfo[0],obj->numtexchans);

		lockmesh(obj,lockflag_read | lockflag_verts | lockflag_faces);
		unlockmesh(obj);

		// Write Faces
		uint32 facedata[3];
		for (spareindex=0; spareindex<obj->numfaces; spareindex++)
		{	PutLittleEndianUINT32((byte *)&facedata[0],obj->face[spareindex].v1);
			PutLittleEndianUINT32((byte *)&facedata[1],obj->face[spareindex].v2);
			PutLittleEndianUINT32((byte *)&facedata[2],obj->face[spareindex].v3);
			osfblockwrite(facedata,12);
		}

		// Write Vertices
		struct OSFvert
		{	float x,y,z, nx,ny,nz, uv[maxtexchannels*16];
		} osfvert;
		float *uvw[maxtexchannels];
		float3 *pos = obj->vertpos;
		float3 *nrm = obj->vertnorm;
		uintf vertsize = 6*sizeof(float);
		for (spareindex=0; spareindex<obj->numtexchans; spareindex++)
		{	vertsize += obj->texelements[spareindex] * sizeof(float);
			uvw[spareindex] = obj->texcoord[spareindex];
		}
		for (vertindex=0; vertindex<obj->numverts; vertindex++)
		{	PutLittleEndianFLOAT32((byte *)&osfvert.x, pos->x);
			PutLittleEndianFLOAT32((byte *)&osfvert.y, pos->y);
			PutLittleEndianFLOAT32((byte *)&osfvert.z, pos->z);
			PutLittleEndianFLOAT32((byte *)&osfvert.nx, nrm->x);
			PutLittleEndianFLOAT32((byte *)&osfvert.ny, nrm->y);
			PutLittleEndianFLOAT32((byte *)&osfvert.nz, nrm->z);
			float  *uv = &osfvert.uv[0];
			for (spareindex=0; spareindex<obj->numtexchans; spareindex++)
			{	if (obj->texelements[spareindex]>0) *uv++ = uvw[spareindex][0];
				if (obj->texelements[spareindex]>1) *uv++ = uvw[spareindex][1];
				if (obj->texelements[spareindex]>2) *uv++ = uvw[spareindex][2];
				if (obj->texelements[spareindex]>3) *uv++ = uvw[spareindex][3];
				uvw[spareindex] = (float *)incVertexPtr(uvw[spareindex],obj);
			}
			osfblockwrite((void *)&osfvert,vertsize);
			pos = (float3 *)incVertexPtr(pos,obj);
			nrm = (float3 *)incVertexPtr(nrm,obj);
		}
		endosfblock();
	}

	// ------------------------------------------------------------------------------------------------------
	// Write out all lights

	// ------------------------------------------------------------------------------------------------------
	// Finish up and free all resources
	writeosfblock(osfhandle, "END:",NULL,0);
	fileClose(osfhandle);
	fcfree(textpool);
	fcfree(texlist);
	fcfree(mtllist);
	return;
* /
/ *
	long i,k;
	// Count number of materials, textpool size requirements, numfaces, numverts, etc
	long nummats = 0;
	dword bytesinpool = 0;
	long numfaces = 0;
	long numverts = 0;
	long numpverts = 0;
	long numtverts = 0;
	bool newmat;
	char *textpoolbuf = (char *)fcalloc(4096,"ExportGroup Text buffer");
	dword textpoolbufsize = 4096;
	char stringbuf[256];

	logfile *osflog = newlogfile("logs\\exportosf.log");

	osflog->log("Exporting file %s based on group3d %x, containing %i objects",filename,g1,g1->numobjs);
	for (i=0; i<((long)g1->numobjs); i++)
	{	obj3d *o = g1->obj[i];
		Material *m = o->material;
		textpoolbuf = addstringtobuf(textpoolbuf,&bytesinpool,&textpoolbufsize,4096, o->name);
		numfaces +=	o->numfaces;
		numverts += o->numverts;
		if (o->numkeyframes>1) numpverts += o->numverts * (o->numkeyframes);
		numtverts += o->numverts * o->numtexchans;
		if (m)
		{
			newmat = true;
			for (k=0; k<i; k++)
			{	if (m==g1->obj[k]->material)
				{	newmat = false;
					osflog->log("%i: Object %s has material %s (aleady in use in %i)",i,o->name,m->name,i);
					break;
				}
			}
			if (newmat)
			{	nummats++;
				textpoolbuf = addstringtobuf(textpoolbuf,&bytesinpool,&textpoolbufsize,4096, m->name);
				k=0;
				while (k<8 && m->texture[k])
				{	filenameinfo(m->texture[k]->name);
					if ((byte)fileactualname[0]<255)
					{	sprintf(stringbuf,"%s.%s",fileactualname,fileextension);
						textpoolbuf = addstringtobuf(textpoolbuf,&bytesinpool,&textpoolbufsize,4096,stringbuf);
					}
					k++;
				}
				osflog->log("%i: Object %s has material %s (NEW)",i,o->name,m->name);
			}
		}
		else
			osflog->log("%i: Object %s has no material",i,o->name);
	}

	for (i=0; i<(long)g1->numlights; i++)
	{	textpoolbuf = addstringtobuf(textpoolbuf,&bytesinpool,&textpoolbufsize,4096, g1->lights[i]->name);
	}
	exporthandle = hFileCreate(filename);
	exportdword(0x2c);			// Header Size
	exportbyte(3);				// Version High Number
	exportbyte(1);				// Version Low  Number
	exportbyte(0);				// Animation Compression
	exportbyte(0);				// Bones Flag
	exportdword(g1->numlights);	// Number of lights
	exportdword(nummats);		// Number of materials
	exportdword(g1->numobjs);	// Number of objects
	exportdword(numfaces);		// Number of faces
	exportdword(numverts);		// Number of vertices
	exportdword(numpverts);		// Number of animation vertices
	exportdword(numtverts);		// Number of texture verticies
	exportdword(bytesinpool);	// Size of text pool
	exportdword(1);				// Flags (Have Normals)
	hFileWrite(exporthandle,textpoolbuf,bytesinpool);

	// This is where we would normally handle the matricies, but they're not yet done

	// Export Materials
	Material **matlookup = (Material **)fcalloc(ptrsize*nummats,"Material Lookup Table");
	dword nextmat = 0;
	for (i=0; i<(long)g1->numobjs; i++)
	{	obj3d *o = g1->obj[i];
		Material *m = o->material;
		if (m)
		{
			newmat = true;
			for (k=0; k<i; k++)
			{	if (m==g1->obj[k]->material)
				{	newmat = false;
					break;
				}
			}
			if (newmat)
			{	matlookup[nextmat++]=m;
				FixedPixelShader *ps = (FixedPixelShader *)m->PixelShader;
				exportdword((dword)(('M'<<24)+('t'<<16)+('l'<<8)+':'));
				exportdword(findstringinbuf(textpoolbuf,bytesinpool,m->name));
				exportdword(0xffffffff);	// ambient ###
				exportdword(0xffffffff);	// diffuse
				exportdword(0xffffffff);	// specular
				exportdword(0xff000000);	// emissive
				exportfloat(4.0f);			// spec_power
//				exportdword(m->ambient);
//				exportdword(m->diffuse);
//				exportdword(m->specular);
//				exportdword(m->emissive);
//				exportfloat(m->spec_power);
//				exportdword(m->flags);		// Update this line to export flags & final blendops
				exportbyte(m->minalpha);
				exportword(0);				// Predefined Script
				exportbyte(0);				// m->bias	###
				// Spare 4 bytes
				exportdword(0);
				// Determine number of texture maps
				dword numtex = 0;
				while (numtex<8 && m->texture[numtex])
				{	numtex++;
				}
				exportdword(numtex);
				for (k=0; k<(long)numtex; k++)
				{	if (m->texture[k])
					{	filenameinfo(m->texture[k]->name);
						if ((byte)fileactualname[0]<255)
						{	sprintf(stringbuf,"%s.%s",fileactualname,fileextension);
							exportdword(findstringinbuf(textpoolbuf,bytesinpool,stringbuf));
						}	else exportdword(0);
					}	else exportdword(0);

					exportbyte(ps->colorop[k]);
					exportbyte(ps->alphaop[k]);
					exportbyte(ps->channel[k]);
					exportbyte(0);				// ### m->modifier[k]);
					exportbyte(0x60);			// ### Was the old Material::mapping variable
				}
			}
		}
	}

	// Export each light
	for (i=0; i<(long)g1->numlights; i++)
	{	light3d *l = g1->lights[i];
		exportdword((dword)(('L'<<24)+('g'<<16)+('t'<<8)+':'));
		exportdword(findstringinbuf(textpoolbuf,bytesinpool,l->name));
		exportdword(l->flags);
		exportfloat(l->color.r);
		exportfloat(l->color.g);
		exportfloat(l->color.b);
		exportfloat(l->specular.r);
		exportfloat(l->specular.g);
		exportfloat(l->specular.b);
		exportfloat(l->matrix[mat_xpos]);
		exportfloat(l->matrix[mat_ypos]);
		exportfloat(l->matrix[mat_zpos]);
		exportfloat(l->matrix[ 8]);
		exportfloat(l->matrix[ 9]);
		exportfloat(l->matrix[10]);
		exportfloat(l->range);
		exportfloat(l->falloff);
		exportfloat(l->attenuation0);
		exportfloat(l->attenuation1);
		exportfloat(l->attenuation2);
		exportfloat(l->innerangle);
		exportfloat(l->outerangle);
	}

	// Export each object
	for (i=0; i<(long)g1->numobjs; i++)
	{	obj3d *o = g1->obj[i];
		exportdword((dword)(('O'<<24)+('b'<<16)+('j'<<8)+':'));
		exportdword(findstringinbuf(textpoolbuf,bytesinpool,o->name));
		exportdword(o->numfaces);
		exportdword(o->numverts);
		exportdword(o->numtexchans);
		exportdword(o->numkeyframes);
		exportdword(0);					// ### framecompression;
		exportdword(o->wirecolor);
		exportdword(0xffffffff);	// Matrix offset (NULL)
		bool matfound = false;
		if (o->material)
		{	for (k=0; k<nummats; k++)
				if (o->material==matlookup[k])
				{	exportlong(k);
					matfound=true;
					break;
				}
			if (!matfound)
				msg("ExportGroup Error",buildstr("Can't find the material for object %i (%s)",i,o->name));
		}
		else
			exportlong(-1);

		// Export face descriptions
		for (k=0; k<(long)o->numfaces; k++)
		{	exportword((word)o->face[k].v1);
			exportword((word)o->face[k].v2);
			exportword((word)o->face[k].v3);
		}

		// Write normals  TODO: Remove normals for objects with normals in posverts
		for (k=0; k<(long)o->numverts; k++)
		{	exportfloat(o->vertnorm[k].x);
			exportfloat(o->vertnorm[k].y);
			exportfloat(o->vertnorm[k].z);
		}

		// Write texture co-ords
		for (k=0; k<(long)(o->numverts*o->numtexchans); k++)
		{	exportfloat(o->texcoord[k][0]);
			exportfloat(o->texcoord[k][1]);
		}

		// Write positional verticies
		if (o->numkeyframes==1)
		{	for (k=0; k<(long)o->numverts; k++)
			{	exportfloat(o->vertpos[k].x+o->worldpos.x);
				exportfloat(o->vertpos[k].z+o->worldpos.z);
				exportfloat(o->vertpos[k].y+o->worldpos.y);
			}
		}	else
		{	dword ofs = 0;
			float3 *pvert = (float3 *)o->animdata;
			for (dword frame=0; frame<o->numkeyframes; frame++)
			{	for (k=0; k<(long)o->numverts; k++)
				{	exportfloat(pvert[k+ofs].x+o->worldpos.x);
					exportfloat(pvert[k+ofs].z+o->worldpos.z);
					exportfloat(pvert[k+ofs].y+o->worldpos.y);
				}
				ofs += o->numverts;
			}
		}
	}
	hFileClose(exporthandle);
	fcfree(matlookup);
}

void blankgroup(group3d *g)
{	g->numobjs	= 0;
	g->obj		= NULL;
	g->material = NULL;
	g->flags	= 0;
	g->uset		= 0;
	g->numframes= 0;
}
*/

// **********************************************************************************************
// ***									Fractal Generator									  ***
// **********************************************************************************************
dword	*accumbuffer;	// Accumulation buffer
word	*indexbuffer;	// Number of fractals that cover this pixel

dword	cirinten;
long	fracwidth,fracheight;

void castlineseg(long x1,long y1, long x2, long y2)
{	if (x2<0 || x1>=fracwidth) return;
	if (x1<0) x1 = 0;
	if (x2>=fracwidth) x2 = fracwidth-1;
	long ofs1 = (y1<<8)+x1;
	long ofs2 = (y2<<8)+x1;
	dword *inten1 = ((dword *)accumbuffer)+ofs1;
	dword *inten2 = ((dword *)accumbuffer)+ofs2;
	word *index1 = indexbuffer+ofs1;
	word *index2 = indexbuffer+ofs2;
	long len = x2-x1;
	for (long i=0; i<=len; i++)
	{	*(inten1++) += cirinten;
		*(inten2++) += cirinten;
		*(index1++) += 1;
		*(index2++) += 1;
	}
}

void castline(long x1,long y1, long x2,long y2)
{	if (y1<  0) y1+=fracheight;
	if (y2<  0) y2+=fracheight;
	if (y1>=fracheight) y1-=fracheight;
	if (y2>=fracheight) y2-=fracheight;
	castlineseg(x1,y1,x2,y2);
	if (x2>=fracwidth)
		castlineseg(x1-fracwidth,y1,x2-fracwidth,y2);
	if (x1<0)
		castlineseg(x1+fracwidth,y1,x2+fracwidth,y2);
}

byte *generatefractal(byte *dest,long width, long height, dword seed, long itterations,long radmax)
{	long i;
	long numpixels = width*height;
	// Allocate temporary data
	dword memneeded =	numpixels*6;				// Buffer to hold current value (dword)
	byte *buf = fcalloc(memneeded,"Fractal Generation Buffer");
	accumbuffer = (dword *)buf;	buf+=numpixels*4;
	indexbuffer = (word  *)buf;	buf+=numpixels*2;

	if (seed>0)
		srand(seed);

	for (i=0; i<numpixels; i++)
	{	indexbuffer[i]=0;
		accumbuffer[i] = 0;
	}

	fracwidth = width;
	fracheight = height;
    // Creating circles with random position, radius and inner colour.
    for (i=0; i<itterations; i++)
    {	// Plot real line
		long cx,cy,rad;
		float radsq;

		cx    = (long)(frand((float)width));	// Circle Center X position
		cy    = (long)(frand((float)height));	// Circle Center Y position
		rad   = (long)(frand((float)radmax));	// Circle Radius
		radsq = (float)(rad*rad);				// Circle Radius squared
		cirinten = (dword)frand(255);			// Circle's intensity

		for (long y=0; y<rad; y++)
		{	long x = (long)sqrt(radsq - y*y);
			long x1 = cx-x;
			long x2 = cx+x;
			castline(x1,cy-y,x2,cy+y+1);
		}
    }

	if (!dest) dest = fcalloc(numpixels,"Generated Fractal");

	for (i=0; i<numpixels; i++)
    {	if (indexbuffer[i]>0)
			dest[i] = (byte)(accumbuffer[i]/(indexbuffer[i]));
		else
			dest[i] = 0;
	}

	fcfree(accumbuffer);
	return dest;
}


vispoint *freevispoint = NULL, *usedvispoint = NULL;

vispoint *allocvispoint(void)
{	// Allocate a new visibility point
	vispoint *v;
	allocbucket(vispoint,v,flags,vis_memstruct,128,"Visibility Determination Points");
	return v;
}

void deletevispoint(vispoint *v)
{	// Delete a visibility point
	deletebucket(vispoint,v);
}

void clearvispoints(void)
{	// Clear vispoint data, ready for the next batch of tests (do this every frame)
	vispoint *v = usedvispoint;
	while (v)
	{	v->flags &= vis_memstruct;
		if (!test_fov(&v->pos,1))
			v->flags |= vis_culledFOV;
		v=v->next;
	}
}

//extern console *viscon;
/*
void testvispoints(obj3d *o)
{	// Perform vispoint tests on this object IGNORING MATERIAL SETTINGS
	vispoint *v = usedvispoint;
	while (v)
	{	if (v->flags & vis_culledBlock)
		{	v = v->next;
			continue;
		}

		bool needfulltest = false;
//		viscon->add("Testing Point %x against %s",v,o->name);
/ *
	Ray { P = starting point;
		  D = NORMALIZED direction vector;
		}
	Sphere { C = center point;
			 R = Radius;
		   }

	Q = ray.P - sphere.C;
	a0 = Q.Dot(Q) - sphere.R * sphere.R
	if (a0 <= 0)
	{
		// ray.P is inside the sphere
		return true;
	}
	//else ray.P is outside the sphere

	a1 = ray.D.Dot(Q);
	if (a1 >= 0)
	{
		// acute angle between P-C and D, C is "behind" ray
		return false;
	}

	//quadratic has a real root if discriminant is nonnegative
	//return (a1*a1 >= a0);
* /

		float qx,qy,qz;
		float dx,dy,dz;
		float a0,a1;
		// Q = ray.P - sphere.C;
		if (!o->matrix)
		{	qx = cammat[mat_xpos] - o->worldpos.x;
			qy = cammat[mat_ypos] - o->worldpos.y;
			qz = cammat[mat_zpos] - o->worldpos.z;
		}	else
		{	qx = o->matrix[mat_xpos] - o->worldpos.x;
			qy = o->matrix[mat_ypos] - o->worldpos.y;
			qz = o->matrix[mat_zpos] - o->worldpos.z;
		}
//		viscon->add("Q = ray.P - sphere.C");
//		viscon->add("                           Qx = %.2f - %.2f = %.2f",cammat[mat_xpos],o->worldx,qx);
//		viscon->add("                           Qy = %.2f - %.2f = %.2f",cammat[mat_ypos],o->worldy,qy);
//		viscon->add("                           Qz = %.2f - %.2f = %.2f",cammat[mat_zpos],o->worldz,qz);
		// 	a0 = Q.Dot(Q) - (sphere.R * sphere.R)
		a0 = _dotproduct(q,q) - o->radius_squared;
//		viscon->add("a0 = Q.Dot(Q) - sphere.R * sphere.R");
//		viscon->add("                           a0 = (%.2f * %.2f) + (%.2f * %.2f) + (%.2f * %.2f) - (%.2f * %.2f)",qx,qx,qy,qy,qz,qz,o->radius,o->radius);
//		viscon->add("                              = %.2f + %.2f + %.2f - (%.2f)",qx*qx,qy*qy,qz*qz,o->radius_squared);
		if (a0<=0) needfulltest = false;
		else
		{	dx = v->pos.x-cammat[mat_xpos];
			dy = v->pos.y-cammat[mat_ypos];
			dz = v->pos.z-cammat[mat_zpos];
			_normalize(d);
			a1 =_dotproduct(d,q);
			if (a1>=0) needfulltest = false;
			else
			if (a1*a1<a0) needfulltest = false;
			else v->flags |= vis_culledBlock;
		}
		v = v->next;
	}
}
*/
/*
void testvispoints(group3d *g)
{	// Perform vispoint tests on this group of objects
	for (dword i=0; i<g->numobjs; i++)
	{	obj3d *o = g->obj[i];
		if (o->material)
		{	if (!(o->material->flags & mtl_zsort))
				testvispoints(o);
		}	else
			testvispoints(o);
	}
}
*/

// Fast Math code
float normalize(float2 *vec)
{	float len = (float)sqrt(vec->x*vec->x + vec->y*vec->y);
	float ool = 1.0f / len;
	vec->x *= ool;
	vec->y *= ool;
	return len;
}

float normalize(float3 *vec)
{	float len = (float)sqrt(vec->x*vec->x + vec->y*vec->y + vec->z*vec->z);
	float ool = 1.0f / len;
	vec->x *= ool;
	vec->y *= ool;
	vec->z *= ool;
	return len;
}

float dotproduct(float2 *v1, float2 *v2)
{	return (v1->x*v2->x + v1->y*v2->y);
}

float dotproduct(float3 *v1, float3 *v2)
{	return (v1->x*v2->x + v1->y*v2->y + v1->z*v2->z);
}

void crossProduct(float3 *n, float3 *v1, float3 *v2)
{	n->x = v1->y * v2->z - v1->z * v2->y;
	n->y = v1->z * v2->x - v1->x * v2->z;
	n->z = v1->x * v2->y - v1->y * v2->x;

}

// **********************************************************************************************
// ***						CPU Independant multi-byte data fetch functions					  ***
// **********************************************************************************************

// Little Endian (Intel-Style) functions
#ifdef BIGENDIAN
uint32 GetLittleEndianUINT32(byte *ptr)
{	//return *(uint32 *)(buf);
	return	((uint32)(ptr[0])      ) +
			((uint32)(ptr[1]) <<  8) +
			((uint32)(ptr[2]) << 16) +
			((uint32)(ptr[3]) << 24) ;
}

uint16 GetLittleEndianUINT16(byte *ptr)
{	//return *(uint16 *)(buf);
	return	((uint16)(ptr[0])      ) +
			((uint16)(ptr[1]) <<  8) ;
}

float32 GetLittleEndianFLOAT32(byte *ptr)
{	dword tmp = GetLittleEndianUINT32(ptr);
	float32 *fltptr = (float32 *)&tmp;
	return *fltptr;
}

void PutLittleEndianUINT32(byte *ptr, uint32 value)
{	ptr[0] = (value & 0xff);
	ptr[1] = (value & 0xff00) >> 8;
	ptr[2] = (value & 0xff0000) >> 16;
	ptr[3] = (value & 0xff000000) >> 24;
}

void PutLittleEndianUINT16(byte *ptr, uint16 value)
{	ptr[0] = (value & 0xff);
	ptr[1] = (value & 0xff00) >> 8;
}

void PutLittleEndianFLOAT32(byte *ptr, float32 value)
{	uint32 *tmp = (uint32 *)&value;
	PutLittleEndianUINT32(ptr,*tmp);
}

#else
// Big-Endian (Motorola-style) Functions
uint32 GetBigEndianUINT32(byte *ptr)
{	//return *(uint32 *)(buf);
	return	((uint32)(ptr[3])      ) +
			((uint32)(ptr[2]) <<  8) +
			((uint32)(ptr[1]) << 16) +
			((uint32)(ptr[0]) << 24) ;
}
uint16 GetBigEndianUINT16(byte *ptr)
{	//return *(uint16 *)(buf);
	return	((uint16)(ptr[1])      ) +
			((uint16)(ptr[0]) <<  8) ;
}

float32 GetBigEndianFLOAT32(byte *ptr)
{	uint32_to_float tmp;
	tmp.i = GetBigEndianUINT32(ptr);
	return tmp.f;
}

void PutBigEndianUINT32(byte *ptr, uint32 value)
{	ptr[3] = (byte)((value & 0xff));
	ptr[2] = (byte)((value & 0xff00) >> 8);
	ptr[1] = (byte)((value & 0xff0000) >> 16);
	ptr[0] = (byte)((value & 0xff000000) >> 24);
}

void PutBigEndianUINT16(byte *ptr, uint16 value)
{	ptr[1] = (byte)((value & 0xff));
	ptr[0] = (byte)((value & 0xff00) >> 8);
}

void PutBigEndianFLOAT32(byte *ptr, float32 value)
{	uint32_to_float tmp;
	tmp.f = value;
	PutBigEndianUINT32(ptr,tmp.i);
}

// **************************************************************
// ***						Generic Storage					  ***
// **************************************************************
// struct genericStorage
// {	byte data[256];
//	byte flags;
//	byte size;
//	byte junk[sizeof(uintf)-2];
//	genericStorage *next, *prev, *link;.
//} *freegenericStorage, *usedgenericStorage;

// **************************************************************
// ***						Stack System					  ***
// **************************************************************
struct stackdata
{	byte		flags;
	uint16		size;
	byte		fill[sizeof(uintf)-3];			// buffer to keep data aligned
	byte		data[256];
	stackdata	*prev, *next, *link;
} *freestackdata, *usedstackdata;

class oemStack : public cStack
{	public:
		stackdata	*firstpage;
		stackdata	*lastpage;

				~oemStack(void);				// Delete the stack
		void	push(void *src, uintf size);	// Push [size] bytes to the stack
		bool	pop(void *dst, uintf size);		// Pull [size] bytes from stack
		void	debug(void);
};

cStack *newStack(void)							// Create a new cStack
{	oemStack *s = new oemStack;
	s->size = 0;
	allocbucket(stackdata, s->firstpage, flags, Generic_MemoryFlag8, 256, "Stack Space");
	s->firstpage->size=0;
	s->lastpage = s->firstpage;
	return s;
}

oemStack::~oemStack(void)
{
}

void oemStack::debug(void)
{
}

void oemStack::push(void *src, uintf pushsize)
{	stackdata *newstack;

	if (pushsize==0) return;

	size += pushsize;

	byte *src8 = (byte *)src;
	while (pushsize>=(256-(uintf)lastpage->size))
	{	uintf copysize = 256-(uintf)lastpage->size;
		memcopy((void *)&lastpage->data[lastpage->size],src8,copysize);
		allocbucket(stackdata, newstack, flags, Generic_MemoryFlag8, 256, "Stack Space");
		src8+=copysize;
		lastpage->size+=(uint16)copysize;
		lastpage->link = newstack;
		lastpage = newstack;
		newstack->size = 0;
		pushsize-=copysize;
	}
	if (pushsize)
	{	memcopy((void *)&lastpage->data[lastpage->size],src8,pushsize);
		lastpage->size += (uint16)pushsize;
	}
}

bool oemStack::pop(void *dst, uintf popsize)
{	if (size<popsize) return false;

	byte *dst8 = (byte *)dst;

	while (lastpage->size < popsize)
	{	byte *writeptr = dst8 + popsize-lastpage->size;
		memcopy(writeptr,lastpage->data,lastpage->size);
		popsize -= lastpage->size;
		stackdata *tmp = firstpage;
		while (tmp->link!=lastpage) tmp = tmp->link;
		tmp->link=NULL;
		deletebucket(stackdata, lastpage);
		lastpage = tmp;
	}
	if (popsize>0)
	{	lastpage->size -= (uint16)popsize;
		memcopy(dst8, lastpage->data + lastpage->size, popsize);
	}
	return true;
}

void initutils(void)
{	freestackdata = NULL;
	usedstackdata = NULL;
//	freegenericStorage = NULL;
//	usedgenericStorage = NULL;
}

void killutils(void)
{	killbucket(stackdata, flags, Generic_MemoryFlag8);
//	killbucket(genericStorage, flags, Generic_MemoryFlag8);
}

#endif
