//	***********************************************************
//	***														***
//	***			Matrix Stack (c) 2004 Stephen Fraser		***
//	***														***
//	***-----------------------------------------------------***
//	***														***
//	***	For use in FC Engine projects only.  This code is	***
//	*** not fault tollerant.  See FC programming manual for ***
//	***	details of use.										***
//	***														***
//	*** Keeps track of multiple levels of joint matricies	***
//	*** For example: the matrices of each bone in a			***
//	*** Skeletal system.  Each bone can have a parent and	***
//	*** any number of child matrices.						***
//	***														***
//	***********************************************************
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <fcio.h>
#include <bonus/MatrixStack.h>
#include <stdio.h>

cMatrixStack::cMatrixStack(uintf MaxLevels, char *StackName, byte *buf)
{	if (buf==NULL)
	{	char reason[256];
		uintf memneeded = sizeof(sMatrixStackLevel) * MaxLevels + txtlen(StackName)+1;
		tprintf(reason,sizeof(reason),"Matrix Stack: %s",StackName);
		buf = fcalloc(memneeded,reason);
		flags = 1;
	}	else
	{	flags = 0;
	}
	level	= (sMatrixStackLevel *)buf;		buf += sizeof(sMatrixStackLevel) * MaxLevels;
	name	= (char *)buf;		buf += txtlen(StackName)+1;
	levels = MaxLevels;
	txtcpy(name,StackName,txtlen(StackName)+1);
	for (uintf i=0; i<levels; i++)
	{	level[i].parent = 0;
		level[i].pivot.x = 0;
		level[i].pivot.y = 0;
		level[i].pivot.z = 0;
		level[i].rotx = 0;
		level[i].roty = 0;
		level[i].rotz = 0;
		makeidentity(level[i].mtx);
		level[i].current = 1;
	}
	level[0].current = 1;
}

cMatrixStack::~cMatrixStack(void)
{	if (flags) fcfree(level);
}

void cMatrixStack::change(uintf _level)
{	if (_level==0)
		level[_level].current = 1;
	else
		level[_level].current = 0;
	for (uintf i=1; i<levels; i++)
	{	if (level[i].parent==_level) change(i);
	}
}

void cMatrixStack::GenerateMatrix(uintf _level)
{	if (_level == 0) return;
	sMatrixStackLevel *l = &level[_level];
	sMatrixStackLevel *p = &level[l->parent];

	if (!p->current) GenerateMatrix(l->parent);

	memcopy(l->mtx, level[l->parent].mtx, sizeof(Matrix));
	movealongx(l->mtx, l->pivot.x);
	movealongy(l->mtx, l->pivot.y);
	movealongz(l->mtx, l->pivot.z);
	if (l->rotx) rotatearoundx(l->mtx, l->rotx);
	if (l->roty) rotatearoundy(l->mtx, l->roty);
	if (l->rotz) rotatearoundz(l->mtx, l->rotz);
	fixrotations(l->mtx);


	l->current = true;
}

void cMatrixStack::copyfrom(cMatrixStack *c)
{	if (c->levels!=levels)
		msg("MatrixStack Error",buildstr("Cannot copy a %i level Matrix Stack over a %i level Matrix Stack"));
	
	for (uintf i=0; i<levels; i++)
	{	level[i].parent = c->level[i].parent;
		level[i].pivot = c->level[i].pivot;
		level[i].rotx = c->level[i].rotx;
		level[i].roty = c->level[i].roty;
		level[i].rotz = c->level[i].rotz;
		float *dm = level[i].mtx;
		float *sm = c->level[i].mtx;
		memcopy(dm, sm, sizeof(Matrix));
		level[i].current = c->level[i].current;
	}
}

