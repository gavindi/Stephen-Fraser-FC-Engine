/******************************************************************************/
/*					  FC_WIN DirectSound8 Audio Device Driver	              */
/*	    			  ---------------------------------------				  */
/*							(c) 2005 by Stephen Fraser						  */
/*                                                                            */
/* Contains the code for FC to work through a DirectX8 compatible audio card  */
/* This is the standard device driver upon which all others will be based.	  */
/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

// Set up speaker config within Control Panel. 
// win95: The feature does not appear to be supported
// win98: Multimedia->Audio Tab->Advanced Properties button in Playback
// winME: ???
// win2k: Sounds & Multimedia->Audio Tab->Advanced button in Sound Playback
// winXP: Sounds Speech and Audio Devices->Change Speaker Settings->Advanced button in Speaker Settings

// #define use3d			// Disable this line for 2D sound handler
#define INITGUID

#include <windows.h>
#include <direct.h>		// Used by loading routines
#include <dsound.h>
// #include <dmusici.h>
#include "../fc_h.h"
#include "fc_win.h"
#include <bucketsystem.h>

extern logfile			*audiolog;

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

#ifdef use3d
#define DriverNameTech	"DSOUND3D_DX8"				// Technical Driver Name
#define DriverNameUser	"DirectX 8 3D Sound Driver"	// User Friendly Driver Name
#define MsgErrorHeader	"DSOUND3D_DX8 Error"		// Title text to use for errors
#else
#define DriverNameTech	"DSOUND_DX7"				// Technical Driver Name
#define DriverNameUser	"DirectX 7 Sound Driver"	// User Friendly Driver Name
#define MsgErrorHeader	"DSOUND_DX7 Error"			// Title text to use for errors
#endif
#define DriverAuthor	"Stephen Fraser"			// Name of the device driver's author
#define DLLFileName		"DSOUND.DLL"				// Filename of the DLL file

void AD_SetListener(float *m, float3 *velocity);

// DLL Interface
typedef HRESULT (WINAPI *DirectSoundCreate7Proc)(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS7, LPUNKNOWN pUnkOuter);
DirectSoundCreate7Proc	DirectSoundCreate7FnPtr;	// Pointer to the DirectSoundCreate8 function
HINSTANCE dsounddllinst = NULL;						// Handle to the DSOUND DLL

#define testds(txt,fatal) { if (FAILED(hr)) audioerror(txt,fatal); }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

struct channelbucket
{
	channelbucket		*next;
	uintf				sound_ID;
	dword				flags;
	void				*OEMdata;
};

struct DSchannelinfo
{	LPDIRECTSOUNDBUFFER		playbuffer;
	LPDIRECTSOUND3DBUFFER	playbuffer3d;
};

HRESULT					hr;
LPDIRECTSOUND			dsound = NULL;		// Pointer to DirectSound
LPDIRECTSOUNDBUFFER		sprimarybuffer;		// Primary sound buffer
LPDIRECTSOUND3DLISTENER	listener3D;			// the 3D Listener for 3D sound
WAVEFORMATEX			splaybackspec;	

DSBUFFERDESC			sbufferdesc;
WAVEFORMATEX			soundspec;
Matrix					DScurrentlistenermatrix;

void audioerror(char *txt, bool fatal)
{	char *m = NULL;
	switch(hr)
	{	case DS_NO_VIRTUALIZATION:		m="The buffer was created, but another 3-D algorithm was substituted."; break;
//		case DS_INCOMPLETE:				m="The method succeeded, but not all the optional effects were obtained. "; break;
		case DSERR_ACCESSDENIED:		m="The request failed because access was denied. ";	break;
		case DSERR_ALLOCATED:			m="The request failed because resources, such as a priority level, were already in use by another caller. ";	break;
		case DSERR_ALREADYINITIALIZED:	m="The object is already initialized. "; break;
		case DSERR_BADFORMAT:			m="The specified wave format is not supported. "; break;
		case DSERR_BUFFERLOST:			m="The buffer memory has been lost and must be restored. "; break;
//		case DSERR_BUFFERTOOSMALL:		m="The buffer size is not great enough to enable effects processing. ";break;
		case DSERR_CONTROLUNAVAIL:		m="The buffer control (volume, pan, and so on) requested by the caller is not available. "; break;
//		case DSERR_DS8_REQUIRED:		m="A DirectSound object of class CLSID_DirectSound8 or later is required for the requested functionality. For more information, see IDirectSound8. ";break;
		case DSERR_GENERIC:				m="An undetermined error occurred inside the DirectSound subsystem. "; break;
		case DSERR_INVALIDCALL:			m="This function is not valid for the current state of this object. "; break;
		case DSERR_INVALIDPARAM:		m="Invalid parameter"; break;
		case DSERR_NOAGGREGATION:		m="The object does not support aggregation. "; break;
		case DSERR_NODRIVER:			m="No sound driver is available for use. "; break;
		case DSERR_NOINTERFACE:			m="The requested COM interface is not available. "; break;
//		case DSERR_OBJECTNOTFOUND:		m="The requested object was not found. ";break;
		case DSERR_OTHERAPPHASPRIO:		m="Another application has a higher priority level, preventing this call from succeeding "; break;
		case DSERR_OUTOFMEMORY:			m="The DirectSound subsystem could not allocate sufficient memory to complete the caller's request. "; break;
		case DSERR_PRIOLEVELNEEDED:		m="The caller does not have the priority level required for the function to succeed. "; break;
		case DSERR_UNINITIALIZED:		m="The IDirectSound::Initialize method has not been called or has not been called successfully before other methods were called. "; break;
		case DSERR_UNSUPPORTED:			m="The function called is not supported at this time."; break;
		default:						m="An unknown error has occured."; break;
	}
	if (fatal)
		msg("Fatal Audio Error",buildstr("Failed to call %s\n%s",txt,m));
	else
	{	audiolog->log("Warning: Failed to call %s, %s",txt,m);
	}
}

// WinMM Variables
WAVEOUTCAPS *sndDevCaps;

bool initSNDHardware(logfile *audiolog, intf *numchannels, void *classfinder)
{
	audiolog->log("SonicBoom Digital Surround Sound Driver for FC Engine (c) 2012 Stephen Fraser");
	audiolog->log("-----------------------------------------------------------------------------");
	uintf numDevs = waveOutGetNumDevs();
	audiolog->log("%i Audio Devices Detected",numDevs);
	sndDevCaps = (WAVEOUTCAPS *)fcalloc(sizeof(WAVEOUTCAPS)*numDevs,"Audio Device Capabilities");
	for (uintf i=0; i<numDevs; i++)
	{	if (MMSYSERR_NOERROR!=waveOutGetDevCaps(i,&sndDevCaps[i],sizeof(WAVEOUTCAPS)))
		{	memfill(&sndDevCaps[i],0,sizeof(WAVEOUTCAPS));
		}
		audiolog->log("%i. %s (%i Channels)",i,sndDevCaps[i].szPname,sndDevCaps[i].wChannels);
	}
	audiolog->log("");

#ifdef use3d
	audiolog->log("DirectSound3D Surround Sound Audio plugin For FC Engine");
	audiolog->log("-------------------------------------------------------");
#else
	audiolog->log("DirectSound Stereo Audio plugin For FC Engine");
	audiolog->log("---------------------------------------------");
#endif
	audiolog->log("");
	// Interface with DSOUND.LIB
	dsounddllinst = LoadLibrary(DLLFileName);
	if (!dsounddllinst) 
	{	audiolog->log("DirectX 7 or above does not appear to be installed");
		audiolog->log("Unable to load "DLLFileName);
		return false;
	}
	DirectSoundCreate7FnPtr = (DirectSoundCreate7Proc)GetProcAddress(dsounddllinst, "DirectSoundCreate7");
	if (!DirectSoundCreate7FnPtr)
	{	audiolog->log("Failed to obtain 'DirectSoundCreate7' interface from "DLLFileName);
		return false;
	}

	// Create DirectSound 
    if (FAILED(hr=DirectSoundCreate7FnPtr(NULL, &dsound, NULL)))  // First parameter is sound device, NULL = default
	{	audioerror("DirectSoundCreate",true);
		return false; 
	}
    // Set co-op level
	if (FAILED(hr=dsound->SetCooperativeLevel(winVars.mainWin, DSSCL_PRIORITY)))
	{	audioerror("SetCooperativeLevel",true);
		dsound->Release();
		return false;
    }

	// Obtain primary buffer
    ZeroMemory(&sbufferdesc, sizeof(sbufferdesc));
    sbufferdesc.dwSize = sizeof(sbufferdesc);
 	sbufferdesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
	sbufferdesc.guid3DAlgorithm = GUID_NULL;

	if (FAILED(hr = dsound->CreateSoundBuffer(&sbufferdesc, &sprimarybuffer, NULL)))
    {	audioerror("CreateSoundBuffer (Primary Buffer)",true);
		dsound->SetCooperativeLevel(winVars.mainWin, DSSCL_NORMAL);
		dsound->Release();
		return false;
	}

    // Set primary buffer format     
	WAVEFORMATEX wfx;
    memfill(&wfx, 0, sizeof(WAVEFORMATEX)); 
    wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; 
    if (FAILED(sprimarybuffer->SetFormat(&wfx)))
	{	audioerror("SetFormat (Primary Buffer",true);
		sprimarybuffer->Release();
		dsound->SetCooperativeLevel(winVars.mainWin, DSSCL_NORMAL);
		dsound->Release();
		return false;
	}

#ifdef use3d
	sprimarybuffer->Release();
    // Obtain primary buffer, asking it for 3D control
    ZeroMemory( &sbufferdesc, sizeof(sbufferdesc) );
    sbufferdesc.dwSize = sizeof(sbufferdesc);
    sbufferdesc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
    if( FAILED(dsound->CreateSoundBuffer( &sbufferdesc, &sprimarybuffer, NULL ) ) )
    {	audioerror("CreateSoundBuffer (Primary 3D Buffer)",true);
		dsound->SetCooperativeLevel(winVars.mainWin, DSSCL_NORMAL);
		dsound->Release();
		return false;
	}
    if( FAILED( sprimarybuffer->QueryInterface( IID_IDirectSound3DListener, (void **)&listener3D ) ) )
    {	audioerror("QueryInterface )Primary 3D Buffer)",true);
		sprimarybuffer->Release();
		dsound->SetCooperativeLevel(winVars.mainWin, DSSCL_NORMAL);
		dsound->Release();
		return false;
	}
	listener3D->SetDistanceFactor(1.0f, DS3D_DEFERRED);
	listener3D->SetDopplerFactor (1.0f, DS3D_DEFERRED);
	listener3D->SetRolloffFactor (1.0f, DS3D_DEFERRED);
	float3 v = {0,0,0};
	AD_SetListener(cammat, &v);
#endif

	char *spkconfig;
	DWORD spkconfigflag;
	if (FAILED( dsound->GetSpeakerConfig(&spkconfigflag) ) )
	{	spkconfigflag = DSSPEAKER_MONO;
		spkconfig = "Failed to detect";
	}	else
	{	switch(spkconfigflag)
		{	case DSSPEAKER_HEADPHONE: spkconfig = "Headphones"; break;
			case DSSPEAKER_MONO: spkconfig = "MONO"; break; 
			case DSSPEAKER_STEREO: spkconfig = "2 Speakers (Stereo)"; break;
			case DSSPEAKER_QUAD: spkconfig = "4 Speakers (Quadraphonic)"; break;
			case DSSPEAKER_SURROUND: spkconfig = "Unknown Surround Sound Configuration"; break;
			case DSSPEAKER_5POINT1: spkconfig = "5.1 (Surround)"; break;
			default: spkconfig = "Unknown Configuration, may actually be better than 5.1"; break;
		}
	}

	audiolog->log("Detected Speaker Configuration: %s",spkconfig);
	return true;
}

void AD_setAudio3DParams(float unitsInMeters, float dopplerScale, float rolloffScale)
{	if (unitsInMeters < 0.001f) unitsInMeters = 0.001f;
	if (dopplerScale < DS3D_MINDOPPLERFACTOR) dopplerScale = DS3D_MINDOPPLERFACTOR;
	if (dopplerScale > DS3D_MAXDOPPLERFACTOR) dopplerScale = DS3D_MAXDOPPLERFACTOR;
	if (rolloffScale < DS3D_MINROLLOFFFACTOR) rolloffScale = DS3D_MINROLLOFFFACTOR;
	if (rolloffScale > DS3D_MAXROLLOFFFACTOR) rolloffScale = DS3D_MAXROLLOFFFACTOR;
	listener3D->SetDistanceFactor(1 / unitsInMeters,DS3D_DEFERRED);
	listener3D->SetDopplerFactor(dopplerScale,DS3D_DEFERRED);
	listener3D->SetRolloffFactor(rolloffScale,DS3D_DEFERRED);
}

void AD_ShutdownAudio(void)
{	if (sprimarybuffer) sprimarybuffer->Release();
	if (dsound) 
	{	dsound->SetCooperativeLevel(winVars.mainWin, DSSCL_NORMAL);
		dsound->Release();
	}
	FreeLibrary(dsounddllinst);
	dsounddllinst = NULL;
}

void AD_SetListener(float *m, float3 *velocity)
{	listener3D->SetPosition(m[mat_xpos],m[mat_ypos],m[mat_zpos],DS3D_DEFERRED);
	listener3D->SetOrientation(m[mat_zvecx],m[mat_zvecy],m[mat_zvecz],
							   m[mat_yvecx],m[mat_yvecy],m[mat_yvecz],DS3D_DEFERRED);
	listener3D->SetVelocity(velocity->x, velocity->y, velocity->z,DS3D_DEFERRED);
}

void AD_Update(void)
{	// Reposition the listener according to the camera matrix
#ifdef use3d
	listener3D->CommitDeferredSettings();				// ### Problem detected here when writing Thunder Racing, commenting it out fixed problem, but probably lost some features
#else
// ### Do something to control panning in 2D
#endif
	

}

void AD_PingChannels(channelbucket *b)
{	// This function gives the audio driver a chance to cycle through all the channels, and make sure they're still playing, 
	// if they've stopped, they must be deleted via the 'stopsound' function (which will then call AD_StopSound)
	while (b)
	{	channelbucket *nextb = b->next;
		DWORD status;
		LPDIRECTSOUNDBUFFER playbuffer = ((DSchannelinfo *)b->OEMdata)->playbuffer;
		playbuffer->GetStatus(&status);
		if (status & DSBSTATUS_BUFFERLOST)
		{	playbuffer->Restore();
		}
		if (!(b->flags & sound_monitor))
		{	if (!(status & DSBSTATUS_PLAYING))
			{	stopsound(b, b->sound_ID);
			}
		}
		b = nextb;
	}
}

uintf AD_GetChannelInfoSize(void)
{	return sizeof(DSchannelinfo);
}

bool setfrequency(void *handle, uintf sound_ID, uintf Hz)
{	if (!handle) return false;
	channelbucket *b = (channelbucket *)handle;
	if (b->sound_ID != sound_ID) return false;
	LPDIRECTSOUNDBUFFER playbuffer = ((DSchannelinfo *)b->OEMdata)->playbuffer;
	if (!playbuffer) return false;
	hr = playbuffer->SetFrequency(Hz);
	if (hr == DS_OK) return true;
	audioerror("setfrequency",true);
	return false;
}

bool positionchannel(void *handle, uintf sound_ID, float *mtx, float3 *velocity)
{	if (!handle) return false;
	channelbucket *b = (channelbucket *)handle;
	if (b->sound_ID != sound_ID) return false;
	LPDIRECTSOUNDBUFFER playbuffer = ((DSchannelinfo *)b->OEMdata)->playbuffer;
	if (!playbuffer) return false;
	LPDIRECTSOUND3DBUFFER playbuffer3d = ((DSchannelinfo *)b->OEMdata)->playbuffer3d;
	playbuffer3d->SetPosition(mtx[mat_xpos],mtx[mat_ypos],mtx[mat_zpos],DS3D_DEFERRED);
//	playbuffer3d->SetConeOrientation(mtx[mat_zvecx],mtx[mat_zvecy],mtx[mat_zvecz],DS3D_DEFERRED);
	playbuffer3d->SetVelocity(0,0,0,DS3D_DEFERRED);
	return true;
}


bool AD_PlaySound(sound *snd, dword flags, float *matrix, channelbucket *b)
{	DSchannelinfo *DSchannel = (DSchannelinfo *)b->OEMdata;

	LPDIRECTSOUNDBUFFER playbuf;
	if (FAILED(dsound->DuplicateSoundBuffer((LPDIRECTSOUNDBUFFER)snd->oemdata,&playbuf)))
	{	return false;
	}
	DSchannel->playbuffer = playbuf;

	uintf dsflags = 0;
	if (flags & sound_loop) dsflags |= DSBPLAY_LOOPING;

#ifdef use3d
	LPDIRECTSOUND3DBUFFER playbuf3d;
	playbuf->QueryInterface( IID_IDirectSound3DBuffer, (VOID**)&playbuf3d);
	DSchannel->playbuffer3d = playbuf3d;
	if (!matrix)
		matrix = DScurrentlistenermatrix;
	
	playbuf3d->SetPosition(matrix[mat_xpos],matrix[mat_ypos],matrix[mat_zpos],DS3D_DEFERRED);
//	playbuf3d->SetConeOrientation(matrix[mat_zvecx],matrix[mat_zvecy],matrix[mat_zvecz],DS3D_DEFERRED);
//	playbuf3d->SetConeOutsideVolume(DSBVOLUME_MAX/*(DSBVOLUME_MIN-DSBVOLUME_MAX)>>1*/,DS3D_DEFERRED);
//	playbuf3d->SetMinDistance(mindist, DS3D_DEFERRED);
//	playbuf3d->SetMaxDistance(maxdist, DS3D_DEFERRED);
	playbuf3d->SetVelocity(0,0,0,DS3D_DEFERRED);
#else
	playbuf->SetVolume(DSBVOLUME_MIN);
#endif
	playbuf->Play(0,0,dsflags);
	return true;
}

void AD_StopSound(void *OEMInfo)
{	LPDIRECTSOUNDBUFFER playbuffer = ((DSchannelinfo *)OEMInfo)->playbuffer;
	playbuffer->Stop();
	playbuffer->Release();
	playbuffer = NULL;
}

void AD_DeleteSound(sound *snd)
{	if (snd->oemdata)
	{	((LPDIRECTSOUNDBUFFER)snd->oemdata)->Release();
	}
}

void AD_NewPCMSound(sound *dest, byte *PCM, uintf bytesize, uintf frequency, bool is16bit)
{	LPDIRECTSOUNDBUFFER sbuf;
	
	memfill(&soundspec, 0, sizeof(WAVEFORMATEX)); 
	soundspec.wFormatTag = WAVE_FORMAT_PCM;
	soundspec.nChannels = 1; 
    soundspec.nSamplesPerSec = frequency;
	if (is16bit)
	{	soundspec.nAvgBytesPerSec = frequency<<1; 
		soundspec.wBitsPerSample = 16;
		soundspec.nBlockAlign = 2;
	}	else
	{	soundspec.nAvgBytesPerSec = frequency;
		soundspec.wBitsPerSample = 8;
		soundspec.nBlockAlign = 1;
	}

    ZeroMemory( &sbufferdesc, sizeof(sbufferdesc) );
    sbufferdesc.dwSize          = sizeof(sbufferdesc);
    sbufferdesc.dwBufferBytes   = bytesize;
    sbufferdesc.guid3DAlgorithm = DS3DALG_DEFAULT;
    sbufferdesc.lpwfxFormat     = &soundspec;
    sbufferdesc.dwFlags         = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLFREQUENCY;
#ifdef use3d
    sbufferdesc.dwFlags         |= DSBCAPS_CTRL3D | DSBCAPS_MUTE3DATMAXDISTANCE;
#endif

	hr=dsound->CreateSoundBuffer(&sbufferdesc, &sbuf, NULL);
	testds("CreateSoundBuffer",true);

    VOID *lpvPtr1, *lpvPtr2;
    DWORD dwBytes1, dwBytes2;
	sbuf->Lock(0,bytesize,		// Start of buffer and number of bytes
               &lpvPtr1,		// Address of lock start
               &dwBytes1,		// Count of bytes locked
               &lpvPtr2,		// Address of wrap around
               &dwBytes2,		// Count of wrap around bytes
               0);				// Flags
	memcpy(lpvPtr1,PCM,dwBytes1);
	if (dwBytes2>0)
		memcpy(lpvPtr2,((byte *)PCM)+dwBytes1,dwBytes2);
	sbuf->Unlock(lpvPtr1, dwBytes1, lpvPtr2, dwBytes2);        
	dest->oemdata = (void *)sbuf;
	setSound3DParams(dest, -1, -1, -1, 0, 0);
}

void setSound3DParams(sound *snd, float beginFalloff, float endFalloff, intf innerangle, intf outerangle, float outsideVolume)
{	LPDIRECTSOUNDBUFFER sbuf = (LPDIRECTSOUNDBUFFER)snd->oemdata;
	if (!sbuf) return;
	LPDIRECTSOUND3DBUFFER sbuf3d;
	sbuf->QueryInterface( IID_IDirectSound3DBuffer, (VOID**)&sbuf3d );
	if (beginFalloff < 0) beginFalloff = horizon;
	if (endFalloff   < 0) endFalloff   = DS3D_DEFAULTMAXDISTANCE;
	sbuf3d->SetMinDistance(beginFalloff,DS3D_DEFERRED);
	sbuf3d->SetMaxDistance(endFalloff,DS3D_DEFERRED);
	if (innerangle>=0)
	{	snd->flags |= sound_reserved1;
		sbuf3d->SetConeAngles(innerangle / 10, outerangle / 10,DS3D_DEFERRED);
	}
	sbuf3d->SetConeOutsideVolume(intf(outsideVolume * (float)(DSBVOLUME_MAX - DSBVOLUME_MIN ) + DSBVOLUME_MIN),DS3D_DEFERRED);
}

// Streamer Interface
HANDLE streameventhandle;					// Handle to the streaming event
LPDIRECTSOUNDBUFFER streambuf;				// pointer to the stream playback buffer
LPDIRECTSOUNDNOTIFY streambuffernotify;		// Notify event for the stream buffer
DSBPOSITIONNOTIFY positionNotification[2];	// Position notification data
dword streampacketsize;						// Size of stream packets
HANDLE streamthreadhandle;					// Handle to streaming thread
DWORD audiostreamthreadid = 0;				// ID number of the streaming thread
intf (*streamfetchfunc)(byte *buffer,intf size);	// pointer to the function to fetch data

void AD_stopstreaming(void)
{	streamingWorks = false;
	if (streambuf) streambuf->Stop();
	if (streameventhandle)  { CloseHandle(streameventhandle);  streameventhandle  = NULL; }
	if (streamthreadhandle) { CloseHandle(streamthreadhandle); streamthreadhandle = NULL; }
	SAFE_RELEASE(streambuffernotify);
	SAFE_RELEASE(streambuf);
}

void AD_pausestreaming(bool pause)
{	if (pause)	streambuf->Stop();
		else	streambuf->Play(0,0,DSBPLAY_LOOPING);
}

uintf laststreampos = 1;

void fillstreambuffer(uintf streampos, uintf streampacketsize)
{	void *buf1,*buf2;
	DWORD size1,size2;

	if(streambuf->Lock(streampos,streampacketsize,
			&buf1,&size1,&buf2,&size2,0)==DSERR_BUFFERLOST) // in bytes
	{	streambuf->Restore();
		streambuf->Lock(streampos,streampacketsize,&buf1,&size1,&buf2,&size2,0);
	}
			
	intf donesize;
	intf todo = size1;
	byte *buf = (byte *)buf1;
	while (todo)
	{	donesize = streamfetchfunc(buf,todo);
		if (donesize>0)	
		{	todo-=donesize;
			buf+=donesize; 
		}	else
		{	streambuf->Unlock(buf1,size1,buf2,size2);
			stopStreaming();
			return;
		}
	}
	todo = size2;
	buf = (byte *)buf2;
	while (todo)
	{	donesize = streamfetchfunc(buf,todo);
		if (donesize>0)	
		{	todo-=donesize; 
			buf+=donesize; 
		}	else
		{	streambuf->Unlock(buf1,size1,buf2,size2);
			stopStreaming();
			return;
		}
	}
	streambuf->Unlock(buf1,size1,buf2,size2);
}

DWORD WINAPI streamerhandler(LPVOID lpParameter)
{	// This is a thread used to control audio streaming	
	// (Thread stored in handle 'streamthreadhandle' and awoken by event 'streameventhandle')
	DWORD streampos;

	while (streamingWorks)
	{	if(WaitForSingleObject(streameventhandle,50)==WAIT_OBJECT_0)
		{	if (streamingWorks)
			{	streamEvents++;
				streambuf->GetCurrentPosition(&streampos,NULL);	// in bytes
				if (streampos<streampacketsize)
					streampos = streampacketsize;
				else
					streampos = 0;
				if (streampos==laststreampos) continue;
				laststreampos = streampos;

				fillstreambuffer(streampos, streampacketsize);
			} // if streamingworks after stream event
		} // if streamevent was a triggering event 
	} // while streamingworks
	streamEvents = 0;
	return 0;
}

bool AD_startstreamer(intf (*fetch)(byte *,intf),dword frequency, dword numchannels, dword packetsize)
{	streambuf = NULL;
	streambuffernotify = NULL;
	streameventhandle = NULL;
	streamthreadhandle = NULL;
	streamfetchfunc = fetch;

	streampacketsize = packetsize;
	WAVEFORMATEX wfx;
	memfill(&wfx,0,sizeof(WAVEFORMATEX));
    wfx.wFormatTag     = WAVE_FORMAT_PCM;
    wfx.nChannels      = (word)numchannels;
    wfx.nSamplesPerSec = frequency;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign    = (wfx.wBitsPerSample * wfx.nChannels) / 8;
    wfx.nAvgBytesPerSec= wfx.nSamplesPerSec*wfx.nBlockAlign;

	memfill(&sbufferdesc,0,sizeof(DSBUFFERDESC));
    sbufferdesc.dwSize	= sizeof(DSBUFFERDESC);
    sbufferdesc.dwFlags	= DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY;
    sbufferdesc.dwBufferBytes=packetsize*2;
    sbufferdesc.lpwfxFormat  =&wfx;

	if(FAILED(dsound->CreateSoundBuffer(&sbufferdesc,&streambuf,NULL)))
	{	return false;
	}

	// Fill the first half of the buffer with the initial data
	fillstreambuffer(0, packetsize);
	
	if (FAILED(streambuf->QueryInterface(IID_IDirectSoundNotify,(LPVOID*)&streambuffernotify)))
	{	return false;
	}

	streameventhandle = CreateEvent(NULL,FALSE,FALSE,"FC Steaming Audio Event");
	if (!streameventhandle)
	{	return false;
	}

	memfill(positionNotification,0,2*sizeof(DSBPOSITIONNOTIFY));
	positionNotification[0].dwOffset    =0;
	positionNotification[0].hEventNotify=streameventhandle;
	positionNotification[1].dwOffset    =packetsize;
	positionNotification[1].hEventNotify=streameventhandle;
	if(FAILED(streambuffernotify->SetNotificationPositions(2,positionNotification)))
	{	return false;
	}
	
	streamingWorks = true;

	streamthreadhandle = CreateThread(NULL,0,streamerhandler,NULL,0,&audiostreamthreadid);
	if (!streamthreadhandle)
	{	return false;
	}

	if (FAILED(streambuf->Play(0,0,DSBPLAY_LOOPING)))
	{	return false;
	}

	return true;
}
