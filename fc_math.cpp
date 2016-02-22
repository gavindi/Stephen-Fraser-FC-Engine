// ------------------------------------------------------------------------------------------------
//											FC Math Library
//
// A collection of various useful math routines (except matricies, look at fc_matricies for that)
// ------------------------------------------------------------------------------------------------
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#include <fcio.h>

void vecSubtract(float3 *result, float3 *a, float3 *b)
{	result->x = a->x - b->x;
	result->y = a->y - b->y;
	result->z = a->z - b->z;
}

bool isPow2(uintf value)
{	uintf testBit = 1;
	while (value>testBit) testBit<<=1;
	return value==testBit;
}

uintf nextPow2(uintf value)
{	uintf testBit = 1;
	while (value>testBit) testBit<<=1;
	return testBit;
}
