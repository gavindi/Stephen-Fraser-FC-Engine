/********************************************************************************/
/*  				 FC_Archives - File Archives System							*/
/*	    			 ----------------------------------							*/
/*	 				   (c) 2008 by Stephen Fraser								*/
/*																				*/
/* Contains the built-in plugins for handling file archives (eg: ZIP, RAR, etc)	*/
/* This file is initialised from fc_files.cpp									*/
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <fcio.h>

// ----------------------------------------------------------------------------
//								FVMA - FlameVM Archive
//								----------------------
// A simple, uncompressed archival system.
// Advantages - Good for files already compressed (eg: JPG, OGG)
//			  - Supports DMA-based loading of files (minimal CPU involvement)
// Disadvantages - No compression applied to files
//				 - Cannot write to the archive once it is created
// ----------------------------------------------------------------------------

struct sFVMA_Header
{	uint32	totalDirs;
	uint32	totalFiles;
	uint32	totalTextPool;
	uint32	startOffset;
	uint32	flags;
	char	magicNumber[4];
};

/*
	Directory Header			// This is the header information written to the disk
	----------------
	uint16	numEntries;
	uint32	txtPoolSize;
	byte	textPool[];
	...
	Each Entry
	----------
	uint32	txtPoolOffset;
	uint32	size;
	uint64	offset;
*/

struct	sFVMA_Entry
{	char		*name;
	uint64		offset;			// Offset within the archive file where the data for this entry is
	uintf		size;			// Files: Size of file,  Dirs: Number of child entries contained within the directory
	sFVMA_Entry *child;			// For directories, points to the first entry within the directory
	sFVMA_Entry	*link;			// Pointer to next entry
};

struct sFVMA_Handle
{	// Must not exceed the byte size of sFileHandle in any way!
	void			*arc_class;		// We can ignore this, it will be equal to 'this'
	int64			seek_pos;		// Current Seek Position within the archive of where we are exracting data
	sFVMA_Entry		*entry;			// Pointer to the entry data containing the file
};

class	cFVMA : public cFileArchive
{	public:
		bool			valid;

	private:
		sFileHandle		*handle;
		sFVMA_Header	header;

		// Needed for thread safety
		sFVMA_Handle	*lastHandle;

		// Globals needed for the mounting process
		char			*textPool;		// Storage area for the text (filenames)
		sFVMA_Entry		*entries;		// Collection of all the entries
		uintf			nextEntry;		// Next available free entry
		uintf			textPoolOfs;	// Offset of the next free byte in the text pool

		// Empty Directory Marker
		sFVMA_Entry		emptyDir;

	public:
		cFVMA(const char *filename);
		~cFVMA(void);

		void		*fileExists(const char *filename);
		sFileHandle *open(const char *filename);		
		sFileHandle *create(const char *filename);
		sFileHandle *openRW(const char *filename);
		int64		seek(sFileHandle *handle, int64 offset, uintf seekflags);
		uintf		read(sFileHandle *handle, void *buf, uintf size);
		uintf		write(sFileHandle *handle, void *buf, uintf size);
		void		close(sFileHandle *handle);
		uintf		getFileSize(const char *filename);
		sFileHandle	*openDir(const char *path);
		bool		readDir(sFileHandle *handle, sDirectoryData *dir);
		void		closeDir(sFileHandle *handle);
	private:
		void		mountDir(sFVMA_Entry *entry);
};

void *cFVMA::fileExists(const char *filename)
{	char flname[256];

	txtcpy(flname,sizeof(flname),filename);

	// Obtain starting point for search
	sFVMA_Entry *entry = entries[0].child;

	char *nameptr = flname;
	for (uintf i=0; nameptr[i]!=0; i++)
	{	if (nameptr[i]=='/')
		{	// Scan through dir tree to find a matching SUBDIRECTORY
			nameptr[i] = 0;
			bool foundSubDir = false;
			while (entry)
			{	if ((entry->child) && (txtcmp(entry->name, nameptr)==0))
				{	entry = entry->child;
					foundSubDir = true;
					nameptr += i + 1;
					break;
				}
				entry = entry->link;
			}
			if (!foundSubDir)
			{	return NULL;
			}
			i=0;
		}
	}
	// Now search for a matching FILE
	while (entry)
	{	if ((!entry->child) && (txtcmp(entry->name, nameptr)==0))
		{	return entry;
		}
		entry = entry->link;
	}
	return NULL;
}

sFileHandle *cFVMA::open(const char *filename)
{	sFVMA_Entry *entry = (sFVMA_Entry *)fileExists(filename);
	if (!entry) return NULL;
	sFileHandle *genHandle = allocArcHandle();
	sFVMA_Handle *arcHandle = (sFVMA_Handle *)genHandle;
	arcHandle->arc_class = this;
	arcHandle->entry = entry;
	arcHandle->seek_pos = entry->offset;
	return genHandle;
}

sFileHandle *cFVMA::create(const char *filename)
{	// Currently not possible to append files into an FVMA archive
	return NULL;
}

sFileHandle *cFVMA::openRW(const char *filename)
{	// Currently not possible to expand on files in an FVMA archive
	return NULL;
}

int64 cFVMA::seek(sFileHandle *genHandle, int64 offset, uintf seekflags)
{	sFVMA_Handle *arcHandle = (sFVMA_Handle *)genHandle;
	sFVMA_Entry *entry = arcHandle->entry;
	switch (seekflags)
	{	case seek_fromstart:
			arcHandle->seek_pos = entry->offset + offset;
			break;
		case seek_fromcurrent:
			arcHandle->seek_pos += offset;
			break;
		case seek_fromend:
			arcHandle->seek_pos = entry->offset + entry->size + offset;
			break;
	}
	// ### Lock archive
	if (lastHandle == arcHandle)
	{	fileSeek(handle, (uintf)entry->offset, seek_fromstart);	// ### Limits archive to 2 gigs!  Need 64 bit seek operation
	}
	// ### Unlock Archive
	return arcHandle->seek_pos - entry->offset;
	// ### TODO: Report error on bogus seek address
}

uintf cFVMA::read(sFileHandle *genHandle, void *buf, uintf size)
{	sFVMA_Handle *arcHandle = (sFVMA_Handle *)genHandle;
	sFVMA_Entry *entry = arcHandle->entry;
	uintf	bytesUsed = (uintf)(arcHandle->seek_pos - entry->offset);
	uintf	bytesRemain = entry->size - bytesUsed;
	if (size > bytesRemain) size = bytesRemain;
	// ### Lock archive
	if (lastHandle != arcHandle)
	{	lastHandle = arcHandle;
		fileSeek(handle, (uintf)arcHandle->seek_pos, seek_fromstart);	// ### Limits archive to 2 gigs!  Need 64 bit seek operation
	}
	uintf bytesRead = fileRead(handle, buf, size);
	// ### Unlock Archive
	arcHandle->seek_pos += bytesRead;
	return bytesRead;
}

uintf cFVMA::write(sFileHandle *handle, void *buf, uintf size)
{	// Currently not possible to write to files in FVMA archive
	return 0;
}

void cFVMA::close(sFileHandle *genHandle)
{	freeArcHandle(genHandle);
}

uintf cFVMA::getFileSize(const char *filename)
{	sFVMA_Entry *entry = (sFVMA_Entry *)fileExists(filename);
	if (!entry) return 0;			// This should never happen as the lowlevel code should stop it prior to getting here
	return (intf)entry->size;
}

sFileHandle	*cFVMA::openDir(const char *path)
{	char flname[256];

	txtcpy(flname,sizeof(flname),path);

	// Obtain starting point for search
	sFVMA_Entry *entry = entries[0].child;

	char *nameptr = flname;
	
	if (*nameptr)
	{	bool finished = false;
		for (uintf i=0; !finished; i++)
		{	if (nameptr[i]=='/' || nameptr[i]==0)
			{	if (nameptr[i]==0) finished = true;
				// Scan through dir tree to find a matching SUBDIRECTORY
				nameptr[i] = 0;
				bool foundSubDir = false;
				while (entry)
				{	if ((entry->child) && (txtcmp(entry->name, nameptr)==0))
					{	entry = entry->child;
						foundSubDir = true;
						nameptr += i + 1;
						break;
					}
					entry = entry->link;
				}
				if (!foundSubDir)
				{	return NULL;
				}
				i=0;
			}	// if ('/' or 0)
		}	// for (i=0...)
	}	// if (*nameptr)

	sFileHandle *genHandle = allocArcHandle();
	genHandle->arc_class = this;
	sFVMA_Handle *arcHandle = (sFVMA_Handle *)genHandle;
	arcHandle->entry = entry;
	return genHandle;
}

bool cFVMA::readDir(sFileHandle *genHandle, sDirectoryData *dir)
{	sFVMA_Handle *arcHandle = (sFVMA_Handle *)genHandle;
	sFVMA_Entry *entry = arcHandle->entry;
	if ((entry) && (entry != &emptyDir))
	{	txtcpy((char *)dir->filename, sizeof(dir->filename), entry->name);
		dir->flags = dirFlag_ReadOnly;
		if (entry->child)
		{	// Directory Entry
			dir->flags |= dirFlag_Directory;
			dir->size = 0;
		}	else
		{	// File Entry
			dir->size = entry->size;
		}
		arcHandle->entry = arcHandle->entry->link;
		return true;
	}	else
	{	txtcpy((char *)dir->filename, sizeof(dir->filename), "");
		dir->flags = 0;
		dir->size = 0;
		return false;
	}
}

void cFVMA::closeDir(sFileHandle *genHandle)
{	freeArcHandle(genHandle);
}


void cFVMA::mountDir(sFVMA_Entry *entry)
{	uintf i;
	sFVMA_Entry *child;

	fileSeek(handle, (uintf)entry->offset, seek_fromstart);		// ### Limits archive to 2 gigs!  Need 64 bit seek operation
	entry->size = fileReadUINT16(handle);
	uintf	txtPoolSize= fileReadUINT32(handle);
	fileRead(handle, textPool+textPoolOfs, txtPoolSize);
	if (entry->size==0)
	{	entry->child = &emptyDir;									// Child Directory appears to be empty
	}	else
		entry->child = &entries[nextEntry];							// Set up the child directory
	for (i=0; i<entry->size; i++)
	{	child = &entries[nextEntry++];
		uintf tpo = fileReadUINT32(handle);
		if (tpo & 0x80000000)
		{	tpo = tpo & 0x7fffffff;
			child->child = child;									// This is just a place-setter to indicate that this is a directory
		}
		else
			child->child = NULL;
		child->name = &textPool[textPoolOfs + tpo];
		child->size = fileReadUINT32(handle);
		child->offset = fileReadUINT64(handle);
		if (i+1<entry->size)
			child->link = &entries[nextEntry];
		else
			child->link = NULL;
	}
	textPoolOfs += txtPoolSize;
	child = entry->child;
	while (child)
	{	if (child->child) mountDir(child);
		child = child->link;
	}
}

cFVMA::cFVMA(const char *filename)
{	// Perform a sanity check on handle structure size
	if (sizeof(sFVMA_Handle) > sizeof(sFileHandle))
		msg("Structure Size Error","sFMVA_Handle exceeds the size of sFileHandle!");
	
	memfill(&header, 0, sizeof(header));

	handle = fileOpen(filename);
	fileSeek(handle, -(int)sizeof(sFVMA_Header), seek_fromend);	// ### Limits archive to 2 gigs!  Need 64 bit seek operation
	header.totalDirs = fileReadUINT32(handle);
	header.totalFiles = fileReadUINT32(handle);
	header.totalTextPool = fileReadUINT32(handle);
	header.startOffset = fileReadUINT32(handle);
	header.flags = fileReadUINT32(handle);
	fileRead(handle, (byte *)&header.magicNumber, 4);
	
	uintf match = 0;
	if (header.magicNumber[0] == 'F') match++;
	if (header.magicNumber[1] == 'V') match++;
	if (header.magicNumber[2] == 'M') match++;
	if (header.magicNumber[3] == 'A') match++;
	if (match<4)
	{	valid = false;
		return;
	}
	valid = true;

	// Allocate memory
	uintf	totalEntries = header.totalDirs + header.totalFiles + 1;				// Add one entry for root Dir
	uintf	memneeded = header.totalTextPool + totalEntries * sizeof(sFVMA_Entry);
	char	reason[256];
	tprintf(reason,256,"Mounting Archive File: %s",filename);
	byte *buf = fcalloc(memneeded, reason);
	textPool = (char *)buf;		buf += header.totalTextPool;

	// Fill in empty directory
	emptyDir.name = NULL;
	emptyDir.offset = 0;
	emptyDir.size = 0;
	emptyDir.child = NULL;
	emptyDir.link = NULL;

	entries = (sFVMA_Entry *)buf;
	nextEntry = 1;
	textPoolOfs = 0;

	// Read Root Directory
	entries[0].name = NULL;
	entries[0].offset = header.startOffset;
	entries[0].link = NULL;

	mountDir(&entries[0]);

	lastHandle = NULL;
}

cFVMA::~cFVMA(void)
{	if (valid)
		fcfree(textPool);
	fileClose(handle);
}

cFileArchive *newcFVMA(const char *filename)
{	return new cFVMA(filename);
}

void initArchives(void)
{	addGenericPlugin((void *)newcFVMA, PluginType_Archive, ".fvm");
}
