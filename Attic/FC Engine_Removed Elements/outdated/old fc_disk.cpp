#include "fc_h.h"			// Standard FC internals
#include <fcntl.h>			// Needed for File Operations
#include <sys/stat.h>		// Needed for File system flags
#include <errno.h>			// Needed for reporting disk errors
#include <bucketsystem.h>

#ifdef PLATFORM_win32
#include <io.h>				// Needed for File Operations
#include <direct.h>			// Needed to access directories
#include <process.h>		// Needed for Multithreading
#endif

#if defined (PLATFORM_mac) || defined (PLATFORM_linux)
#define O_BINARY 0			// Mac / Linux does not care about O_BINARY
#include <unistd.h>			// Needed for close function
#include <dirent.h>			// Needed to search through directories
#define dirForewardSlash
#define _chmod chmod
#endif

// Some compilers support POSIX file handling (open, read, close, etc) and not ISO ANSI (_open, _read, etc), so patch in support for them
#ifndef _open
#define _open open
#endif

#ifndef _read
#define _read read
#endif

#ifndef _write
#define _write write
#endif

#ifndef _close
#define _close close
#endif

#ifndef _lseek
#define _lseek lseek
#endif

void initbackgroundloader(void);
void killbackgroundloader(void);
char *fixfilename(char *filename);

bool	backgroundloadersupported;
long 	filesize;
char	filepath[256],fileactualname[128],fileextension[32];

class fcdisk : public fc_disk
{	long FileExists(char *filename)									{return ::FileExists(filename);};
	long hFileOpen(char *filename)									{return ::hFileOpen(filename);};
	long hFileOpenRW(char *filename)								{return ::hFileOpenRW(filename);};
	long hFileCreate(char *filename)								{return ::hFileCreate(filename);};
	dword hFileSeek(long handle,long offset,dword seekflags)		{return ::hFileSeek(handle,offset,seekflags);};
	long hFileRead(long handle, void *DestPtr,long size)			{return ::hFileRead(handle,DestPtr,size);};
	long hFileWrite(long handle,void *SourcePtr,long size)			{return ::hFileWrite(handle,SourcePtr,size);};
	void hFileClose(long handle)									{		::hFileClose(handle);};
	long FileGetSize(char *filename)								{return ::FileGetSize(filename);};
	long FileSave(char *filename, void *SourcePtr, long numbytes)	{return ::FileSave(filename,SourcePtr,numbytes);};
	byte *FileLoad(char *filename, dword generalflags)				{return ::FileLoad(filename, generalflags);};
	void ChangeDirString(char *currentdir,char *addition)			{		::ChangeDirString(currentdir,addition);};
	bool FileNameCmp(char *srcfile, char *filespec)					{return ::FileNameCmp(srcfile,filespec);};
	void filenameinfo(char *filename)								{		::filenameinfo(filename);};
	void filefind(char *_filespec)									{		::filefind(_filespec);};
	char *findnext(void)											{return ::findnext();};
	char *findnextdir(void)											{return ::findnextdir();};
	long MountArchive(char *filename)								{return ::MountArchive(filename);};
	void UnmountArchive(long handle)								{		::UnmountArchive(handle);};
	void *getvarptr(char *name);
} *OEMdisk;

void *fcdisk::getvarptr(char *name)
{	if (txticmp(name,"filesize")==0) return &filesize;
	if (txticmp(name,"filepath")==0) return filepath;
	if (txticmp(name,"fileactualname")==0) return fileactualname;
	if (txticmp(name,"fileextension")==0) return fileextension;
	if (txticmp(name,"cmdline")==0) return cmdline;
	if (txticmp(name,"programflname")==0) return programflname;
	if (txticmp(name,"programfilename")==0) return programflname;
	if (txticmp(name,"backgroundloadersupported")==0) return &backgroundloadersupported;
	return NULL;
}

// Variables and structures needed for file archival system
#define maxmyhandles 20
#define maxarc 5
#define maxarcdirs 128
#define arcFileTypeNameLen 60

struct arcfiletype
{	char	flname[arcFileTypeNameLen];
	dword	startpos;
	dword	size;
};

struct 
{	long	startpos;
	long	size;
	long	currentseek;
	long	arcvol;
}	myhandle[maxmyhandles];

struct 
{	long		handle;
	long		numfiles;
	arcfiletype	*file;
}	arcvol[maxarc];

char arcdir[maxarcdirs][arcFileTypeNameLen];
dword numarcdirs;

fc_disk *initdisk(void)
{	dword count;

	filesize=0;
	// Init file archival system
	for (count=0; count<maxmyhandles; count++)
		myhandle[count].arcvol=-1;
	for (count=0; count<maxarc; count++)
		arcvol[count].handle=-1;
	numarcdirs=0;

	OEMdisk = new fcdisk;
	initbackgroundloader();
	return OEMdisk;
}

void killdisk(void)
{
	killbackgroundloader();
	delete OEMdisk;
	// Close all archive volumes
	for (dword count=0; count<maxarc; count++)
		if (arcvol[count].handle>=0)
			UnmountArchive(count);
}

bool FileNameCmp(char *srcfile, char *filespec)
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
				}
				else
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

//*********************************************************************************
//***					File and Archive handling functions						***
//*********************************************************************************
char thisdir[256];

long MountArchive(char *filename)
{	char tail[4];
	long tmp;
	long nextarcvol=-1;

	for (tmp=0; tmp<maxarc; tmp++)
	{	if (arcvol[tmp].handle==-1)
		{	nextarcvol=tmp;
			break;
		}
	}

	if (nextarcvol==-1)
		msg("Archival Overflow","This program is accessing too many archives");

	long handle = arcvol[nextarcvol].handle = hFileOpen(filename);

	hFileSeek(handle,-8,seek_fromend);
	hFileRead(handle,&tail,4);
	if (tail[0]!='.' || tail[1]!='B' || tail[2]!='I' || tail[3]!='G')
	{	hFileClose(handle);
		arcvol[nextarcvol].handle=-1;
		return -1;
	}
	hFileRead(handle,&tmp,4);
	hFileSeek(handle,tmp,seek_fromstart);
	hFileRead(handle,&arcvol[nextarcvol].numfiles,4);
	// load the directory tree into a memory buffer
	char memexcuse[256];
	txtcpy(memexcuse,buildstr("%s directory tree",filename),sizeof(memexcuse));
	arcvol[nextarcvol].file=(arcfiletype *)fcalloc(arcvol[nextarcvol].numfiles*sizeof(arcfiletype),memexcuse);
	hFileRead(handle,arcvol[nextarcvol].file,arcvol[nextarcvol].numfiles*sizeof(arcfiletype));
	
	// Scan archive for subdirectories
	for (tmp=0; tmp<arcvol[nextarcvol].numfiles; tmp++)
	{	dword i;
		bool flag;
		txtcpy(arcvol[nextarcvol].file[tmp].flname, fixfilename(arcvol[nextarcvol].file[tmp].flname),arcFileTypeNameLen);
		txtcpy(thisdir,arcvol[nextarcvol].file[tmp].flname,sizeof(thisdir));
		for (i=txtlen(thisdir)-1; i>0; i--)
			if (thisdir[i]=='/') { thisdir[i]=0; break; }
		flag=false;
		for (i=0; i<numarcdirs; i++)
			if (!txticmp(thisdir,arcdir[i])) flag=true;
		if (!flag)
		{	txtcpy(arcdir[numarcdirs++],thisdir,arcFileTypeNameLen);
		}
	}
	nextarcvol++;
	return nextarcvol-1;
}

// ---------------------- UnmountArchive API Command -------------------------------
void UnmountArchive(long handle)
{	
	if (handle==-1) return;
	if (handle>maxarc) return;

	hFileClose(arcvol[handle].handle);
	fcfree(arcvol[handle].file);
	arcvol[handle].handle=-1;
}

// ---------------------- fileexists API Command -------------------------------
long FileExists(char *filename)
{	// Returns	0 - File not found							(* = not implemented yet)
	//		   -1 - Actual file found
	//			otherwise, the handle of the archive is returned +1

	int temphandle;
	temphandle=_open(filename, O_RDONLY | O_BINARY);
	if (temphandle!=-1) 
	{	_close(temphandle);
		return(-1);
	}
	
	// Fix any '\\' occurances in the file name
//	dword i=0,j;
//	while (i<txtlen(filename)-1)
//	{	if (filename[i]=='\\' && filename[i+1]=='\\')
//		{	for (j=i+1; j<txtlen(filename)-1; j++)
//			filename[j]=filename[j+1];
//			filename[txtlen(filename)-1]=0;
//		}
//		if (filename[i]=='/') filename[i]='\\';
//		i++;
//	}
//	filename = fixfilename(filename);

	// check for existence in archive
	for (long count=0; count<maxarc; count++)
		if (arcvol[count].handle>=0)
		{	long numfiles=arcvol[count].numfiles;
			for (long i=0; i<numfiles; i++)
				if (!(txticmp(filename,arcvol[count].file[i].flname)))
					return count+1;
		}
	return 0;
}

long hFileOpen(char *flname)
{   long handle;
	char filename[256];
	txtcpy(filename,flname,sizeof(filename));

	// Fix any '\\' occurances in the file name
//	dword i=0,j;
//	while (i<txtlen(filename)-1)
//	{	if (filename[i]=='\\' && filename[i+1]=='\\')
//		{	for (j=i+1; j<txtlen(filename)-1; j++)
//			filename[j]=filename[j+1];
//			filename[txtlen(filename)-1]=0;
//		}
//		if (filename[i]=='/') filename[i]='\\';
//		i++;
//	}

	handle=_open(filename, O_RDONLY | O_BINARY);
    if (handle==-1)
    {	// A real disk file can't be found.  Try the archives
		for (long count=0; count<maxarc; count++)	// scan each archive
		if (arcvol[count].handle>=0)
		{	long numfiles=arcvol[count].numfiles;
			for (long i=0; i<numfiles; i++)			// scan through files within archive
				if (!(txticmp(filename,arcvol[count].file[i].flname))) // compare filenames
				{	// file found in archive - get a handle number for it
					for (long j=0; j<maxmyhandles; j++)
					if (myhandle[j].arcvol==-1)
					{	handle=j;
						break;
					}
					// now, fill in our handle
					myhandle[handle].arcvol=count;
					myhandle[handle].currentseek=0;
					myhandle[handle].size = arcvol[count].file[i].size;
					myhandle[handle].startpos = arcvol[count].file[i].startpos;
					handle = -2-handle;
					break;
				}
			if (handle!=-1) break;
		}
		if (handle==-1) msg("Disk Error",buildstr("Couldn't open file %s",filename));
    }
	return handle;
} 

// ----------------------- hFileOpenRW API Command --------------------------------
long hFileOpenRW(char *filename)
{	dword flags =  O_BINARY | O_RDWR;
	if (!FileExists(filename)) flags |= O_CREAT | O_TRUNC;
	long handle = _open(filename, O_BINARY | O_RDWR);
	if (handle==-1)
	{	msg("Disk Error",buildstr("Couldn't open read/write file %s",filename));
	}
	return handle;
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

// ---------------------- hfilecreate API Command -------------------------------
long hFileCreate(char *filename)
{   long handle;
	handle = _open(fixfilename(filename), O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
    if (handle==-1)
    {	msg("Disk Error",buildstr("Couldn't create file %s\nIt may be write protected",filename));
    }
	return handle;
}

// ------------------------ hfileseek API Command -------------------------------
dword hFileSeek(long handle,long offset,dword seekflags)
{	if (handle>=0)
	{	switch (seekflags)
		{	case seek_fromstart:	return _lseek(handle,offset,SEEK_SET); break;
			case seek_fromcurrent:	return _lseek(handle,offset,SEEK_CUR); break;
			case seek_fromend:		return _lseek(handle,offset,SEEK_END); break;
			default: msg("Disk Error","Invalid Seek flags");
		}
		return 0;
	}	else
	{	long tmphandle=-(handle+2);
		long seekpos = myhandle[tmphandle].currentseek;
		switch (seekflags)
		{	case seek_fromstart:	seekpos=offset; break;
			case seek_fromcurrent:	seekpos+=offset; break;
			case seek_fromend:		seekpos=myhandle[tmphandle].size+offset; break;
			default: msg("Disk Error","Invalid Seek flags");
		}
		if (seekpos<0) seekpos=0;
		if (seekpos>myhandle[tmphandle].size) seekpos=myhandle[tmphandle].size;
		myhandle[tmphandle].currentseek = seekpos;
		return seekpos;
	}
}

// ----------------------- hfileread API Command --------------------------------
long hFileRead(long handle, void *DestPtr,long size)
{   long flsize;
	if (handle>=0)
	{	flsize=_read(handle,(void *)DestPtr,size);
		if (flsize==-1)
		{	msg("Disk Error","Error reading file");
		}
		filesize=flsize;
		return (flsize);
	}	else
	{	// File exists in an archive
		long tmphandle = -(handle+2);
		long archandle = myhandle[tmphandle].arcvol;
		long eof = myhandle[tmphandle].size-myhandle[tmphandle].currentseek;
		if (size>eof) size=eof;
		// Seek to position
		hFileSeek(arcvol[archandle].handle,myhandle[tmphandle].startpos+myhandle[tmphandle].currentseek,seek_fromstart);
		flsize=_read(arcvol[archandle].handle,(void *)DestPtr,size);
		if (flsize==-1)
		{	msg("Disk Error","Error reading file");
		}
		filesize=flsize;
		myhandle[tmphandle].currentseek+=flsize;
		return (flsize);
	}
}

// ----------------------- hFileReadUINT32 API Command --------------------------------
UINT16 hFileReadUINT16(long handle)
{	byte buf[2];
	uint16 result;
	hFileRead(handle, &buf, 2);
	result = ((uint16)(buf[1])<<8) + (uint16)(buf[0]);
	return result;
}

// ----------------------- hFileReadUINT32 API Command --------------------------------
UINT32 hFileReadUINT32(long handle)
{	byte buf[4];
	uint32 result;
	hFileRead(handle, &buf, 4);
	result = ((uint32)(buf[3])<<24) + ((uint32)(buf[2])<<16) + ((uint32)(buf[1])<<8) + (uint32)(buf[0]);
	return result;
}

UINT64 hFileReadUINT64(long handle)
{	byte buf[8];
	uint64 result;
	hFileRead(handle, &buf, 8);
	result = ((uint32)(buf[7])<<56) + ((uint32)(buf[6])<<48) + ((uint32)(buf[5])<<40) + (uint32)(buf[4]<<32) +
			 ((uint32)(buf[3])<<24) + ((uint32)(buf[2])<<16) + ((uint32)(buf[1])<< 8) + (uint32)(buf[0]);
	return result;
}

// ---------------------- hfilewrite API Command --------------------------------
long hFileWrite(long handle,void *SourcePtr,long size)
{	long flsize;
	char *errrep = "Unknown Error";
	if (size==0)
	{	filesize = 0;
		return 0;
	}
	flsize=_write(handle,SourcePtr,size);
    if (flsize!=size)
    {	if (errno==EBADF) errrep="Bad File Handle";
		if (errno==ENOSPC) errrep="Not Enough Space";
		if (errno==EINVAL) errrep="Invalid Arguments";
		msg("Disk Error",buildstr("Handle %x (size was %i, should have been %i)\nReason Given = %s (%i)\nSource Pointer %x, Size = %i\nError writing file ",handle,flsize,size,errrep,errno,SourcePtr,size));
    }
	filesize=flsize;
    return (flsize);
}

// ----------------------- hFileWriteUINT16 API Command --------------------------------
void hFileWriteUINT16(long handle, UINT16 value)
{	byte buf[2];
	buf[0] = (byte)(value & 0xff);
	buf[1] = (byte)((value & 0x0000ff00) >> 8);
	hFileWrite(handle, buf, 2);
}

// ----------------------- hFileWriteUINT32 API Command --------------------------------
void hFileWriteUINT32(long handle, UINT32 value)
{	byte buf[4];
	buf[0] = (byte)(value & 0xff);
	buf[1] = (byte)((value & 0x0000ff00) >> 8);
	buf[2] = (byte)((value & 0x00ff0000) >> 16);
	buf[3] = (byte)((value & 0xff000000) >> 24);
	hFileWrite(handle, buf, 4);
}

// ----------------------- hFileWriteUINT64 API Command --------------------------------
void hFileWriteUINT64(long handle, UINT64 value)
{	byte buf[8];
	buf[0] = (byte) (value & 0x00000000000000ff);
	buf[1] = (byte)((value & 0x000000000000ff00) >> 8);
	buf[2] = (byte)((value & 0x0000000000ff0000) >> 16);
	buf[3] = (byte)((value & 0x00000000ff000000) >> 24);
	buf[4] = (byte)((value & 0x000000ff00000000) >> 32);
	buf[5] = (byte)((value & 0x0000ff0000000000) >> 40);
	buf[6] = (byte)((value & 0x00ff000000000000) >> 48);
	buf[7] = (byte)((value & 0x7f00000000000000) >> 56);
	hFileWrite(handle, buf, 8);
}

// ---------------------- hfileclose API Command --------------------------------
void hFileClose(long handle)
{   if (handle>=0)
	{	_close(handle);
	}	else
	{	long tmphandle=-(handle+2);
		myhandle[tmphandle].arcvol=-1;
	}
}

// ---------------------- filegetsize API Command ------------------------------
long FileGetSize(char *filename)
{	long handle = hFileOpen(filename);
	filesize = hFileSeek(handle,0,seek_fromend);
	hFileClose(handle);
	return filesize;
}

// ----------------------- savefile API Command --------------------------------
long FileSave(char *filename, void *SourcePtr, long numbytes)
{	long handle = hFileCreate(filename);
	filesize = hFileWrite(handle,SourcePtr,numbytes);
    hFileClose(handle);
    return filesize;
}

// ---------------------- loadmemraw API Command --------------------------------
byte *FileLoad(char *filename, dword generalflags)
{   long handle;
	long flsize;
	byte *temp;

    handle = hFileOpen(filename);
	flsize = hFileSeek(handle,0,seek_fromend);
	hFileSeek(handle,0,seek_fromstart);
	if (generalflags & genflag_usetempbuf)
		temp = alloctempbuffer(flsize,filename);
	else
		temp=fcalloc(flsize,filename);
    filesize=hFileRead(handle,temp,flsize);
    hFileClose(handle);

    return temp;
}

// ---------------------- changedirstring API Command --------------------------------
void ChangeDirString(char *currentdir,char *addition)
{	if (txticmp(addition,".."))
	{	txtcat(currentdir,addition,256);	// ### Maybe we should take a maximum size from the caller
		txtcat(currentdir,"\\",256);
	}
	else
	{	long x = txtlen(currentdir)-2;
		while (x>0 && currentdir[x]!='\\' && currentdir[x]!='/') x--;
		if (x>0) currentdir[x+1]=0;
	}
}

// ---------------------- filenameinfo API Command --------------------------------
void filenameinfo(char *filename)
{	// Given a filename like c:\games\fcgame\gamedata.dat, this function sets the filepath to
	// "c:\games\fcgame\"  fileactualname to "gamedata"  and fileextension to "dat"
	long ofs, lastdot, lastbs, nlength;
	
	// Work out path
	lastbs = -1;
	nlength = txtlen(filename);
	for (ofs=0; ofs<nlength; ofs++)
		if (filename[ofs]=='\\' || filename[ofs]=='/') lastbs=ofs;
	lastbs++;
	txtcpy(filepath,filename,sizeof(filepath));
	filepath[lastbs] = 0;

	// Work out extension
	lastdot = -1;
	for (ofs=lastbs; ofs<nlength; ofs++)
	{	if (filename[ofs]=='.') lastdot=ofs;
	}
	if (lastdot<0)
	{	fileextension[0]=0;
		txtcpy(fileactualname,filename+lastbs,sizeof(fileactualname));
		return;
	}
	txtcpy(fileextension,filename+lastdot+1,sizeof(fileextension));
	
	// Now work out the actual name
	txtcpy(fileactualname,filename+lastbs,sizeof(fileactualname));
	fileactualname[lastdot-lastbs] = 0;
}

// ********************************************************************************
// ***								Background Loader							***
// ********************************************************************************
backgroundloaddata *usedbackgroundloaddata=NULL,*freebackgroundloaddata=NULL;

#ifdef win32
dword bgl_status;	// 0 = background loader thread is running.  1 = waiting to terminate.  2 = Terminated

void _cdecl backgroundloader(void *)
{	while (bgl_status==0)
	{	if (!usedbackgroundloaddata)
		{	// Nothing waiting to be loaded
			threadsleep(100);	
		}
		else
		{	// This is where we load the next file
			backgroundloaddata *bgl = usedbackgroundloaddata;
			if ((bgl->flags & 1) == 1)
			{	bgl->buffer = fcalloc(bgl->filesize,bgl->filename);
				long handle = hFileOpen(bgl->filename);
				while (bgl->loaded<bgl->filesize)
				{	bgl->loaded += hFileRead(handle,bgl->buffer,4096);
				}
				if (bgl->loaded>=bgl->filesize)
				{	bgl->flags++;
					bgl->finished(bgl);
					deletebucket(backgroundloaddata,bgl);
				}
			}	else
			{	// There is something waiting to be loaded, but it's not fully initialized yet ... just sleep for a bit
				threadsleep(5);
			}
		}
	}
	bgl_status = 2;
	_endthread();
}

void initbackgroundloader(void)
{	bgl_status = 0;
	_beginthread(backgroundloader,0,NULL);
	backgroundloadersupported = true;
}

void killbackgroundloader(void)
{	bgl_status = 1;
	while (bgl_status==1) checkmsg();
	backgroundloadersupported = false;
}

backgroundloaddata *backgroundload(char *flname,void *userdata,void (*_finished)(backgroundloaddata *))
{	bool forcealloc;
	dword flsize = FileGetSize(flname);
	backgroundloaddata *bgl;
	allocbucket(backgroundloaddata,bgl,flags,0x80000000,32,"Background Loading Buffer");
//	if (forcealloc)
//		bgl->flags = 0x80000000;
//	else bgl->flags = 0;
	bgl->userdata = userdata;
	txtcpy(bgl->filename,flname,sizeof(bgl->filename));
	bgl->filesize = flsize;
	bgl->loaded = 0;
	bgl->finished = _finished;
	bgl->flags |= 1;
	return bgl;
}
#else
// Background loader currently only supported on the win32 platform
void initbackgroundloader(void)
{	backgroundloadersupported = false;
}

void killbackgroundloader(void)
{
}

backgroundloaddata *backgroundload(char *flname,void *userdata,void (*_finished)(backgroundloaddata *))
{//	bool forcealloc;
	dword flsize = FileGetSize(flname);
	backgroundloaddata *bgl;
	allocbucket(backgroundloaddata,bgl,flags,0x80000000,32,"Background Loading Buffer");
//	if (forcealloc)
//		bgl->flags = 0x80000000;
//	else bgl->flags = 0;
	bgl->userdata = userdata;
	txtcpy(bgl->filename,flname,BackgroundLoadFilenameLen);
	bgl->filesize = flsize;
	bgl->loaded = 0;
	bgl->finished = _finished;
	bgl->flags |= 1;

	
	bgl->buffer = fcalloc(bgl->filesize,bgl->filename);
	long handle = hFileOpen(bgl->filename);
	while (bgl->loaded<bgl->filesize)
	{	bgl->loaded += hFileRead(handle,bgl->buffer,4096);
	}
	if (bgl->loaded>=bgl->filesize)
	{	bgl->flags++;
		bgl->finished(bgl);
		deletebucket(backgroundloaddata,bgl);
	}
	
	return NULL;
}
#endif

// ********************************************************************************************
// ***									File Find System									***
// ********************************************************************************************
#ifdef PLATFORM_win32
// File finding system
_finddata_t _filefind;  // Windows filefind structure
char *filefindspec;		// Filespec to search for
long findingfile=0;		// 0 = idle; 1=have fspec; 2=searching
long filefind_handle;	// Windows filefind handle
int  filefind_rv=-1;	// Last return value from windows filefind

char filefindbuf[128],*ffindlhs,*ffindrhs;
char filefindbuf2[128];
char filespec[128];
bool foundbackdir;

// --------------------- filefind API command ----------------------
void filefind(char *_filespec) // noarc
{	dword i,j;

	findingfile=0;

	// fix the filespec, in case it has '//' in it somewhere
	txtcpy(filespec,_filespec,sizeof(filespec));
//consoleadd("Commenced a search on %s",filespec);
	foundbackdir = false;
	i=0;
	while (i<txtlen(filespec)-1)
	{	if (filespec[i]=='\\' && filespec[i+1]=='\\')
		{	for (j=i+1; j<txtlen(filespec)-1; j++)
				filespec[j]=filespec[j+1];
			filespec[txtlen(filespec)-1]=0;
	}
		i++;
	}
//showstep(buildstr("Ran a disgnostic on the filespec to remove occurances of \\\\ (now %s)",filespec));

	// Convert to lower case
	for (i=0; i<txtlen(filespec); i++)
	{	filespec[i] = tolower_array[filespec[i]];
	}
//consoleadd("Performing search on %s",filespec);

	if (findingfile) _findclose(filefind_handle);	// Close any previous file searches
	txtcpy(filefindbuf,filespec,sizeof(filefindbuf));
	filefindspec=filespec;
	findingfile=1;

	// Find seperator (if any)
	ffindlhs=filefindbuf;
	ffindrhs=NULL;
	i=0;
	while (i<txtlen(filespec) && filespec[i]!='*') i++;
	if (i<txtlen(filespec)-1) filefindbuf[i]=0;
	ffindrhs=ffindlhs+i+1;
//consoleadd("split filespec into these 2 portions : '%s' and '%s'",ffindlhs,ffindrhs);

/*	backgroundrgb = 0x800000;
	cls();
	lock3d();
	consoleadd(filespec);
	consoleadd(ffindlhs);
	if (ffindrhs)	consoleadd(ffindrhs); else consoleadd("NULL");
	consoleadd("----------");
	consoledraw();
	unlock3d();
	update();
	while (!keydown[fckey_SPACE]) checkmsg();
	while (keydown[fckey_SPACE]) checkmsg();
*/
}

long searcharc,arcprog,dirprog;
// --------------------- filenext API command ----------------------
char *myfindnext(void)	// noarc
{
	if (findingfile==0) return NULL;

	if (findingfile==1) 
	{	//showstep("Finding First occurance of file (looking for REAL File");
		filefind_handle=_findfirst(filefindspec,&_filefind);
		if (filefind_handle!=-1)
		{	findingfile=2;
			return _filefind.name;
		} else
		{	findingfile=3;
			_findclose(filefind_handle);
//			return NULL;
		}
	}

	if (findingfile==2)
	{	//showstep("Finding next occurance of file (looking for REAL File)");
		filefind_rv=_findnext(filefind_handle,&_filefind);
		if (filefind_rv!=-1) 
		{	return _filefind.name;
		} else 
		{	findingfile=3;
			_findclose(filefind_handle);
//			return NULL;
		}
	}

	if (findingfile==3)
	{	//showstep("No more real files found, will start searching archives");
		// initialize archive search system
		searcharc=0;
		arcprog=0;
		findingfile=4;
		dirprog=0;
	}

	dword i;
	if (findingfile==4)
	{	//showstep(buildstr("Finding NEXT archived file (starting at mount point %i, file %i)",searcharc,arcprog));
		char *ret = NULL;
		// Examine the next name of the arc file
		while (searcharc<maxarc)
		{	//showstep(buildstr("Investigating archive mount point %i",searcharc));
			if (arcvol[searcharc].handle>=0)
			{	//showstep(buildstr("Archive mount point %i appears to be mounted",searcharc));
				char *name=arcvol[searcharc].file[arcprog].flname;
				arcprog++;
				if (arcprog>=arcvol[searcharc].numfiles)
				{	arcprog=0;
					searcharc++;
				}
				ret = name;
				_filefind.attrib =0;
				// Do a validity check on ret, if any tests fail, set RET to NULL
				if (txticmp(name+txtlen(name)-txtlen(ffindrhs),ffindrhs)) {ret=NULL; continue;}// check RHS
				for (i=0; i<txtlen(ffindlhs); i++)// check LHS
				{	char x2 = name[i];
					if (x2>='A' && x2<='Z') x2 += 'a'-'A'; 
					if (ffindlhs[i]!=x2) 
					{	ret=NULL;
						break;
					}
				}

				if (ret==NULL) continue;
				// Finally, do a test on the gap between ffindlhs & ffindrhs for a '\'
				char *k=ffindlhs+txtlen(ffindlhs);
				while (k<ffindrhs)
					if (*k++=='\\') ret=NULL;
				if (ret) break;	
			}
			else searcharc++;
		}
		if (ret) 
		{	txtcpy(filefindbuf2,ret,sizeof(filefindbuf2));
			// Remove the directory data from the filename
			for (i=txtlen(ret)-1; i>=0; i--)
			{	if (ret[i]=='\\') {i++;break;}
			}
			return filefindbuf2+i;
		}
	}
	return NULL;
}

char *findnext(void)
{	char *x;
	x = myfindnext();
	while (x!=NULL && _filefind.attrib & _A_SUBDIR)
	{	x = myfindnext();
	}
	return x;
}

//#define maxarcdirs 128
//char _arcdir[maxarcdirs][60], *arcdir[maxarcdirs];
//dword numarcdirs;
char _junk[256];
// --------------------- filenext API command ----------------------
char *findnextdir(void)	// ###
{	char *result;
	char *tmp = myfindnext();

//	consoleadd("*");
	while (tmp)
	{	//consoleadd(tmp);
		if (_filefind.attrib & _A_SUBDIR)
		{	if (tmp[0]!='.' || tmp[1]!=0) 
			{	if (txtcmp(tmp,"..")==0)	
					foundbackdir = true;
					return tmp;
			}
		}
		tmp = myfindnext();		
	}

	// Start searching through archives

	// Take off any *.* at the end of the directory name
	if (filespec[txtlen(filespec)-1]=='*')
	{	filespec[txtlen(filespec)-1]=0;
		if (filespec[txtlen(filespec)-1]=='.' && filespec[txtlen(filespec)-2]=='*')
			filespec[txtlen(filespec)-2]=0;
	}

	bool foundmatch;
	foundmatch=false;
	dword j;

//	### This code is necessary for directories contained totally within an archive
//  ### It's disabled here because it currently returns '..' even at 'c:\'
//	if (dirprog==0 && !foundbackdir)
//	{	foundbackdir=true;
//		if (filespec[0]=='*')
//			return "..";
//	}

	for (j=dirprog; j<numarcdirs; j++)
	{	char *tmp = arcdir[j];
		//consoleadd("Testing %s against %s",tmp,filespec);
		if (txtlen(filespec)<=txtlen(tmp))
		{	foundmatch=true;
			// Scan for a match on the left hand side
			dword i;
			for (i=0; i<txtlen(filespec); i++)
			{	char c = tmp[i];
				if (c>='A' && c<='Z') c += 'a'-'A'; 
				if (c!=filespec[i]) 
				{	foundmatch=false;
					//consoleadd("Discarded by Left test on char[%l] out of %l",i,txtlen(filespec));
					break;
				}
			}
			// Scan for occurances of '\' 
			for (i=txtlen(filespec)+1; i<txtlen(tmp); i++)
			{	if (tmp[i]=='\\')
				{	foundmatch=false;
					//consoleadd("Discarded by \\ test on character %l",i);
					break;
				}
			}
		}	else //consoleadd("Discarded by size test");
		if (foundmatch) break;
	}

	if (foundmatch)
	{	result=arcdir[j];
		char *z = result;
		while (*z)
		{	if (*z=='\\') result=z+1;
			z++;
		}
	}	else result=NULL;

	dirprog=j+1;

/*	if (foundmatch)
	{	bool xxx=false;
		if (locked3d)
		{	xxx=true;
			unlock3d();
		}	
		backgroundrgb=0x808080;
		cls();
		if (foundmatch)
			consoleadd("Searching for Directory %s and found %s",filespec,result);
		else
			consoleadd("Did not find a match for %s",filespec);
		lock3d();
		consoledraw();
		unlock3d();
		update();
		while (!keydown[fckey_ENTER]) checkmsg();
		while ( keydown[fckey_ENTER]) checkmsg();
		if (xxx)
			lock3d();
	}
*/
	if (result)
	{	txtcpy(_junk,result,sizeof(_junk));
		txtcat(_junk,"***",sizeof(_junk));
		result = _junk;
	}
	return result;
}
#else	// if win32
// Filefind not yet supported on other platforms
void filefind(char *_filespec)
{
}

char *findnext(void)
{	return NULL;
}

char *findnextdir(void)
{	return NULL;
}
#endif