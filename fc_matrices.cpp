/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
// ********************************************************************************************
// ***			                         Matrix Math										***
// ********************************************************************************************
//		 Task    		  | Function Name
//------------------------+-----------------------
// Look Up				  | rotatearoundx( speed);
// Look Down		      | rotatearoundx(-speed);
// Turn Left		      | rotatearoundy( speed);
// Turn Right			  | rotatearoundy(-speed);
// Roll Clockwise		  | rotatearoundz( speed);
// Roll Counter Clockwise | rotatearoundz(-speed);
// Creep Correction		  | fixrotations();		  
// Assign to Camera		  | setcammatrix();		  

#include "fc_h.h"

extern  void (*updatecammatrix)(void);			// Indicates that the camera matrix has been updated 

float	modelViewProjectionMatrix[16];
float	modelViewMatrix[16];
float	cammat[16], invcammat[16];
float	projectionMatrix[16];
//Matrix	InvObjMat;								// Invert of the local matrix (used to translate points to local object space for Vertex Shaders)

void initmatrix(void)
{	// No Initialisation required as yet
}

void killmatrix(void)
{	// No initialization required as yet
}

void makeidentity(float *mat)
{	mat[ 0]=1;	mat[ 1] = 0;	mat[ 2] = 0;	mat[ 3] = 0;
	mat[ 4]=0;	mat[ 5] = 1;	mat[ 6] = 0;	mat[ 7] = 0;
	mat[ 8]=0;	mat[ 9] = 0;	mat[10] = 1;	mat[11] = 0;
	mat[12]=0;	mat[13] = 0;	mat[14] = 0;	mat[15] = 1;
}

void rotatearound( float *mat,long x, long y, long z )
{	float m00,m01, sm00,sm01,sm02;
	float m10,m11, sm10,sm11,sm12;
	float m20,m21, sm20,sm21,sm22;

	while (x<   0) x+=3600;
	while (x>3600) x-=3600;
	while (y<   0) y+=3600;
	while (y>3600) y-=3600;
	while (z<   0) z+=3600;
	while (z>3600) z-=3600;


	// ************** Rotate Around Y *******************************
	m00 = mat[ 0],
	m20 = mat[ 8],
	m01 = mat[ 1],
	m21 = mat[ 9];

	sm00 = mycos[y],
	sm20 = mysin[y],
	sm02 =-sm20,
	sm22 = sm00;

	// Compute column 0
	mat[ 0] = ( sm00 * m00 ) + ( sm02 * m20 );
	mat[ 8] = ( sm20 * m00 ) + ( sm22 * m20 );

	// Compute column 1
	mat[ 1] = ( sm00 * m01 ) + ( sm02 * m21 );
	mat[ 9] = ( sm20 * m01 ) + ( sm22 * m21 );

	// ************** Rotate Around X *******************************
	m10 = mat[ 4],
	m20 = mat[ 8],
	m11 = mat[ 5],
	m21 = mat[ 9];

	sm11 =  mycos[x],
	sm12 =  mysin[x],
	sm21 = -sm12,
	sm22 =  sm11;

	// Compute column 0
	mat[ 4] = ( sm11 * m10 ) + ( sm12 * m20 );
	mat[ 8] = ( sm21 * m10 ) + ( sm22 * m20 );
   
	// Compute column 1
	mat[ 5] = ( sm11 * m11 ) + ( sm12 * m21 );
	mat[ 9] = ( sm21 * m11 ) + ( sm22 * m21 );

	// ************** Rotate Around Z *******************************
 	m00 = mat[ 0];
	m01 = mat[ 1];
	m10 = mat[ 4],
	m11 = mat[ 5];

	sm00 = mycos[z],
	sm01 = mysin[z],
	sm10 =-sm01,
	sm11 = sm00;

	// Compute column 0
	mat[ 0] = ( sm00 * m00 ) + ( sm01 * m10 );
	mat[ 4] = ( sm10 * m00 ) + ( sm11 * m10 );

	// Compute column 1
	mat[ 1] = ( sm00 * m01 ) + ( sm01 * m11 );
	mat[ 5] = ( sm10 * m01 ) + ( sm11 * m11 );
}

void rotatearoundx(float *mat,long a)
{	float  m10, m20, m11, m21;
	float sm11,sm12,sm21,sm22;
	// ************** Rotate Around X *******************************
	m10 = mat[ 4],
	m20 = mat[ 8],
	m11 = mat[ 5],
	m21 = mat[ 9];

	sm11 =  mycos[a],
	sm12 =  mysin[a],
	sm21 = -sm12,
	sm22 =  sm11;

	// Compute column 0
	mat[ 4] = ( sm11 * m10 ) + ( sm12 * m20 );
	mat[ 8] = ( sm21 * m10 ) + ( sm22 * m20 );
   
	// Compute column 1
	mat[ 5] = ( sm11 * m11 ) + ( sm12 * m21 );
	mat[ 9] = ( sm21 * m11 ) + ( sm22 * m21 );
}

void rotatesmootharoundx(float *mat,float a)
{	float  m10, m20, m11, m21;
	float sm11,sm12,sm21,sm22;
	// ************** Rotate Around X *******************************
	m10 = mat[ 4],
	m20 = mat[ 8],
	m11 = mat[ 5],
	m21 = mat[ 9];

	sm11 =  (float)cos(a),
	sm12 =  (float)sin(a),
	sm21 = -sm12,
	sm22 =  sm11;

	// Compute column 0
	mat[ 4] = ( sm11 * m10 ) + ( sm12 * m20 );
	mat[ 8] = ( sm21 * m10 ) + ( sm22 * m20 );
   
	// Compute column 1
	mat[ 5] = ( sm11 * m11 ) + ( sm12 * m21 );
	mat[ 9] = ( sm21 * m11 ) + ( sm22 * m21 );
}

void rotatearoundy(float *mat,long a)
{	float  m00, m20, m01, m21;
	float sm00,sm20,sm02,sm22;
	// ************** Rotate Around Y *******************************
	m00 = mat[ 0],
	m20 = mat[ 8],
	m01 = mat[ 1],
	m21 = mat[ 9];

	sm00 = mycos[a],
	sm20 = mysin[a],
	sm02 =-sm20,
	sm22 = sm00;

	// Compute column 0
	mat[ 0] = ( sm00 * m00 ) + ( sm02 * m20 );
	mat[ 8] = ( sm20 * m00 ) + ( sm22 * m20 );

	// Compute column 1
	mat[ 1] = ( sm00 * m01 ) + ( sm02 * m21 );
	mat[ 9] = ( sm20 * m01 ) + ( sm22 * m21 );
}

void rotatearoundz(float *mat,long a)
{	float  m00, m01, m10, m11;
	float sm00,sm01,sm10,sm11;
	// ************** Rotate Around Z *******************************
 	m00 = mat[ 0];
	m01 = mat[ 1];
	m10 = mat[ 4],
	m11 = mat[ 5];

	sm00 = mycos[a],
	sm01 = mysin[a],
	sm10 =-sm01,
	sm11 = sm00;

	// Compute column 0
	mat[ 0] = ( sm00 * m00 ) + ( sm01 * m10 );
	mat[ 4] = ( sm10 * m00 ) + ( sm11 * m10 );

	// Compute column 1
	mat[ 1] = ( sm00 * m01 ) + ( sm01 * m11 );
	mat[ 5] = ( sm10 * m01 ) + ( sm11 * m11 );
}

void rotatesmootharoundz(float *mat,float a)
{	float  m00, m01, m10, m11;
	float sm00,sm01,sm10,sm11;
	// ************** Rotate Around Z *******************************
 	m00 = mat[ 0];
	m01 = mat[ 1];
	m10 = mat[ 4],
	m11 = mat[ 5];

	sm00 = (float)cos(a),
	sm01 = (float)sin(a),
	sm10 =-sm01,
	sm11 = sm00;

	// Compute column 0
	mat[ 0] = ( sm00 * m00 ) + ( sm01 * m10 );
	mat[ 4] = ( sm10 * m00 ) + ( sm11 * m10 );

	// Compute column 1
	mat[ 1] = ( sm00 * m01 ) + ( sm01 * m11 );
	mat[ 5] = ( sm10 * m01 ) + ( sm11 * m11 );
}

void rotatesmootharoundy(float *mat,float a)
{	float  m00, m20, m01, m21;
	float sm00,sm20,sm02,sm22;
	// ************** Rotate Around Y *******************************
	m00 = mat[ 0];
	m20 = mat[ 8];
	m01 = mat[ 1];
	m21 = mat[ 9];

	sm00 = (float)cos(a);
	sm20 = (float)sin(a);
	sm02 =-sm20;
	sm22 = sm00;

	// Compute column 0
	mat[ 0] = ( sm00 * m00 ) + ( sm02 * m20 );
	mat[ 8] = ( sm20 * m00 ) + ( sm22 * m20 );

	// Compute column 1
	mat[ 1] = ( sm00 * m01 ) + ( sm02 * m21 );
	mat[ 9] = ( sm20 * m01 ) + ( sm22 * m21 );
}

void fixrotations(float *mat)
{  float vuX,vuY,vuZ, vfX,vfY,vfZ, vrX,vrY,vrZ;
   float magnitude;
   // Compute column 2 (as col0 x col1 )
   mat[ 2] = ( mat[ 4] * mat[ 9] ) -
             ( mat[ 8] * mat[ 5] );
   mat[ 6] = ( mat[ 8] * mat[ 1] ) -
             ( mat[ 0] * mat[ 9] );
   mat[10] = ( mat[ 0] * mat[ 5] ) -
             ( mat[ 4] * mat[ 1] );
   // Recompute column 0 as col1 x col2

   // Obtain FOREWARD vector
   vfX = mat[ 2];
   vfY = mat[ 6];
   vfZ = mat[10];

   // Obtain UP vector
   vuX = mat[ 1];
   vuY = mat[ 5];
   vuZ = mat[ 9];

   // Obtain RIGHT vector
   vrX = ( vuY * vfZ ) - ( vuZ * vfY );
   vrY = ( vuZ * vfX ) - ( vuX * vfZ );
   vrZ = ( vuX * vfY ) - ( vuY * vfX );

   // Normalize the direction vectors

   // Normalize UP
   magnitude = 1.0f/(float) sqrt( vuX * vuX + vuY * vuY + vuZ * vuZ );
   mat[ 1] = vuX * magnitude;
   mat[ 5] = vuY * magnitude;
   mat[ 9] = vuZ * magnitude;

   // Normalize RIGHT
   magnitude = 1.0f/(float) sqrt( vrX * vrX + vrY * vrY + vrZ * vrZ );
   mat[ 0] = vrX * magnitude;
   mat[ 4] = vrY * magnitude;
   mat[ 8] = vrZ * magnitude;

   // Normalize FOREWARD
   magnitude = 1.0f/(float) sqrt( vfX * vfX + vfY * vfY + vfZ * vfZ );
   mat[ 2] = vfX * magnitude;
   mat[ 6] = vfY * magnitude;
   mat[10] = vfZ * magnitude;
}

void scaleMatrix(float *mat, float scale)
{	for (uintf i=0; i<12; i++)
		mat[i] *= scale;
}

void scaleMatrix(float *mat, float3 scale)
{	float tmp = scale.x;
	mat[0] *= tmp;
	mat[1] *= tmp;
	mat[2] *= tmp;
	mat[3] *= tmp;
	tmp = scale.y;
	mat[4] *= tmp;
	mat[5] *= tmp;
	mat[6] *= tmp;
	mat[7] *= tmp;
	tmp = scale.z;
	mat[8] *= tmp;
	mat[9] *= tmp;
	mat[10] *= tmp;
	mat[11] *= tmp;
}

void clearrotations(float *mat)	
{	mat[ 0] = 1;	mat[ 1] = 0;	mat[ 2] = 0;
	mat[ 4] = 0;	mat[ 5] = 1;	mat[ 6] = 0;
	mat[ 8] = 0;	mat[ 9] = 0;	mat[10] = 1;
}

void movealongx(float *mat,float speed)
{	mat[12] += mat[ 0] * speed;
	mat[13] += mat[ 1] * speed;
	mat[14] += mat[ 2] * speed;
}

void movealongy(float *mat,float speed)
{	mat[12] += mat[ 4] * speed;
	mat[13] += mat[ 5] * speed;
	mat[14] += mat[ 6] * speed;
}

void movealongz(float *mat,float speed)
{	mat[12] += mat[ 8] * speed;
	mat[13] += mat[ 9] * speed;
	mat[14] += mat[10] * speed;		
}

void translatematrix(float *mat,float x, float y, float z)
{	mat[12] += x;
	mat[13] += y;
	mat[14] += z;
}

void matlookat(float *mat,float sx,float sy,float sz, float ex,float ey,float ez)
{	float xx,xy,xz, yx,yy,yz, zx,zy,zz, len;
	zx = ex-sx;
	zy = ey-sy;
	zz = ez-sz;
	len = 1.0f/(float)sqrt(zx*zx+zy*zy+zz*zz);
	zx*=len;
	zy*=len;
	zz*=len;

	// Calculate UP Vector
	yx = 0 - zy*zx;
	yy = 1 - zy*zy;
	yz = 0 - zy*zz;
	len = 1.0f/(float)sqrt(yx*yx+yy*yy+yz*yz);
	yx*=len;
	yy*=len;
	yz*=len;

	// Calculate X basis vector (U x V)
	xx = yy*zz-yz*zy;
	xy = yz*zx-yx*zz;
	xz = yx*zy-yy*zx;

	mat[ 0]=xx; mat[ 1]=xy; mat[ 2]=xz; mat[ 3]=0;
	mat[ 4]=yx; mat[ 5]=yy; mat[ 6]=yz; mat[ 7]=0;
	mat[ 8]=zx; mat[ 9]=zy; mat[10]=zz; mat[11]=0;
	mat[12]=sx; mat[13]=sy; mat[14]=sz; mat[15]= 1;
}

void matlookalong(float *mat,float sx,float sy,float sz, float zx,float zy,float zz)
{	float xx,xy,xz, yx,yy,yz, len;

	// Calculate UP Vector
	yx = 0 - zy*zx;
	yy = 1 - zy*zy;
	yz = 0 - zy*zz;
	len = 1.0f/(float)sqrt(yx*yx+yy*yy+yz*yz);
	yx*=len;
	yy*=len;
	yz*=len;

	// Calculate X basis vector (U x V)
	xx = yy*zz-yz*zy;
	xy = yz*zx-yx*zz;
	xz = yx*zy-yy*zx;

	mat[ 0]=xx; mat[ 1]=xy; mat[ 2]=xz; mat[ 3]=0;
	mat[ 4]=yx; mat[ 5]=yy; mat[ 6]=yz; mat[ 7]=0;
	mat[ 8]=zx; mat[ 9]=zy; mat[10]=zz; mat[11]=0;
	mat[12]=sx; mat[13]=sy; mat[14]=sz; mat[15]= 1;
}

void matrixinvert( float *q, float *a )
{	q[ 0]=a[ 0]; q[ 1]=a[ 4]; q[ 2]=a[ 8];  q[ 3]=0;
	q[ 4]=a[ 1]; q[ 5]=a[ 5]; q[ 6]=a[ 9];  q[ 7]=0;
	q[ 8]=a[ 2]; q[ 9]=a[ 6]; q[10]=a[10];  q[11]=0;
    q[12]=-( a[12] * a[ 0] + a[13] * a[1] + a[14] * a[2] );
    q[13]=-( a[12] * a[ 4] + a[13] * a[5] + a[14] * a[6] );
    q[14]=-( a[12] * a[ 8] + a[13] * a[9] + a[14] * a[10] );
    q[15] = 1.0f;
}

bool invertMatrixAccurate(float *out, const float *m)
{	
#define SWAP_ROWS(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

	float wtmp[4][8];
	float m0, m1, m2, m3, s;
	float *r0, *r1, *r2, *r3;

	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

	r0[0] = MAT(m,0,0), r0[1] = MAT(m,0,1),
	r0[2] = MAT(m,0,2), r0[3] = MAT(m,0,3),
	r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

	r1[0] = MAT(m,1,0), r1[1] = MAT(m,1,1),
	r1[2] = MAT(m,1,2), r1[3] = MAT(m,1,3),
	r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

	r2[0] = MAT(m,2,0), r2[1] = MAT(m,2,1),
	r2[2] = MAT(m,2,2), r2[3] = MAT(m,2,3),
	r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

	r3[0] = MAT(m,3,0), r3[1] = MAT(m,3,1),
	r3[2] = MAT(m,3,2), r3[3] = MAT(m,3,3),
	r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

	// choose pivot - or die
	if (fabs(r3[0])>fabs(r2[0])) SWAP_ROWS(r3, r2);
	if (fabs(r2[0])>fabs(r1[0])) SWAP_ROWS(r2, r1);
	if (fabs(r1[0])>fabs(r0[0])) SWAP_ROWS(r1, r0);
	if (0.0 == r0[0]) 
	{	return false;
	}

	// eliminate first variable
	m1 = r1[0]/r0[0]; m2 = r2[0]/r0[0]; m3 = r3[0]/r0[0];
	s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
	s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
	s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) 
	{	r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; 
	}
	s = r0[5];
	if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
	s = r0[6];
	if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
	s = r0[7];
	if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

	// choose pivot - or die
	if (fabs(r3[1])>fabs(r2[1])) SWAP_ROWS(r3, r2);
	if (fabs(r2[1])>fabs(r1[1])) SWAP_ROWS(r2, r1);
	if (0.0 == r1[1]) 
	{	return false;
	}

	// eliminate second variable
	m2 = r2[1]/r1[1]; m3 = r3[1]/r1[1];
	r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
	s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
	s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
	s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
	s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

	// choose pivot - or die
	if (fabs(r3[2])>fabs(r2[2])) SWAP_ROWS(r3, r2);
	if (0.0 == r2[2]) 
	{	return false;
	}

	// eliminate third variable
	m3 = r3[2]/r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
	r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
	r3[7] -= m3 * r2[7];

	// last check
	if (0.0 == r3[3]) 
	{	return false;
	}

	s = 1.0f/r3[3];              // now back substitute row 3
	r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

	m2 = r2[3];                 // now back substitute row 2
	s  = 1.0f/r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
	r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
	r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
	r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

	m1 = r1[2];                 // now back substitute row 1
	s  = 1.0f/r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
	r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
	r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

	m0 = r0[1];                 // now back substitute row 0
	s  = 1.0f/r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
	r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

	MAT(out,0,0) = r0[4]; MAT(out,0,1) = r0[5],
	MAT(out,0,2) = r0[6]; MAT(out,0,3) = r0[7],
	MAT(out,1,0) = r1[4]; MAT(out,1,1) = r1[5],
	MAT(out,1,2) = r1[6]; MAT(out,1,3) = r1[7],
	MAT(out,2,0) = r2[4]; MAT(out,2,1) = r2[5],
	MAT(out,2,2) = r2[6]; MAT(out,2,3) = r2[7],
	MAT(out,3,0) = r3[4]; MAT(out,3,1) = r3[5],
	MAT(out,3,2) = r3[6]; MAT(out,3,3) = r3[7]; 
	
	return true;

#undef MAT
#undef SWAP_ROWS
}

/*
float fastYrot(float *tmpMtx, uintf angle)
{	// Quickly generate a fixed Y rotation matrix
	// This code works, however it is disabled because I don't yat have a rotX or rotY version
	uintf angle = 0;
	float vfZ = mycos[angle];
	float vrX = mycos[angle];
	float vrZ = mysin[angle];
	float vrZSq = vrZ*vrZ;
	float rightMagnitude = (float) sqrt( vrX * vrX + vrZ * vrZ );
	float leftMagnitude = (float) sqrt( vrZSq + vfZ * vfZ );

	tmpMtx[ 0] = vrX / rightMagnitude;
	tmpMtx[ 1] = 0.0f;
	tmpMtx[ 2] =-vrZ / leftMagnitude;
	tmpMtx[ 3] = 0.0f;

	tmpMtx[ 4] = 0.0f;
	tmpMtx[ 5] = 1.0f;
	tmpMtx[ 6] = 0.0f;
	tmpMtx[ 7] = 0.0f;

	tmpMtx[ 8] = vrZ / rightMagnitude;
	tmpMtx[ 9] = 0.0f;
	tmpMtx[10] = vfZ / leftMagnitude;
	tmpMtx[11] = 0.0f;

	tmpMtx[12]=0;
	tmpMtx[13]=0;
	tmpMtx[14]=0;
	tmpMtx[15]=1.0f;
}
*/

void matmult(float *result, float *a, float *b)
{	long element = 0;
	for( long i=0; i<4; i++ ) 
    {	long ii=i<<2;
		for( long j=0; j<4; j++ ) 
			result[element++] = a[j   ] * b[ii  ] +
								a[j+ 4] * b[ii+1] +
								a[j+ 8] * b[ii+2] +
								a[j+12] * b[ii+3];
	}
}

void transformPointByMatrix(float3 *dest, float *m, float3 *point)
{ 	dest->x = (point->x * m[mat_xvecx] + point->y * m[mat_yvecx] + point->z * m[mat_zvecx] + m[mat_xpos]);
	dest->y = (point->x * m[mat_xvecy] + point->y * m[mat_yvecy] + point->z * m[mat_zvecy] + m[mat_ypos]);
	dest->z = (point->x * m[mat_xvecz] + point->y * m[mat_yvecz] + point->z * m[mat_zvecz] + m[mat_zpos]);
}

void rotateVectorByMatrix(float3 *dest, float *m, float3 *point)
{ 	dest->x = (point->x * m[mat_xvecx] + point->y * m[mat_xvecy] + point->z * m[mat_xvecz]);
	dest->y = (point->x * m[mat_yvecx] + point->y * m[mat_yvecy] + point->z * m[mat_yvecz]);
	dest->z = (point->x * m[mat_zvecx] + point->y * m[mat_zvecy] + point->z * m[mat_zvecz]);
}
void setcammatrix(void)
{	// Indicates that the camera matrix has changed, and needs to be updated in the system
	matrixinvert( invcammat, cammat );
	updatecammatrix();
}

void prepMatrix(float *modelMatrix)
{	matmult(modelViewMatrix, invcammat, modelMatrix);
	matmult(modelViewProjectionMatrix, projectionMatrix, modelViewMatrix);
}

void displaymatrix(float *mtx, intf xpos, intf ypos)	// For debugging purposes ... displays a matrix using textout function
{	for (intf y=0; y<4; y++)
		for (intf x=0; x<4; x++)
		{	textout (x*50+xpos,y*15+ypos,"%.2f",mtx[(y<<2)+x]);
		}
}

void logMatrix(float *mtx, logfile *log)	// For debugging purposes ... displays a matrix using textout function
{	for (intf y=0; y<4; y++)
	{	int row = y*4;
		log->log("%6.2f, %6.2f, %6.2f, %6.2f",mtx[row+0],mtx[row+1],mtx[row+2],mtx[row+3]);
	}
}
