/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#include "fc_h.h"

struct swstate
{	long			vpx,vpy,vpw,vph;		// The viewport
	dword			renderstate,rendermode;
	dword			fillmode;
	dword			ambientlight,specularcolor,fogcolor;
	//D3DCULL			cullmode;
	float			fogstart,fogend,horizon,bumpscale,zoom;
	byte			shademode;
};

class sw : public SWrender
{	private:
		Matrix	projectionmat;
		Matrix	viewscalemat;
		Matrix	projview;				// Cache of projectionmat * viewscalemat
		Matrix	invcammat;
		Matrix	camprojview;			// Cache of projview * invcammat
		Matrix	tmpmat;					// Used when an object does not have a matrix

		// Field of view planes (see D3Drender for details)
		float	fovnx[6],fovny[6],fovnz[6],fovd[6];

		dword	RenderWidth,RenderHeight;
		float	camd;					// Camera's distance from the origin (probably not needed anymore)
		long	hfov,vfov;
		swstate	currentswstate,backupswstate;

		void	calcvisvert(float3 *v,long fx,long fy);
		void	setfov(long plane, float px, float py, float pz, float nx, float ny, float nz);
		void	calcfov(long plane, float3 *v0, float3 *v1,float *tempmat);
		void	GenerateViewScaleMatrix(void);
		void	GenerateProjectionMatrix(void);
		void	restorestate(void *state);

	public:
				sw(void);
		void	setcammatrix(void);
		void	changeviewport(long x1,long y1,long x2,long y2);
		void	sethorizon(float h);
		float	test_fov(float3 *pos,float rad);
//		dword	testobjvis(obj3d *obj);
//		void	drawobj3d(obj3d *obj);
//		bool	drawobj3doncolor(obj3d *obj,dword color);
};

struct functype
{	char name[25];
	void *fnptr;
} swvidfuncs[]={	{"xxx",			NULL},
};

dword oc_color;

void swrender_interface(char *fname, void **func)
{	// Given a text description of a function, return a pointer to it
//	long numfuncs = sizeof(swvidfuncs)/sizeof(functype);
//	for (long i=0; i<numfuncs; i++)
//	{	if (txticmp(swvidfuncs[i].name,fname)==0)
//		{	*func = swvidfuncs[i].fnptr;
//			return;
//		}
//	}
	msg("Unknown Video Driver function",fname);
}

// ****************************************************************************************
// ***						Display Control Matrices									***
// ****************************************************************************************

void sw::calcvisvert(float3 *v,long fx,long fy)
{	// Calculates a visibility vertex
	Matrix mat;
	memcopy(mat, cammat, sizeof(float) * 16);
	rotatearoundy(mat,fx);
	rotatearoundx(mat,fy);
	fixrotations(mat);
	v->x = horizon * mat[mat_zvecx] + mat[mat_xpos];
	v->y = horizon * mat[mat_zvecy] + mat[mat_ypos];
	v->z = horizon * mat[mat_zvecz] + mat[mat_zpos];
}

void sw::setfov(long plane, float px, float py, float pz, float nx, float ny, float nz)
{	// Sets the field of view plane.
	fovnx[plane]=nx;
	fovny[plane]=ny;
	fovnz[plane]=nz;
	fovd [plane]=px*nx + py*ny + pz*nz;
}

void sw::calcfov(long plane, float3 *v0, float3 *v1,float *tempmat)
{	// Given 2 points and a matrix, calculate a fov plane)
	// Generate 2 vectors from the provided data
	float v1x = v1->x - v0->x;
	float v1y = v1->y - v0->y;
	float v1z = v1->z - v0->z;

	float v2x = tempmat[mat_xpos] - v0->x;
	float v2y = tempmat[mat_ypos] - v0->y;
	float v2z = tempmat[mat_zpos] - v0->z;

	// Use those 2 vectors to calculate a normal
	float nx,ny,nz;
	_crossproduct(n,v1,v2);
	_normalize(n);
	// Send this data to setfov function
	setfov(plane,tempmat[12], tempmat[13], tempmat[14], nx,ny,nz);
}

void sw::setcammatrix(void)
{	// Invert matrix
	matrixinvert(invcammat,cammat);
	matmult(camprojview,projview,invcammat);

	float3 v[4];

	calcvisvert(&v[0],3600-(hfov),3600-(vfov));
	calcvisvert(&v[1],3600-(hfov),     (vfov));
	calcvisvert(&v[2],     (hfov),3600-(vfov));
	calcvisvert(&v[3],     (hfov),     (vfov));

	// Near Plane
	setfov(5,cammat[12],
			cammat[13],
			cammat[14],
			cammat[8], cammat[9], cammat[10]);
	// Far Plane
	setfov(0,cammat[12] + cammat[ 8] * horizon,
			cammat[13] + cammat[ 9] * horizon,
			cammat[14] + cammat[10] * horizon,
			-cammat[ 8],- cammat[ 9],- cammat[10]);
	// Left Plane
	calcfov(1,&v[0],&v[1],cammat);
	// Right Plane
	calcfov(2,&v[3],&v[2],cammat);
	// Top Plane
	calcfov(3,&v[2],&v[0],cammat);
	// Bottom Plane
	calcfov(4,&v[1],&v[3],cammat);
	camd = cammat[8]*cammat[mat_xpos] + cammat[9]*cammat[mat_ypos] + cammat[10]*cammat[mat_zpos];
}

void sw::changeviewport(long x1,long y1,long x2,long y2)
{	currentswstate.vpx		= x1;
    currentswstate.vpy		= y1;
    currentswstate.vpw		= RenderWidth  = x2-x1;
	currentswstate.vph		= RenderHeight = y2-y1;

	viewscalemat[0] = (float)currentswstate.vpw/2;
	viewscalemat[5] = -(float)currentswstate.vph/2;
	viewscalemat[10]= 1;
	viewscalemat[12]= (float)currentswstate.vpx+(float)currentswstate.vpw/2;
	viewscalemat[13]= (float)currentswstate.vpy+(float)currentswstate.vph/2;
	viewscalemat[14]= 0;
	matmult(projview,viewscalemat,projectionmat);
	matmult(camprojview,projview,invcammat);
}

void sw::GenerateProjectionMatrix(void)
{	// Generate D3D Projection Matrix
	float fFOV = 0.392699081698724154807830422909938f/currentswstate.zoom;	// pi / 8		// This can be considdered a ZOOM factor (0.4 = standard, 0.1 = zoom in, 1.0 = zoom out
	float aspect = (float)currentswstate.vph / (float)currentswstate.vpw;
	float w = aspect * (float)( cos(fFOV)/sin(fFOV) );		// 0.75 * (0.9238795 / 0.38268343)
	float h =   1.0f * (float)( cos(fFOV)/sin(fFOV) );		// 1.0  * (0.9238795 / 0.38268343)
	float Q = currentswstate.horizon / ( currentswstate.horizon - 1 );

	float fFOVdeg = (fFOV/(3.14159265f*2))*3600;
	hfov = (long)(fFOVdeg / aspect); // (180/8 = 22.5.  aspect = 0.75.  22.5/aspect = 30).
	vfov = (long)(fFOVdeg);			 // (180/8 = 22.5).
	projectionmat[ 0] = w;
	projectionmat[ 5] = h;
	projectionmat[10] = Q;
	projectionmat[11] = 1.0f;
	projectionmat[14] = -Q;

	matmult(projview,viewscalemat,projectionmat);
	matmult(camprojview,projview,invcammat);
}

void sw::GenerateViewScaleMatrix(void)
{	viewscalemat[0] = (float)(currentswstate.vpw/2)+0.5f;
	viewscalemat[5] = -(float)(currentswstate.vph/2)+0.5f;
	viewscalemat[10]= 1-0;//(*DDK->horizon)-1;
	viewscalemat[12]= (float)currentswstate.vpx+(float)currentswstate.vpw/2;
	viewscalemat[13]= (float)currentswstate.vpy+(float)currentswstate.vph/2;
	viewscalemat[14]= 0;
	matmult(projview,viewscalemat,projectionmat);
	matmult(camprojview,projview,invcammat);
}

void sw::sethorizon(float h)
{	currentswstate.horizon = h;	// ### for full SW renderer, also set 'horizon' to h
    GenerateProjectionMatrix();
    //SetTransform( D3DTS_PROJECTION, (D3DMATRIX *)&projectionmat );
}

void sw::restorestate(void *state)
{	if (state)
	{	// Copy this state to the default state variables
		memcopy(&currentswstate,state,sizeof(swstate));
	}
//	setrenderstate( D3DRS_CULLMODE, currentstate.cullmode );
//	setrenderstate( D3DRS_SHADEMODE, currentstate.shademode );
//	d3d_setfillmode(currentstate.fillmode);
//	d3d_setambientlight(currentstate.ambientlight);
//	d3d_setspecularcolor(currentstate.specularcolor);
//	dword renderstate = currentstate.renderstate;
//	myviewport *v = &currentstate.Viewport;
	changeviewport(currentswstate.vpx, currentswstate.vpy, currentswstate.vpw, currentswstate.vph);
	sethorizon(currentswstate.horizon);
//	if (renderstate & RENDERSTATE_ZBUFFER) d3d_enableZbuffer();
//		else d3d_disableZbuffer();
//	if (renderstate & RENDERSTATE_ZWRITES) d3d_enableZwrites();
//		else d3d_disableZwrites();
//	if (renderstate & RENDERSTATE_ANTIALIAS) d3d_enableAA();
//		else d3d_disableAA();
//	if (renderstate & RENDERSTATE_SPECULAR) d3d_enablespecular();
//		else d3d_disablespecular();
//	if (renderstate & RENDERSTATE_DITHER) d3d_enabledithering();
//		else d3d_disabledithering();
//	if (renderstate & RENDERSTATE_FOG)
//	{	setrenderstate(D3DRS_FOGENABLE, TRUE);
//		d3d_setfog(currentstate.fogcolor, currentstate.fogstart, currentstate.fogend);
//	}	else d3d_disablefog();
//	setrenderstate(D3DRS_ALPHATESTENABLE , TRUE);
//	setrenderstate(D3DRS_ALPHAFUNC,D3DCMP_GREATEREQUAL);
//	for (long i=0; i<maxlights; i++)
//	{	if ((usedlight[i] & 1))		// Bit 0 = light in use.  Bit 1 = light enabled
//			d3d_changelight(lightparent[i]);
//		if ((usedlight[i] & 2))
//			d3d_reenablelight(lightparent[i]);
//	}
//
//	for (i=0; i<8; i++)
//	{	// Luminance Scale (only on tex formats with luminance)
//		SetTextureStageState( i, D3DTSS_BUMPENVLSCALE, F2DW(4.0f) );
//		// Luminance Offset - doesn't appear to do much at all
//		SetTextureStageState( i, D3DTSS_BUMPENVLOFFSET, F2DW(0.0f) );
//		// Lumpiness scale
//		SetTextureStageState( i, D3DTSS_BUMPENVMAT00, F2DW(currentstate.bumpscale) );
//		SetTextureStageState( i, D3DTSS_BUMPENVMAT01, F2DW(0.0f) );
//		SetTextureStageState( i, D3DTSS_BUMPENVMAT10, F2DW(0.0f) );
//		SetTextureStageState( i, D3DTSS_BUMPENVMAT11, F2DW(currentstate.bumpscale) );
//		// Set up anisotropic filterring factor
//		if (bestmin == D3DTEXF_ANISOTROPIC)
//			SetTextureStageState( i, D3DTSS_MAXANISOTROPY, devicecaps.MaxAnisotropy );
//	}
//
//	cleartexturecache();
}


// This routine renders a flat-bottomed triangle to the screen.  The
// reason the triangle must have a flat bottom is so that the delta
// X values for the left and right sides can then be guaranteed to
// never change over the entire span of the triangle, making the code
// simpler and faster.
//
#define swpolyshift 14
#define swpolymult  0x4000
void FB_Triangle( int x1, int y1,  int x2, int y23,  int x3,  dword color )
{	long dx_left, dx_width;
	long curx_left, curx_right, curx_width;

	long Height = (y23 - y1);
	if (!Height) return;

	// FC's hline function cannot handle lines with a negative width (it just culls them), so if the width is negative,
	// flip the triangle.
	if (x2>x3)
	{	long tmp = x2;
		x2 = x3;
		x3 = tmp;
	}

	// Determine the change in X (i.e. slope) for left and right sides
	dx_left  = (x2 - x1)/Height;
	dx_width = (x3 - x2)/Height;

	curx_left = curx_right = x1;
	curx_width = 1<<swpolyshift;
	// Draw each scan line from top to bottom
	pset(x1,y1,color);
	if ( y1 < y23 )
	{	if (y23>clipy2) y23=clipy2;
		for ( int y = y1+1; y <= y23; y++ )
		{	// Adjust X coordinates
			curx_left  += dx_left;
			curx_width += dx_width;
			hline( curx_left>>swpolyshift, y, curx_width>>swpolyshift, color );
		}
	}
	else
	{	if (y23<clipy1) y23=clipy1;
		for ( int y = y1-1; y >= y23; y-- )
		{	// Adjust X coordinates
			curx_left  -= dx_left;
			curx_width -= dx_width;
			hline( curx_left>>swpolyshift, y, curx_width>>swpolyshift, color );
		}
	}
}

extern uintf	*ylookup;

// --------------------------- hline API Command -------------------------------
void hline_oc(long x, long y, long width, dword color, bool *pixdrawn)
{	if (x>=clipx2 || y<clipy1 || y>=clipy2) return;

	if (x<clipx1)
    {	width-=(clipx1-x);
    	x=clipx1;
    }
    if (x+width>clipx2)
    {	width=clipx2-x;
    }
    if (width<1) return;

	dword *dest = (dword*)((byte *)lfb+ylookup[y]+(x<<2));

	if (*pixdrawn)
	{	for (long i=0; i<width; i++)
		{	if (*dest==oc_color) *dest=color;
			dest++;
		}
	}	else
	{	for (long i=0; i<width; i++)
			if (*dest==oc_color)
			{	*dest=color;
				*pixdrawn = true;
				for (; i<width; i++)
					if (*dest==oc_color)
						*dest=color;
					else
						dest++;
			}	else
			dest++;
	}
}

void FB_Triangle_oc( int x1, int y1,  int x2, int y23,  int x3,  dword color, bool *pixdrawn )
{	long dx_left, dx_width;
	long curx_left, curx_right, curx_width;

	long Height = (y23 - y1);
	if (!Height) return;

	// FC's hline function cannot handle lines with a negative width (it just culls them), so if the width is negative,
	// flip the triangle.
	if (x2>x3)
	{	long tmp = x2;
		x2 = x3;
		x3 = tmp;
	}

	// Determine the change in X (i.e. slope) for left and right sides
	dx_left  = (x2 - x1)/Height;
	dx_width = (x3 - x2)/Height;

	curx_left = curx_right = x1;
	curx_width = 1<<swpolyshift;
	// Draw each scan line from top to bottom
//	pset(x1,y1,color);
	if ( y1 < y23 )
	{	if (y23>clipy2) y23=clipy2;
		for ( int y = y1+1; y <= y23; y++ )
		{	// Adjust X coordinates
			curx_left  += dx_left;
			curx_width += dx_width;
			hline_oc( curx_left>>swpolyshift, y, curx_width>>swpolyshift, color,pixdrawn );
		}
	}
	else
	{	if (y23<clipy1) y23=clipy1;
		for ( int y = y1-1; y >= y23; y-- )
		{	// Adjust X coordinates
			curx_left  -= dx_left;
			curx_width -= dx_width;
			hline_oc( curx_left>>swpolyshift, y, curx_width>>swpolyshift, color, pixdrawn );
		}
	}
}


// This routine is the basic triangle routine core to the AstVGA
// class.  It works by splitting a triangle horizontally in half
// so that you have two flat sided triangles.  These two subtriangles
// are then drawn independently.
void sw_DrawTriangle( float vx1,float vy1,  float vx2, float vy2,  float vx3, float vy3, dword color)
{	long tmp;
	long x1 = (long)(vx1*swpolymult);
	long y1 = (long)(vy1*swpolymult);
	long x2 = (long)(vx2*swpolymult);
	long y2 = (long)(vy2*swpolymult);
	long x3 = (long)(vx3*swpolymult);
	long y3 = (long)(vy3*swpolymult);

	// Sort our vertices from top to bottom
	if ( y3 < y1 )
	{	tmp = y3;
		y3 = y1;
		y1 = tmp;
		tmp = x3;
		x3 = x1;
		x1 = tmp;
	}

	if ( y2 < y1 )
	{	tmp = y2;
		y2 = y1;
		y1 = tmp;
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}

	if ( y3 < y2 )
	{	tmp = y3;
		y3 = y2;
		y2 = tmp;
		tmp = x3;
		x3 = x2;
		x2 = tmp;
	}

	if ((y3-y1)<swpolymult) return;		// poly is too small to draw (would cause a 'divide by 0' error)

	// If the triangle has no horizontal edges then split it into
	// two triangles that do.
	if ( y2 != y1 && y3 != y1 && y3 != y2 )
	{	long	xn;
		long	ratio;

		// Determine new x value:
		//   xn = ( x3 - x1 ) * ( ( y2 - y1 ) / ( y3 - y1 ) ) + x1

		// Precompute the ratio of ( y2 - y1 ) / ( y3 - y1 )
		ratio = ( y2 - y1 ) / (( y3 - y1 )>>swpolyshift);

		// xn = ( ( x3 - x1 ) * ratio ) + x1
		xn =   (long)( (( x3 - x1 )>>swpolyshift) * ratio ) + x1;

		// Draw the two subtriangles
		if ( xn < x2 )
		{	FB_Triangle( x1, y1>>swpolyshift, xn, y2>>swpolyshift, x2, color );
			FB_Triangle( x3, y3>>swpolyshift, x2, y2>>swpolyshift, xn, color );
		}	else
		{	FB_Triangle( x1, y1>>swpolyshift, x2, y2>>swpolyshift, xn, color );
			FB_Triangle( x3, y3>>swpolyshift, x2, y2>>swpolyshift, xn, color );
		}
	}
	// Top is flat
	else if ( y1 == y2 )
	{	if ( x1 < x2 )
		{	FB_Triangle( x3, y3>>swpolyshift, x2, y2>>swpolyshift, x1, color );
		}	else
		{	FB_Triangle( x3, y3>>swpolyshift, x1, y2>>swpolyshift, x2, color );
		}
	}
	// Bottom is flat
	else if ( y2 == y3 )
	{	if ( x2 < x3 )
		{	FB_Triangle( x1, y1>>swpolyshift, x2, y2>>swpolyshift, x3, color );
		}	else
		{	FB_Triangle( x1, y1>>swpolyshift, x3, y2>>swpolyshift, x2, color );
		}
	}

}

// On Color  version of DrawTriangle
void sw_DrawTriangle_oc( float vx1,float vy1,  float vx2, float vy2,  float vx3, float vy3, dword color,bool *pixdrawn)
{	long tmp;
	long x1 = (long)(vx1*swpolymult);
	long y1 = (long)(vy1*swpolymult);
	long x2 = (long)(vx2*swpolymult);
	long y2 = (long)(vy2*swpolymult);
	long x3 = (long)(vx3*swpolymult);
	long y3 = (long)(vy3*swpolymult);

	// Sort our vertices from top to bottom
	if ( y3 < y1 )
	{	tmp = y3;
		y3 = y1;
		y1 = tmp;
		tmp = x3;
		x3 = x1;
		x1 = tmp;
	}

	if ( y2 < y1 )
	{	tmp = y2;
		y2 = y1;
		y1 = tmp;
		tmp = x2;
		x2 = x1;
		x1 = tmp;
	}

	if ( y3 < y2 )
	{	tmp = y3;
		y3 = y2;
		y2 = tmp;
		tmp = x3;
		x3 = x2;
		x2 = tmp;
	}

	if ((y3-y1)<swpolymult) return;		// poly is too small to draw (would cause a 'divide by 0' error)

	// If the triangle has no horizontal edges then split it into
	// two triangles that do.
	if ( y2 != y1 && y3 != y1 && y3 != y2 )
	{	long	xn;
		long	ratio;

		// Determine new x value:
		//   xn = ( x3 - x1 ) * ( ( y2 - y1 ) / ( y3 - y1 ) ) + x1

		// Precompute the ratio of ( y2 - y1 ) / ( y3 - y1 )
		ratio = ( y2 - y1 ) / (( y3 - y1 )>>swpolyshift);

		// xn = ( ( x3 - x1 ) * ratio ) + x1
		xn =   (long)( (( x3 - x1 )>>swpolyshift) * ratio ) + x1;

		// Draw the two subtriangles
		if ( xn < x2 )
		{	FB_Triangle_oc( x1, y1>>swpolyshift, xn, y2>>swpolyshift, x2, color, pixdrawn );
			FB_Triangle_oc( x3, y3>>swpolyshift, x2, y2>>swpolyshift, xn, color, pixdrawn );
		}	else
		{	FB_Triangle_oc( x1, y1>>swpolyshift, x2, y2>>swpolyshift, xn, color, pixdrawn );
			FB_Triangle_oc( x3, y3>>swpolyshift, x2, y2>>swpolyshift, xn, color, pixdrawn );
		}
	}
	// Top is flat
	else if ( y1 == y2 )
	{	if ( x1 < x2 )
		{	FB_Triangle_oc( x3, y3>>swpolyshift, x2, y2>>swpolyshift, x1, color, pixdrawn );
		}	else
		{	FB_Triangle_oc( x3, y3>>swpolyshift, x1, y2>>swpolyshift, x2, color, pixdrawn );
		}
	}
	// Bottom is flat
	else if ( y2 == y3 )
	{	if ( x2 < x3 )
		{	FB_Triangle_oc( x1, y1>>swpolyshift, x2, y2>>swpolyshift, x3, color, pixdrawn );
		}	else
		{	FB_Triangle_oc( x1, y1>>swpolyshift, x3, y2>>swpolyshift, x2, color, pixdrawn );
		}
	}

}

float sw::test_fov(float3 *pos,float rad)
{	// Returns TRUE if object is visible
	for (long i=0; i<5; i++) // 6
	{	if ((pos->x*fovnx[i] + pos->y*fovny[i] + pos->z*fovnz[i] - fovd[i]) < rad)
			return -1;
	}
	return rad + pos->x*fovnx[5] + pos->y*fovny[5] + pos->z*fovnz[5] - fovd[5];	// ### Need to verify this line
}

/*
dword sw::testobjvis(obj3d *obj)
{	// Returns non-zero if this object is visible
	float x,y,z;
//	if (currentstate.cullmode == D3DCULL_CCW) return 1;

	if (obj->matrix)
	{	x = obj->matrix[mat_xpos];
		y = obj->matrix[mat_ypos];
		z = obj->matrix[mat_zpos];
	}	else
	{	x = obj->worldpos.x;
		y = obj->worldpos.y;
		z = obj->worldpos.z;
	}

	float rad = -obj->radius;

	for (long i=0; i<6; i++) // 6
	{	if ((x*fovnx[i] + y*fovny[i] + z*fovnz[i] - fovd[i]) < rad)
			return 0;
	}
	return 1;
}
*/

/*
#define sw_minz 1
void sw::drawobj3d(obj3d *obj)
{	msg("Incomplete Code","sw::drawObj3dOnColor not yet ported to FlameVM");
/ *	dword col,i;
	Matrix usemat;
	float sx1,sx2,sx3, sy1,sy2,sy3;
	float nx1,nx2, ny1,ny2, ratio;

	if (!(obj->flags & obj_nofov))
		if (!testobjvis(obj)) return;

	long _clipx1 = clipx1;
	long _clipx2 = clipx2;
	long _clipy1 = clipy1;
	long _clipy2 = clipy2;

	clipperSetMax();
	clipperShrink(currentswstate.vpx, currentswstate.vpy, currentswstate.vpx+currentswstate.vpw, currentswstate.vpy+currentswstate.vph);

	if (obj->matrix)
		matmult(usemat,camprojview,obj->matrix);
	else
	{	tmpmat[12]=obj->worldpos.x;
		tmpmat[13]=obj->worldpos.y;
		tmpmat[14]=obj->worldpos.z;
		matmult(usemat,camprojview,tmpmat);
	}

//	if (obj->material)
//		col = obj->material->diffuse;
//	else
		col = obj->wirecolor;

	for (i=0; i<obj->numfaces; i++)
	{	dword polyclip = 0;
		float x1,x2,x3,y1,y2,y3,z1,z2,z3;

		float3 *v1 = (float3 *)&obj->vertpos[obj->face[i].v1];
		float3 *v2 = (float3 *)&obj->vertpos[obj->face[i].v2];
		float3 *v3 = (float3 *)&obj->vertpos[obj->face[i].v3];

		x1 = v1->x * usemat[0] + v1->y * usemat[4] + v1->z * usemat[ 8] + usemat[12];
		y1 = v1->x * usemat[1] + v1->y * usemat[5] + v1->z * usemat[ 9] + usemat[13];
		z1 = v1->x * usemat[3] + v1->y * usemat[7] + v1->z * usemat[11] + usemat[15];
		if (z1<sw_minz) polyclip |= 1;

		x2 = v2->x * usemat[0] + v2->y * usemat[4] + v2->z * usemat[ 8] + usemat[12];
		y2 = v2->x * usemat[1] + v2->y * usemat[5] + v2->z * usemat[ 9] + usemat[13];
		z2 = v2->x * usemat[3] + v2->y * usemat[7] + v2->z * usemat[11] + usemat[15];
		if (z2<sw_minz) polyclip |= 2;

		x3 = v3->x * usemat[0] + v3->y * usemat[4] + v3->z * usemat[ 8] + usemat[12];
		y3 = v3->x * usemat[1] + v3->y * usemat[5] + v3->z * usemat[ 9] + usemat[13];
		z3 = v3->x * usemat[3] + v3->y * usemat[7] + v3->z * usemat[11] + usemat[15];
		if (z3<sw_minz) polyclip |= 4;

		switch(polyclip)
		{	case 0:		// Special case - No clipped vertices
				sx1 = x1/z1;
				sx2 = x2/z2;
				sx3 = x3/z3;
				sy1 = y1/z1;
				sy2 = y2/z2;
				sy3 = y3/z3;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);
				continue;

			case 1:		// Vertex 1 is behind, create 2 new polys
				ratio = (z2 - sw_minz) / (z2 - z1);
				nx1 = x2 - (x2 - x1) * ratio;
				ny1 = y2 - (y2 - y1) * ratio;			// n1 = Edge 1/2
				sx1 = nx1/sw_minz;
				sx2 = x2/z2;
				sx3 = x3/z3;
				sy1 = ny1/sw_minz;
				sy2 = y2/z2;
				sy3 = y3/z3;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);	// Draw N1-2-3
				ratio = (z3 - sw_minz) / (z3 - z1);
				nx2 = x3 - (x3 - x1) * ratio;
				ny2 = y3 - (y3 - y1) * ratio;			// n2 = Edge 1/3
				sx1 = x3/z3;
				sx2 = nx2/sw_minz;
				sx3 = nx1/sw_minz;
				sy1 = y3/z3;
				sy2 = ny2/sw_minz;
				sy3 = ny1/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)			// Draw 3-N2-N1
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);
				continue;

			case 2:		// Vertex 2 is behind, create 2 new polys
				ratio = (z3 - sw_minz) / (z3 - z2);
				nx1 = x3 - (x3 - x2) * ratio;
				ny1 = y3 - (y3 - y2) * ratio;			// n1 = Edge 3/2
				sx1 = nx1/sw_minz;
				sx2 = x3/z3;
				sx3 = x1/z1;
				sy1 = ny1/sw_minz;
				sy2 = y3/z3;
				sy3 = y1/z1;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);	// Draw N1-2-3
				ratio = (z1 - sw_minz) / (z1 - z2);
				nx2 = x1 - (x1 - x2) * ratio;
				ny2 = y1 - (y1 - y2) * ratio;			// n2 = Edge 1/2
				sx1 = x1/z1;
				sx2 = nx2/sw_minz;
				sx3 = nx1/sw_minz;
				sy1 = y1/z1;
				sy2 = ny2/sw_minz;
				sy3 = ny1/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)			// Draw 3-N2-N1
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);
				continue;


			case 3:		// Vertex 1 & 2 is behind, create 1 new polys	### Not Verified ###
				ratio = (z3 - sw_minz) / (z3 - z1);
				nx1 = x3 - (x3 - x1) * ratio;
				ny1 = y3 - (y3 - y1) * ratio;

				ratio = (z3 - sw_minz) / (z3 - z2);
				nx2 = x3 - (x3 - x2) * ratio;
				ny2 = y3 - (y3 - y2) * ratio;

				sx1 = nx1/sw_minz;
				sx2 = x3/z3;
				sx3 = nx2/sw_minz;
				sy1 = ny1/sw_minz;
				sy2 = y3/z3;
				sy3 = ny2/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);
				continue;

			case 4:		// Vertex 3 is behind, create 2 new polys	### Not Verified ###
				ratio = (z1 - sw_minz) / (z1 - z3);
				nx1 = x1 - (x1 - x3) * ratio;
				ny1 = y1 - (y1 - y3) * ratio;			// n1 = Edge 1/3
				sx1 = nx1/sw_minz;
				sx2 = x1/z1;
				sx3 = x2/z2;
				sy1 = ny1/sw_minz;
				sy2 = y1/z1;
				sy3 = y2/z2;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);	// Draw 1-2-N1
				ratio = (z2 - sw_minz) / (z2 - z3);
				nx2 = x2 - (x2 - x3) * ratio;
				ny2 = y2 - (y2 - y3) * ratio;			// n2 = Edge 2/3
				sx1 = x2/z2;
				sx2 = nx2/sw_minz;
				sx3 = nx1/sw_minz;
				sy1 = y2/z2;
				sy2 = ny2/sw_minz;
				sy3 = ny1/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)			// Draw 3-N2-N1
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);
				continue;

			case 5:		// Vertex 3 & 1 is behind, create 1 new polys
				ratio = (z2 - sw_minz) / (z2 - z1);
				nx1 = x2 - (x2 - x1) * ratio;
				ny1 = y2 - (y2 - y1) * ratio;

				ratio = (z2 - sw_minz) / (z2 - z3);
				nx2 = x2 - (x2 - x3) * ratio;
				ny2 = y2 - (y2 - y3) * ratio;

				sx1 = nx1/sw_minz;
				sx2 = x2/z2;
				sx3 = nx2/sw_minz;
				sy1 = ny1/sw_minz;
				sy2 = y2/z2;
				sy3 = ny2/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);
				continue;

			case 6:		// Vertex 3 & 2 is behind, create 1 new polys
				ratio = (z1 - sw_minz) / (z1 - z2);
				nx1 = x1 - (x1 - x2) * ratio;
				ny1 = y1 - (y1 - y2) * ratio;

				ratio = (z1 - sw_minz) / (z1 - z3);
				nx2 = x1 - (x1 - x3) * ratio;
				ny2 = y1 - (y1 - y3) * ratio;

				sx1 = x1/z1;
				sx2 = nx1/sw_minz;
				sx3 = nx2/sw_minz;
				sy1 = y1/z1;
				sy2 = ny1/sw_minz;
				sy3 = ny2/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle(sx1,sy1, sx2,sy2, sx3,sy3, col);
				continue;

			case 7:		// special case - entire polygon clipped
				continue;

		}
	}
	clipperSetMax();
	clipperShrink(_clipx1,_clipy1,_clipx2,_clipy2);
	objsdrawn++;
	polysdrawn+=obj->numfaces;
* /
}
*/
/*
bool sw::drawobj3doncolor(obj3d *obj,dword color)
{	msg("Incomplete Code","sw::drawObj3dOnColor not yet ported to FlameVM");
/ *	dword col,i;
	Matrix usemat;
	float sx1,sx2,sx3, sy1,sy2,sy3;
	float nx1,nx2, ny1,ny2, ratio;
	bool  pixdrawn = false;
	oc_color = color;

	if (!(obj->flags & obj_nofov))
		if (!testobjvis(obj)) return false;

	long _clipx1 = clipx1;
	long _clipx2 = clipx2;
	long _clipy1 = clipy1;
	long _clipy2 = clipy2;

	clipperSetMax();
	clipperShrink(currentswstate.vpx, currentswstate.vpy, currentswstate.vpx+currentswstate.vpw, currentswstate.vpy+currentswstate.vph);

	if (obj->matrix)
		matmult(usemat,camprojview,obj->matrix);
	else
	{	tmpmat[12]=obj->worldpos.x;
		tmpmat[13]=obj->worldpos.y;
		tmpmat[14]=obj->worldpos.z;
		matmult(usemat,camprojview,tmpmat);
	}

//	if (obj->material)
//		col = obj->material->diffuse;
//	else
		col = obj->wirecolor;

	for (i=0; i<obj->numfaces; i++)
	{	dword polyclip = 0;
		float x1,x2,x3,y1,y2,y3,z1,z2,z3;

		float3 *v1 = (float3 *)&obj->vertpos[obj->face[i].v1];
		float3 *v2 = (float3 *)&obj->vertpos[obj->face[i].v2];
		float3 *v3 = (float3 *)&obj->vertpos[obj->face[i].v3];

		x1 = v1->x * usemat[0] + v1->y * usemat[4] + v1->z * usemat[ 8] + usemat[12];
		y1 = v1->x * usemat[1] + v1->y * usemat[5] + v1->z * usemat[ 9] + usemat[13];
		z1 = v1->x * usemat[3] + v1->y * usemat[7] + v1->z * usemat[11] + usemat[15];
		if (z1<sw_minz) polyclip |= 1;

		x2 = v2->x * usemat[0] + v2->y * usemat[4] + v2->z * usemat[ 8] + usemat[12];
		y2 = v2->x * usemat[1] + v2->y * usemat[5] + v2->z * usemat[ 9] + usemat[13];
		z2 = v2->x * usemat[3] + v2->y * usemat[7] + v2->z * usemat[11] + usemat[15];
		if (z2<sw_minz) polyclip |= 2;

		x3 = v3->x * usemat[0] + v3->y * usemat[4] + v3->z * usemat[ 8] + usemat[12];
		y3 = v3->x * usemat[1] + v3->y * usemat[5] + v3->z * usemat[ 9] + usemat[13];
		z3 = v3->x * usemat[3] + v3->y * usemat[7] + v3->z * usemat[11] + usemat[15];
		if (z3<sw_minz) polyclip |= 4;

		switch(polyclip)
		{	case 0:		// Special case - No clipped vertices
				sx1 = x1/z1;
				sx2 = x2/z2;
				sx3 = x3/z3;
				sy1 = y1/z1;
				sy2 = y2/z2;
				sy3 = y3/z3;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);
				continue;

			case 1:		// Vertex 1 is behind, create 2 new polys
				ratio = (z2 - sw_minz) / (z2 - z1);
				nx1 = x2 - (x2 - x1) * ratio;
				ny1 = y2 - (y2 - y1) * ratio;			// n1 = Edge 1/2
				sx1 = nx1/sw_minz;
				sx2 = x2/z2;
				sx3 = x3/z3;
				sy1 = ny1/sw_minz;
				sy2 = y2/z2;
				sy3 = y3/z3;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);	// Draw N1-2-3
				ratio = (z3 - sw_minz) / (z3 - z1);
				nx2 = x3 - (x3 - x1) * ratio;
				ny2 = y3 - (y3 - y1) * ratio;			// n2 = Edge 1/3
				sx1 = x3/z3;
				sx2 = nx2/sw_minz;
				sx3 = nx1/sw_minz;
				sy1 = y3/z3;
				sy2 = ny2/sw_minz;
				sy3 = ny1/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)			// Draw 3-N2-N1
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);
				continue;

			case 2:		// Vertex 2 is behind, create 2 new polys
				ratio = (z3 - sw_minz) / (z3 - z2);
				nx1 = x3 - (x3 - x2) * ratio;
				ny1 = y3 - (y3 - y2) * ratio;			// n1 = Edge 3/2
				sx1 = nx1/sw_minz;
				sx2 = x3/z3;
				sx3 = x1/z1;
				sy1 = ny1/sw_minz;
				sy2 = y3/z3;
				sy3 = y1/z1;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);	// Draw N1-2-3
				ratio = (z1 - sw_minz) / (z1 - z2);
				nx2 = x1 - (x1 - x2) * ratio;
				ny2 = y1 - (y1 - y2) * ratio;			// n2 = Edge 1/2
				sx1 = x1/z1;
				sx2 = nx2/sw_minz;
				sx3 = nx1/sw_minz;
				sy1 = y1/z1;
				sy2 = ny2/sw_minz;
				sy3 = ny1/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)			// Draw 3-N2-N1
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);
				continue;


			case 3:		// Vertex 1 & 2 is behind, create 1 new polys	### Not Verified ###
				ratio = (z3 - sw_minz) / (z3 - z1);
				nx1 = x3 - (x3 - x1) * ratio;
				ny1 = y3 - (y3 - y1) * ratio;

				ratio = (z3 - sw_minz) / (z3 - z2);
				nx2 = x3 - (x3 - x2) * ratio;
				ny2 = y3 - (y3 - y2) * ratio;

				sx1 = nx1/sw_minz;
				sx2 = x3/z3;
				sx3 = nx2/sw_minz;
				sy1 = ny1/sw_minz;
				sy2 = y3/z3;
				sy3 = ny2/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);
				continue;

			case 4:		// Vertex 3 is behind, create 2 new polys	### Not Verified ###
				ratio = (z1 - sw_minz) / (z1 - z3);
				nx1 = x1 - (x1 - x3) * ratio;
				ny1 = y1 - (y1 - y3) * ratio;			// n1 = Edge 1/3
				sx1 = nx1/sw_minz;
				sx2 = x1/z1;
				sx3 = x2/z2;
				sy1 = ny1/sw_minz;
				sy2 = y1/z1;
				sy3 = y2/z2;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);	// Draw 1-2-N1
				ratio = (z2 - sw_minz) / (z2 - z3);
				nx2 = x2 - (x2 - x3) * ratio;
				ny2 = y2 - (y2 - y3) * ratio;			// n2 = Edge 2/3
				sx1 = x2/z2;
				sx2 = nx2/sw_minz;
				sx3 = nx1/sw_minz;
				sy1 = y2/z2;
				sy2 = ny2/sw_minz;
				sy3 = ny1/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)			// Draw 3-N2-N1
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);
				continue;

			case 5:		// Vertex 3 & 1 is behind, create 1 new polys
				ratio = (z2 - sw_minz) / (z2 - z1);
				nx1 = x2 - (x2 - x1) * ratio;
				ny1 = y2 - (y2 - y1) * ratio;

				ratio = (z2 - sw_minz) / (z2 - z3);
				nx2 = x2 - (x2 - x3) * ratio;
				ny2 = y2 - (y2 - y3) * ratio;

				sx1 = nx1/sw_minz;
				sx2 = x2/z2;
				sx3 = nx2/sw_minz;
				sy1 = ny1/sw_minz;
				sy2 = y2/z2;
				sy3 = ny2/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);
				continue;

			case 6:		// Vertex 3 & 2 is behind, create 1 new polys
				ratio = (z1 - sw_minz) / (z1 - z2);
				nx1 = x1 - (x1 - x2) * ratio;
				ny1 = y1 - (y1 - y2) * ratio;

				ratio = (z1 - sw_minz) / (z1 - z3);
				nx2 = x1 - (x1 - x3) * ratio;
				ny2 = y1 - (y1 - y3) * ratio;

				sx1 = x1/z1;
				sx2 = nx1/sw_minz;
				sx3 = nx2/sw_minz;
				sy1 = y1/z1;
				sy2 = ny1/sw_minz;
				sy3 = ny2/sw_minz;
				if ((sx2-sx1)*(sy3-sy1)-(sy2-sy1)*(sx3-sx1)<0)
					sw_DrawTriangle_oc(sx1,sy1, sx2,sy2, sx3,sy3, col, &pixdrawn);
				continue;

			case 7:		// special case - entire polygon clipped
				continue;

		}
	}
	clipperSetMax();
	clipperShrink(_clipx1,_clipy1,_clipx2,_clipy2);
	objsdrawn++;
	polysdrawn+=obj->numfaces;
	return pixdrawn;
* /
return 0;
}
*/

sw::sw(void)
{	// Renderring Matricies
	makeidentity(viewscalemat);
	makeidentity(invcammat);
	makeidentity(tmpmat);

	if (horizon==0)
		horizon		= 5000;

//	currentswstate.renderstate		= RENDERSTATE_ZBUFFER | RENDERSTATE_ZWRITES | RENDERSTATE_DITHER;
//	currentswstate.rendermode		= 0;
//	currentswstate.ambientlight		= 0x00ffffff;
//	currentswstate.fillmode			= fillmode_solid;
//	currentswstate.shademode		= D3DSHADE_GOURAUD;
//	currentswstate.cullmode			= D3DCULL_CW;
	currentswstate.vpx				= 0;
	currentswstate.vpy				= 0;
	currentswstate.vpw				= current2DRenderTarget->width;
	currentswstate.vph				= current2DRenderTarget->height;
	currentswstate.horizon			= horizon;
//	currentswstate.bumpscale		= 1.0f;
	currentswstate.zoom				= 1.0f;
	restorestate(NULL);
//	d3d_savestate(&backupstate);
	GenerateProjectionMatrix();
	GenerateViewScaleMatrix();
}

SWrender *newSWrender(void)			// Create a new instance of the SWrender class
{	return (SWrender *)new sw;
}


