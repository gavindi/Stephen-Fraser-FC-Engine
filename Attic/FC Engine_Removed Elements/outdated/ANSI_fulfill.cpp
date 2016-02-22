#include <fcio.h>

#if defined(PLATFORM_mac)

int strcmp ( const char * src, const char * dst )
{	int result = 0 ;
	while ((!result) && *dst)
	{	result = *(byte *)src++ - *(byte *)dst++);
	}
	return( result );
}

int stricmp ( const char * dst, const char * src )
{	int result = 0 ;
	while ((!result) && *dst)
	{	result = tolower_array[*(byte *)src++] - tolower_array[*(byte *)dst++)];
	}
	return( result );
}

#endif