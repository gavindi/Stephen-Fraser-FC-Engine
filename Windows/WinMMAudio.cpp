/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <windows.h>
//#include <stdio.h>
#include <mmsystem.h>
#include <fcio.h>

#define BUFFERSIZE	30				// buffer size in milliseconds (smaller numbers introduce crackle, larger numbers introduce lag)

/*
    ----------------------------------------
                    NOTES
    ----------------------------------------
    There is no point reading devcaps - it provides the device name, and the rest of the data is essentially useless

*/

void initSWAudioMixer(uintf samplesPerSecond, uintf samplesPerBuffer, uintf bitsPerSample, uintf numSpeakers);
void audioMixer(void);                  // Prototype for the generic software mixer
void audioMixTo16(int16 *outputBuffer, uintf numSamples); // Prototype for the function that converts the mixer data to signed 16 bit

HWAVEOUT	WinMM_hwaveout;			// Handle to the WaveOut device
WAVEHDR		WinMM_header[2];		// Wave Headers - describe the audio contained within the buffers
byte		*WinMM_buffer[2];		// pointers to playback buffers
intf		WinMM_mixFreq;			// frequency of the target audio (44100)
intf       	nextBuffer;				// next buffer to be mixed
intf		bufferSizeBytes;		// buffer size in bytes
intf		bufferSizeSamples;		// buffer size in samples
intf		audioSemaphore;         // Tells us when it's safe to handle hardware Interrupts
intf        audioNumSpeakers;       // Number of speakers playing sudio

intf	AudioUnsafeIRQ = 0;

void audioError(const char *func, MMRESULT result)
{	// This function takes error messages generated by the Multimedia system and generates readable text
	const char *text="Unknown Error";
	switch (result)
	{	case MMSYSERR_NOERROR:		return;
		case MMSYSERR_BADDEVICEID:	text = "Specified device identifier is out of range.";	break;
		case MMSYSERR_NODRIVER:		text = "No device driver is present.";					break;
		case MMSYSERR_NOMEM:		text = "Unable to allocate or lock memory.";			break;
		case MMSYSERR_NOTENABLED:	text = "driver failed enable";							break;
		case MMSYSERR_ALLOCATED:	text = "device already allocated";						break;
		case MMSYSERR_INVALHANDLE:	text = "device handle is invalid";						break;
		case MMSYSERR_NOTSUPPORTED:	text = "function isn't supported";						break;
		case MMSYSERR_BADERRNUM:	text = "error value out of range";						break;
		case MMSYSERR_INVALFLAG:	text = "invalid flag passed";							break;
		case MMSYSERR_INVALPARAM:	text = "invalid parameter passed";						break;
		case MMSYSERR_HANDLEBUSY:	text = "handle being used simultaneously on another thread (eg callback)";		break;
		case MMSYSERR_INVALIDALIAS:	text = "specified alias not found";						break;
		case MMSYSERR_BADDB:		text = "bad registry database";							break;
		case MMSYSERR_KEYNOTFOUND:	text = "registry key not found";						break;
		case MMSYSERR_READERROR:	text = "registry read error";							break;
		case MMSYSERR_WRITEERROR:	text = "registry write error";							break;
		case MMSYSERR_DELETEERROR:	text = "registry delete error";							break;
		case MMSYSERR_VALNOTFOUND:	text = "registry value not found";						break;
		case MMSYSERR_NODRIVERCB:	text = "driver does not call DriverCallback";			break;
	}
	if (audioLog) audioLog->log("Audio Error occured calling function %s: %s\n",func,text);
}

static void CALLBACK WinMM_CallBack(HWAVEOUT hwo,UINT uMsg,DWORD dwInstance,DWORD dwParam1,DWORD dwParam2)
{	audioSemaphore++;		// Indicate that it is not safe to receive a hardware interrupt
	if (uMsg==WOM_DONE)		// Only react to WOM_DONE signals (playing a buffer has finished)
	{	if (audioSemaphore>1)
		{	// It's not safe to handle a hardware interrupt now, fail gracefully (this causes stuttering audio)
			waveOutWrite(WinMM_hwaveout,&WinMM_header[nextBuffer],sizeof(WAVEHDR));	// Keep audio playing no matter what
			nextBuffer=1-nextBuffer;
			audioSemaphore--;
			AudioUnsafeIRQ++;
			return;
		}
		audioMixer();
		audioMixTo16((int16*)WinMM_buffer[nextBuffer], bufferSizeSamples * audioNumSpeakers);
		waveOutWrite(WinMM_hwaveout,&WinMM_header[nextBuffer],sizeof(WAVEHDR));	// Send sound buffer to the audio hardware
		nextBuffer=1-nextBuffer;									// Flip the buffer number (0,1,0,1,0,1,etc)
	}
	audioSemaphore--;	// Indicate that it is now safe to receive hardware interrupts again
	return;
}

void initWinMM(void)
{   WAVEFORMATEX	wfe;		// Format of the mixing buffers we will be sending to the audio hardware to play
	MMRESULT		mmr;		// Temporary container to hold the result of Multimedia function calls

	audioLog->log("Initializing WinMM Sound System");
	int numDevs = waveOutGetNumDevs();	    // Retrieve the number of multimedia sound playing devices in the system
	audioLog->log("%i WaveOut devices detected\n",numDevs);
	if (numDevs==0) return;					// No audio devices detected, or an error occured
    audioNumSpeakers = 2;
	WinMM_mixFreq = 44100;
	uint16	sampleSize = 2 * audioNumSpeakers;	// 16 bit stereo  (16 bits * number of speakers)
	bufferSizeBytes = WinMM_mixFreq*sampleSize*BUFFERSIZE/1000;	// Calculate the size of the buffers in BYTES
	bufferSizeSamples = WinMM_mixFreq*BUFFERSIZE/1000;			// Calculate the size of the buffers in SAMPLES

    initSWAudioMixer(WinMM_mixFreq,bufferSizeSamples,16,audioNumSpeakers);

	wfe.wFormatTag = WAVE_FORMAT_PCM;		// We are genertating uncompressed Pulse-Code Modulation ...
	wfe.nChannels = audioNumSpeakers;		// In stereo (2 channels)
	wfe.nSamplesPerSec=WinMM_mixFreq;		// at 44 Khz
	wfe.nAvgBytesPerSec=WinMM_mixFreq * sampleSize;	// Consuming 44100 * 4 bytes per second
	wfe.nBlockAlign=sampleSize;				// 1 block = size of each sample * number of channels
	wfe.wBitsPerSample=16;					// 16 bit audio
	wfe.cbSize=0;							// size of extensible data ... plain PCM is fine, no extensible data needed

	// Open a handle to the WaveOut device (like files, we must open a device before accessing it)
	mmr=waveOutOpen( &WinMM_hwaveout,		// &hwaveout points to where we want to store the handle to the WaveOut device
					 WAVE_MAPPER,			// We want the system's DEFAULT playback device (you should ALWAYS use this device)
					 &wfe,					// Points to the format description of the sound we will be playing
					 (DWORD_PTR)WinMM_CallBack, // Points to the function to call when we finish playing a buffer
					 0,						// A number we send to the callback function, can be anything we like
					 CALLBACK_FUNCTION);	// What special features of playback do we require ... only one: call our callback function
	if (mmr!=MMSYSERR_NOERROR)
	{	audioError("waveOutOpen",mmr);	// Report any error that occured
		return;
	}

	byte *tmp = (byte *)fcalloc(bufferSizeBytes*2,"WinMM Audio Buffers");
	for (int i=0; i<2; i++)		// For each buffer (there will only ever be 2 buffers)
	{	WinMM_buffer[i] = tmp + bufferSizeBytes * i;						// Allocate memory for buffer
		memfill(WinMM_buffer[i], 0, bufferSizeBytes);						// Fill the buffer with silence
		WinMM_header[i].lpData = (LPSTR)WinMM_buffer[i];					// Tell the header where the buffer is located
		WinMM_header[i].dwBufferLength = bufferSizeBytes;					// Tell the header the size of the buffer
		mmr=waveOutPrepareHeader(WinMM_hwaveout,&WinMM_header[i],sizeof(WAVEHDR));	// Prepare the header (Make the sound system aware that it will need to play this type of sound)
		if (mmr!=MMSYSERR_NOERROR)
		{	audioError("waveOutOpen",mmr);						// Report any errors
			return;
		}
	}
	nextBuffer=0;														// The next buffer to process is buffer #0

	audioSemaphore = 0;									// Tell the system it's save to receive hardware interrupts

	// Force sound system to start playing sounds (Sound begins playing the moment the first buffer is written to the device)
	mmr=waveOutWrite(WinMM_hwaveout,&WinMM_header[0],sizeof(WAVEHDR));	// Write the first buffer (will be silent)
	if (mmr!=MMSYSERR_NOERROR)
	{	audioError("waveOutWrite",mmr);						// Report any errors
		return;
	}
	mmr=waveOutWrite(WinMM_hwaveout,&WinMM_header[1],sizeof(WAVEHDR));	// Write the second buffer (will be silent)
	if (mmr!=MMSYSERR_NOERROR)
	{	audioError("waveOutWrite",mmr);						// Report any errors
		return;
	}

}
