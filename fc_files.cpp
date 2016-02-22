/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include "fc_h.h"			// Standard FC internals
#include <bucketSystem.h>

#define maxHandles 32

cFileArchive *initOS_Archive(void);	// Prototype used only by this module

sFileHandle	_fileHandles[maxHandles];
byte		_fileHandleInUse[maxHandles];

char *fixfilename(char *filename);

uintf 	filesize;
char	filepath[256],fileactualname[128],fileextension[32];

cFileArchive *OS_Archive;				// Pointer to the file access routines for the OS's filesystem
cFileArchive *firstArchive;				// First archive in a linked list to use when searching for files

void initdisk(void)
{	for (uintf i=0; i<maxHandles; i++)
	{	_fileHandleInUse[i] = 0;
	}
	OS_Archive = initOS_Archive();
	OS_Archive->next = NULL;
	firstArchive = OS_Archive;

	filesize=0;
}

cFileArchive *mountArchiveAs(const char *filename, const char *extension)
{	cFileArchive *(*func)(const char *) = (cFileArchive *(*)(const char *))getPluginHandler(extension, PluginType_Archive);
	if (!func)
		msg("File Error",buildstr("%s is not a supported archive file format",filename));
	cFileArchive *arc = func(filename);
	arc->next = NULL;

	// Links an archive into the engine's file search system, allowing files within the archive to be accessed through
	// functions like fileOpen, fileRead, etc.
	if (!firstArchive)
	{	firstArchive = arc;
		return arc;
	}
	cFileArchive *search = firstArchive;
	while (search->next) search = search->next;
	search->next = arc;
	return arc;
}

cFileArchive *mountArchive(const char *filename)
{	char filetype[32];
	fileNameInfo(filename);
	tprintf((char *)filetype,32,".%s",fileextension);
	return mountArchiveAs(filename, filetype);
}

bool removeArchive(cFileArchive *arc)
{	// Removes an archive from the file search system, typically performed during the archive's destructor function.
	// Ensures the engine will not attempt to access files within the archive once it is unmounted.
	if (firstArchive==arc)
	{	firstArchive = arc->next;
		return true;
	}
	cFileArchive *search = firstArchive;
	while (search->next != arc)
	{	search = search->next;
		if (!search) return false;
	}
	search->next = arc->next;
	return true;
}

cFileArchive::~cFileArchive(void)
{	removeArchive(this);
}

void killdisk(void)
{	// unmount all file archives - ### This code does NOT YET WORK!  Any application-mounted archives that have not been unmounted will CRASH the system!
//	while (firstArchive)
//	{	cFileArchive *arc = firstArchive;
//		while (arc->next) arc = arc->next;
//		delete arc;
//	}
}

bool fileNameCmp(const char *srcfile, const char *filespec)
{	long srcindex = 0;
	long fspecindex = 0;
	while (filespec[fspecindex])
	{	switch(filespec[fspecindex])
		{	case '?' :
				if (srcfile[srcindex]==0) return false; // compare '?' with end-of-string = failure
				else
				{	srcindex++;
					fspecindex++;
				}
				break;
			case '*' :
				if (srcfile[srcindex]==filespec[fspecindex+1])
				{	fspecindex++;
				}	else
				{	srcindex++;
					if (srcfile[srcindex]==0)
					{	if (filespec[fspecindex+1]==0) return true;
						return false;
					}
				}
				break;
			default :
				if (filespec[fspecindex++]!=srcfile[srcindex++]) return false;
				break;
		}
	}
	if (srcfile[srcindex]) return false;
	return true;
}

char fixedfilenamebuffer[256];
char *fixfilename(char *filename)
{	dword index=0;
	while ((index<254) && filename[index])
	{	if (filename[index]=='\\')
			fixedfilenamebuffer[index]='/';
		else
			fixedfilenamebuffer[index]=filename[index];
		index++;
	}
	fixedfilenamebuffer[index]=0;
	return fixedfilenamebuffer;
}

//*********************************************************************************
//***							File handling functions							***
//*********************************************************************************
// ---------------------- fileExists API Command -------------------------------
void *fileExists(const char *filename)
{	cFileArchive *arc = firstArchive;
	void *found = NULL;
	while (arc)
	{	found = arc->fileExists(filename);
		if (found) return found;
		arc = arc->next;
	}
	return NULL;
}

sFileHandle *fileOpen(const char *flname)
{	char error[1024];
	cFileArchive *arc = firstArchive;
	sFileHandle *handle = NULL;
	while (arc)
	{	handle = arc->open(flname);
		if (handle) return handle;
		arc = arc->next;
	}
	tprintf(error,sizeof(error),"File not found: %s",flname);
	msg("File Error",error);
	return NULL;
}

// ----------------------- hFileOpenRW API Command --------------------------------
sFileHandle *fileOpenRW(const char *filename)
{	// First, we need to see if the file already exists, and if so, open it for Read/Write
	sFileHandle *result;
	cFileArchive *arc = firstArchive;
	void *found = NULL;
	while (arc)
	{	found = arc->fileExists(filename);
		if (found) break;
		arc = arc->next;
	}
	if (!found)
	{	// The file does not exist, we need to create a new one
		result = fileCreate(filename);
		// The file now exists, but it is only open for Write access ... close it and reopen for RW
		fileClose(result);
		return fileOpenRW(filename);
	}
	// The file does exist within the archive pointed to by 'arc'.  So open it for RW
	result = arc->openRW(filename);
	if (!result)
	{	msg("File Error",buildstr("Could not open file %s for Read/Write.  It may be marked as Read-Only",filename));
	}
	return result;
}

// ---------------------- hfilecreate API Command -------------------------------
sFileHandle *fileCreate(const char *filename)
{	sFileHandle *result;
	cFileArchive *arc = firstArchive;
	while (arc)
	{	result = arc->create(filename);
		if (result) return result;
		arc = arc->next;
	}
	// At this point, no archive has been able to create this file, as a last resort, try OS_Archive
	result = OS_Archive->create(filename);
	if (!result)
	{	msg("File Error",buildstr("Failed to create new file %s",filename));
	}
	return result;
}

// ------------------------ hfileseek API Command -------------------------------
uintf fileSeek(sFileHandle *handle,int64 offset, uintf seekflags)
{	cFileArchive *arc = handle->arc_class;
	return (uintf)arc->seek(handle, offset, seekflags);
}

// ----------------------- hfileread API Command --------------------------------
uintf fileRead(sFileHandle *handle, void *DestPtr,uintf size)
{	cFileArchive *arc = handle->arc_class;
	return arc->read(handle, DestPtr, size);
}

// ----------------------- hFileReadUINT16 API Command --------------------------------
UINT16 fileReadUINT16(sFileHandle *handle)
{	byte buf[2];
	uint16 result;
	fileRead(handle, &buf, 2);
	result = ((uint16)(buf[1])<<8) + (uint16)(buf[0]);
	return result;
}

// ----------------------- hFileReadUINT32 API Command --------------------------------
UINT32 fileReadUINT32(sFileHandle *handle)
{	byte buf[4];
	uint32 result;
	fileRead(handle, &buf, 4);
	result = ((uint32)(buf[3])<<24) + ((uint32)(buf[2])<<16) + ((uint32)(buf[1])<<8) + (uint32)(buf[0]);
	return result;
}

UINT64 fileReadUINT64(sFileHandle *handle)
{	byte buf[8];
	uint64 result;
	fileRead(handle, &buf, 8);
	result = ((uint64)(buf[7])<<56) + ((uint64)(buf[6])<<48) + ((uint64)(buf[5])<<40) + ((uint64)(buf[4])<<32) +
			 ((uint64)(buf[3])<<24) + ((uint64)(buf[2])<<16) + ((uint64)(buf[1])<< 8) + ((uint64)(buf[0]));
	return result;
}

// ---------------------- hfilewrite API Command --------------------------------
uintf fileWrite(sFileHandle *handle,void *SourcePtr,uintf size)
{	cFileArchive *arc = handle->arc_class;
	return arc->write(handle, SourcePtr, size);
}

// ----------------------- hFileWriteUINT16 API Command --------------------------------
void fileWriteUINT16(sFileHandle *handle, UINT16 value)
{	byte buf[2];
	buf[0] = (byte)(value & 0xff);
	buf[1] = (byte)((value & 0x0000ff00) >> 8);
	fileWrite(handle, buf, 2);
}

// ----------------------- hFileWriteUINT32 API Command --------------------------------
void fileWriteUINT32(sFileHandle *handle, UINT32 value)
{	byte buf[4];
	buf[0] = (byte)(value & 0xff);
	buf[1] = (byte)((value & 0x0000ff00) >> 8);
	buf[2] = (byte)((value & 0x00ff0000) >> 16);
	buf[3] = (byte)((value & 0xff000000) >> 24);
	fileWrite(handle, buf, 4);
}

// ----------------------- hFileWriteUINT64 API Command --------------------------------
void fileWriteUINT64(sFileHandle *handle, UINT64 value)
{	byte buf[8];
	buf[0] = (byte)((value    ) & 0xff);
	buf[1] = (byte)((value>> 8) & 0xff);
	buf[2] = (byte)((value>>16) & 0xff);
	buf[3] = (byte)((value>>24) & 0xff);
	buf[4] = (byte)((value>>32) & 0xff);
	buf[5] = (byte)((value>>40) & 0xff);
	buf[6] = (byte)((value>>48) & 0xff);
	buf[7] = (byte)((value>>56) & 0xff);
	fileWrite(handle, buf, 8);
}

// ---------------------- hfileclose API Command --------------------------------
void fileClose(sFileHandle *handle)
{	cFileArchive *arc = handle->arc_class;
	arc->close(handle);
}

// ---------------------- filegetsize API Command ------------------------------
uintf fileGetSize(const char *filename)
{	sFileHandle *handle = fileOpen(filename);
	uintf result = fileSeek(handle,0,seek_fromend);
	fileClose(handle);
	return result;
}

// ----------------------- savefile API Command --------------------------------
uintf fileSave(const char *filename, void *SourcePtr, uintf numBytes)
{	sFileHandle *handle = fileCreate(filename);
	uintf result = fileWrite(handle,SourcePtr,numBytes);
    fileClose(handle);
    return result;
}

// ---------------------- fileLoad API Command --------------------------------
byte *fileLoad(const char *filename, uintf generalflags)
{   sFileHandle *handle;
	uintf flsize;
	byte *temp;

    handle = fileOpen(filename);
	flsize = fileSeek(handle,0,seek_fromend);
	fileSeek(handle,0,seek_fromstart);
	if (generalflags & genflag_usetempbuf)
		temp = alloctempbuffer(flsize,filename);
	else
		temp=fcalloc(flsize,filename);
    filesize=fileRead(handle,temp,flsize);
    fileClose(handle);

    return temp;
}

// ---------------------- changedirstring API Command --------------------------------
void ChangeDirString(char *currentdir,const char *addition)
{	if (txticmp(addition,".."))
	{	txtcat(currentdir,256,addition);	// ### Maybe we should take a maximum size from the caller
		txtcat(currentdir,256,"\\");
	}
	else
	{	long x = txtlen(currentdir)-2;
		while (x>0 && currentdir[x]!='\\' && currentdir[x]!='/') x--;
		if (x>0) currentdir[x+1]=0;
	}
}

// ---------------------- filenameinfo API Command --------------------------------
void fileNameInfo(const char *filename)
{	// Given a filename like c:\games\fcgame\gamedata.dat, this function sets the filepath to
	// "c:\games\fcgame\"  fileactualname to "gamedata"  and fileextension to "dat"
	long ofs, lastdot, lastbs, nlength;

	// Work out path
	lastbs = -1;
	nlength = txtlen(filename);
	for (ofs=0; ofs<nlength; ofs++)
		if (filename[ofs]=='\\' || filename[ofs]=='/') lastbs=ofs;
	lastbs++;
	if (lastbs==0) txtcpy(filepath,sizeof(filepath),".");
	else
	{	txtcpy(filepath,sizeof(filepath),filename);
		filepath[lastbs] = 0;
	}

	// Work out extension
	lastdot = -1;
	for (ofs=lastbs; ofs<nlength; ofs++)
	{	if (filename[ofs]=='.') lastdot=ofs;
	}
	if (lastdot<0)
	{	fileextension[0]=0;
		txtcpy(fileactualname,sizeof(fileactualname),filename+lastbs);
		return;
	}
	txtcpy(fileextension,sizeof(fileextension),filename+lastdot+1);

	// Now work out the actual name
	txtcpy(fileactualname,sizeof(fileactualname),filename+lastbs);
	fileactualname[lastdot-lastbs] = 0;
}

// ------------------------------------------ Find a file in a string of paths -------------------------------------
bool fileFindInPath(char *result, uintf size, const char *filename, const char *path)
{	// Finds a file from amongst a list of paths.  The path string is a series of semi-colon (;) seperated strings.
	char tempflname[1024];

	// First see if the file exists in the current directory
	if (fileExists(filename))
	{	txtcpy(result, size, filename);
		return true;
	}

	// Search for texture among search directories
	uintf currchar =0;
	uintf length=txtlen(path);
	uintf firstchar = 0;
	while (currchar<=length)
	{	if (path[currchar]==';' || texpath[currchar]==0)
		{	char prevval = texpath[currchar];
			texpath[currchar]=0;
			tprintf(tempflname,sizeof(tempflname),"%s%s",path+firstchar,filename);
			if (fileExists(tempflname))
			{	texpath[currchar]=prevval;
				txtcpy(result, size, tempflname);
				return true;
			}
			texpath[currchar]=prevval;
			firstchar=currchar+1;
		}
		currchar++;
	}
	result[0]=0;
	return false;
}

// ------------------------------------------ Archive System Service Routines -------------------------------------
sFileHandle *allocArcHandle(void)
{	for (uintf i=0; i<maxHandles; i++)
	{	if (_fileHandleInUse[i]==0)
		{	_fileHandleInUse[i]=1;
			memfill(&_fileHandles[i],0,sizeof(sFileHandle));
			return &_fileHandles[i];
		}
	}
	msg("Out of Handles","This program opened too many files at once");
	return NULL;
}

void freeArcHandle(sFileHandle *handle)
{	for (uintf i=0; i<maxHandles; i++)
	{	if (&_fileHandles[i]==handle)
		{	_fileHandleInUse[i]=0;
			return;
		}
	}
	// ### Should generate warning here - invalid handle
}

