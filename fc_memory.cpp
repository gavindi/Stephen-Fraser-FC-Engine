/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
//*********************************************************************************
//***					Memory Allocation and Tracking functions				***
//*********************************************************************************
#include <stdlib.h>
#include "fc_h.h"
#define maxmallocs 16384		// This is the maximum number of memory cells to be traced

struct	mem_tempbuffer
{	byte	*buffer;
	uintf	size;
	bool	locked;
};

logfile			*memorylog = NULL;
byte			*mallocaddr[maxmallocs];
long			mallocsize[maxmallocs];
uint32 			malloced,maxmalloced;
mem_tempbuffer	tmpbuffer = {NULL,0,true};

// Initialize memory services
void initmemoryservices(void)
{	for (long count=0; count<maxmallocs; count++)
		mallocaddr[count]=NULL;
	malloced=0;
	maxmalloced=0;
#ifdef fcdebug
	memorylog = newlogfile("logs/memory.log");
#else
	memorylog = newlogfile(NULL);
#endif
	tmpbuffer.buffer = NULL;
	tmpbuffer.size = 0;
	tmpbuffer.locked = false;
}

// Kill off memory services
void killmemory(void)
{	if (tmpbuffer.buffer) 
	{
		byte *tmp = tmpbuffer.buffer;
		tmpbuffer.buffer[tmpbuffer.size] = 0x88;
		fcfree(tmp);
	}

	if (memorylog)
	{	memorylog->log("");
		memorylog->log("----------------------------------------------------------");

		long notfreed=0;

		if (tmpbuffer.locked)
		{	memorylog->log("%x Freed (Temporary Buffer)",(long)tmpbuffer.buffer);			// ### Typecast pointer to int
			notfreed++;
		}
		// Deallocate all remaining memory
		for (long count=0; count<maxmallocs; count++)
			if (mallocaddr[count])
			{	memorylog->log("%.8x not freed",mallocaddr[count]);
				//fcfree(mallocaddr[count]);
				notfreed++;
			}

		if (notfreed>0)
		{	memorylog->log("");
			memorylog->log("FC Engine reports that it had to release %i blocks",notfreed);
			memorylog->log("of memory that were not freed by this application.");
		}
		memorylog->log("Estimated maximum memory used (excludes video / audio resources) = %i mb",maxmalloced / (1024*1024));
		delete memorylog;
	}
}

void  memfill(void *adr, byte data, uintf size)
{	// TODO: Write 64 bit version of this function
	uintf i;
	uint32 datablock;
	uint8 *adr8;
	uint32 *adr32;

	if (size<4)
	{	adr8 = (uint8 *)adr;
		for (i=0; i<size; i++)
			adr8[i]=data;
		return;
	}	else
	{	adr8 = (uint8 *)adr;
		while ((size>0) && ((uintf)adr & 0x03))
		{	*adr8++ = data;
			size--;
		}
		datablock = (data << 8) + data;
		datablock = (datablock<<16) + datablock;
		adr32 = (uint32 *)adr8;
		while(size>3)
		{	*adr32++ = datablock;
			size -= 4;
		}
		adr8 = (uint8 *)adr32;
		for (i=0; i<size; i++)
			adr8[i]=data;
	}
}

void memcopy(void *dest, void *src, uintf size)
{	uint32 *dest32 = (uint32 *)dest;
	uint32 *src32 = (uint32 *)src;
	while (size>3)
	{	*dest32++ = *src32++;
		size -=4;
	}
	uint8 *dest8 = (uint8*)dest32;
	uint8 *src8 = (uint8 *)src32;
	while (size>0)
	{	*dest8++ = *src8++;
		size--;
	}
}

// ------------------------ fcalloc API Command --------------------------------
byte *fcalloc(long size,const char *name)
{
	byte *temp=(byte *)malloc(size+1);

	if (temp==NULL)
    {	char *errorMsg = buildstr("Insufficient Memory allocating %i bytes for %s",size,name);
		memorylog->log("*** Fatal Error : %s",errorMsg);
		msg("Memory Error",errorMsg);
    	exit(EXIT_FAILURE);
    }
	temp[size]=0x88;

    malloced+=size;
	if (malloced>maxmalloced) maxmalloced=malloced;

	long cellnum = -1;
	for (long count=0; count<maxmallocs; count++)
		if (!mallocaddr[count])
		{	mallocaddr[count]=temp;
			mallocsize[count]=size;
			cellnum = count;
			break;
		}
	if (cellnum<0)
	{	memorylog->log("*** All Memory-test cells full, memory protection no longer available ***");
	}

	memorylog->log("%x Allocated %s (%i bytes) in [cell %i]",(long)temp,name,size,cellnum);			// ### Typecast pointer to int
	return temp;
}

byte *alloctempbuffer(uintf size, const char *reason)
{	if (tmpbuffer.locked)
		return fcalloc(size,reason);
	tmpbuffer.locked = true;
	if (tmpbuffer.size<size)
	{	if (tmpbuffer.buffer)
		{
			if (tmpbuffer.buffer[tmpbuffer.size] != 0x89)
			{	errorOnExit("Memory block overflow! (TempBuffer)");
				memorylog->log("*** Memory block (TempBuffer) has overflowed! ***");
			}
			tmpbuffer.buffer[tmpbuffer.size] = 0x88;
			fcfree(tmpbuffer.buffer);
		}
		tmpbuffer.buffer = fcalloc(size, "Temporary Buffers");
		tmpbuffer.buffer[size] = 0x89;
		tmpbuffer.size = size;
	}
	return tmpbuffer.buffer;
}

void freetempbuffer(void *addr)
{	if (addr==tmpbuffer.buffer)
	{	if (tmpbuffer.locked)
		{	tmpbuffer.locked = false;
		}
		else
			memorylog->log("*** Attempted to free the tempbuffer but it wasn't allocated!");
	}
}

// ----------------------- fcrealloc API Command -------------------------------
byte *fcrealloc(void *addr,long size, const char *reason)
{	
	long adrsafe = 0;
	long realloccell = -1;
	long count;

	char reason_[256];

	if (addr==0) 
	{	//errorOnExit("Memory error - attempted to reallocate memory with a NULL pointer");
		//memorylog->log("*** Tried to reallocate memory with a NULL pointer! ***");
		tprintf(reason_,sizeof(reason_),"%s (realloc NULL)",reason);
		return fcalloc(size,reason_);
	}

	for (count=0; count<maxmallocs; count++)
	{	if (mallocaddr[count]==addr)
		{	if (mallocaddr[count][mallocsize[count]]!=0x88)
			{	errorOnExit("Memory error - Memory block overflow! Address %x [cell %i]",(long)addr,count);
				memorylog->log("*** Memory block at address %x [cell %i] has overflowed! ***",(long)addr,count);
			}
			realloccell = count;
			adrsafe=1;
			break;
		}
	}

	if (!adrsafe) 
	{	errorOnExit("Memory error - Attempted to reallocate memory which was not allocated! (Address %x)",(long)addr);
		memorylog->log("*** Attempted to reallocate %x but it's not allocated! ***",(long)addr);
		tprintf(reason_,sizeof(reason_),"%s (realloc unknown pointer)",reason);
		return fcalloc(size,reason_);
	}

	void *orgptr = addr;
	//size;
	byte *newptr = (byte *)realloc(addr,size+1);
	newptr[size]=0x88;
	if (realloccell>=0)
	{	mallocaddr[count]=newptr;
		mallocsize[count]=size;
	}

	if (newptr==NULL)
    {	msg("Error","Insufficient Memory for reallocation");
    	exit(EXIT_FAILURE);
    }

	memorylog->log("%.8x reallocated to %.8x %s (%i bytes) in [cell %i]",(long)orgptr,(long)newptr,reason,size,realloccell);
	return newptr;
}

// ------------------------ fcalloc2d API command --------------------------------
void *fcalloc2d(long width, long height, long entrysize, const char *reason)
{	// Allocates a 2D array as a single allocation.  Sets up all necessary pointers
	dword *memblock = (dword *)fcalloc(width*height*entrysize + height*ptrsize,reason);
	dword *memptr = memblock;
	byte *tablestart = ((byte *)memblock)+height*ptrsize;
	for (long i=0; i<height; i++)
		*memptr++=(long)(tablestart+entrysize*i);			// ### Typecast pointer to int  (int -> long added to pointer)
	return (void *)memblock;
}

// ------------------------ memlog API command --------------------------------
void memlog(char *msg)
{	if (memorylog) memorylog->log(msg);
}

// ------------------------ fcfree API Command --------------------------------
void _fcfree(void *addr)	// fcfree is actually a macro in fcio.h
{
	long adrsafe = 0;
	long count;

	if (addr==0) 
	{	memorylog->log("*** Tried to free memory with a NULL pointer! ***");
		return;
	}

	for (count=0; count<maxmallocs; count++)
	{	if (mallocaddr[count]==addr)
		{	if (mallocaddr[count][mallocsize[count]]!=0x88)
			{	errorOnExit("Memory block overflow! Address %x [cell %i]",(long)addr,count);
				memorylog->log("*** Memory block at address %x [cell %i] has overflowed! ***",(long)addr,count);
			}
			mallocaddr[count]=NULL;
			malloced-=mallocsize[count];
			adrsafe=1;
			break;
		}
	}

	if (!adrsafe) 
	{	memorylog->log("*** Attempted to free %x but it's not allocated! ***",(long)addr);			// ### Typecast pointer to int
		return;
	}
	memorylog->log("%.8x Freed [cell %i]",(long)addr,count);										// ### Typecast pointer to int

	free(addr);
}

// ****************************************************************************************
// ***								MemCell Functions									***
// ****************************************************************************************

#define MemBlockNameLen 32

struct sMemBlock
{	uintf		verify1;				// ( 4) Postblock-verify
	uintf		size;					// ( 8) Size of this block
	char		name[MemBlockNameLen];	// (40) Name of this block
	sMemBlock	*next;					// (44) Pointer to the next block
	sMemBlock	*prev;					// (48) Pointer to the previous block
};

class OEMMemoryCell : public cMemoryCell
{	private:
		char	name[128];
		uintf	heapsize;
		uintf	numallocs;
		uintf	maxmalloced;
		uintf	currentlyalloced;
		byte	*memheap;
		byte	*memend;
		sMemBlock *freeblock, *usedblock;
		logfile	*log;
		bool	prealloced;

	public:
		OEMMemoryCell(const char *name, const char *filename, byte *buffer, uintf ByteSize);
		~OEMMemoryCell(void);
		void init(char *filename, byte *buffer, uintf ByteSize);
		void *malloc(uintf size, const char *reason);
		void free(void *address);
		void *calloc(uintf num, uintf size, const char *reason);
		void *realloc(void *memblock, uintf size);
		void memReport(const char *name);
	private:
		void splitblock(sMemBlock *small, uintf size);
		byte* lowlevel_malloc(uintf size, const char *reason);
		bool trytojoin(sMemBlock *b1, sMemBlock *b2);
		void lowlevel_free(byte *d);
};

cMemoryCell *newMemoryCell(const char *name, const char *filename, uintf ByteSize, byte *buffer)
{	return new OEMMemoryCell(name, filename, buffer, ByteSize);
}

OEMMemoryCell::OEMMemoryCell(const char *_name, const char *filename, byte *buffer, uintf ByteSize)
{	heapsize = ByteSize;
	if (!buffer)
	{	memheap = fcalloc(ByteSize,_name);
		prealloced = false;
	}
	else
	{	memheap = buffer;
		prealloced = true;
	}
	memend = memheap + ByteSize;
	freeblock = (sMemBlock *)memheap;
	freeblock->verify1 = 0x12345678;
	freeblock->size = ByteSize - sizeof(sMemBlock);
	freeblock->next = NULL;
	freeblock->prev = NULL;
	usedblock = NULL;

	log = newlogfile(filename);
	log->log("Memory Log for %s",_name);
	numallocs = 0;
	maxmalloced = sizeof(sMemBlock);
	currentlyalloced = maxmalloced;
	// prealloced = true;
	txtcpy(name, sizeof(name),_name);
}

OEMMemoryCell::~OEMMemoryCell(void)
{	log->log("================================================================");
	sMemBlock *b = usedblock;
	while (b)
	{	log->log("Unfreed : %s : %i bytes (%x)",b->name, b->size, (byte *)b + sizeof(sMemBlock));
		b=b->next;
	}
	log->log("Maximum Memory Allocated at one time is %i kb",maxmalloced);
	delete log;
	if (!prealloced) fcfree(memheap);
}

void OEMMemoryCell::splitblock(sMemBlock *small, uintf size)
{	sMemBlock *freebit = (sMemBlock *)(((byte *)small) + size + sizeof(sMemBlock));
	if (small->prev)
		small->prev->next = freebit;
	else
		freeblock = freebit;
	freebit->prev = small->prev;
	freebit->next = small->next;
	if (freebit->next) freebit->next->prev = freebit;
	freebit->size = (small->size - sizeof(sMemBlock))-size;
	freebit->verify1 = 0x12345678;
	small->size = size;
}

byte* OEMMemoryCell::lowlevel_malloc(uintf size, const char *reason)
{	int i;

	// Align everything to 16 bytes
	size = (size + 15) & 0x7FFFFFF0;
	
	if (size==0) 
	{	size = 16;
	}

	if (!freeblock)
	{	log->log("Out of memory while allocating %i bytes for %s",size, reason);
		return NULL;
	}
	sMemBlock *small = freeblock;
	sMemBlock *m = small->next;

	while (m)
	{	// First, look for a container large enough to hold the block
		if ((small->size < size) && (m->size >= size))
		{	small = m;
		}	else
		// Now that we have a valid block, look for the smallest one
		if ((m->size >= size) && (small->size > m->size))
		{	small = m;
		}
		m = m->next;
	}

	// Check if we actually have enough memory
	if (small->size < size)
	{	log->log("Out of memory while allocating %i bytes for %s (Possibly due to fragmentation)",size, reason);
		delete log;
		memReport("logs/memoryCell_Failure.log");
		msg("MemoryCell full",buildstr("Memorycell %s is out of memory allocating %i bytes (%s)",name,size,reason));
		return NULL;
	}

	// Massage the reason to ensure it won't overflow the buffer
	char newreason[32];
	int reasonsize = txtlen(reason);
	if (reasonsize>31) reasonsize = 31;
	for (i=0; i<reasonsize; i++)
		newreason[i] = reason[i];
	newreason[reasonsize] = 0;
	
	if (small->size > (size + (uintf)sizeof(sMemBlock)))
	{	// 'small' now points to the smallest available block, we may need to split it into a used and unused section
		splitblock(small, size);
		if (usedblock) usedblock->prev = small;
		small->next = usedblock;
		small->prev = NULL;
		txtcpy(small->name, MemBlockNameLen, newreason);
		usedblock = small;
		currentlyalloced += small->size;
		if (currentlyalloced>maxmalloced) maxmalloced = currentlyalloced;
		return ((byte *)small) + sizeof(sMemBlock);
	}	else
	{	// This block is just large enough, but not big enough to be split, allocate the full size of this block to prevent
		// Orphaning the spare bytes, and creating a permanent blockage in the memory cell
		txtcpy(small->name, MemBlockNameLen, newreason);
		if (small->prev) small->prev->next = small->next; else freeblock = small->next;
		if (small->next) small->next->prev = small->prev;
		small->next = usedblock;
		if (usedblock) usedblock->prev = small;
		small->prev = NULL;
		usedblock = small;
		currentlyalloced += small->size;
		if (currentlyalloced>maxmalloced) maxmalloced = currentlyalloced;
		return ((byte *)small) + sizeof(sMemBlock);
	}
}

void *OEMMemoryCell::malloc(uintf size, const char *reason)
{	byte *b = lowlevel_malloc(size,reason);
	log->log("Alloc : %s : %i bytes (%x)",reason, size, b);
	memfill(b, 0, size);
	return (void *)b;
}

bool OEMMemoryCell::trytojoin(sMemBlock *b1, sMemBlock *b2)
{	if ((byte *)b2 == (byte *)b1 + b1->size + sizeof(sMemBlock))
	{	// Unlink B2
		if (b2->prev) b2->prev->next = b2->next; else freeblock = b2->next;
		if (b2->next) b2->next->prev = b2->prev;
		// Add B2's size to B1
		b1->size += b2->size + sizeof(sMemBlock);
		return true;
	} else return false;
}

void OEMMemoryCell::lowlevel_free(byte *d)
{	byte *ptr = NULL;
	if (!d) return;

	sMemBlock *b = usedblock;
	while (b)
	{	ptr = ((byte *)b) + sizeof(sMemBlock);
		if (d == ptr) break;
		b = b->next;
	}
	if (d != ptr)
	{	log->log("Warning : Cannot free memory at address %x as it was not allocated", d);
		if ((byte *)d < memheap)
		{	log->log("... It is also out of range of the managed memory area!  (Too low)");
		}
		if ((byte *)d > memend)
		{	log->log("... It is also out of range of the managed memory area!  (Too high)");
		}		
		
		return;
	}

	// Validate the block
	if (b->verify1 != 0x12345678)
	{	log->log("*** CRITICAL ERROR ... BUFFER OVERFLOW DETECTED AT ADDRESS %X ***",d);
	}

	currentlyalloced -= b->size;

	// Add block back into the free blocks
	if (freeblock) 
		freeblock->prev = b;
	if (b->next) b->next->prev = b->prev; 
	if (b->prev) b->prev->next = b->next; else usedblock = b->next;
	b->next = freeblock;
	b->prev = NULL;
	freeblock = b;

	log->log("Freed : %s : %i bytes (%x)",b->name, b->size, (byte *)b + sizeof(sMemBlock));

	sMemBlock *bj = freeblock;
	while (bj)
	{	if (trytojoin(bj,b)) {b=bj;break;}
		bj = bj->next;
	}

	bj = freeblock;
	while (bj)
	{	if (trytojoin(b,bj)) {break;}
		bj = bj->next;
	}
}

void* OEMMemoryCell::calloc(uintf nitems,uintf size, const char *reason)
{
	return this->malloc(size * nitems,reason);
}

void OEMMemoryCell::free(void *d)
{	lowlevel_free((byte *)d);
//	sanitycheck(2);
}

void *OEMMemoryCell::realloc(void *d, uintf size)
{	byte *ptr = NULL;

	if (!d) return this->malloc(size, "realloc(NULL)");

	sMemBlock *b = usedblock;
	while (b)
	{	ptr = ((byte *)b) + sizeof(sMemBlock);
		if (d == ptr) break;
		b = b->next;
	}
	if (d != ptr)
	{	log->log("Warning : Cannot reallocate memory at address %x as it was not allocated", d);
		if ((byte *)d < memheap)
		{	log->log("... It is also out of range of the managed memory area!  (Too low)");
		}
		if ((byte *)d > memend)
		{	log->log("... It is also out of range of the managed memory area!  (Too high)");
		}		
		return NULL;	
	}

	if (b->size > (uintf)size) return (void *)((byte *)b + sizeof(sMemBlock));
	byte *dst = (byte *)this->malloc(size, b->name);
	memcopy(dst, d, b->size);
	this->free(d);
	return dst;
/*
	// ### Improvement ### - We should probably try and link this block to the next block to prevent a data move
	sMemBlock *join = (sMemBlock*)((byte *)b + b->size + sizeof(sMemBlock));
	sMemBlock *j = freeblock;
	while (j)
	{	if (j==join) break;
		j = j->next;
	}

	if (j && (b->size + j->size + sizeof(sMemBlock) > size)) 
	{
	}	else
	{
	}
	return NULL;
*/
}

void OEMMemoryCell::memReport(const char *filename)
{	int i,numkb,printpos,numblocks,freemem;
	uintf largestfreeblock;
	bool corrupttables = false;

	logfile *reportlog = newlogfile(filename);
	numblocks = heapsize / 16;
	numkb = heapsize / 1024;
	char *memdata = (char*)fcalloc(numblocks,"memReport");
	memfill(memdata, '?', numblocks);
	largestfreeblock = 0;
	freemem = 0;
	sMemBlock *b = freeblock;
	while (b)
	{	int offset = (byte *)b - memheap;
		if (offset<0 || offset>(int)heapsize)
		{	corrupttables = true;
			break;
		}
		if (b->verify1 != 0x12345678)
		{	corrupttables = true;
			break;
		}
		freemem += b->size;
		if (b->size > largestfreeblock) largestfreeblock = b->size;

		offset >>= 4;
		for (i=0; i<(int)((sizeof(sMemBlock))>>4); i++)
		{	memdata[i+offset] = 'O';
		}
		offset += (sizeof(sMemBlock))>>4;
		for (i=0; i<(int)((b->size)>>4); i++)
		{	memdata[i+offset] = '.';
		}
		b=b->next;
	}

	b = usedblock;
	while (b)
	{	int offset = (byte *)b - memheap;
		if (offset<0 || offset>(int)heapsize)
		{	corrupttables = true;
			break;
		}
		if (b->verify1 != 0x12345678)
		{	corrupttables = true;
			break;
		}
		offset >>= 4;
		for (i=0; i<(int)((sizeof(sMemBlock))>>4); i++)
		{	memdata[i+offset] = 'O';
		}
		offset += (sizeof(sMemBlock))>>4;
		for (i=0; i<(int)((b->size)>>4); i++)
		{	memdata[i+offset] = '#';
		}
		b=b->next;
	}

	reportlog->log("  Memory Report");
	reportlog->log("------------------");
	reportlog->log("         Heap Size : %i kb", numkb);
	reportlog->log("       Free Memory : %i kb", freemem / 1024);
	reportlog->log("Largest Free Block : %i kb", largestfreeblock/1024);
	reportlog->log("      Maximum used : %i kb", maxmalloced/1024);
	if (corrupttables) {
	reportlog->log("Corruption Detected: YES!");}
	else {
	reportlog->log("Corruption Detected: No");}
	reportlog->log("");
	
	reportlog->log("Key: # = allocated, O = Header, . = Free, ? = Corrupt or Orphaned");
	reportlog->log("+----------------------------------------------------------------+");
	printpos = 1;
	char buf[256];
	buf[0] = '|';
	for (i=0; i<numblocks; i++)
	{	buf[printpos++] = memdata[i];
		if (printpos==65) 
		{	buf[printpos++] = '|';
			buf[printpos++] = '\r';
			buf[printpos++] = '\n';
			buf[printpos++] = 0;
			reportlog->log(buf);
			printpos = 1;
		}
	}
	if (printpos > 1)
	{	while (printpos <65) buf[printpos++] = ' ';
		buf[printpos++] = 0;
		buf[printpos++] = '|';
		reportlog->log(buf);
	}
	reportlog->log("+----------------------------------------------------------------+");
	reportlog->log("");
	reportlog->log("Description of Allocations:");
	reportlog->log("---------------------------");
	b = usedblock;
	while (b)
	{	int offset = (byte *)b - memheap;
		if (offset<0 || offset>(int)heapsize)
		{	break;
		}
		if (b->verify1 != 0x12345678)
		{	break;
		}
		byte *ptr = (byte *)b + sizeof(sMemBlock);
//		reportlog->log("%08X : %s (%i bytes)",ptr-memheap, b->name, b->size);
		reportlog->log("%08X : ",ptr-memheap);
		reportlog->log("%s",b->name);
		reportlog->log("(%i bytes)",b->size);
		b=b->next;
	}
	if (b) {
	reportlog->log("*** Corruption in tables prevents further analysis ***");
	}
	delete reportlog;
	fcfree(memdata);
}

// ****************************************************************************************
// ***								Utility Functions									***
// ****************************************************************************************

// ------------------------ synctest API Command --------------------------------
void synctest(const char *funcname,byte *array, long asize, byte *ptr)
{	// When allocating several memory blocks with only one alloc, use this function to
	// verify that the destination pointer is at the end of the block
	if (array+asize!=ptr)
		msg("Data Synchronisation Error",buildstr("Function: %s\nArray (%x) + Size (%x) = %x\nBut I got %x (difference of %x)",funcname,array,asize,array+asize,ptr,abs((array+asize)-ptr)));
}

// -------------------- allocarrayelement API Command ----------------------------
long allocarrayelement(bool *array,long maxelements)
{	// Given an array of bools, locate one that's false.  The idea of this is that another array
	// with the same number of elements should exist where a BOOL entry of FALSE means the
	// entry is empty.  This way, applications can avoid memory allocations, which fragment memory
	// A return value of -1 means a failure (no empty cells are available)
	for (long i=0; i<maxelements; i++)
		if (!array[i]) 
		{	array[i]=true;
			return i;
		}
	return -1;
}

