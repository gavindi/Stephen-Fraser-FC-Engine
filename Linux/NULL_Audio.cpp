/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include "../fc_h.h"

struct channelbucket
{	channelbucket		*next;
	uintf				sound_ID;
	dword				flags;
	void				*OEMdata;
};

bool initSNDHardware(logfile *audiolog, intf *numchannels, void *classfinder)
{	return false;
}

void AD_ShutdownAudio(void)
{
}

uintf AD_GetChannelInfoSize(void)
{	return 0;
}

void AD_setAudio3DParams(float unitsInMeters, float dopplerScale, float rolloffScale)
{
}

void AD_Update(void)
{
}

void AD_PingChannels(channelbucket *b)
{
}

void AD_StopSound(void *OEMInfo)
{
}

bool AD_PlaySound(sound *snd, dword flags, float *matrix, channelbucket *b)
{	return NULL;
}

void AD_NewPCMSound(sound *dest, byte *PCM, uintf bytesize, uintf frequency, bool is16bit)
{
}

void AD_DeleteSound(sound *snd)
{
}

void AD_SetListener(float *m, float3 *velocity)
{
}

void AD_stopstreaming(void)
{
}

void AD_pausestreaming(bool pause)
{
}

bool AD_startstreamer(intf (*fetch)(byte *,intf),dword frequency, dword numchannels, dword packetsize)
{	return false;
}
