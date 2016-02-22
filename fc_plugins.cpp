// ****************************************************************************
// ***							Plugin Manager								***
// ***							--------------								***
// *** Init Dependancies:	Memory, Text									***
// *** Init Function:		fc_plugin *initplugins(void)					***
// *** Term Function:		void killplugins(void)							***
// ***																		***
// ****************************************************************************

/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/

#include "fc_h.h"
#include <bucketSystem.h>

class fcplugin : public fc_plugin
{	public:
	void addGenericPlugin(void *handler, uintf type, const char *name)	{::addGenericPlugin(handler,type, name);};
	bool removeGenericPlugin(const char *name, uintf type)				{return ::removeGenericPlugin(name, type);};
	void *getPluginHandler(const char *name, uintf type)					{return ::getPluginHandler(name, type);};
	bool getFirstPlugin(uintf type, sFindPlugin *fp)				{return ::getFirstPlugin(type, fp);};
	bool getNextPlugin(sFindPlugin *fp)									{return ::getNextPlugin(fp);};
} *OEMplugins;

const char *plugintypenames[] =
{	"Image (Bitmap) Reader",		// PluginType_ImageReader
	"Image (Bitmap) Writer",		// PluginType_ImageWriter
	"Geometry Reader",				// PluginType_GeometryReader	/* Depricated */
	"Geometry Writer",				// PluginType_GeometryWriter	/* Depricated */
	"Sound Reader",					// PluginType_SoundReader
	"Sound Writer",					// PluginType_SoundWriter
	"Audio Streamer",				// PluginType_AudioStream
	"Compression",					// PluginType_Compression
	"File Archive",					// PluginType_Archive
};

#define GenericPluginNameLen 32

struct sGenericPlugin
{	void			*handler;
	char			name[GenericPluginNameLen];
	sGenericPlugin	*next, *prev;
	uintf			flags;
	uintf			plugintype;
} *freesGenericPlugin, *usedsGenericPlugin;

#define PluginFileSpecLen 256
logfile	*pluginlog;
char pluginfilespecs[NumPluginTypes][PluginFileSpecLen];

void initplugins(byte *memPtr)
{	// msg("Engine Initialization Failure","InitPlugins Called before Memory System is Initialized");
	if (!_fctext) msg("Engine Initialization Failure","InitPlugins Called before Text System is Initialized");
	_fcplugin = new fcplugin;
	freesGenericPlugin = NULL;
	usedsGenericPlugin = NULL;
	for (uintf i=0; i<NumPluginTypes; i++)
		pluginfilespecs[i][0]=0;
#ifdef fcdebug
	pluginlog = newlogfile("logs/Plugins.log");
#else
	pluginlog = newlogfile(NULL);
#endif
}	

void killplugins(void)
{	pluginlog->log("Closing down plugin system");
	killbucket(sGenericPlugin, flags, Generic_MemoryFlag32);
	delete pluginlog;
	delete OEMplugins;
}	

bool addGenericPlugin(void *handler, uintf type, const char *name)
{	while (name[0]=='.') name++;
	sGenericPlugin *newplugin;
	if (txtlen(name)>=GenericPluginNameLen) 
	{	pluginlog->log("Warning: Plugin name '%s' is too long, plugin may not function!",name);
		LastEngineWarning = EngineWarning_StringTooLarge;
		EngineWarningParam = (void *)name;
		return false;
	}
	allocbucket(sGenericPlugin, newplugin,flags,Generic_MemoryFlag32,64,"Engine Plugin Management"); 
	newplugin->handler = handler;
	txtcpy(newplugin->name, GenericPluginNameLen, name);
	newplugin->plugintype = type;
	
	if (type<=NumPluginTypes)
	{	// Build up a filespec string
		uintf currlen = txtlen(pluginfilespecs[type]);
		if ((currlen+txtlen(name)+4)<PluginFileSpecLen)
		{	txtcat(pluginfilespecs[type],PluginFileSpecLen,"*");
			txtcat(pluginfilespecs[type],PluginFileSpecLen,name);
			txtcat(pluginfilespecs[type],PluginFileSpecLen,";");
		}
		pluginlog->log("Adding plugin (%s): %s",plugintypenames[type],name);
	}	else
		pluginlog->log("Adding custom plugin (%i): %s",type,name);
	return true;
}

bool removeGenericPlugin(const char *name, uintf type)
{	while (name[0]=='.') name++;
	sGenericPlugin *p = usedsGenericPlugin;
	while (p)
	{	if (p->plugintype==type)
		{	if (txticmp(name, p->name)==0) 
			{	// Found a plugin to delete
				deletebucket(sGenericPlugin, p);
				pluginlog->log("Removing Plugin: %s",name);
				// Remove it from the filespec string
				if (type<=NumPluginTypes)
				{	pluginfilespecs[type][0]=0;
					p = usedsGenericPlugin;
					while (p)
					{	if (p->plugintype==type)
						{	uintf currlen = txtlen(pluginfilespecs[type]);
							if ((currlen+txtlen(name)+3)<255)
							{	txtcat(pluginfilespecs[type],PluginFileSpecLen,"*");
								txtcat(pluginfilespecs[type],PluginFileSpecLen,name);
								txtcat(pluginfilespecs[type],PluginFileSpecLen,";");
							}
						}
						p = p->next;
					}
				}
				return true;
			}
		}
		p = p->next;
	}
	if (type<=NumPluginTypes)
	{	pluginlog->log("Warning: Attempted to remove a (%s) plugin called '%s' but I couldn't find it",plugintypenames[type],name);
	}	else
	{	pluginlog->log("Warning: Attempted to remove a custom plugin (%i) called '%s' but I couldn't find it",type,name);
	}
	return false;
}

void *getPluginHandler(const char *name, uintf type)
{	while (name[0]=='.') name++;
	sGenericPlugin *p = usedsGenericPlugin;
	while (p)
	{	if (p->plugintype==type)
		{	if (txticmp(name, p->name)==0) 
			{	// pluginlog->log("Accessing Plugin: %s",name);
				return p->handler;
			}
		}
		p = p->next;
	}
	return NULL;
}

bool pluginsearch(uintf type, sFindPlugin *fp)
{	sGenericPlugin *p = (sGenericPlugin *)fp->plugin;
	while (p)
	{	if (p->plugintype == type) 
		{	fp->plugin = p;
			fp->description = p->name;
			return true;
		}
		p = p->next;	
	}
	fp->plugin = NULL;
	fp->description = NULL;
	return false;
}

bool getFirstPlugin(uintf type, sFindPlugin *fp)
{	fp->plugin = usedsGenericPlugin;
	return pluginsearch(type, fp);
}

bool getNextPlugin(sFindPlugin *fp)
{	uintf type = ((sGenericPlugin *)fp->plugin)->plugintype;
	fp->plugin = ((sGenericPlugin *)fp->plugin)->next;
	return pluginsearch(type, fp);
}

sEngineModule mod_plugins =
{	"plugins",
	0,
	initplugins,
	killplugins
};
