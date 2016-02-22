/************************************************************************************/
/*							Audio Importers & Streamers								*/
/*	    					---------------------------								*/
/*	 					(c) 1998-2005 by Stephen Fraser								*/
/*																					*/
/* Contains the functions for loading and decrypting various audio files			*/
/*																					*/
/* Init Dependancies:	Plugins														*/
/* Init Function:		void initaudioplugins(void)									*/
/* Term Function:		<none>														*/
/*																					*/
/*------------------------------  W A R N I N G  -----------------------------------*/
/*  This source file is only for the use of Stephen Fraser.  Any other use without	*/
/* my express permission may result in legal action being taken!					*/
/************************************************************************************/

/*----------------------------  License Update  -------------------------------*/
/* 27-Oct-2015: This file is licensed under GPL 2.  Any use outside of GPL 2   */
/* can be granted with the express permission of the estate of Stephen H.Fraser*/
/*******************************************************************************/

#include <fcio.h>

extern logfile	*audiolog;

/*
sound *genSoundError(uintf errorCode)
{	sound *s = newsound();
	s->errorCode = errorCode;
	return s;
}

// ************************************************************************
// ***							WAV Reader								***
// ************************************************************************
sound *readWAV(char *flname, byte *samples, uintf datasize)
{	uint32 frequency,avgbytespersec,samplesize;
	uint16 audioFormat, numchannels, blockalign, bitsPerSample, extraParamSize;

	byte *chunkStart = samples;
	intf bytesRemain = (intf)datasize;
	while(bytesRemain>0)
	{	char *chunkID = (char *)chunkStart;
		uint32 chunkSize = GetLittleEndianUINT32(chunkStart+4);
		chunkStart += 8;
		datasize-=8;
		if (txtncmp(chunkID,"RIFF",4)==0)
		{	// RIFF chunk - make sure it's WAVE format.  This chunk contains all other chunks as sub-chunks
			if (txtncmp((char *)chunkStart,"WAVE",4)!=0)
				msg("Bad WAV file","RIFF chunk is not of WAVE format");
			chunkSize = 4;
		}	else
		if (txtncmp(chunkID,"fmt ",4)==0)
		{	// fmt chunk
			audioFormat		= GetLittleEndianUINT16(chunkStart);
			numchannels		= GetLittleEndianUINT16(chunkStart+2);
			frequency		= GetLittleEndianUINT32(chunkStart+4);
			avgbytespersec	= GetLittleEndianUINT32(chunkStart+8);
			blockalign		= GetLittleEndianUINT16(chunkStart+12);
			bitsPerSample	= GetLittleEndianUINT16(chunkStart+14);
			if (chunkSize>16)
			{	extraParamSize	= GetLittleEndianUINT16(chunkStart+16);
			}
			else
			{	extraParamSize	= 0;
			}
			samplesize = avgbytespersec/(frequency*(uint32)numchannels);
		}	else
		if (txtncmp(chunkID,"data",4)==0)
		{	// data chunk
			return newsound(chunkStart, chunkSize, frequency, (bitsPerSample==16));
		}	else
		{	// Unrecognised chunk - Skip over it
			//msg("Bad WAV file",buildstr("Unrecognised chunk: %c%c%c%c",chunk->chunkID[0],chunk->chunkID[1],chunk->chunkID[2],chunk->chunkID[3]));
		}
		chunkStart += chunkSize;
		bytesRemain -= chunkSize;
	}
	return genSoundError(soundError_noAudio);
/ *
// ----------------------------------------------
	dword hdrRIFFsize;

	if (txtncmp((char *)samples,"RIFF",4)!=0)
	{	audiolog->log("%s has an invalid header.  It may not be a WAVE file",flname);
		return newsound();
	}
	hdrRIFFsize = GetLittleEndianUINT32(samples+4);

	if (txtncmp((char *)samples+8,"WAVEfmt ",8)!=0)
	{	audiolog->log("%s has an invalid header.  It may not be a WAVE file",flname);
		return newsound();
	}

	hdrRIFFsize = GetLittleEndianUINT32(samples+16);
	if ((hdrRIFFsize!=16) && (hdrRIFFsize!=18))
	{	audiolog->log("%s has an incorrect header size (%i, Expecting 16 or 18).  Are you sure it's using PCM encoding?",flname,hdrRIFFsize);
		return newsound();
	}

	audioFormat = GetLittleEndianUINT16(samples+20);
	if (audioFormat!=1)
	{	audiolog->log("%s has an invalid header.  It may not be a WAVE file, or may not be PCM encoded",flname);
		return newsound();
	}

	numchannels = GetLittleEndianUINT16(samples+22);	// 0x0001
	frequency	= GetLittleEndianUINT32(samples+24);	// 0x00002B11 (= 11025)
	avgbytespersec=GetLittleEndianUINT32(samples+28);	// 0x00002B11 (= 11025)
	blockalign	= GetLittleEndianUINT16(samples+32);	// 0x0001
	bitsPerSample= GetLittleEndianUINT16(samples+34);	// 0x0008 Bits Per Sample //This is a useless value so we re-use a variable we've finished with
	samplesize=avgbytespersec/(frequency*(dword)numchannels);

	samples += 36;

	if (hdrRIFFsize==18)
	{	// New WAV format, extra 2 bytes here
		samples += 2;
	}

	if (txtncmp((char *)samples,"data",4)!=0)
	{	audiolog->log("%s has an invalid header.  It may not be a WAVE file",flname);
		return newsound();
	}
	if (numchannels>1)
	{	audiolog->log("%s is not a MONO sound.  It cannot be played",flname);
		return newsound();
	}

	hdrRIFFsize = GetLittleEndianUINT32(samples+4);		// 0x00005FC6  Size in bytes of the sample

	return newsound(samples+8, hdrRIFFsize, frequency, (samplesize==2));
* /
}





// ************************************************************************
// ***							WAV Streamer							***
// ************************************************************************
sFileHandle *wavstreamhandle=NULL;

long wavefetch(byte *buffer,long size)
{	if (!wavstreamhandle) return 0;
	long bytesread = fileRead(wavstreamhandle,buffer,size);
	if (bytesread<size) { fileClose(wavstreamhandle); wavstreamhandle = NULL; }
	bytesStreamed+=bytesread;
	return bytesread;
}

void wavestop(void)
{	if (!wavstreamhandle) return;
	fileClose(wavstreamhandle);
	wavstreamhandle = NULL;
}

bool wavstreamer(char *flname)
{	byte wavhdr[44];
	wavstreamhandle = fileOpen(flname);
	fileRead(wavstreamhandle,wavhdr,44);

	streamerdata sdat;
	sdat.frequency = 44100;
	sdat.numchannels = 2;
	sdat.packetsize = 32768;
	sdat.callback = wavefetch;
	sdat.stop = wavestop;
	sdat.seek = NULL;
	sdat.seekpage = NULL;
	sdat.pause = NULL;
	sdat.flags = 0;
	return startstreamer(&sdat);
}

// ************************************************************************
// ***							3rd party libs							***
// ************************************************************************
bool oggstreamer(char *flname);
bool modstreamer(char *flname);

// ************************************************************************
// ***						Register all Audio Plugins					***
// ************************************************************************
void initaudioplugins(void)
{	addGenericPlugin((void *)readWAV,	  PluginType_SoundReader, ".wav");

	addGenericPlugin((void *)wavstreamer, PluginType_AudioStream, ".wav");
	addGenericPlugin((void *)oggstreamer, PluginType_AudioStream, ".ogg");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".669");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".amf");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".asy");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".dsm");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".far");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".gdm");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".gt2");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".imf");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".it");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".m15");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".med");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".mod");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".mtm");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".okt");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".s3m");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".stm");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".stx");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".ult");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".uni");
	addGenericPlugin((void *)modstreamer, PluginType_AudioStream, ".xm");
}
*/
