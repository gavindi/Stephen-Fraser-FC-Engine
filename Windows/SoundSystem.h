/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
struct sWavFile
{	// A sRecording represents a sound recording in memory (eg: a gunshot sound)
	int 	numSamples;				// Number of samples (length of the recording)
	float	*samples;				// Pointer to samples converted to floats
};

sWavFile *loadWAV(const char *filename);		// load a Wav file
void initSoundSystem(void);                     // Initialise the sound system
void playWavFile(sWavFile *s, float leftVol, float rightVol);
