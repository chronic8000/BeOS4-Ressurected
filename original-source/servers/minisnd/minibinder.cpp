
#include "minibinder.h"
#include "miniplay.h"
#include "snddev.h"			//	for g_debug

#include <Locker.h>
#include <Autolock.h>
#include <stdio.h>
#include <string.h>
#include <Debug.h>
#include <PlaySound.h>
#include <Beep.h>

using namespace std;


//#define trace puts(__PRETTY_FUNCTION__);
#define trace

static BLocker sLock("minibinder::sLock");
static minibinder * sBinder;


struct {
	int		what;
	char *	name;
	int		channels;
}
level_map[] = {
	{ miniMainOut,	"volume",	2 },
	{ miniCDIn,		"cd",		2 },
	{ miniPhoneIn,	"modem",	1 },
	{ miniLineIn,	"line",		2 },
	{ miniMicIn,	"mic",		1 },
	{ miniEvent,	"events",	2 },
	{ miniCapture,	"capture",	2 },
};
static const char * kADCSourceName = "source";

static const char * kInfoName = "info";
static const char * kInfo =
"volume, cd, modem, line, mic and events are volume controls which "
"go from 0.0 to 1.0. Negative values mean that they are muted. \n"
"capture sets the input gain in the range 0.0 to 1.0, or for mic "
"sources, 1.0 to 2.0 will add 20 dB of gain. source determines the "
"recording source, with -5 meaning microphone (see miniplay.h for more). \n"
"The beep property can be written to start a named system beep; the "
"numeric value is the negative error code or positive beep handle. \n"
;

static const char * kBeepName = "beep";



void make_binder()
{
	trace
	BAutolock lock(sLock);
	if (sBinder == NULL)
	{
		sBinder = new minibinder;
		sBinder->Acquire();
		status_t err = sBinder->StackOnto(BinderNode::Root()["service"]["audio"]);
		fprintf(stderr, "StackOnto(service/audio) returns %s\n", strerror(err));
	}
}

void break_binder()
{
	trace
	BAutolock lock(sLock);
	if (sBinder != NULL)
	{
		sBinder->Release();
		sBinder = 0;
	}
}

void update_binder_property_vol(int channel, float value, bool mute)
{
	trace
	BAutolock lock(sLock);
	if (sBinder)
	{
		const char * name = 0;
		for (int ix=0; ix<sizeof(level_map)/sizeof(level_map[0]); ix++)
		{
			if (level_map[ix].what == channel)
			{
				name = level_map[ix].name;
				break;
			}
		}
		if (name)
		{
			sBinder->update_property(name, BinderNode::property((double)(mute ? -value : value)));
		}
		else
		{
			fprintf(stderr, "update_binder_value_vol(%d) not known\n", channel);
		}
	}
}

void update_binder_property_adc(int value)
{
	trace
	BAutolock lock(sLock);
	if (sBinder)
	{
		sBinder->update_property(kADCSourceName, BinderNode::property(value));
	}
}




minibinder::minibinder() :
	m_beep(-1)
{
	trace
	status_t err;

	update_property(kInfoName, property(kInfo));

	//	Even though this data item sits in m_beep, we put it
	//	in the cache so that name iteration will work.
	update_property(kBeepName, property(-1));

	//	read the initial values here and put 'em in the array
	for (int ix=0; ix<sizeof(level_map)/sizeof(level_map[0]); ix++)
	{
		float vl, vr;
		bool mute;
		if (0 <= (err = mini_get_volume(level_map[ix].what, &vl, &vr, &mute)))
		{
			update_property(level_map[ix].name, property((double)(mute ? -vl : vl)));
		}
		else
		{
			fprintf(stderr, "mini_get_volume(%s): %s\n", level_map[ix].name, strerror(err));
		}
	}
	int32 source;
	if (0 <= (err = mini_get_adc_source(&source)))
	{
		update_property(kADCSourceName, property((int)source));
	}
	else
	{
		fprintf(stderr, "mini_get_adc_source(): %s\n", strerror(err));
	}
}


minibinder::~minibinder()
{
	trace
}

//	Internally, we changed something, so keep this cache in sync
//	without calling back into the internals as in WriteProperty.
void 
minibinder::update_property(const char *name, const property &value)
{
	trace
	ASSERT(sLock.IsLocked());
	ASSERT(strlen(name) < sizeof(minibinder::info::name));
	if (g_debug & 8)
	{
		fprintf(stderr, "update_property(%s, %f)\n", name, value.Number());
	}
	for (vector<minibinder::info>::iterator ptr(m_properties.begin());
		ptr != m_properties.end();
		ptr++)
	{
		uint32 size = sizeof(static_cast<minibinder::info*>(NULL)->name);
		if (!strncasecmp(name, (*ptr).name, size - 1))
		{
			bool changed = false;
			if ((*ptr).value != value)
			{
				changed = true;
			}
			(*ptr).value = value;
			if (changed)
			{
				if (g_debug & 8)
				{
					fprintf(stderr, "NotifyListeners(B_PROPERTY_CHANGED, '%s')\n", (*ptr).name);
				}
				NotifyListeners(B_PROPERTY_CHANGED, (*ptr).name);
			}
			return;
		}
	}
	minibinder::info i;
	strncpy(i.name, name, sizeof(i.name));
	i.name[sizeof(i.name)-1] = 0;
	i.value = value;
	m_properties.push_back(i);
	if (g_debug & 8)
	{
		fprintf(stderr, "NotifyListeners(B_PROPERTY_ADDED, '%s')\n", i.name);
	}
	NotifyListeners(B_PROPERTY_ADDED, i.name);
}

status_t 
minibinder::OpenProperties(void **cookie, void *copyCookie)
{
	trace
	BAutolock lock(sLock);
	*cookie = (void *)new int32(copyCookie ? *(int32*)copyCookie : 0);
	return B_OK;
}

status_t 
minibinder::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	trace
	BAutolock lock(sLock);
	if (*(int32 *)cookie < 0 || *(int32 *)cookie >= m_properties.size())
	{
		return ENOENT;
	}
	const char * name = m_properties[*(int32 *)cookie].name;
	int l = strlen(name);
	strncpy(nameBuf, name, *len);
	*len = l;
	*(int32 *)cookie += 1;
	return B_OK;
}

status_t 
minibinder::CloseProperties(void *cookie)
{
	trace
	BAutolock lock(sLock);
	delete (int32 *)cookie;
	return B_OK;
}

put_status_t 
minibinder::WriteProperty(const char *name, const property &prop)
{
	trace

	//	beep?
	if (!strcasecmp(name, kBeepName))
	{
		stop_sound(m_beep);
		BString s(prop.String());
		if (g_debug & 8)
		{
			fprintf(stderr, "WriteProperty(%s), %s (%d chars)\n", name, s.String(), (int)s.Length());
		}
		m_beep = system_beep(s.Length() ? s.String() : NULL);
		if (g_debug & 8)
		{
			fprintf(stderr, "m_beep is %ld\n", m_beep);
		}
		return put_status_t(m_beep, false);
	}

	//	don't lock, 'cause we ain't accessin' da props

	//	source mux?
	if (!strcasecmp(name, kADCSourceName))
	{
		if (prop.Type() != BinderNode::property::number)
		{
			if (g_debug)
			{
				fprintf(stderr, "snd_server: property %s requires a number\n", name);
			}
//			return B_BAD_TYPE;
		}
		int s = (int)prop.Number();
		if (s != miniMainOut && s != miniCDIn && s != miniLineIn && s != miniMicIn)
		{
			if (g_debug)
			{
				fprintf(stderr, "invalid adc source: %d\n", s);
			}
			return EINVAL;
		}
		if (g_debug & 8)
		{
			fprintf(stderr, "binder %s source = %d\n", name, s);
		}
		return put_status_t(mini_set_adc_source(s), false);
	}

	//	volume of some sort?
	for (int ix=0; ix<sizeof(level_map)/sizeof(level_map[0]); ix++)
	{
		if (!strcasecmp(name, level_map[ix].name))
		{
			if (prop.Type() != BinderNode::property::number)
			{
				if (g_debug)
				{
					fprintf(stderr, "snd_server: property %s requires a number\n", name);
				}
//				return B_BAD_TYPE;
			}
			float f = prop.Number();
			//	We don't update our internal list just yet. Instead, we queue
			//	an update for the actual service thread, and let that change
			//	cause the cache to update (and send notifications).
			bool mute = false;
			if (f < 0.)
			{
				f = -f;
				mute = true;
			}
			if (g_debug & 8)
			{
				fprintf(stderr, "binder %s volume = %f (%s)\n", name, f, mute ? "muted" : "not muted");
			}
			return put_status_t(mini_adjust_volume(level_map[ix].what, f, f,
				VOL_SET_VOL | (mute ? VOL_SET_MUTE : VOL_CLEAR_MUTE)), false);
		}
	}
	if (g_debug & 8)
	{
		fprintf(stderr, "WriteProperty(%s) falls through\n", name);
	}
	return put_status_t(ENOENT, true);	//	allow chaining
}

get_status_t 
minibinder::ReadProperty(const char *name, property &prop, const property_list &args)
{
	trace
	BAutolock lock(sLock);

	if (!strcasecmp(name, kBeepName))
	{
		prop = property((double)m_beep);
		return get_status_t(B_OK, false);
	}

	for (vector<minibinder::info>::iterator ptr(m_properties.begin());
		ptr != m_properties.end();
		ptr++)
	{
		uint32 size = sizeof(static_cast<minibinder::info*>(NULL)->name);
		if (!strncasecmp((*ptr).name, name, size - 1))
		{
			prop = (*ptr).value;
			if (g_debug & 8)
			{
				fprintf(stderr, "ReadProperty(%s) returns %f\n", name, prop.Number());
			}
			return get_status_t(B_OK);
		}
	}
	if (g_debug & 8)
	{
		fprintf(stderr, "ReadProperty(%s) falls through\n", name);
	}
	return get_status_t(ENOENT, true);	//	allow chaining
}

