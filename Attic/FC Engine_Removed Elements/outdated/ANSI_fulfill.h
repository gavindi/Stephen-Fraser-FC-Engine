// This file is intended as a fullfilment header for systems that do not appear to offer the full set of ANSI library functions.
// Of cause, I could be wrong, and these functions may be just helper functions provided by MSVC, however, where they are not 
// provided by the platform's libraries, they are provided within the engine itself.
#if defined(PLATFORM_mac)

#define strcmpi(a,b) stricmp(a,b)
int strcmp ( const char * src, const char * dst );
int stricmp ( const char * dst, const char * src );

#endif

