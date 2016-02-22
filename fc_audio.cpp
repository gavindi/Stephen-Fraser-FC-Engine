/******************************************************************************/
/*  					 FC_WIN Audio Device Driver           */
/*	    				 --------------------------           */
/*	 				   (c) 1998-2002 by Stephen Fraser    */
/*                                                                            */
/* Contains the FC Audio (sound) handling routines.                           */
/* Special requirements - if the sound system fails, it must elegantly produce*/
/* silence.  Under no circumstances is the sound system to exit the engine    */
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#define use3d

#include "fc_h.h"
#include <bucketSystem.h>
#define maxPlayers 128				// Maximum number of sounds that can play at once

void initWinMM(void);
logfile *audioLog = NULL;

extern intf     bufferSizeBytes;		// buffer size in bytes
extern intf     bufferSizeSamples;		// buffer size in samples
extern intf     audioNumSpeakers;       // Number of speakers playing sudio
float    *audioIntermediateBuffer;    // pointer to intermediate mixing buffer

struct sPlayer
{	sWavFile	*wav;               // pointer to the WAV file currently playing
	int 		position;           // the position we are currently up to in our mixing
	float		leftVol, rightVol;  // Panning data
    intf        userID;             // User-specified ID number (sent to callback function)
    void        (*callback)(intf id);// Function to call upon completion of sound (callback occurs DURING mixer operation)
    sPlayer     *next;              // Linked list to the next player object in this chain
} audioPlayer[maxPlayers];

sPlayer     *freeChannels, *usedChannels;

void initSoundSystem(void)
{	audioLog = newlogfile("logs/audio.log");

	// Clear out all playing channels, so nothing is playing
	for (int i=0; i<maxPlayers; i++)
	{	audioPlayer[i].wav=NULL;
        if (i<maxPlayers-1)
            audioPlayer[i].next = &audioPlayer[i+1];
        else
            audioPlayer[i].next = NULL;
	}
    freeChannels = &audioPlayer[0];
    usedChannels = NULL;
    initWinMM();
}

void playWavFile(sWavFile *s, float leftVol, float rightVol)
{	if (s==NULL) return;
	// Step 1 - find an emply player slot
	sPlayer *p = freeChannels;
	if (p==NULL) return;
	freeChannels = p->next;
	p->next = usedChannels;
	usedChannels = p;

	p->position = 0;
	p->leftVol = leftVol;
	p->rightVol = rightVol;
	p->wav = s;
}

float audioMixerScaler;

void initSWAudioMixer(uintf samplesPerSecond, uintf samplesPerBuffer, uintf bitsPerSample, uintf numSpeakers)
{  	audioIntermediateBuffer = (float *)fcalloc(sizeof(float)*audioNumSpeakers*bufferSizeSamples,"Audio Mixer Intermediate Buffer");	// Allocate the mixing buffer
}

bool mixSound(sPlayer *thisPlayer)
{	// This function is only called by 'mixer' it mixes a single, mono sound into the buffer.
    // Returns TRUE if the playing sample finished
	int mixSamples = bufferSizeSamples;		// Number of samples within the buffer
	int numSrcSamples = thisPlayer->wav->numSamples - thisPlayer->position;
	if (numSrcSamples<mixSamples) mixSamples = numSrcSamples;		// If we'll reach the end of this sRecording while filling this buffer, only mix in the remaining samples
	float *srcSample = thisPlayer->wav->samples + thisPlayer->position;
	float leftVolume = thisPlayer->leftVol;
	float rightVolume = thisPlayer->rightVol;

	int bufferIndex = 0;
	for (int j=0; j<mixSamples; j++)
	{   audioIntermediateBuffer[bufferIndex++] += srcSample[j] * leftVolume;
		audioIntermediateBuffer[bufferIndex++] += srcSample[j] * rightVolume;
	}
	thisPlayer->position += mixSamples;
	if (thisPlayer->position>=thisPlayer->wav->numSamples)
	{	// We've finished playing this WavFile, clear it from the channel
        thisPlayer->wav = NULL;
		if (usedChannels==thisPlayer)
		{   usedChannels = thisPlayer->next;
		}   else
		{   // We need to find 'thisPlayer' in the playing linked list
            sPlayer *tmp = usedChannels;
            while (tmp)
            {   if (tmp->next==thisPlayer)
                {   tmp->next=thisPlayer->next;
                    break;
                }
                tmp = tmp->next;
            }
        }
        thisPlayer->next = freeChannels;
        freeChannels = thisPlayer;
        return true;
	}
	return false;
}

bool mixSoundStereo(sPlayer *thisPlayer) /* ### To be enabled and tested ### */
{	// This function is only called by 'mixer' it mixes a single, stereo sound into the front two speakers.
    // Returns TRUE if the playing sample finished
	int mixSamples = bufferSizeSamples;		// Number of samples within the buffer
	int numSrcSamples = thisPlayer->wav->numSamples - thisPlayer->position;
	if (numSrcSamples<mixSamples) mixSamples = numSrcSamples;		// If we'll reach the end of this sRecording while filling this buffer, only mix in the remaining samples
	float *srcSample = thisPlayer->wav->samples + thisPlayer->position;
	float leftVolume = thisPlayer->leftVol;
	// float rightVolume = thisPlayer->rightVol;

	int bufferIndex = 0;
	for (int j=0; j<mixSamples; j++)
	{   audioIntermediateBuffer[bufferIndex] += srcSample[j] * leftVolume;
		audioIntermediateBuffer[bufferIndex+1] += srcSample[j+1] * leftVolume;
		bufferIndex += audioNumSpeakers;
	}
	thisPlayer->position += mixSamples;
	if (thisPlayer->position>=thisPlayer->wav->numSamples)
	{	// We've finished playing this WavFile, clear it from the channel
        thisPlayer->wav = NULL;
		if (usedChannels==thisPlayer)
		{   usedChannels = thisPlayer->next;
		}   else
		{   // We need to find 'thisPlayer' in the playing linked list
            sPlayer *tmp = usedChannels;
            while (tmp)
            {   if (tmp->next==thisPlayer)
                {   tmp->next=thisPlayer->next;
                    break;
                }
                tmp = tmp->next;
            }
        }
        thisPlayer->next = freeChannels;
        freeChannels = thisPlayer;
        return true;
	}
	return false;
}

void audioMixer(void)
{	// Global mixer function - mixes all playing sounds together into the buffer

	// Step 1 - fill the intermediate buffer with silence
	int bufferSizeSamplesAllChannels = bufferSizeSamples * 2;
	for (int i=0; i<bufferSizeSamplesAllChannels; i++)
	{	audioIntermediateBuffer[i] = 0.0f;
	}

	// Step 2 - mix each playing channel into the intermediate buffer
	sPlayer *thisPlayer = usedChannels;
	while (thisPlayer)
	{   sPlayer *nextPlayer = thisPlayer->next;
        if (thisPlayer->wav) mixSound(thisPlayer);	// wav is the last parameter to set, it's possible the mixer is called while a new sound is being prepared, so the if statement blocks that
        thisPlayer = nextPlayer;
	}

	// Step 3 (noise/static removal) - determine the minumum and maximum overdrive of the generated buffer
	float maxVolume = 1.5f;
	float minVolume = -1.5f;
	for (int i=0; i<bufferSizeSamplesAllChannels; i++)
	{	if (audioIntermediateBuffer[i]>maxVolume) maxVolume = audioIntermediateBuffer[i];
		if (audioIntermediateBuffer[i]<minVolume) minVolume = audioIntermediateBuffer[i];
	}
	// Calculate the largest overdrive whether it be a maximum or minimum value
	if (-minVolume>maxVolume) maxVolume = -minVolume;

	// Calculate the scale factor for scaling samples back to 16 bit
	audioMixerScaler = 32767.0f / maxVolume;
}

void audioMixTo16(int16 *outputBuffer, uintf numSamples)
{	// Convert samples back into 16 bit
	for (uintf i=0; i<numSamples; i++)
	{	outputBuffer[i] = (int16)(audioIntermediateBuffer[i] * audioMixerScaler);
	}
}


// ********************************************************************************************************************
// ***                                End of Mixer Code, Begining of Stream Loaders                                 ***
// ********************************************************************************************************************


// WAV loading variables
uint8 KSDATAFORMAT_SUBTYPE_PCM[] = { 0x01,0x00,0x00,0x00,  0x00,0x00,0x10,0x00,  0x80,0x00,0x00,0xAA,  0x00,0x38,0x9B,0x71 };

struct sKnownWAVFormat
{	uint16	    FormatID;
	const char	*name;
};

sKnownWAVFormat knownWAVFormat[]={	{ 0x0000, "[Unrecognised]" },
									{ 0x0001, "PCM" },
									{ 0x0003, "IEEE_FLOAT" },
									{ 0xFFFE, "WAVE_FORMAT_EXTENSIBLE" }
								};
const int numKnownWAVFormats = sizeof(knownWAVFormat)/sizeof(sKnownWAVFormat);	// Number of known wave formats

int WAVFindKnownFormat(uint16 FormatID)
{	for (int i=0; i<numKnownWAVFormats; i++)
	{	if (knownWAVFormat[i].FormatID==FormatID) return i;
	}
	return 0;
}

uint16 getWAV16(byte *ptr)
{	return ptr[0] + (ptr[1]*256);
}

uint32 getWAV32(byte *ptr)
{   return ptr[0] + (ptr[1]*256) + (ptr[2]*65536) + (ptr[3]*16777216);
}

sWavFile *readWAV(const char *flname, byte *samples, int datasize)
{	sWavFile s;
	sWavFile *result = NULL;
	// uint32 hdrRIFFsize;						// This is the size of the overall RIFF chunk
	uint32 bytesPerSecond;					// Bytes per second
	uint16 blockAlign;						// Number of bytes for one sample including all channels
	uint16 extensibleSize;					// Size (in bytes) of the Extensible WAV data
	uint16 validBitsPerSample;				// validBitsPerSample and s.bitsPerSample may not match (but we need them to), for something like 20 bit samples, the bits per sample would be 24 or 32, and validBitsPerSample would be 20
	byte *ptr;

	uint16	codec;							// Should always be 1 for PCM
	uint16	numChannels;					// Number of channels (if 1, can be used as 3D sound, otherwise can only be played around the player)
	uint16	bitsPerSample;					// 8, 16, or 32 bits (32 bits = IEEE float)
	uint32	Hz;								// Speed of playback, eg: 44100 (44Khz)
	uint32  channelMask;					// Details of which speaker to use		// Not yet used

	ptr = samples;
	memfill(&s,0,sizeof(s));
//	printf("--------------------------------------\n");
//	printf("Loading WAV file: %s\n",flname);
	// Read RIFF chunk header (RIFF encapsulates entire WAV file)
	if (txtncmp((char *)ptr,"RIFF",4)!=0)
	{
		audioLog->log("%s has an invalid header.  It may not be a WAVE file.  Specifically, header did not start with 'RIFF'.\n",flname);
		return NULL;
	}
	getWAV32(ptr+4);	// Skip over RIFFsize (we don't need it)
	if (txtncmp((char *)(ptr+8),"WAVE",4)!=0)
	{	audioLog->log("%s has an invalid header.  It may not be a WAVE file.  Expected token 'WAVE', received %c%c%c%c\n",flname,ptr[8],ptr[9],ptr[10],ptr[11]);
		return NULL;
	}
	ptr += 12;
	datasize -= 12;

	// Read in next chunk
	while (datasize>0)
	{	uint32 chunkSize = getWAV32(ptr+4);
//		printf("Chunk %c%c%c%c %i bytes\n",ptr[0],ptr[1],ptr[2],ptr[3],(int)chunkSize);
		if (txtncmp((char *)ptr,"fmt ",4)==0)
		{	// Format chunk
			if (chunkSize>=14)
			{	codec			= getWAV16(ptr+8);		// format type (1=PCM)
				numChannels 	= getWAV16(ptr+10);	    // number of channels (i.e. 1=mono, 2=stereo...)
				Hz			    = getWAV32(ptr+12);	    // sample rate
			    bytesPerSecond	= getWAV32(ptr+16);	    // Bytes per second
				blockAlign		= getWAV16(ptr+20);	    // Number of bytes for one sample including all channels
			}

			int WAVFormatIndex = WAVFindKnownFormat(codec);
			if (WAVFormatIndex==0) return NULL;

			if (chunkSize>=18)							// 18 = sizeof(WAVEFORMATEX) - a data structure limited to Microsoft Windows
			{	bitsPerSample	= getWAV16(ptr+22);		// number of bits per sample of mono data
			    extensibleSize	= getWAV16(ptr+24);		// the count in bytes of the size of extra information (after cbSize)
			}	else
			{
//              printf("Estimating BitsPerSample!\n");
				bitsPerSample	= (uint16)(((bytesPerSecond / Hz) / numChannels) * 8);	// number of bits per sample of mono data
			    extensibleSize	= 0;								// the count in bytes of the size of extra information (after cbSize)
			}

			if (chunkSize>=40)										// 40 = sizeof(WAVEFORMATEXTENSIBLE) - a data structure limited to Microsoft Windows
			{
//              printf("Loading extra WAVE_FORMAT_EXTENSIBLE data\n");
				if (codec!=0xFFFE)									// WAVE_FORMAT_EXTENSIBLE = 0xFFFE
				{	audioLog->log("%s contains a header with WAVE_FORMAT_EXTENSIBLE data, but the Format Tag is 0x%04X (expecting 0xFFFE)\n",flname, codec);
					return NULL;
				}
				if (extensibleSize<22)
				{	audioLog->log("%s contains a header with WAVE_FORMAT_EXTENSIBLE data, but there is less than the necessary 22 bytes of added data\n",flname);
					return NULL;
				}
				validBitsPerSample = getWAV16(ptr+26);
				if (validBitsPerSample!=bitsPerSample)
				{	audioLog->log("%s contains %i bits per sample where %i are valid.  I don't know how to handle that scenario.\n",flname,bitsPerSample,validBitsPerSample);
					return NULL;
				}
				channelMask = getWAV32(ptr+28);
				for (int i=0; i<(int)sizeof(KSDATAFORMAT_SUBTYPE_PCM); i++)
				{	if (ptr[32+i]!=KSDATAFORMAT_SUBTYPE_PCM[i])
					{	audioLog->log("%s contains WAVE_FORMAT_EXTENSIBLE data, but the subformat is not of type PCM.\n",flname);
						return NULL;
					}
				}
				codec = 1;
			} else
			{	// File did not contain WAVE_FORMAT_EXTENSIBLE data (is old file format), create the data automatically from what we know and guesswork.
				extensibleSize = 0;
				validBitsPerSample=bitsPerSample;
				switch (numChannels)
				{	case 1: channelMask = 0x00000004;	break;	//SPEAKER_FRONT_CENTER;
					case 2: channelMask = 0x00000003;	break;	//SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
					case 4: channelMask = 0x00000033;	break;	//SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
					case 5: channelMask = 0x00000037;	break;	//SPEAKER_FRONT_CENTER | SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
					case 6: channelMask = 0x0000003F;	break;	//SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
				}
				codec = 1;
			}

//			printf("Format: %s, Channels: %i, Sample Rate: %i, BytesPerSec: %i, BlockAlign: %i, Bits: %i\n",knownWAVFormat[WAVFormatIndex].name, (int)numChannels, (int)Hz, (int)bytesPerSecond, (int)blockAlign, (int)bitsPerSample);
//			printf("            Valid Bits Per Sample: %i, WAVE Format to use: %s, Channel Mask: 0x%04X\n",(int)validBitsPerSample,knownWAVFormat[codec].name,(unsigned int)channelMask);
		}	else

		if (txtncmp((char *)ptr,"data",4)==0)
		{	// Data chunk

			// To make life easier for ourselves for now, we will limit what kind of WAV files we support
			// We will limit ourselves to a 16 bit single channel at 44 Khz.
			// We will then convert that to 32 bit FLOAT to make mixing extremely easy
			if (numChannels!=1)
			{	audioLog->log("Error: File %s is not 1 channel (=%i channels)",flname);
				return NULL;
			}
			if (bitsPerSample!=16)
			{	audioLog->log("Error: File %s is not 16 bits (=%i bits)",flname,bitsPerSample);
				return NULL;
			}
			if (Hz != 44100)
			{	audioLog->log("Error: File %s is not 44100 Hz (=%i Hz)",flname,Hz);
				return NULL;
			}

			// This is a valid sample
			int numSamples = chunkSize / blockAlign;
			result = (sWavFile *)fcalloc(sizeof(sWavFile) + numSamples * sizeof(float),flname);
			result->numSamples = numSamples;
			result->samples = (float *)(((byte *)result) + sizeof(sWavFile));
			int16 *src = (int16*)(ptr+8);
			float *dst = result->samples;
			for (int i=0; i<numSamples; i++)
			{	dst[i] = (float)src[i] / 32768.0f;
			}
			audioLog->log("Loaded WAV file: %s (%i bit, %i Hz, %i channels, %i samples",flname,bitsPerSample,Hz, numChannels, numSamples);
		}	else
		{	// Unrecognised chunk - we can safely ignore these
			//audioLog("File %s contains unsupported chunk %c%c%c%c",flname,ptr[0],ptr[1],ptr[2],ptr[3]);
		}

		ptr += chunkSize+8;
		datasize -= chunkSize+8;
	}

	return result;
}

sWavFile *loadWAV(const char *filename)
{	int size;
    byte *raw = fileLoad(filename);
    size=filesize;
	sWavFile *result = readWAV(filename,raw,size);
    fcfree(raw);
	return result;
}

/*
struct channelbucket
{	channelbucket		*next;
	uintf				sound_ID;
	dword				flags;
	void				*OEMdata;
}	*audiobuckets=NULL,*fullbucket=NULL,*freebucket=NULL;

// Audio Driver functions
bool initSNDHardware(logfile *audiolog, intf *numchannels, void *classfinder);
void AD_ShutdownAudio(void);
uintf AD_GetChannelInfoSize(void);
void AD_setAudio3DParams(float unitsInMeters, float dopplerScale, float rolloffScale);
void AD_Update(void);
void AD_PingChannels(channelbucket *b);
void AD_StopSound(void *OEMInfo);
bool AD_PlaySound(sound *snd, dword flags, float *matrix, channelbucket *b);
void AD_NewPCMSound(sound *dest, byte *PCM, uintf bytesize, uintf frequency, bool is16bit);
void AD_DeleteSound(sound *snd);
void AD_SetListener(float *m, float3 *velocity);

void initaudioplugins(void);
void audioerror(char *txt, bool fatal);

sound		*freesound,*usedsound;
logfile		*audiolog;
bool		audioneedsupdate;

void killaudio(void)
{	if (!audioworks) return;
	if (streamingWorks) stopStreaming();
	while (usedsound)
		deletesound(usedsound);
	audioworks = false;
	killbucket(sound,flags,sound_memstruct);
	if (audiobuckets) fcfree(audiobuckets);
	AD_ShutdownAudio();
}

// ******************* initaudio API function **********************
void initaudio(intf numchannels)
{	audioworks=false;				// Assume audio system does not work
	audiolog = newlogfile("logs\\audio.log");
	if (numchannels<32) numchannels = 32;
	audioworks = initSNDHardware(audiolog, &numchannels, (void *)classfinder);
	if (!audioworks) return;

	freesound = NULL;
	usedsound = NULL;

	audiolog->log("Allocating %i player channels",numchannels);
	uintf OEMsize = AD_GetChannelInfoSize();
	byte *buf = fcalloc(numchannels*(sizeof(channelbucket)+OEMsize),"Audio Channels");
	audiobuckets = (channelbucket *)buf;		buf += numchannels*sizeof(channelbucket);
	for (intf i=0; i<numchannels; i++)
	{	audiobuckets[i].next = &audiobuckets[i+1];
		audiobuckets[i].OEMdata = (void *)buf;	buf += OEMsize;
	}
	audiobuckets[numchannels-1].next = NULL;
	audiobuckets[numchannels-1].OEMdata = (void *)buf;	buf += OEMsize;
	freebucket = audiobuckets;
	initaudioplugins();
	audioneedsupdate = true;
}

intf soundsplaying(void)
{	intf result = 0;
	channelbucket *b = fullbucket;
	while (b)
	{	result++;
		b=b->next;
	}
	return result;
}

void stopsound(void *s, uintf sound_ID)
{	if (!s) return;
	channelbucket *killb = (channelbucket *)s;
	if (killb->sound_ID != sound_ID) return;
	if (!killb->OEMdata) return;
	AD_StopSound(killb->OEMdata);
	channelbucket *b = fullbucket;
	while (b)
	{	if (b->next==killb)
		{	b->next = killb->next;
			killb->next = freebucket;
			freebucket = killb;
			break;
		}
		b=b->next;
	}
	if (killb==fullbucket)
	{	fullbucket=killb->next;
		killb->next = freebucket;
		freebucket = killb;
	}
}

void updatelistener(float *m, float3 *velocity)
{	AD_SetListener(m, velocity);
	audioneedsupdate = true;
}

void setAudio3DParams(float unitsInMeters, float dopplerScale, float rolloffScale)
{	AD_setAudio3DParams(unitsInMeters, dopplerScale, rolloffScale);
	audioneedsupdate = true;
}

void audio_update(void)
{	if (audioneedsupdate) AD_Update();
	audioneedsupdate = false;
	AD_PingChannels(fullbucket);
}

// ******************* newsound API function *********************
sound *newsound(void)
{	sound *newsnd;
	allocbucket(sound,newsnd,flags,sound_memstruct,1024,"Sound Cache");
	newsnd->rate = 0;
	newsnd->length = 0;
	newsnd->oemdata = NULL;
	return newsnd;
}

sound *newsound(byte *PCM,dword bytesize,dword frequency,bool is16bit)
{	sound *dest = newsound();
	if (!audioworks) return dest;

	AD_NewPCMSound(dest, PCM, bytesize, frequency, is16bit);

	if (is16bit)
		dest->length = bytesize;
	else
		dest->length = bytesize>>1;
	dest->rate = frequency;
	dest->flags = (dest->flags & sound_memstruct) | 1;
	if (is16bit) dest->flags |= sound_16bit;
	return dest;
}

char CurrentSoundName[128];											// Filename of sound currently being imported
char *opensoundstring=pluginfilespecs[PluginType_SoundReader];

sound *newsound(char *flname, byte *PCMdata, uintf datasize)
{	txtcpy(CurrentSoundName,sizeof(CurrentSoundName),flname);
	sound *dest = NULL;

	char *ext=flname;
	char *fln = flname;
	// Scan through the filename to find the extension.
	while (*fln!=0)
	{	if (*fln=='.') ext = fln;
		fln++;
	}

	sound *(*importer)(char *, byte *, uintf) = (sound *(*)(char *, byte *, uintf))getPluginHandler(ext, PluginType_SoundReader);
	if (importer) dest = importer(flname, PCMdata, datasize);

	if (dest==NULL)
		msg("Image File Error",buildstr("%s is either an unknown, or invalid audio file",flname));
	return dest;
}

sound *newsound(char *flname)
{	if (!fileExists(flname))
	{	audiolog->log("File not found: %s");
		return newsound();
	}
	byte *audiofile = fileLoad(flname, genflag_usetempbuf);
	sound *dest = newsound(flname, audiofile, filesize);
	freetempbuffer(audiofile);
	return dest;
}

void deletesound(sound *snd)
{	AD_DeleteSound(snd);
	snd->oemdata = NULL;
	snd->flags &= sound_memstruct;
	deletebucket(sound,snd);
}

uintf next_soundID = 0;

// ********************* playsound API function ********************
void *playsound(sound *snd, dword flags, float *matrix, uintf *sound_ID)
{	if (!audioworks) return NULL;
	if (!snd->oemdata) return NULL;

	// Allocate a playback bucket
	channelbucket *b = freebucket;
	if (!b) return NULL;

	if (!AD_PlaySound(snd, flags, matrix, b)) return NULL;	// Error - return as cleanly as possible

	freebucket = b->next;
	b->next = fullbucket;
	fullbucket = b;

	if (sound_ID) *sound_ID = next_soundID;
	b->sound_ID = next_soundID;
	next_soundID++;
	b->flags = flags;
	audioneedsupdate = true;
	return b;
}

void playsound(sound *snd, dword flags)
{	playsound(snd, flags, cammat, NULL);
}

// ********************************************************************************
// ***							Streaming Interface Code						***
// ********************************************************************************
void AD_stopstreaming(void);
void AD_pausestreaming(bool pause);
bool AD_startstreamer(intf (*fetch)(byte *,intf),dword frequency, dword numchannels, dword packetsize);

void (*streamstopfunc)(void)=NULL;			// pointer to the function to stop stream playback
void (*streampausefunc)(bool)=NULL;			// pointer to the function to pause stream playback
bool streamingWorks = false;				// Is the streaming system in working order?
dword streamEvents;
dword bytesStreamed;						// Diagnostic Data

void stopStreaming(void)
{	if (!streamingWorks) return;
	streamingWorks = false;
	AD_stopstreaming();
	if (streamstopfunc) streamstopfunc();
	streamstopfunc = NULL;
}

void pauseStreaming(bool pause)
{	if (!streamingWorks) return;
	if (streampausefunc) streampausefunc(pause);
	AD_pausestreaming(pause);
}

bool startstreamer(streamerdata *data)
{	if (streamingWorks) stopStreaming();
	streamEvents = 0;
	bytesStreamed = 0;
	streamingWorks = false;
	streampausefunc = data->pause;
	streamstopfunc  = data->stop;
	if (!AD_startstreamer(data->callback,data->frequency,data->numchannels,data->packetsize))
	{	stopStreaming();
		return false;
	}
	return true;
}

bool startStreaming(char *flname)
{	if (!fileExists(flname))
	{	audiolog->log("Streamer Error: File not found %s",flname);
		return false;
	}

	char *ext=flname;
	char *fln = flname;
	// Scan through the filename to find the extension.
	while (*fln!=0)
	{	if (*fln=='.') ext = fln;
		fln++;
	}

	bool (*streamer)(char *) = (bool (*)(char *))getPluginHandler(ext, PluginType_AudioStream);
	if (!streamer)
	{	audiolog->log("Streamer Error: No plugin exists which can stream file %s",flname);
		return false;
	}
	return streamer(flname);
}

*/
