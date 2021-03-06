/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
// Helper Framework provides a series of classes and functions which may
// help you to get started in learning the FC engine.  You will probably
// want to use your own framework once you become more experienced.

#include <fcio.h>
#include "helperframework.h"

cView::cView(bool aimWithMouse)
{	mouseAims = aimWithMouse;
	rotx = 0;
	roty = 0;
	speed = 10;
	makeidentity(matrix);
	if (aimWithMouse)
	{	input->setMouse(screenwidth>>1, screenheight>>1);
		checkmsg();
	}
};

//bool lastappactive = true;

void cView::update(void)
{//	if (!AppActive) return;				// ### <-- This must be fixed up (should come from another variable)
//	{	lastappactive = false;
//		return;
//	}
//	if (lastappactive)
//	{	setmouse(screenwidth>>1, screenheight>>1);
//		checkmsg();
//	}	else
//	{	lastappactive = true;
//	}

	input->getMouse();
	if (mouseAims)
	{	rotx+=input->mouse[0].x-(screenwidth>>1);		
		roty+=input->mouse[0].y-(screenheight>>1);
		while (rotx<0) rotx+=3600;
		while (roty<0) roty+=3600;
		while (rotx>3599) rotx-=3600;
		while (roty>3599) roty-=3600;
	
		matrix[0]=matrix[5]=matrix[10]=1;
		matrix[1]=matrix[2]=matrix[3]=matrix[4]=matrix[6]=matrix[7]=matrix[8]=matrix[9]=matrix[11]=0;
		rotatearoundy(matrix,rotx);
		rotatearoundx(matrix,roty);
		fixrotations(matrix);

		input->setMouse(screenwidth>>1, screenheight>>1);
		checkmsg();
	}

	// Move the camera according to the keyboard commands
	if (keyDown['w'])	movealongz(matrix, speed);	// W moves forewards
	if (keyDown['s'])	movealongz(matrix,-speed);	// S moves backwards
	if (keyDown['a'])	movealongx(matrix,-speed);	// A strafes left
	if (keyDown['d'])	movealongx(matrix, speed);	// Z strafes right
	if (keyDown['e'])	movealongy(matrix, speed);	// E rises up
	if (keyDown['c'])	movealongy(matrix,-speed);	// C lowers down
}

void cView::setcamera(void)
{	memcopy(cammat, matrix, sizeof(Matrix));
	setcammatrix();
}


//*********************************************************************
cWireFrame::cWireFrame(void)
{	mtl = newmaterial("WireFrame");
	FixedVertexShader *fs = (FixedVertexShader *)mtl->VertexShader;
	fs->diffuse.a = 1.0f;	fs->diffuse.r = 0.0f;	fs->diffuse.g = 0.0f;	fs->diffuse.b = 0.0f;	
	fs->ambient.a = 1.0f;	fs->ambient.r = 0.0f;	fs->ambient.g = 0.0f;	fs->ambient.b = 0.0f;	
	fs->specular.a = 1.0f;	fs->specular.r = 0.0f;	fs->specular.g = 0.0f;	fs->specular.b = 0.0f;	
	fs->bias = 1;
	glow = 0;
}

cWireFrame::~cWireFrame(void)
{	deletematerial(mtl);
}

void cWireFrame::animate(void)
{	glow++;
	                                        
	intf iglow = abs(glow)*2;
	if (iglow>255) iglow=255;
	float fglow = (float)iglow / 255.0f;
	FixedVertexShader *fs = (FixedVertexShader *)mtl->VertexShader;
	fs->emissive.a = 1.0f;	fs->emissive.r = fglow;	fs->emissive.g = fglow;	fs->emissive.b = fglow;	
}

void cWireFrame::drawobj3d(obj3d *obj, bool usewirecolor, uintf fillmode)
{	Material *tmpmtl = obj->material;	// Remember material so it may be restored
	intf tmpobjsdrawn = objsdrawn;		// Remember 'objsdrawn' so we don't update it
	setfillmode(fillmode);				// Go into wireframe drawing mode
	if (usewirecolor)
		obj->material = NULL;			// Draw a material-free object
	else
		obj->material = mtl;			// Use the wireframe material
	::drawobj3d(obj);					// Draw the object (using global draw function)
	setfillmode(fillmode_solid);		// Return to regular filled-mode drawing
	obj->material = tmpmtl;				// restore the material
	objsdrawn = tmpobjsdrawn;			// restore the objsdrawn counter
}

//*********************************************************************
intf cConstantTimer_Counter;

void cConstantTimer_Callback(void)
{	cConstantTimer_Counter++;
}

cConstantTimer::cConstantTimer(uintf FreqHz)
{	cConstantTimer_Counter = 0;
	set_timer_task(cConstantTimer_Callback, FreqHz);
}

cConstantTimer::~cConstantTimer(void)
{	set_timer_task(NULL,100);
	cConstantTimer_Counter = 0;
}

void cConstantTimer::reset(void)
{	cConstantTimer_Counter = 0;
}

void cConstantTimer::doProcess(void (*process_func)(void))
{	while (!cConstantTimer_Counter) checkmsg();//waitmsg();
	while (cConstantTimer_Counter)
	{	if (process_func) process_func();
		cConstantTimer_Counter--;
		checkmsg();
	}
}

//*********************************************************************
