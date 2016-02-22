// ********************************************************************************
// ***																			***
// ***								Light Manager								***
// ***																			***
// ********************************************************************************
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include "fc_h.h"
#include <bucketSystem.h>

#define lightalloc 117	// (34*4 bytes) = just under 32kb
#define llinkalloc 1024
#define maxhwlights 64	// Change this as more powerful hardware becomes available

struct lightlink						// A lightlink chain is not something you have to concern yourself with
{	light3d		*light;
	dword		flags;
	lightlink	*chain;
	lightlink	*prev,*next;
};

lightlink *usedlightlink,*freelightlink;		//, *useFreeLightsOnly;
light3d *freelights,*usedlightsF,*usedlightsM;	// Bucket variables for lighting system
light3d *activelight[maxhwlights];				// Array of current in-use lights
dword numlights;								// Number of lights supported by renderer
dword numfreelights;							// Number of current freerange lights.  Managed lights = numlights-numfreelights
uintf lastmanagedFixedlightcount;
uintf lastmanagedScriptedlightcount;

class fclighting : public fc_lighting
{	public:
	light3d *newlight(dword pool = light_freerange)			{return ::newlight(pool);};
	void _deletelight(light3d *l)							{		::_deletelight(l);};
	void deletealllights(void)								{		::deletealllights();};
	light3d *getfirstlight(long pool = light_freerange)		{return	::getfirstlight(pool);};
	void movelighttopool(long pool)							{		::movelighttopool(pool);};
//	void managelighting(obj3d *o, light3d **lightlist, dword numlights,dword method=0)	{::managelighting(o,lightlist,numlights,method);};
} *OEMlighting = NULL;

fc_lighting *initlighting(void)
{	OEMlighting = new fclighting();
	// This function requires the video driver to have been initialized (number of supported lights)
	freelights = NULL;
	usedlightsF = NULL;
	usedlightsM = NULL;
	usedlightlink = NULL;
	freelightlink = NULL;
	numfreelights=0;
	lastmanagedFixedlightcount = 0;			// Number of lights used during the last draw operation

	// Obtain renderer's lighting capabilities
//	if (videoinfo.NumLights>maxhwlights) videoinfo.NumLights = maxhwlights;
	numlights = maxhwlights;
	return OEMlighting;
}

void killlighting(void)
{	if (!OEMlighting) return;	// Haven't been initialized yet, exit without doing anything
	deletealllights();
	// Free up lighting cache
	light3d *tmp,*next;
	// move parent buckets into another list
	light3d *del = NULL;
	tmp = freelights;
	while (tmp)
	{	next = tmp->next;
		if (tmp->flags & light_memstruct)
		{	tmp->next = del;
			del = tmp;
		}
		tmp = next;
	}
	// Remove it all
	tmp = del;
	while (tmp)
	{	next = tmp->next;
		fcfree(tmp);
		tmp = next;
	}

	killbucket(lightlink,flags,light_memstruct);
	delete OEMlighting;
}

void deletealllights(void)	// This is a lazy man's function - you should never call this
{	// Clears out all lights from memory
	while (usedlightsF)
		_deletelight(usedlightsF);
	while (usedlightsM)
		_deletelight(usedlightsM);
}

light3d *newlight(dword pool)
{	if (!freelights)
	{	freelights = (light3d *)fcalloc(lightalloc*sizeof(light3d),"Light Cache");
		for (long i=0; i<lightalloc; i++)
		{	freelights[i].index = -1;
			freelights[i].flags = 0;
			freelights[i].prev = &freelights[i-1];
			freelights[i].next = &freelights[i+1];
		}
		freelights[0].prev = NULL;
		freelights[lightalloc-1].next = NULL;
		freelights[0].flags = light_memstruct;
	}

	light3d *tmp = freelights;
	tmp->flags &= light_memstruct;				// Clear all bits except memstruct
	freelights = tmp->next;
	if (freelights)
		freelights->prev = NULL;

	if (numfreelights>=numlights-1) pool = light_managed;

	if (pool==light_freerange)
	{	tmp->next = usedlightsF;
		if (usedlightsF)
			usedlightsF->prev = tmp;
		usedlightsF = tmp;
		tmp->index = numfreelights;
		activelight[numfreelights++] = tmp;
	}	else
	{	tmp->flags |= light_managed;
		tmp->next = usedlightsM;
		if (usedlightsM)
			usedlightsM->prev = tmp;
		usedlightsM = tmp;
		tmp->index = -1;
	}

	tmp->flags |= pool;

	makeidentity(tmp->matrix);
	tmp->name			= "Unnamed";
	tmp->flags			|=light_direction;
	tmp->color.r		= 1.0f;
	tmp->color.g		= 1.0f;
	tmp->color.b		= 1.0f;
	tmp->specular.r		= 0.0f;
	tmp->specular.g		= 0.0f;
	tmp->specular.b		= 0.0f;
	tmp->attenuation0	= 1.0f;
	tmp->attenuation1	= 0.001f;
	tmp->attenuation2	= 0.0f;
	tmp->innerangle		= 3.14159f / 7;
	tmp->outerangle		= 3.14159f / 4;
	tmp->range			= horizon*2;
	tmp->falloff		= 1.0f;
	return tmp;
}

void _deletelight(light3d *l)
{	l->flags &= ~light_enabled;
	l->color.b = 0;
	l->color.g = 0;
	l->color.r = 0;
	l->specular.b = 0;
	l->specular.g = 0;
	l->specular.r = 0;
	changelight(l);

	// Scan list of freerange lights to see if it's a freerange or managed light
	light3d *fl = usedlightsF;
	while (fl)
	{	if (fl==l)
		{	numfreelights--;
			break;
		}
		fl = fl->next;
	}

	if (usedlightsF == l)
		usedlightsF = l->next;
	if (usedlightsM == l)
		usedlightsM = l->next;
	if (l->prev)
		l->prev->next = l->next;
	if (l->next)
		l->next->prev = l->prev;
	l->next = freelights;
	if (freelights)
		freelights->prev = l;
	freelights=l;
	l->prev = NULL;
}

Material *LightBillboard = NULL;

void DrawLightBillboard(light3d *l, float w, float h)
{	msg("Incomplete Code","DrawLightBillboard requires a prebuilt material with adequate shaders");
	if (!LightBillboard)
	{	LightBillboard = newmaterial("Light Corona");
		Material *m = LightBillboard;

		m->texture[0] = newTextureFromFile("flare1.tga",0);
		//ps->colorop[0] = blendop_modulate;
		//ps->alphaop[0] = blendop_unchanged;
		m->texture[1] = copytexture(m->texture[0]);
		//ps->colorop[1] = blendop_add;
		//ps->alphaop[1] = blendop_unchanged;
		//m->channel[1] = 0;
		m->finalBlendOp = finalBlend_add;
		m->finalBlendSrcScale = blendParam_One;
		m->finalBlendDstScale = blendParam_One;
		m->diffuse.a = 1.0f;	m->diffuse.r = 0.0f;	m->diffuse.g = 0.0f;	m->diffuse.b = 0.0f;
	}
//	fs = (FixedVertexShader *)LightBillboard->VertexShader;
//	fs->emissive.r = l->color.r;
//	fs->emissive.g = l->color.g;
//	fs->emissive.b = l->color.b;
//	drawbillboard(LightBillboard,(float3 *)&l->matrix[mat_xpos],w,h);
}

// ************************************************************************
// ***						Dynamic Lighting							***
// ************************************************************************
//extern logfile *lightinglog;				// ### Temporary only

//dword mlights = 0;

/*
void managelighting(obj3d *o, light3d **lightlist, dword numlights,dword method)
{
//	logfile *lightinglog = newlogfile("lighting.txt");//NULL);

	light3d *closest[maxhwlights];
	dword i,j;
	float ox,oy,oz;
	lightlink *newlink;

	// If there is any current lighting information, release it all back to the free pool
	newlink = (lightlink *)o->lighting;
	while (newlink)
	{	lightlink *ll = newlink->chain;
		deletebucket(lightlink,newlink);
		newlink = ll;
	}
	o->lighting = NULL;

	if (o->matrix)
	{	ox = o->matrix[mat_xpos];
		oy = o->matrix[mat_ypos];
		oz = o->matrix[mat_zpos];
	}	else
	{	ox = o->worldpos.x;
		oy = o->worldpos.y;
		oz = o->worldpos.z;
	}

	uintf maxlights = maxhwlights;//videoinfo.NumLights;
	if (o->maxlights<maxlights) maxlights = o->maxlights;

	for (i=0; i<maxlights; i++)
		closest[i]=NULL;

	long lastcell = 0;

	light3d *l;
	if (lightlist==NULL)
	{	l = usedlightsM;
		while (l)
		{	if (!(l->flags & light_enabled))
			{	l = l->next;
				continue;
			}
			// Calculate distance to from object to light
			float dx = (ox-l->matrix[mat_xpos]);
			float dy = (oy-l->matrix[mat_ypos]);
			float dz = (oz-l->matrix[mat_zpos]);
			float dist = dx*dx+dy*dy+dz*dz;				// dist = sqr(distance) between light and object
			float allowdist = l->range + o->radius;
			allowdist = allowdist*allowdist;			// allowdist = sqr(allowable distance)
			l->manager = dist;
			if (allowdist>dist)
			{
//				lightinglog->log("Light %s is Within Range",l->name);
				j=0;
				while (j<maxlights)
				{	if (closest[j]==NULL)
					{	closest[j]=l;
						lastcell++;// = j+1;
//						lightinglog->log("Light %s inserted into slot %i because it was empty",l->name,j);
						break;
					}
					if (closest[j]->manager>l->manager)
					{
//						lightinglog->log("Light %s is swapped with %s in slot %i",l->name,closest[j]->name,j);
						uintf k = lastcell;
						if (k>maxlights-1) k = maxlights-1;
						for (;k>j; k--)
							closest[k] = closest[k-1];
						closest[j]=l;
						lastcell++;
						break;
					}
					j++;
				}
			}
			l=l->next;
		}
	}	else
	{	for (i=0; i<numlights; i++)
		{	l = lightlist[i];
			if ((l->flags & light_managed)==0)
			{	//lightinglog->log("Light %s (%x) is freerange and therefore not considdered for management",l->name,l->flags);
				continue;
			}
			if (!(l->flags & light_enabled)) continue;

			// Calculate distance to from object to light
			float dx = (ox-l->matrix[mat_xpos]);
			float dy = (oy-l->matrix[mat_ypos]);
			float dz = (oz-l->matrix[mat_zpos]);
			float dist = dx*dx+dy*dy+dz*dz;
			float allowdist = l->range + o->radius;
			allowdist = allowdist*allowdist;
			l->manager = dist;
			if (allowdist>dist)
			{	//lightinglog->log("Light %s is Within Range",l->name);
				j=0;
				while (j<maxlights)
				{	if (closest[j]==NULL)
					{	closest[j]=l;
						lastcell = j+1;
						//lightinglog->log("Light %s inserted into slot %i because it was empty",l->name,j);
						break;
					}
					if (closest[j]->manager>l->manager)
					{	//lightinglog->log("Light %s is swapped with %s in slot %i",l->name,closest[j]->name,j);
						uintf k = lastcell;
						if (k<maxlights-1) k = maxlights-1;
						for (;k>j; k--)
							closest[k-1] = closest[k];
						closest[j]=l;
					}
					j++;
				}	// while (j<maxlights)
			}	// if within allowable distance
		}	// for each light
	}	// else (lightlist)

	if (lastcell>0)
	{
/ *		bool usevs = false;
		if (o->material)
		{	if (o->material->flags & mtl_vertexshader)
			{	usevs = true;
			}
		}
		if (usevs)
		{	// Uses a scripted vertex shader - Lights must be added in SEQUENTIAL ORDER
			for (long l=0; l<lastcell; l++)
			{	allocbucket(lightlink, newlink,flags,light_memstruct, 1024, "Lighting Buffer");
				newlink->light = closest[l];
				newlink->chain = (lightlink *)o->lighting;
				o->lighting = newlink;
//				lightinglog->log("Object '%s' uses light '%s' at distance %.2f",o->name,closest[l]->name,closest[l]->manager);
			}
		}	else
* /
		{	// Uses a fixed function vertex shader - Lights must be added in REVERSE ORDER
			for (long l=lastcell-1; l>=0; l--)
			{	allocbucket(lightlink, newlink,flags,light_memstruct, 1024, "Lighting Buffer");
				newlink->light = closest[l];
				newlink->chain = (lightlink *)o->lighting;
				o->lighting = newlink;
//				lightinglog->log("Object '%s' uses light '%s' at distance %.2f",o->name,closest[l]->name,closest[l]->manager);
			}
		}
	}	else
	{	o->lighting = NULL;
	}

/ *
	newlink = o->lighting;
	while (newlink)
	{	lightinglog->log("*** '%s'",newlink->light->name);
		newlink=newlink->next;
	}
	lightinglog->log("");
* /
//delete(lightinglog);
}
*/
/*
void managelighting(newobj *o, light3d **lightlist, dword numlights,dword method)
{
//	logfile *lightinglog = newlogfile("lighting.txt");//NULL);

	light3d *closest[maxhwlights];
	dword i,j;
	float ox,oy,oz;
	lightlink *newlink;

	// If there is any current lighting information, release it all back to the free pool
	newlink = (lightlink *)o->lighting;
	while (newlink)
	{	lightlink *ll = newlink->chain;
		deletebucket(lightlink,newlink);
		newlink = ll;
	}
	o->lighting = NULL;

	ox = o->matrix[mat_xpos];
	oy = o->matrix[mat_ypos];
	oz = o->matrix[mat_zpos];

	uintf maxlights = maxhwlights;//videoinfo.NumLights;
	if (o->maxlights<maxlights) maxlights = o->maxlights;

	for (i=0; i<maxlights; i++)
		closest[i]=NULL;

	long lastcell = 0;

	light3d *l;
	if (lightlist==NULL)
	{	l = usedlightsM;
		while (l)
		{	if (!(l->flags & light_enabled))
			{	l = l->next;
				continue;
			}
			// Calculate distance to from object to light
			float dx = (ox-l->matrix[mat_xpos]);
			float dy = (oy-l->matrix[mat_ypos]);
			float dz = (oz-l->matrix[mat_zpos]);
			float dist = dx*dx+dy*dy+dz*dz;				// dist = sqr(distance) between light and object
			float allowdist = l->range + o->dimensions.radius;
			allowdist = allowdist*allowdist;			// allowdist = sqr(allowable distance)
			l->manager = dist;
			if (allowdist>dist)
			{
//				lightinglog->log("Light %s is Within Range",l->name);
				j=0;
				while (j<maxlights)
				{	if (closest[j]==NULL)
					{	closest[j]=l;
						lastcell++;// = j+1;
//						lightinglog->log("Light %s inserted into slot %i because it was empty",l->name,j);
						break;
					}
					if (closest[j]->manager>l->manager)
					{
//						lightinglog->log("Light %s is swapped with %s in slot %i",l->name,closest[j]->name,j);
						uintf k = lastcell;
						if (k>maxlights-1) k = maxlights-1;
						for (;k>j; k--)
							closest[k] = closest[k-1];
						closest[j]=l;
						lastcell++;
						break;
					}
					j++;
				}
			}
			l=l->next;
		}
	}	else
	{	for (i=0; i<numlights; i++)
		{	l = lightlist[i];
			if ((l->flags & light_managed)==0)
			{	//lightinglog->log("Light %s (%x) is freerange and therefore not considdered for management",l->name,l->flags);
				continue;
			}
			if (!(l->flags & light_enabled)) continue;

			// Calculate distance to from object to light
			float dx = (ox-l->matrix[mat_xpos]);
			float dy = (oy-l->matrix[mat_ypos]);
			float dz = (oz-l->matrix[mat_zpos]);
			float dist = dx*dx+dy*dy+dz*dz;
			float allowdist = l->range + o->dimensions.radius;
			allowdist = allowdist*allowdist;
			l->manager = dist;
			if (allowdist>dist)
			{	//lightinglog->log("Light %s is Within Range",l->name);
				j=0;
				while (j<maxlights)
				{	if (closest[j]==NULL)
					{	closest[j]=l;
						lastcell = j+1;
						//lightinglog->log("Light %s inserted into slot %i because it was empty",l->name,j);
						break;
					}
					if (closest[j]->manager>l->manager)
					{	//lightinglog->log("Light %s is swapped with %s in slot %i",l->name,closest[j]->name,j);
						uintf k = lastcell;
						if (k<maxlights-1) k = maxlights-1;
						for (;k>j; k--)
							closest[k-1] = closest[k];
						closest[j]=l;
					}
					j++;
				}	// while (j<maxlights)
			}	// if within allowable distance
		}	// for each light
	}	// else (lightlist)

	if (lastcell>0)
	{
/ *		bool usevs = false;
		if (o->material)
		{	if (o->material->flags & mtl_vertexshader)
			{	usevs = true;
			}
		}
		if (usevs)
		{	// Uses a scripted vertex shader - Lights must be added in SEQUENTIAL ORDER
			for (long l=0; l<lastcell; l++)
			{	allocbucket(lightlink, newlink,flags,light_memstruct, 1024, "Lighting Buffer");
				newlink->light = closest[l];
				newlink->chain = (lightlink *)o->lighting;
				o->lighting = newlink;
//				lightinglog->log("Object '%s' uses light '%s' at distance %.2f",o->name,closest[l]->name,closest[l]->manager);
			}
		}	else
* /
		{	// Uses a fixed function vertex shader - Lights must be added in REVERSE ORDER
			for (long l=lastcell-1; l>=0; l--)
			{	allocbucket(lightlink, newlink,flags,light_memstruct, 1024, "Lighting Buffer");
				newlink->light = closest[l];
				newlink->chain = (lightlink *)o->lighting;
				o->lighting = newlink;
//				lightinglog->log("Object '%s' uses light '%s' at distance %.2f",o->name,closest[l]->name,closest[l]->manager);
			}
		}
	}	else
	{	o->lighting = NULL;
	}

/ *
	newlink = o->lighting;
	while (newlink)
	{	lightinglog->log("*** '%s'",newlink->light->name);
		newlink=newlink->next;
	}
	lightinglog->log("");
* /
//delete(lightinglog);
}
*/
extern void (*disablelights)(uintf firstlight, uintf lastlight);	// LightManagement Code only - Disable unseen lights

dword lightflagsneeded = light_managed | light_enabled;

/*
void preplighting(obj3d *o)
{	uintf maxlights;
	uintf lightnum;

	lightlink *ll = (lightlink *)o->lighting;
/ *
	if (o->material->flags & mtl_vertexshader)
	{	// Object is using a SCRIPTED vertex shader
		// 	float4 Light0Pos, Light0Col;							// (RO) Light 0 Position / Color
		// ShaderVars GlobalShaderVars;
		float4 *gsc0 = &GlobalShaderVars.GSC_0;
		if (!ll)
		{	// No lighting info in object - Delete all managed lights from the light system
			for (uintf i=numfreelights; i<lastmanagedScriptedlightcount; i++)
				*lightcolptr[i] = *gsc0;
			lastmanagedFixedlightcount = numfreelights;
			return;
		}

		lightnum = numfreelights;	// find next free light slot

		// Calculate the maximum number of lights to be made available
		if (o->maxlights > 8)
			maxlights = 8;
		else
			maxlights = o->maxlights;

		// Add each light one at a time
		while (ll!=NULL && lightnum<maxlights)
		{	light3d *l = ll->light;
			if ((l->flags & lightflagsneeded) == lightflagsneeded)
			{	lightposptr[lightnum]->x = l->matrix[mat_xpos];
				lightposptr[lightnum]->y = l->matrix[mat_ypos];
				lightposptr[lightnum]->z = l->matrix[mat_zpos];
				lightposptr[lightnum]->w = 1;

				lightcolptr[lightnum]->r = l->color.r;
				lightcolptr[lightnum]->g = l->color.g;
				lightcolptr[lightnum]->b = l->color.b;
				lightnum++;
			}
			ll=ll->chain;
		}

		// Remove any excess lights at the end
		for (uintf i=lightnum; i<lastmanagedScriptedlightcount; i++)
			*lightcolptr[i] = *gsc0;
		lastmanagedScriptedlightcount = lightnum;
	}	else
* /	{	// Object is using a FIXED vertex shader
		if (!ll)
		{	// No lighting info in object - Delete all managed lights from the light system
			if (lastmanagedFixedlightcount > numfreelights)
				disablelights(numfreelights, lastmanagedFixedlightcount);
			lastmanagedFixedlightcount = numfreelights;
			return;
		}

		lightnum = numfreelights;	// find next free light slot

		// Calculate the maximum number of lights to be made available
		if (maxhwlights / *videoinfo.NumLights* /<o->maxlights)
			maxlights = maxhwlights;//videoinfo.NumLights;
		else
			maxlights = o->maxlights;

		// Add each light one at a time
		while (ll!=NULL && lightnum<maxlights)
		{	light3d *l = ll->light;
			if ((l->flags & lightflagsneeded) == lightflagsneeded)
			{	l->index = lightnum++;
				changelight(l);
			}
			ll=ll->chain;
		}

		// Remove any excess lights at the end
		if (lastmanagedFixedlightcount > lightnum)
			disablelights(lightnum, lastmanagedFixedlightcount);
		lastmanagedFixedlightcount = lightnum;
	}
}
*/

light3d *getfirstlight(long pool)
{	if (pool==light_freerange)
		return usedlightsF;
	else
		return usedlightsM;
}

void movelighttopool(long pool)
{	// ### This code has not been written yet
}

// ************************************************************************
// ***							Baked Lighting							***
// ************************************************************************
/*
#define BLmemneeded(obj) (obj->numverts * 2 * sizeof(float3))

float3 *BakedLightCreateBuffer(obj3d *obj)
{	uintf memneeded = BLmemneeded(obj);
	float3 *buf = (float3 *)fcalloc(memneeded,"Baked Lighting Buffer");
	memfill(buf, 0, memneeded);
	return buf;
}

void BakedLightsClear(obj3d *obj, float3 *buffer)
{	uintf memneeded = BLmemneeded(obj);
	memfill(buffer, 0, memneeded);
}
*/
/*
void BakedLightAdd(group3d *world, obj3d *obj, light3d *light, float3 *buffer)
{	msg("Incomplete Code","BakedLightAdd not yet ported to flameVM");
	float3 color;				// light generated for this vertex from the light
	float3 specular;			// specular light generated for this vertex from the light
	float3 lightvec;			// Object-space light direction vector
	float3 vec2light;			// Vector from the vertex to the light
	float  dist2light;			// Distance from the vector to the light
	float3 ObjSpaceLightPos;	// The light's position in relation to the object
	float3 *vert;				// Pointer to the current vertex within the object
	float3 *norm;				// Pointer to the normal vector for this vertex
	float  dot;					// Dot product of the light direction vector and vec2light
	float attenuation;			// Attenuation factor
	Matrix mtx;					// pointer to the object matrix
	Matrix tmpmtx;
	uintf i;
	bool lightingadded;

	if (obj->matrix)
	{	matrixinvert(mtx,obj->matrix);
	}
	else
	{	makeidentity(tmpmtx);
		tmpmtx[mat_xpos] = obj->worldpos.x;
		tmpmtx[mat_ypos] = obj->worldpos.y;
		tmpmtx[mat_zpos] = obj->worldpos.z;
		matrixinvert(mtx,tmpmtx);
	}

	transformPointByMatrix(&ObjSpaceLightPos, mtx, (float3 *)&light->matrix[mat_xpos]);
	color.x = light->matrix[mat_zvecx];	// We're using color as a temp variable here!
	color.y = light->matrix[mat_zvecy];
	color.z = light->matrix[mat_zvecz];
	rotateVectorByMatrix(&lightvec, mtx, &color);

	vert = obj->vertpos;
	norm = obj->vertnorm;

	float cosOuter = (float)cos(light->outerangle*0.5f);
	float cosInner = (float)cos(light->innerangle*0.5f);

	for (i=0; i<obj->numverts; i++)
	{	vec2light.x = vert->x - ObjSpaceLightPos.x;
		vec2light.y = vert->y - ObjSpaceLightPos.y;
		vec2light.z = vert->z - ObjSpaceLightPos.z;
		dist2light = normalize(&vec2light);
		lightingadded = true;

		switch(light->flags & light_typemask)
		{	case light_direction:
				dot = -dotproduct(&lightvec,norm);
				if (dot>0)
				{	color.x = light->color.r * dot;
					color.y = light->color.g * dot;
					color.z = light->color.b * dot;
				}	else
				{	lightingadded = false;
				}
				specular.x = 0;
				specular.y = 0;
				specular.z = 0;
				break;
			case light_omni:
				dot = -dotproduct(&vec2light,norm);
				if ((dist2light>light->range)||(dot<0))
				{	lightingadded = false;
					break;
				}
				attenuation = 1/(light->attenuation0 + light->attenuation1*dist2light + light->attenuation2*dist2light*dist2light);
				dot *= attenuation;
				color.x = light->color.r * dot;
				color.y = light->color.g * dot;
				color.z = light->color.b * dot;
				specular.x = 0;
				specular.y = 0;
				specular.z = 0;
				break;
			case light_spot:
			{	dot = dotproduct(&lightvec,&vec2light);
				float dot2 = -dotproduct(&vec2light,norm);
				if ((dist2light>light->range) || (dot<cosOuter) || (dot2<0))
				{	lightingadded = false;
					break;
				}
				if (dot<cosInner)
				{	// Apply Falloff
					dot = (float)pow((dot - cosOuter) / (cosInner - cosOuter),light->falloff);
					specular.x = 0;
					specular.y = 0;
					specular.z = 0;
				}	else
				{	specular.x = light->specular.r * dot;
					specular.y = light->specular.g * dot;
					specular.z = light->specular.b * dot;
				}

				attenuation = dot/(light->attenuation0 + light->attenuation1*dist2light + light->attenuation2*dist2light*dist2light);
				dot *= dot2 * attenuation;
				color.x = light->color.r * dot;
				color.y = light->color.g * dot;
				color.z = light->color.b * dot;
				break;
			}
			default:
				lightingadded = false;
				specular.x = 0;
				specular.y = 0;
				specular.z = 0;
				color.x = 0;
				color.y = 0;
				color.z = 0;
				break;
		}

		if (lightingadded && world)
		{	// Perform a collision detection to see if the light source is obscured from this vertex
			float3 linestart;
			collideData cd;
			linestart.x = light->matrix[mat_xpos] + vec2light.x*0.0001f;
			linestart.y = light->matrix[mat_ypos] + vec2light.y*0.0001f;
			linestart.z = light->matrix[mat_zpos] + vec2light.z*0.0001f;
			cd.lineStart = *(float3*)&light->matrix[mat_xpos];
			cd.direction = vec2light;
			cd.distance = dist2light * 0.9999f;
			cd.FaceNormal = NULL;

			for (uintf j=0; j<world->numobjs; j++)
			{	obj3d *o = world->obj[j];
				if (o==obj) continue;
				cd.target = o;
				cd.startFace = 0;
				cd.flags = 0;
				if (collide(&cd))
				{	lightingadded = false;
					break;
				}
			}
		}

		if (lightingadded)
		{	uintf bufindex = i<<1;
			buffer[bufindex].x += color.x;
			buffer[bufindex].y += color.y;
			buffer[bufindex].z += color.z;
			bufindex++;
			buffer[bufindex].x += specular.x;
			buffer[bufindex].y += specular.y;
			buffer[bufindex].z += specular.z;
		}
		vert = (float3 *)incVertexPtr(vert, obj);
		norm = (float3 *)incVertexPtr(norm, obj);
	}
}
*/
/*
#define BLclamp(dest, _src)				\
	{	src = _src;						\
		if (src<0.0f) dest=0;			\
		else if (src>1.0f) dest=255;	\
		else dest = (uint32)(src * 255);\
	}
*/
/*
void BakeLightsIntoObject(obj3d *obj, float3 *buffer)
{	uint32 *diffuse = obj->vertdiffuse;
	uint32 *specular = obj->vertspecular;
	float src;
	uint32 r,g,b;

	for (uintf i=0; i<obj->numverts; i++)
	{	if (obj->flags & obj_hasdiffuse)
		{	BLclamp(r,buffer[i<<1].x);
			BLclamp(g,buffer[i<<1].y);
			BLclamp(b,buffer[i<<1].z);
			*diffuse = (r<<16)+(g<<8)+(b);
			diffuse = (uint32 *)incVertexPtr(diffuse, obj);
		}
		if (obj->flags & obj_hasspecular)
		{	BLclamp(r,buffer[(i<<1)+1].x);
			BLclamp(g,buffer[(i<<1)+1].y);
			BLclamp(b,buffer[(i<<1)+1].z);
			*specular = (r<<16)+(g<<8)+(b);
			specular = (uint32 *)incVertexPtr(specular, obj);
		}
	}
}
*/
