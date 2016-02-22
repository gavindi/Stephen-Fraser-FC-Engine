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
struct	sMatrixStackLevel
{	uintf	parent;			// Read/Write  (invalid for level 0)
	float3	pivot;			// Read/Write  (invalid for level 0)
	intf	rotx;			// Read/Write  (invalid for level 0)
	intf	roty;			// Read/Write  (invalid for level 0)
	intf	rotz;			// Read/Write  (invalid for level 0)
	Matrix	mtx;			// Read, Can only write level[0]
	uintf	current;		// Read        (invalid for level 0)
};

// memory needed = 	sizeof(sMatrixStackLevel)*MaxLevels + strlen(StackName)+1;
class   cMatrixStack
{	public:
		sMatrixStackLevel	*level;		
		uintf				levels;
		char				*name;
		uintf				flags;

		cMatrixStack(uintf MatrixLevels,char *name, byte *membuffer=NULL);
		~cMatrixStack(void);
		void change(uintf level);
		void GenerateMatrix(uintf level);		// Returns without doing anything for level 0
		void copyfrom(cMatrixStack *c);
};
