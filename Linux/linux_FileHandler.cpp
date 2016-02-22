/************************************************************************************/
/*						   WINDOWS File Access Source.								*/
/*	    				   ---------------------------								*/
/*  					   (c) 2008 by Stephen Fraser								*/
/*																					*/
/* Contains the Windows-Specific code to handle data files							*/
/*																					*/
/* Must provide the following Interfaces:											*/
/*	class cDerivedOS_Archive : public cFileArchive;	// Class for handling files		*/
/*	cFileArchive *initOS_Archive(void);				// Returns a pointer to class	*/
/*																					*/
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

// Information for your reading pleasure: http://www.gnu.org/software/libc/manual/html_node/File-System-Interface.html#File-System-Interface

#include <fcntl.h>			// Needed for File Operations
#include <sys/stat.h>		// Needed for File system flags
#include <errno.h>			// Needed for reporting disk errors
#include <dirent.h>			// Linux Only - Directory Entries
#include <unistd.h>

#include "../fc_h.h"

#define maxDirHandles 5

#ifndef O_BINARY
#define O_BINARY 0
#endif

const int filePermission = S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

struct sLinuxOS_DirHandle
{	DIR			*dir;
	dirent		*dirEntry;
	char		findDataPath[256];
	uintf		inuse;		// 0 = This Directory Handle is free, 1 = Just allocated, not yet used, 2 = Allocated and in use
};

struct sLinuxOS_Handle
{	// Must not exceed the byte size of sFileHandle in any way!
	void				*arc_class;	// We can ignore this, it will be equal to 'this'
	int					handle;		// Standard Windows Handle
	sLinuxOS_DirHandle	*dirHandle;	// Data structure used to search directories
};

int		LinuxOS_BogusData;	// Needed to give us something to point to

class cLinuxOS_Archive : public cFileArchive
{	public:
		sLinuxOS_DirHandle	dirHandle[maxDirHandles];
//					cLinuxOS_Archive::cLinuxOS_Archive(void);
					~cLinuxOS_Archive(void);
		void		*fileExists(const char *filename);
		sFileHandle *open(const char *filename);
		sFileHandle *create(const char *filename);
		sFileHandle *openRW(const char *filename);
		int64		seek(sFileHandle *handle, int64 offset, uintf seekflags);
		uintf		read(sFileHandle *handle, void *buf, uintf size);
		uintf		write(sFileHandle *handle, void *buf, uintf size);
		void		close(sFileHandle *);
		uintf		getFileSize(const char *filename);
		sFileHandle	*openDir(const char *path);
		bool		readDir(sFileHandle *handle, sDirectoryData *dir);
		void		closeDir(sFileHandle *handle);
};

//cLinuxOS_Archive::cLinuxOS_Archive(void)
//{
//}

void	*cLinuxOS_Archive::fileExists(const char *filename)
{	int temphandle;
	temphandle=::open(filename, O_RDONLY | O_BINARY);
	if (temphandle!=-1)
	{	::close(temphandle);
		return(&LinuxOS_BogusData);
	}	else
	return NULL;

}

sFileHandle *cLinuxOS_Archive::open(const char *filename)
{	int handle = ::open(filename, O_RDONLY | O_BINARY);
	if (handle==-1) return NULL;

	sFileHandle *genHandle = allocArcHandle();
	sLinuxOS_Handle *winHandle = (sLinuxOS_Handle *)genHandle;
	winHandle->arc_class = this;
	winHandle->handle=handle;
	return genHandle;
}

sFileHandle *cLinuxOS_Archive::create(const char *filename)
{	int handle = ::open(filename, O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, filePermission);
    if (handle==-1) return NULL;

	sFileHandle *genHandle = allocArcHandle();
	sLinuxOS_Handle *winHandle = (sLinuxOS_Handle *)genHandle;
	winHandle->arc_class = this;
	winHandle->handle=handle;
	return genHandle;
}


sFileHandle *cLinuxOS_Archive::openRW(const char *filename)
{	int flags =  O_BINARY | O_RDWR;
	if (!fileExists(filename)) flags |= O_CREAT | O_TRUNC;
	int handle = ::open(filename, flags, filePermission);
    if (handle==-1) return NULL;

	sFileHandle *genHandle = allocArcHandle();
	sLinuxOS_Handle *winHandle = (sLinuxOS_Handle *)genHandle;
	winHandle->arc_class = this;
	winHandle->handle=handle;
	return genHandle;
}

int64 cLinuxOS_Archive::seek(sFileHandle *genHandle, int64 offset, uintf seekflags)
{	int handle = ((sLinuxOS_Handle *)genHandle)->handle;
	switch (seekflags)
	{	case seek_fromstart:	return lseek64(handle,offset,SEEK_SET); break;
		case seek_fromcurrent:	return lseek64(handle,offset,SEEK_CUR); break;
		case seek_fromend:		return lseek64(handle,offset,SEEK_END); break;
		default: msg("Disk Error","Invalid Seek flags");
	}
	return 0;
}

uintf cLinuxOS_Archive::read(sFileHandle *genHandle, void *buf, uintf size)
{	int handle = ((sLinuxOS_Handle *)genHandle)->handle;
	int readsize=::read(handle,buf,size);
	if (readsize==-1)
	{	msg("Disk Error","Error reading file");
	}
	filesize=readsize;
	return (uintf)readsize;
}

uintf cLinuxOS_Archive::write(sFileHandle *genHandle, void *buf, uintf size)
{	int handle = ((sLinuxOS_Handle *)genHandle)->handle;
	uintf flsize;
	char *errrep = (char *)"Unknown Error";
	if (size==0)
	{	filesize = 0;
		return 0;
	}
	flsize=::write(handle,buf,size);
    if (flsize!=size)
    {	if (errno==EBADF) errrep=(char *)"Bad File Handle";
		if (errno==ENOSPC) errrep=(char *)"Not Enough Space";
		if (errno==EINVAL) errrep=(char *)"Invalid Arguments";
		msg("Disk Error",buildstr("Handle %x (size was %i, should have been %i)\nReason Given = %s (%i)\nSource Pointer %x, Size = %i\nError writing file ",handle,flsize,size,errrep,errno,buf,size));
    }
	filesize=flsize;
    return (flsize);
}

void cLinuxOS_Archive::close(sFileHandle *genHandle)
{	int handle = ((sLinuxOS_Handle *)genHandle)->handle;
	::close(handle);
	freeArcHandle(genHandle);
}

uintf cLinuxOS_Archive::getFileSize(const char *filename)
{	int handle = ::open(filename, O_RDONLY | O_BINARY);
	filesize = (uintf)::lseek64(handle,0,SEEK_END);
	::close(handle);
	return filesize;
}

sFileHandle	*cLinuxOS_Archive::openDir(const char *path)
{	// allocate a DirHandle
	sLinuxOS_DirHandle *useHandle = NULL;
	for (uintf i=0; i<maxDirHandles; i++)
	{	if (dirHandle[i].inuse==0)
		{	useHandle = &dirHandle[i];
			useHandle->inuse = 1;
			break;
		}
	}
	if (!useHandle) return NULL;

	sFileHandle *genHandle = allocArcHandle();
	sLinuxOS_Handle *arcHandle = (sLinuxOS_Handle *)genHandle;

	arcHandle->dirHandle = useHandle;
	tprintf(useHandle->findDataPath, 256, "%s/*.*",path);
	return genHandle;
}

bool cLinuxOS_Archive::readDir(sFileHandle *genHandle, sDirectoryData *dir)
{
/*	sLinuxOS_Handle *arcHandle = (sLinuxOS_Handle *)genHandle;
	sLinuxOS_DirHandle *findHandle = arcHandle->dirHandle;

	bool retry = true;
	while (retry)
	{	switch(findHandle->inuse)
		{	case 0:								// This handle has not been allocated!
				return false;

			case 1:								// This is our first read
				arcHandle->handle = _findfirst((char *)findHandle->findDataPath, &findHandle->finddata_t);
				if (arcHandle->handle==-1)
				{	return false;				// Path not found
				}
				findHandle->inuse = 2;
				break;

			case 2:								// This is a subsequent read
				if (_findnext(arcHandle->handle, &findHandle->finddata_t)!=0) return false;
				break;

			default:							// Error condition - unknown 'inuse' value
				return false;
		}
		retry = false;
		if (txtcmp(findHandle->finddata_t.name,".")==0) retry = true;
		if (txtcmp(findHandle->finddata_t.name,"..")==0) retry = true;
	}
	txtcpy((char *)dir->filename,sizeof(dir->filename), findHandle->finddata_t.name);
	dir->size = findHandle->finddata_t.size;
	dir->flags = 0;
	uintf attrib = findHandle->finddata_t.attrib;
	if (attrib & _A_HIDDEN) dir->flags |= dirFlag_Hidden;
	if (attrib & _A_RDONLY) dir->flags |= dirFlag_ReadOnly;
	if (attrib & _A_SUBDIR) dir->flags |= dirFlag_Directory;
	return true;
*/
	return false;
}

void cLinuxOS_Archive::closeDir(sFileHandle *genHandle)
{/*
	sLinuxOS_Handle *arcHandle = (sLinuxOS_Handle *)genHandle;
	sLinuxOS_DirHandle *findHandle = arcHandle->dirHandle;
	if (findHandle->inuse>0)
	{	_findclose(arcHandle->handle);
		findHandle->inuse = 0;
	}
	freeArcHandle(genHandle);
*/
}

cLinuxOS_Archive::~cLinuxOS_Archive(void)
{
}

cFileArchive *initOS_Archive(void)
{	if (sizeof(sLinuxOS_Handle) > sizeof(sFileHandle))
		msg("Structure Size Error","sLinuxOS_Handle exceeds the size of sFileHandle!");
	cLinuxOS_Archive *arc = new cLinuxOS_Archive;
	for (uintf i=0; i<maxDirHandles; i++)
		arc->dirHandle[i].inuse = 0;
	return arc;
}
