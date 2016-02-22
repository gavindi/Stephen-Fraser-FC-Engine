// Helper Framework provides a series of classes and functions which may
// help you to get started in learning the FC engine.  You will probably
// want to use your own framework once you become more experienced.

// cView provides a simple interface for the camera to allow it to freely 
// fly around the world.  It uses world-axis rotations, so it's not
// suitable for flight simulators.  Use W,S,A,D,E and C keys to move.
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
class cView
{
public:
	bool	mouseAims;					// if true, the mouse is used to aim.  When setting this to true, center the mouse in the window, then call checkmsg ... { input->setMouse(screenwidth>>1, screenheight>>1); checkmsg(); }
	intf	rotx, roty;
	float	speed;						// How fast does the camerra move
	Matrix	matrix;						// The current view matrix
			cView(bool aimWithMouse);	// if aimWithMouse is true, the mouse will be used for aiming straight away (and the mouse is centered in the window)
	void	update(void);
	void	setcamera(void);
};


// cWireFrame makes it easy to draw objects in wireframe
class cWireFrame
{	char		glow;
private:
	Material	*mtl;
public:
	cWireFrame(void);
	~cWireFrame(void);
	void animate(void);
	void drawobj3d(obj3d *obj, bool usewirecolor, uintf fillmode);
};


// cConstantTimer makes it easy to make a program run at a set rate on 
// any speed of hardware (frame rate may drop, game speed remains constant)
//	cConstantTimer *mytimer = new cConstantTimer(100);
//	while (!finished)
//  {  mytimer->doProcess(playgame);
//     drawscreen();
//  }
//  ...
//  void playgame(void)
//  { ...
//  }
class cConstantTimer
{	
public:
	cConstantTimer(uintf FreqHz);
	~cConstantTimer(void);
	void reset(void);				// Use the reset function if there has been a long pause (like loading new data from disk), this prevents a large number of callbacks from occuring simultaneously.
	void doProcess(void (*process_func)(void));
};
