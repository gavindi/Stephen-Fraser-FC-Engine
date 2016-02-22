/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
// ****************************************************************************
// ***						Text Handling Functions 						***
// ****************************************************************************
#include <stdarg.h>
#include <stdlib.h>
#include "fc_h.h"
#include "bucketSystem.h"

char	end_of_line[]="\r\n";
char	stringreason[]="String cache";
char	builtstr[8192];

class fctext : public fc_text
{	public:
		logfile *newlogfile(const char *filename)					{return ::newlogfile(filename);};
		console *newconsole(long maxlines, long height)				{return ::newconsole(maxlines,height);};
		textfile *newtextfile(const char *flname)					{return ::newtextfile(flname);};
		textfile *newtextfile(const char *buffer,long size,bool _freemem) {return ::newtextfile(buffer,size,_freemem);};
		char *_cdecl buildstr(const char *str, ...);
		void skipblanks(char *data, long *ofs)						{		::skipblanks(data,ofs);};
		void stripquotes(char *txt)									{		::stripquotes(txt);};
		char *geteoltoken(char *data, long *ofs, char *sep)			{return ::geteoltoken(data, ofs, sep);};
		char *gettoken(char *data, long *ofs, char *sep)			{return ::gettoken(data, ofs, sep);};
		float getfloattoken(char *data, long *ofs, char *sep)		{return ::getfloattoken(data,ofs,sep);};
		uintf getuinttoken(char *data, long *ofs, char *sep)		{return ::getuinttoken(data,ofs,sep);};
} *OEMtext;

void killtext(void)
{	delete OEMtext;
}

// ****************************************************************************
// ***							String.H Replacements						***
// ****************************************************************************
void txtcpy(char *dest, uintf maxlength, const char *src)
{	// Copy a string from src to dest, up to a maximum of 'maxlength' characters.  This is to avoid potential
	// buffer overflow vulnerabilities ... and of cause to prevent security shutdown on Athlon 64 CPUs
	maxlength--;	// Leave room for terminating 0 character
	dword charcount = 0;
	while ((*src!=0) && (charcount<maxlength))
	{	*dest++ = *src++;
		charcount++;
	}
	*dest = 0;
}

void txtcat(char *dest, uintf maxlength, const char *src)
{	uintf curlen = txtlen(dest);
	if (curlen < maxlength)
	{	maxlength -= curlen;
		txtcpy(dest+curlen,maxlength, src);
	}
}

uintf txtlen(const char *text)
{	if (!text) msg(GenericError,"Call to 'txtlen' with NULL string");
	uintf len = 0;
 	while (text[len]) len++;
	return len;
}

intf txtcmp(const char *txt1, const char *txt2)
{	if ((!txt1) || (!txt2)) msg(GenericError,"Call to 'txtcmp' with a NULL string");
	while (1)
	{	if (*txt1 == 0 && *txt2 == 0) return 0;
		char dif=*txt1-*txt2;
		if (dif) return dif;
		txt1++;
		txt2++;
	}
}

intf txtncmp(const char *txt1, const char *txt2,uintf len)
{	if ((!txt1) || (!txt2)) msg(GenericError,"Call to 'txtcmp' with a NULL string");
	while (len>0)
	{	if (*txt1 == 0 && *txt2 == 0) return 0;
		char dif=*txt1-*txt2;
		if (dif) return dif;
		txt1++;
		txt2++;
		len--;
	}
	return 0;
}

intf txtnicmp(const char *txt1, const char *txt2,uintf len)
{	if ((!txt1) || (!txt2)) msg(GenericError,"Call to 'txtnicmp' with a NULL string");
	while (len>0)
	{	if (*txt1 == 0 && *txt2 == 0) return 0;
		char dif=toupper_array[(intf)*txt1]-toupper_array[(intf)*txt2];
		if (dif) return dif;
		txt1++;
		txt2++;
		len--;
	}
	return 0;
}

intf txticmp(const char *txt1, const char *txt2)
{	if ((!txt1) || (!txt2)) msg(GenericError,"Call to 'txtcmp' with a NULL string");
	while (1)
	{	if (*txt1 == 0 && *txt2 == 0) return 0;
		char dif=toupper_array[(intf)*txt1]-toupper_array[(intf)*txt2];
		if (dif) return dif;
		txt1++;
		txt2++;
	}
}

bool editText(sEditText *edit, uint8 kg)
{	bool changed = false;
	char *text = edit->text;
	intf tl = (intf)txtlen(text);
	intf i;

	if (kg>31 && kg<128)
	{	// Insert character at this position
		if (tl>0)
		{	for (i=tl; i>=(intf)edit->cursor && i>=0; i--)
				text[i+1]=text[i];
			text[edit->cursor++]=kg;
		}	else
		{	text[0]=kg;
			text[1]=0;
			edit->cursor=1;
		}
		changed = true;
	}	else
	{	if (kg==key_LEFT  && edit->cursor>0) edit->cursor--;	// Left
		if (kg==key_RIGHT && (intf)edit->cursor<tl) edit->cursor++;	// Right
		if (kg==key_HOME) edit->cursor=0;						// Home
		if (kg==key_END) edit->cursor=tl;						// End
		if (kg==key_DELETE && (intf)edit->cursor<tl)			// DELETE
		{	for (i=edit->cursor; i<=tl; i++)
				text[i]=text[i+1];
			changed = true;
		}
		if (kg==key_BACKSPACE && edit->cursor>0)					// BACKSPACE
		{	edit->cursor--;
			for (i=edit->cursor; i<=tl; i++)
				text[i]=text[i+1];
			changed = true;
		}
	}
	return changed;
}

// ****************************************************************************
// ***							LogFile Class								***
// ****************************************************************************
class logoem : public logfile
{	private:
		sFileHandle *handle;
	public:
		logoem(const char *filename);				// Create a log file
		~logoem(void);						// Close the file
		void _cdecl log(const char *txt, ...);	// Wrte an entry to the log file (use sprintf style text formatting)
};

logfile *newlogfile(const char *filename)
{	return new logoem(filename);
}

logoem::logoem(const char *filename)
{	if (filename)
		handle = fileCreate(filename);
	else
		handle = 0;
}

logoem::~logoem(void)
{	if (handle)
		fileClose(handle);
}

void _cdecl logoem::log(const char *txt,...)
{	char buf[4096];
	if (handle)
	{	constructstring(buf, txt, sizeof(buf));
		fileWrite(handle,buf,txtlen(buf));
		fileWrite(handle,end_of_line,2);
	}
}

// ****************************************************************************
// ***							Console Class								***
// ****************************************************************************
#define conlinelen 1024

class conoem : public console
{	private:
		long maxlines,numlines,height,x,y,lastlinestay;
		char *lines;
		dword flags;
		sFileHandle *handle;
	public:
		conoem(long maxlines, long height);
		~conoem(void);
		void cls(void);
		void setpos(long _x, long _y);
		void logfile(const char *filename);
		void closelog(void);
		void _cdecl add(const char *str, ...);
		void draw();
		void setflags(dword flags);
		void clearflags(dword flags);
};

conoem::conoem(long _maxlines, long _height)
{	height=_height;
	if (_maxlines>100)
		_maxlines=100;
	maxlines=_maxlines;
	lines = (char *)fcalloc(maxlines*conlinelen,"Text Console");
	cls();
	x		= 0;
	y		= 0;
	handle	= NULL;
	flags	= 0;
	lastlinestay=0;
}

console *newconsole(long maxlines, long height)
{	return new conoem(maxlines,height);
}

conoem::~conoem(void)
{	if (handle)
		fileClose(handle);
	fcfree(lines);
}

void conoem::cls()
{	long count;
	for (count=0; count<maxlines; count++)
		lines[count<<8]=0;
	numlines=0;
}

void conoem::setpos(long _x, long _y)
{	x = _x;
	y = _y;
}

void conoem::logfile(const char *filename)
{	if (filename)
		handle = fileCreate(filename);
}

void conoem::closelog(void)
{	if (handle)
		fileClose(handle);
	handle = NULL;
}

void _cdecl conoem::add(const char *str, ...)
{	char buf[conlinelen];
	constructstring(buf, str, sizeof(buf));

	if (numlines==maxlines)
	{	for (int count=0; count<numlines; count++)
			txtcpy(&lines[count<<8],conlinelen,&lines[(count+1)<<8]);
		numlines--;
	}
	if (lastlinestay) numlines--;
	txtcpy(&lines[numlines<<8],conlinelen,buf);
	numlines++;
	if (handle)
	{	fileWrite(handle,buf,txtlen(buf));
		fileWrite(handle,end_of_line,2);
	}
	if (flags & conflag_stayonline) lastlinestay=1;
							   else lastlinestay=0;
}

void conoem::setflags(dword _flags)
{	flags |= _flags;
}

void conoem::clearflags(dword _flags)
{	flags &= ~_flags;
}

void conoem::draw()
{	long count;

	for (count=0; count<numlines; count++)
		textout(x,y+count*height,&lines[count<<8]);
}

// ****************************************************************************
// ***							Textfile system								***
// ****************************************************************************
class textfileoem : public textfile
{	private:
		char *memory;
		intf tflsize,memptr;
		intf nextLine;
		bool freemem;
		char tline[2048];
	public:
		textfileoem(const char *flname);
		textfileoem(byte *buffer,long size,bool _freemem);
		~textfileoem(void);
		char *fixtline(void);		// Used internally to fix the tline return variable when it overflows
		char *readln(void);
		char *readlnSingle(void);
		char *peekln(void);
		void reset(void);
};

textfileoem::textfileoem(const char *flname)
{	memory = (char *)fileLoad(flname);
	tflsize = filesize;
	memptr = 0;
	nextLine = 1;
	freemem = true;
	txtcpy(filename,sizeof(filename),flname);
}

textfile *newtextfile(const char *flname)
{	return new textfileoem(flname);
}

textfileoem::textfileoem(byte *buffer,long size,bool _freemem)
{	memory = (char *)buffer;
	tflsize =size;
	memptr = 0;
	nextLine = 1;
	freemem = _freemem;
	tprintf(filename,sizeof(filename),"From Memory %x",buffer);
}

textfile *newtextfile(const char *buffer,long size,bool _freemem)
{	return new textfileoem((byte *)buffer,size,_freemem);
};

char *textfileoem::fixtline(void)
{	tline[sizeof(tline)-1] = 0;
	return tline;
}

char *textfileoem::readln(void)
{	if (memptr>=tflsize) return NULL;
	linenum=nextLine;
	uintf i=0;
	char c;
	c = memory[memptr++];
	while (c!=10 && c!=13 && memptr<=tflsize)
	{	if (c=='"')
		{	// Handle strings that pass multiple lines
			tline[i++]=c;								// Write the opening double-quote to tline
			if (i>=sizeof(tline)) return fixtline();	// Line is too long to process!  Bail out
			while ((c=memory[memptr++])!='"' && memptr<=tflsize)
			{	if (c=='\n') nextLine++;
				tline[i++]=c;
				if (i>=sizeof(tline)) fixtline();		// Line is too long to process!  Bail out
			}
		}
		tline[i++]=c;
		if (i>=sizeof(tline)) fixtline();				// Line is too long to process!  Bail out

		c = memory[memptr++];
	}
	tline[i]=0;
	memptr--;
	bool loop = true;
	while (loop)		// This line causes the readln to skip blank lines
	{	loop = false;
		if (memory[memptr]==13 && memptr<tflsize) { memptr++; loop = true; nextLine++;}
		if (memptr>=tflsize) break;
		if (memory[memptr]==10 && memptr<tflsize) { memptr++; loop = true; }
		if (memptr>=tflsize) break;
	}
	return tline;
}

char *textfileoem::readlnSingle(void)
{	linenum++;
	if (memptr>=tflsize) return NULL;

	uintf i=0;
	while (memory[memptr]!=10 && memory[memptr]!=13 && memptr<tflsize && i<sizeof(tline))
	{	tline[i++]=memory[memptr++];
	}
	tline[i]=0;

	bool loop = true;
	while (loop)		// This line causes the readln to skip blank lines
	{	loop = false;
		if (memory[memptr]==13 && memptr<tflsize) { memptr++; loop = true; }
		if (memptr>=tflsize) break;
		if (memory[memptr]==10 && memptr<tflsize) { memptr++; loop = true; }
		if (memptr>=tflsize) break;
	}
	return tline;
}

char *textfileoem::peekln(void)
{	intf currentLineNum = linenum;
	intf currentMemPtr = memptr;
	char *result = readlnSingle();
	linenum=currentLineNum;
	memptr=currentMemPtr;
	return result;
}

void textfileoem::reset(void)
{	memptr = 0;
	linenum = 0;
}

textfileoem::~textfileoem(void)
{	if (freemem) fcfree(memory);
}

// ****************************************************************************
// ***								Text Parser								***
// ****************************************************************************
char token_[1024];
void skipblanks(const char *data, long *ofs)
{	long slen = txtlen(data)-1;
	long x = *ofs;
	while ((data[x]==32 || data[x]==9) && x<slen) x++;
	*ofs=x;
}

void stripquotes(char *txt)
{	if (txt[0]!='"') return;
	uintf index = 0;
	uintf nextquote = 0xffffffff;
	while (txt[index])
	{	txt[index]=txt[index+1];
		if (txt[index]=='"' && nextquote==0xffffffff) nextquote = index;
		index++;
	}
	if (nextquote!=0xffffffff)
	{	index = nextquote;
		while (txt[index])
		{	txt[index]=txt[index+1];
			index++;
		}
	}
}

char *geteoltoken(char *data, long *ofs, char *sep)
{	skipblanks(data,ofs);
	long x = *ofs;
	long tofs=0;
	while (	data[x]!=13  && data[x]!=10  && data[x]!=0)
	{	token_[tofs]=data[x];
		tofs++;
		x++;
	}
	token_[tofs]=0;
	*ofs=x+1;
	return (char *)&token_;
}

char *gettoken(char *data, long *ofs, char *sep)	// ### looks like it could be uintf instead, should be merged into gettokencustom
{	skipblanks(data,ofs);
	long x = *ofs;
	long tofs=0;

	if (data[x]=='"')
	{	token_[tofs++]=data[x++];
		while (data[x]!='"')
		{	token_[tofs++]=data[x++];
		}
	}

	while (	data[x]!=',' && data[x]!=' ' && data[x]!='.' &&
			data[x]!='(' && data[x]!=')' && data[x]!='=' &&
			data[x]!='[' && data[x]!=']' && data[x]!='+' &&
			data[x]!='*' && data[x]!='/' && data[x]!='-' &&
			data[x]!=13  && data[x]!=10  && data[x]!=0 &&
			data[x]!=9)
	{	token_[tofs]=data[x];
		tofs++;
		x++;
	}
	*sep = data[x];
	token_[tofs]=0;
	if (*sep!=0)
		*ofs=x+1;
	else
		*ofs=x;
	return (char *)&token_;
}

char *gettokencustom(const char *data, long *ofs, char *sep, const char *breakers)
{	skipblanks(data,ofs);
	uintf x = (uintf)*ofs;
	uintf tofs=0;
	while (true)
	{	bool breaking = false;
		for (uintf i=0; breakers[i]!=0; i++)
			if (data[x]==breakers[i])
			{	breaking = true;
				break;
			}
		if (breaking) break;
		if (data[x]==0) break;
		if (tofs>=sizeof(token_)-1) break;
		token_[tofs]=data[x];
		tofs++;
		x++;
	}
	*sep = data[x];
	token_[tofs]=0;
	*ofs=x+1;
	return (char *)&token_;
}

/*
char *getStringToken(char *data, long *ofs, char *sep)
{	char *token = gettoken(data,ofs,sep);
	if (token[0]=='"')
	{	uintf len = txtlen(token);
		uintf i;
		for (i=1; i<len; i++)
			token[i=1]=token[i];
		token[i]=0;
	}
	return token;

/ *	long x = *ofs;
	while (data[x]==' ') x++;

	if (data[x]!='"' && data[x]!='\'')
	{	token_[0]=0;
		return (char *)&token_;
	}
	char termchar = data[x];
	x++;
	long tofs=0;
	while (data[x]!=termchar)
	{	char c = data[x++];
		if (!c)
		{	data = readln();
			x=0;
		}	else
			token_[tofs++]=c;
	}

	*sep = data[x];
	token_[tofs]=0;
	*ofs = x+1;
	return (char *)&token_;
}
*/

float getfloattoken(char *data, long *ofs, char *sep)
{	skipblanks(data,ofs);
	long x = *ofs;
	long tofs=0;
	while (	data[x]!=',' && data[x]!=' ' &&
			data[x]!='(' && data[x]!=')' && data[x]!='=' &&
			data[x]!='[' && data[x]!=']' && data[x]!='+' &&
			data[x]!='*' && data[x]!='/' &&
			data[x]!=13  && data[x]!=10  && data[x]!=0)
	{	token_[tofs]=data[x];
		tofs++;
		x++;
	}
	*sep = data[x];
	token_[tofs]=0;
	*ofs=x+1;
	return (float)atof(token_);
}

uintf getuinttoken(char *data, long *ofs, char *sep)
{	char *x = gettoken(data,ofs,sep);
	return atoi(x);
}

// ****************************************************************************
// ***								String Handler							***
// ****************************************************************************
const uintf CharsInString = ((REGSIZE * 2 - (ptrsize * 3)) - 2);

struct sString
{	char	data[CharsInString];	// Contains the actual text (must be even sized, to allow for unicode support)
	byte	spare1;					// This byte is unused (to keep data an even length
	byte	flags;					// Flags needed for memory management
	sString	*link;					// Link to the next string (in case it doesn't all fit into this string)
	sString	*prev,*next;			// Memory management links
} *freesString, *usedsString;

sString *allocsString(void)
{	sString *tmp;
	allocbucket(sString, tmp, flags, Generic_MemoryFlag8, 2048, stringreason);
	return tmp;
}

class oemString : public cString
{	private:
		uintf		oemlength;
		uintf		charsInLastBlock;
		sString		*head, *tail;
	public:
				oemString(void);
				~oemString(void);
		void	toChars(char *buffer, uintf size);	// Convert string to a char array, limited to [size] bytes
		cString	*copy(void);						// Copy this string, and return a pointer to the copy.
		cString *left(uintf size);					// Copy the leftmost [size] characters into another string
		cString *right(uintf size);					// Copy the rightmost [size] characters into another string
		cString *mid(uintf start, uintf size);		// Copy the middle [size] characters (starting at [start]) into another string
		uintf	length(void);						// Returns the length of the string
		intf	compare(cString *other);			// Returns 0 if strings match, <0 if other<this, >0 if other>this
		intf	compare(const char *other);				// Returns 0 if strings match, <0 if other<this, >0 if other>this
		intf	icompare(cString *other);			// Same as 'compare' but case insensitive
		intf	icompare(const char *other);				// Same as 'compare' but case insensitive
		void	append(cString *addition);			// Append the [addition] string to the end of this string
		void	append(const char *addition);				// Append the [addition] string to the end of this string
		intf	search(cString *searchstring, uintf start);	// Search the string from position [start] for [searchstring], if found, returns the index of the first occurance of the searchstring, or -1 if it's not found.
		intf	search(const char *searchstring, uintf start);	// Search the string from position [start] for [searchstring], if found, returns the index of the first occurance of the searchstring, or -1 if it's not found.
		intf	writeToFile(int handle);			// Write the string to a file.  The file must be open for writing.  No End-Of-Line characters are written after the string.
};

oemString::oemString(void)
{	oemlength = 0;
	charsInLastBlock = 0;
	head = allocsString();
	head->link = NULL;
	tail = head;
}

oemString::~oemString(void)
{	sString *s = head;
	while (s)
	{	sString *l = s->link;
		deletebucket(sString, s);
		s = l;
	}
}

void	oemString::toChars(char *buffer, uintf size)
{	// Convert string to a char array, limited to [size] bytes
	if (size<1) return;
	uintf charstocopy = size - 1;
	if (charstocopy>oemlength) charstocopy=oemlength;
	sString *s = head;
	while (charstocopy>CharsInString)
	{	memcopy(buffer, s->data, CharsInString);
		buffer += CharsInString;
		charstocopy -= CharsInString;
		s = s->link;
	}
	if (charstocopy>0)
	{	memcopy(buffer, s->data, charstocopy);
		buffer += charstocopy;
	}
	*buffer = 0;
}

cString	*oemString::copy(void)
{	// Copy this string, and return a pointer to the copy.
	oemString *result = (oemString *)newString(NULL);
	sString *dst = result->head;
	sString *src = head;
	uintf size = oemlength;
	while (size>=CharsInString)
	{	memcopy(dst->data, src->data, CharsInString);
		sString *tmp = allocsString();
		dst->link = tmp;
		dst = tmp;
		src = src->link;
		size -= CharsInString;
	}
	if (size>0)
	{	memcopy(dst->data, src->data, size);
	}
	result->oemlength = oemlength;
	result->tail = dst;
	result->charsInLastBlock = charsInLastBlock;
	return result;
}

cString *oemString::left(uintf size)
{	// Copy the leftmost [size] characters into another string
	oemString *result = (oemString *)newString(NULL);
	sString *dst = result->head;
	sString *src = head;
	if (size>oemlength) size = oemlength;
	while (size>=CharsInString)
	{	memcopy(dst->data, src->data, CharsInString);
		sString *tmp = allocsString();
		dst->link = tmp;
		dst = tmp;
		result->tail = dst;
		src = src->link;
		size -= CharsInString;
		result->oemlength += CharsInString;
	}
	if (size>0)
	{	memcopy(dst->data, src->data, size);
		result->oemlength += size;
		result->charsInLastBlock = size;
	}
	return result;
}

cString *oemString::right(uintf size)
{	// Copy the rightmost [size] characters into another string
	if (size>oemlength) size = oemlength;
	return mid(oemlength-size, size);
}

cString *oemString::mid(uintf start, uintf size)
{	// Copy the middle [size] characters (starting at [start]) into another string
	oemString *result = (oemString *)newString(NULL);
	if (start > oemlength) return result;
	if (start+size > oemlength) size = oemlength-start;
	result->oemlength = size;
	sString *dst = result->head;
	sString *src = head;
	while (start >= CharsInString)
	{	src = src->link;
		start -= CharsInString;
	}

	// Optimise opportunity ...
	// If start=0, do a block copy operation
	// else, a while loop, with 2 inner loops ...
	// loop1: while bytesinSRC < blockend
	// loop2: while bytesinDST < blockend

	uintf writepos = 0;

	while (size>0)
	{	dst->data[writepos++] = src->data[start++];
		if (start>=CharsInString)
		{	start = 0;
			src = src->link;
		}
		if (writepos>=CharsInString)
		{	writepos = 0;
			dst->link = allocsString();
			dst = dst->link;
			result->tail = dst;
		}
		size--;
	}
	result->charsInLastBlock = writepos;
	return result;
}

uintf	oemString::length(void)
{	// Returns the length of the string
	return oemlength;
}

intf	oemString::compare(cString *other)
{	// Returns 0 if strings match, <0 if other<this, >0 if other>this
	msg("cString Class","Incomplete Function");	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	return 0;
}

intf	oemString::compare(const char *other)
{	// Returns 0 if strings match, <0 if other<this, >0 if other>this
	msg("cString Class","Incomplete Function");	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	return 0;
}

intf	oemString::icompare(cString *other)
{	// Same as 'compare' but case insensitive
	msg("cString Class","Incomplete Function");	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	return 0;
}

intf	oemString::icompare(const char *other)
{	// Same as 'compare' but case insensitive
	msg("cString Class","Incomplete Function");	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	return 0;
}

void	oemString::append(cString *addition)
{	// Append the [addition] string to the end of this string
	msg("cString Class","Incomplete Function");	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}

void	oemString::append(const char *addition)
{	// Append the [addition] string to the end of this string
	sString *tmp;
	uintf charsremain = txtlen(addition);
	oemlength += charsremain;
	uintf blockremain = CharsInString - charsInLastBlock;
	while (charsremain >= blockremain)
	{	memcopy((void *)&tail->data[charsInLastBlock], (void *)addition, blockremain);
		addition += blockremain;
		charsremain -= blockremain;
		blockremain = CharsInString;
		charsInLastBlock = 0;
		tmp = allocsString();
		tail->link = tmp;
		tail = tmp;
	}
	if (charsremain)
	{	memcopy((void *)&tail->data[charsInLastBlock], (void *)addition, charsremain);
		charsInLastBlock += charsremain;
	}
}

intf	oemString::search(cString *searchstring, uintf start)
{	// Search the string from position [start] for [searchstring], if found, returns the index of the first occurance of the searchstring, or -1 if it's not found.
	msg("cString Class","Incomplete Function");	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	return 0;
}

intf	oemString::search(const char *searchstring, uintf start)
{	// Search the string from position [start] for [searchstring], if found, returns the index of the first occurance of the searchstring, or -1 if it's not found.
	msg("cString Class","Incomplete Function");	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	return 0;
}

intf	oemString::writeToFile(int handle)
{	// Write the string to a file.  The file must be open for writing.  No End-Of-Line characters are written after the string.
	msg("cString Class","Incomplete Function");	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	return 0;
}

cString _cdecl *newString(const char *text,...)
{	char		buffer[2048];
	oemString	*result;

	result = new oemString();

	if (text)
	{	constructstring(buffer, text, sizeof(buffer));
		result->append(buffer);
	}
	return result;
}

// ----------------------------------------------------------------------------

intf _cdecl tprintf(char *dest, uintf size, const char *txt, ...)
{	constructstring(dest, txt, size);
	return txtlen(dest);
}


void txtcatf(char *dest, uintf size, const char *txt, ...)
{	uintf curlen = txtlen(dest);
	if (curlen < size)
	{	size -= curlen;
		constructstring(dest+curlen,txt,size);
	}
}

char *_cdecl buildstr(const char *txt,...)
{	constructstring(builtstr,txt,sizeof(builtstr));
	return builtstr;
}

char *_cdecl fctext::buildstr(const char *txt,...)
{	constructstring(builtstr,txt,sizeof(builtstr));
	return builtstr;
}

bool fetchVersionMajorMinorFromString(const char *str, int *major, int *minor)
{	// Scans the input string 'str' to find a version number, eg : 'OpenGL ES 2.0 by NVidia Corporation' will return 2 in major, and 0 in minor
	uintf i;

	*major=0;
	*minor=0;

	// Find the first numeric character in str
	if (!str) return false;
	for (i=0; str[i]!=0; i++)
	{	if (str[i]>='0' && str[i]<='9') break;
	}
	if (str[i]==0) return false;
	// Now look for a period.  If we don't find one, result is still valid but will be in the form 'x.0'
	for (; str[i]!=0; i++)
	{	if (str[i]<'0' || str[i]>'9') break;
		*major = *major*10 + (int)(str[i]-'0');
	}
	if (str[i]!='.')
		return true;
	// Now look for any non-numeric value
	for (; str[i]!=0; i++)
	{	if (str[i]<'0' || str[i]>'9') break;
		*minor = *minor*10 + (int)(str[i]-'0');
	}
	return true;
}

struct sStringFormat
{	char flags;
	intf width;
	intf precision;
	byte size;
	char type;
	uintf formatSize;
	char padding;
};

const char *validVStringCodes="-= #~123456789~.~lhILw~0~cCdiouxXeEfgGaAnpsS";

bool getVStringFormat(const char *buffer, sStringFormat *sf)
{	uintf i;
	bool success = false;
	while (!success)
	{	sf->type = buffer[sf->formatSize++];
		if (sf->type==0) return false;			// unexpected end of string
		uintf block=0;
		for	(i=0; validVStringCodes[i]!=0; i++)
		{	if (validVStringCodes[i]=='~') block++;
			else
			{	if (sf->type==validVStringCodes[i]) break;
			}
		}
		if (validVStringCodes[i]==0) return false;	// Invalid format character

		switch(block)
		{	case 0:		// flags ( -= # )
				sf->flags = sf->type;
				break;
			case 1:		// width ( 123456789 ) followup characters can also be 0
				sf->width = sf->type-'0';
				while ((buffer[sf->formatSize] >= '0') && (buffer[sf->formatSize] <= '9'))
					sf->width = sf->width * 10 + buffer[sf->formatSize++]-'0';
				break;
			case 2:		// precision ( . ) followed by one or more 0-9 digits
				sf->precision = 0;
				while ((buffer[sf->formatSize] >= '0') && (buffer[sf->formatSize] <= '9'))
					sf->precision = sf->precision * 10 + buffer[sf->formatSize++]-'0';
				break;
			case 3:		// size specifier ( lhILw )
				if (sf->type=='l')	// long
				{	if (buffer[sf->formatSize]!='l')
					{	sf->size = 4;					// Size = 4: long int	(intf)
					}	else
					{	sf->size = 6;					// Size = 6: long long	(int64)
						sf->formatSize++;
					}
				} else

				if (sf->type=='h')
				{	sf->size = 1;						// Size = 1: short int	(int16)
				}	else

				if (sf->type=='I')
				{	if ((buffer[sf->formatSize]=='3') && (buffer[sf->formatSize+1]!='2'))
					{	sf->size = 2;					// Size = 2: int32		(int32)
						sf->formatSize+=2;
					}	else
					if ((buffer[sf->formatSize]=='6') && (buffer[sf->formatSize+1]!='4'))
					{	sf->size = 3;					// Size = 3: int64		(int64)
						sf->formatSize+=2;
					}	else
					{	sf->size = 5;					// Size = 5: size_t		(intf)
					}
				}	else
				if (sf->type=='L')
				{	sf->size = 7;						// Size = 7: long double (long double)
				}	else
				if (sf->type=='h')
				{	sf->size = 8;						// Size = 8: char		(char)
				}	else
				if (sf->type=='w')
				{	sf->size = 9;						// Size = 9: Wide Char	(char)
				}
				break;
			case 4:										// Padding Character ( 0 )
				sf->padding = sf->type;
				break;
			case 5:
				success = true;
		}
	}
	return true;
}

char vstringHexLC[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
char vstringHexUC[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void myvstrintf(char *buffer, uintf size, const char *format, va_list argptr)
{	sStringFormat sf;							// ### FIXME: This code is incorrectly using precision where it should be using width, and then padding with 0's where it should pad with spaces in some areas.  See function getVStringFormat for formatting assistance
	char	*type_s;
	char	type_c;
	intf	type_i, digit_i;
	float	type_f;

	intf	digitsRemain;
	uintf	bufsize = 0;
	uintf	i=0;
	byte	digit;
	bool	pad0;
	char	*hexChars;

	size--;										// Leave room for terminating NULL character
	while (format[i]!=0)
	{	if (format[i]!='%')
		{	buffer[bufsize++]=format[i];
		}	else
		{	sf.flags = 0;
			sf.width = 1;
			sf.precision = 1;
			sf.size = 0;
			sf.formatSize = 0;
			sf.padding = ' ';
			getVStringFormat(format+i+1,&sf);
			pad0 = false;
			i+=sf.formatSize;
			switch(sf.type)
			{	case '%':
					buffer[bufsize++]='%';
					break;
				case 'x':
				case 'X':
					if (sf.type=='x')
						hexChars = (char *)vstringHexLC;
					else
						hexChars = (char *)vstringHexUC;
					type_i = va_arg(argptr, int);
					if (type_i<0)
					{	buffer[bufsize++]='-';
						if (bufsize==size) break;
						type_i = -type_i;
					}
					digit_i = 0x10000000;
					digitsRemain = 8;

					while (digitsRemain)
					{	digit = 0;
						while (type_i >= digit_i)
						{	pad0=true;
							digit++;
							type_i -= digit_i;
						}
						if ((digit>0) || (digitsRemain <=sf.width)) pad0 = true;
						if (pad0) buffer[bufsize++]=hexChars[digit];
						if (bufsize==size) break;
						digit_i >>= 4;
						digitsRemain--;
					}
					break;

				case 'i':
					type_i = va_arg(argptr, int);
					if (type_i<0)
					{	buffer[bufsize++]='-';
						if (bufsize==size) break;
						type_i = -type_i;
					}
					digit_i = 1000000000;
					digitsRemain = 10;

					while (digitsRemain)
					{	digit = 0;
						while (type_i >= digit_i)
						{	pad0=true;
							digit++;
							type_i -= digit_i;
						}
						if ((digit>0) || (digitsRemain <=sf.width)) pad0 = true;
						if (pad0) buffer[bufsize++]='0'+digit;
						if (bufsize==size) break;
						digit_i /= 10;
						digitsRemain--;
					}
					break;

				case 'f':									// ### This code is a cheap hack - it does not take width or precision into account
					type_f = (float)va_arg(argptr, double);
					type_i = (int)(type_f * 100.0f);
					if (type_i<0)
					{	buffer[bufsize++]='-';
						if (bufsize==size) break;
						type_i = -type_i;
					}
					digit_i = 1000000000;
					digitsRemain = 10;

					while (digitsRemain)
					{	digit = 0;
						if (digitsRemain==3) pad0=true;		// We need to make sure we get at least 0.00
						if (digitsRemain==2)
						{	buffer[bufsize++]='.';
							if (bufsize==size) break;
						}
						while (type_i >= digit_i)
						{	pad0=true;
							digit++;
							type_i -= digit_i;
						}
						if ((digit>0) || (digitsRemain <=sf.precision)) pad0 = true;
						if (pad0)
						{	buffer[bufsize++]='0'+digit;
							if (bufsize==size) break;
						}
						digit_i /= 10;
						digitsRemain--;
					}
					break;

				case 's':
					type_s = va_arg(argptr, char*);
					if (sf.precision==1) sf.precision = txtlen(type_s);
					while((bufsize<size) && (sf.precision>0))
					{	buffer[bufsize++] = *type_s++;
						sf.precision--;
					}
					break;

				case 'c':
					type_c = va_arg(argptr, int);	// 2nd parameter was char but GCC recommended changing it to int
					if (bufsize<size)
					{	buffer[bufsize++] = type_c;
					}
					break;

			}
		}
		i++;
		if (bufsize==size)	break;
	}
	buffer[bufsize]=0;
}

fc_text *inittext(void)
{	OEMtext = new fctext;
	freesString = NULL;
	usedsString = NULL;
	return OEMtext;
}

