// ------------------------- Background Loader: Depricated ------------------------------
extern bool	backgroundloadersupported;	// Informs your application if the background loader is supported on this system.  If it is not, any calls to the background loader will load the file immediately, pausing your application while it is loading
#define BackgroundLoadFilenameLen 256

struct backgroundloaddata					// The background loader can load a file in the background while the CPU continues execution
{	char filename[BackgroundLoadFilenameLen];// Filename to load.  The name is copied here.
	dword filesize;							// Size in bytes of the file
	dword loaded;							// The number of bytes currently loaded (useful for a progress indicator)
	void  (*finished)(backgroundloaddata *);// A function to call when loading has been completed
	void  *userdata;						// A pointer to some user data.  This pointer is optional, but can be used to pass additional data to the callback function
	byte  *buffer;							// Pointer to the buffer containing the loaded data
	dword flags;							// Any special flags, the engine uses a few flags internally which you should not play with.
	backgroundloaddata *next,*prev;			// Points to next and previous record (internal engine use only)
};
backgroundloaddata *backgroundload(char *flname,void *userdata,void (*_finished)(backgroundloaddata *));	// Loads a file in the background, allowing the CPU to continue on.  There will be some speed loss on system memory, and disk access, and a minor CPU speed penalty while background loading is occuring.  See programming manual for additional information.  The file is placed on a queue and loaded when the device is ready, and all previous load requests have been filled (First come, first served basis).
// In the event that background loading is not supported on the system (backgroundloadersupported = false), the file will be loaded immediately, the callback function will be called, (with a valid baclgroundloaddata structure) and then the function will return NULL.
											

long MountArchive(char *filename);			// Mounts an archive into the application's local directory
void UnmountArchive(long handle);			// Unmounts an archive
void filefind(char *filespec);				// Start searching for files by this name
char *findnext(void);						// Aquire the name of the next file matching the filespec specified in filefind
char *findnextdir(void);					// Aquire the name of the next directory matching the filespec specified in filefind
