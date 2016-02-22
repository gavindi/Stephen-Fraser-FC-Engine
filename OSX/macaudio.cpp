/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include "fc_h.h"
#include <unistd.h>
#include <libmikmod-coreaudio/mikmod.h>
MODULE *module;

void killaudio(void)
{	if (!audioworks) return;
	Player_Stop();
	Player_Free(module);
    MikMod_Exit();
}

void initaudio(long numchannels)
{	/* register all the drivers */
    MikMod_RegisterAllDrivers();
    /* register all the module loaders */
    MikMod_RegisterAllLoaders();

    /* initialize the library */
    md_mode |= DMODE_SOFT_MUSIC;
    if (MikMod_Init("")) {
        fprintf(stderr, "Could not initialize sound, reason: %s\n",
                MikMod_strerror(MikMod_errno));
        return;
    }
    MikMod_SetNumVoices(numchannels>>1,numchannels>>1);
	module = Player_Load("TanksData/music.mod", numchannels>>1, 0);
    if (module) 
	{	/* start module */
        md_musicvolume = 64;
		Player_Start(module);
		audioworks = true;
	}
}

void updatelistener(void)
{
}

long soundsplaying(void)
{	long result = 0;
	return result;
}

void stopsound(void *killb)
{
}

void audio_update(void)
{	if (audioworks)
		MikMod_Update();
}

// ******************* newsound API function *********************
sound *newsound(void)
{	return NULL;
}

sound *newsound(char *flname)
{	SAMPLE *sample = Sample_Load(flname);
	sound *result = (sound *)fcalloc(sizeof(sound),flname);
	result->oemdata = (void *)sample;
	result->rate = sample->speed;
	result->length = sample->length;
	result->flags = 0x80000000;
	if (sample->flags & SF_16BITS)
		result->flags |= sound_16bit;
	if (sample->flags & SF_STEREO)
		result->flags |= 2;
	else
		result->flags |= 1;
	return result;
}

sound *newsound(byte *PCM,dword bytesize,dword frequency,bool is16bit)
{	return NULL;
}


void deletesound(sound *snd)
{
}

void *playsound(sound *snd,dword flags,float *matrix)
{	Sample_Play((SAMPLE *)snd->oemdata,0,0);
	return NULL;
}


// ******************* midiinit API function ***************************
void midiinit(void)
{
}

// ******************* midiplay API function ***************************
void midiplay(char *flname)
{
}

// ******************* midistop API function ***************************
void midistop(void)
{
}

// ******************* midiclose API function ***************************
void midiclose(void)
{
}

void modinit(void)
{
}

void modplay(char *){}
void modstop(void){}
void mod_nextposition(void){}
void mod_prevposition(void){}
void mod_setposition(long){}
void modclose(void){}


