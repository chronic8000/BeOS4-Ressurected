
#include <Debug.h>
#include <OS.h>
#include <stdio.h>
#include <R3MediaDefs.h>
#include <sound.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <map>

#include "minimix.h"
#include "mixfunc.h"
#include "minilocker.h"
#include "miniautolock.h"
#include "miniapp.h"
#include "mixerinfo.h"
#include "snddev.h"
#include "olddev.h"
#include "gamedev.h"
#include "minibinder.h"

using namespace std;

static std::map<std::string, std::string> g_xArgs;

const char *
find_xarg(const char * arg)
{
	map<string, string>::iterator ptr(g_xArgs.find(string(arg)));
	if (ptr == g_xArgs.end()) return getenv(arg);
	return (*ptr).second.c_str();
}

static void
sig_quit(
	int sig)
{
	g_quit = true;
}

//	not thread safe (to save stack space)
static char *
find_device(
	char * storage)
{
	char * e = strlen(storage)+storage;
	DIR * d = opendir(storage);
	static struct dirent * dent;
	static struct stat st;

	if (!d) {
		fprintf(stderr, "error: opendir(%s): %s\n", storage, strerror(errno));
		return NULL;
	}
	e[0] = '/';
	while ((dent = readdir(d)) != NULL) {
		if (dent->d_name[0] == '.') {
			continue;
		}
		strcpy(&e[1], dent->d_name);
		if (lstat(storage, &st) < 0) {
			fprintf(stderr, "error: stat(%s): %s\n", storage, strerror(errno));
			continue;
		}
		if (S_ISDIR(st.st_mode)) {
			char * r = find_device(storage);
			if (r != NULL) {
				closedir(d);
				return r;
			}
		}
		else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
			closedir(d);
			return storage;
		}
		else {
			fprintf(stderr, "question: what is %s? (%o)\n", storage, st.st_mode);
		}
	}
	e[0] = 0;	//	re-set storage after we mangled it
	closedir(d);
	return NULL;
}

static void
teardown()
{
	//	start quitting
	g_quit = true;
	close_port(g_port);

	//	reap preferences thread (if any)
	delete_sem(g_prefSnooze);
	if (g_prefThread > 0) {
		status_t s;
		wait_for_thread(g_prefThread, &s);
	}

	if (g_serviceThread != find_thread(NULL)) {
		status_t s;
		wait_for_thread(g_serviceThread, &s);
	}
	//	close and remove resources
	delete g_dev;
	g_dev = 0;
}

static status_t
queue_mix_buffer(
	int32 buffer_id)
{
	int32 channel_id;
	cloned_buffer* buffer;
	status_t err = g_dev->find_mix_buffer(buffer_id, &channel_id, &buffer);
	if (err < B_OK)
	{
		fprintf(stderr, "queue_mix_buffer(): find_buffer(): %s\n",
			strerror(err));
	}
	if (buffer->size_used)
	{
		fprintf(stderr, "queue_mix_buffer(): WARNING: buffer %ld still in use!\n",
			buffer_id);
	}
	buffer->size_used = buffer->size;
	return g_mix.QueueBuffer(channel_id, buffer);
}

static void
make_mixchannel(
	new_mixchannel_info * info)
{
//fixme:	We should care about info->i_name in some way...
	port_id reply = info->o_error;
	cloned_buffer** buffers = 0;
	info->o_id = -1;

	// flesh out or reject requested format
	info->o_error = g_dev->validate_new_mix_channel(info);
	if (info->o_error < 0) {
		fprintf(stderr, "error: make_mixchannel(): validate_new_mix_channel(): %s\n", strerror(info->o_error));
		goto send_reply;
	}
	// format must be fully spec'd by now
	ASSERT(info->io_format.format != media_multi_audio_format::wildcard.format);
	ASSERT(info->io_format.frame_rate != media_multi_audio_format::wildcard.frame_rate);
	ASSERT(info->io_format.channel_count != media_multi_audio_format::wildcard.channel_count);
	ASSERT(info->io_format.byte_order == B_MEDIA_HOST_ENDIAN);
	ASSERT(info->io_format.buffer_size > 0);
	// allocate a channel
	info->o_error = g_mix.NewChannel(info);
	if (info->o_error < 0) {
		fprintf(stderr, "error: make_mixchannel(): %s\n", strerror(info->o_error));
		goto send_reply;
	}
	// allocate buffer set
	if(info->io_buffer_count <= 0)
		info->io_buffer_count = 1;
	if(info->io_buffer_count > MAX_MIXCHANNEL_BUFFERS)
		info->io_buffer_count = MAX_MIXCHANNEL_BUFFERS;
	buffers = (cloned_buffer**)alloca(sizeof(cloned_buffer*) * info->io_buffer_count);
	info->o_error = g_dev->alloc_mix_buffers(
		info->o_id, info->io_format.buffer_size, info->io_buffer_count, buffers);
	if (info->o_error < 0)
	{
		fprintf(stderr, "error: make_mixchannel(): alloc_mix_buffers() returns %s\n",
			strerror(info->o_error));
		g_mix.RemoveChannel(info->o_id);
		info->o_id = -1;
		goto send_reply;
	}
	// copy buffer descriptions into reply
	for(int32 n = 0; n < info->io_buffer_count; n++)
		memcpy(&info->o_buffers[n], buffers[n], sizeof(minisnd_buffer));
	
send_reply:
	status_t err = write_port_etc(reply, 0, info, sizeof(*info), B_TIMEOUT, SND_TIMEOUT);
	if (err < 0) {
		fprintf(stderr, "error: make_mixchannel(): write_port_etc() returns %s\n",
			strerror(err));
	}
}

static void
remove_mixchannel(
	int32 ch)
{
	status_t err = g_mix.RemoveChannel(ch);
	if (err < 0) {
		fprintf(stderr, "error: remove_mixchannel(%ld): %s\n", ch, strerror(err));
	}
}

static void
make_capturechannel(
	new_capturechannel_info * info)
{
	status_t err;
	port_id reply = info->o_error;
	info->o_error = g_dev->new_capture_channel(info);
	err = write_port_etc(reply, 0, info, sizeof(*info), B_TIMEOUT, SND_TIMEOUT);
	if (err < B_OK)
	{
		fprintf(stderr, "make_capturechannel(): write_port_etc(): %s\n", strerror(err));
	}
}

static void
remove_capturechannel(
	int32 ch)
{
	status_t err = g_dev->remove_capture_channel(ch);
	if (err < B_OK)
	{
		fprintf(stderr, "make_capturechannel(): remove_capture_channel(): %s\n", strerror(err));
	}
}

static void
get_status(
	port_id port)
{
static char str[8192];
	int l = 0;
	strcpy(str+l, "Channels active:\n");
	l += strlen(str+l);
	g_mix.GetStatus(str+l, 8192-l);
	l += strlen(str+l);
	write_port_etc(port, 0, str, l, B_TIMEOUT, SND_TIMEOUT);
}

static void
suspend_operation()
{
	g_mix.SetSuspended(true);
}

static void
resume_operation()
{
	g_mix.SetSuspended(false);
}

static void
toggle_operation(
	bigtime_t * timeout)
{
	bool toSuspend = !g_mix.IsSuspended();
	g_mix.SetSuspended(toSuspend);
	if ((toSuspend == false) && (*timeout == B_INFINITE_TIMEOUT)) {
		*timeout = 0;
	}
}

static const char *
msgname(
	int code)
{
	static struct {
		int code;
		const char * name;
	} names[] = {
#define N(x) { x, #x }
		N(MAKE_MIX_CHANNEL),
		N(REMOVE_MIX_CHANNEL),
		N(MAKE_CAPTURE_CHANNEL),
		N(REMOVE_CAPTURE_CHANNEL),
		N(GET_STATUS),
		N(SET_VOLUME),
		N(GET_VOLUME),
		N(SET_ADC_SOURCE),
		N(GET_ADC_SOURCE),
		N(SUSPEND_OPERATION),
		N(RESUME_OPERATION),
		N(TOGGLE_OPERATION),
#undef N
	};
	for (int ix=0; ix<sizeof(names)/sizeof(names[0]); ix++)
	{
		if (names[ix].code == code) return names[ix].name;
	}
	return (code >= 0) ? "buffer" : "unknown";
}

static status_t
service_loop(
	void *)
{
	int32 code;
	const size_t size = 256;
	void* ptr = malloc(size);
	status_t err;
	bigtime_t timeout = B_INFINITE_TIMEOUT;

	g_serviceThread = find_thread(NULL);

	while (true) {
		if (g_quit) {
			close_port(g_port);
			if (g_debug & 1) fprintf(stderr, "info: g_quit detected\n");
		}
		err = read_port_etc(g_port, &code, ptr, size, B_ABSOLUTE_TIMEOUT, timeout);
		if (err < 0) {
			if ((err == B_WOULD_BLOCK) || (err == B_TIMED_OUT)) {
				timeout = g_dev->do_mix();
				continue;
			}
			if (err != B_INTERRUPTED) {
				fprintf(stderr, "error: read_port_etc(): %s\n", strerror(err));
				break;
			}
			continue;
		}
		if (g_debug & 4) fprintf(stderr, "message: %d (%s)\n", code, msgname(code));
		switch (code) {
		case MAKE_MIX_CHANNEL:
			make_mixchannel((new_mixchannel_info*)ptr);
			break;
		case REMOVE_MIX_CHANNEL:
			remove_mixchannel(*(int32*)ptr);
			break;
		case MAKE_CAPTURE_CHANNEL:
			make_capturechannel((new_capturechannel_info*)ptr);
			break;
		case REMOVE_CAPTURE_CHANNEL:
			remove_capturechannel(*(int32*)ptr);
			break;
		case GET_STATUS:
			get_status(*(port_id *)ptr);
			break;
		case SET_VOLUME:
			g_dev->set_volume((set_volume_info *)ptr);
			break;
		case GET_VOLUME:
			g_dev->get_volume((get_volume_info *)ptr);
			break;
		case SET_ADC_SOURCE:
			g_dev->set_adc_mux(*(int32*)ptr);
			break;
		case GET_ADC_SOURCE:
			write_port_etc(*(port_id*)ptr, g_dev->get_adc_mux(), 0, 0, B_TIMEOUT, SND_TIMEOUT);
			break;
		case SUSPEND_OPERATION:
			suspend_operation();
			break;
		case RESUME_OPERATION:
			resume_operation();
			if (timeout == B_INFINITE_TIMEOUT) {
				timeout = 0;
			}
			break;
		case TOGGLE_OPERATION:
			toggle_operation(&timeout);
			break;
		default:
			err = queue_mix_buffer(code);
			if(err < 0)
			{
				fprintf(stderr, "warning: queue_mix_buffer(%ld) returns %s\n", code, strerror(err));
			}
			else
			{
				if(timeout == B_INFINITE_TIMEOUT)
					timeout = 0;
			}
			break;
			break;
		}
	}
	if ((err < 0) && (err != B_BAD_PORT_ID)) {
		fprintf(stderr, "info: service_loop() exits: %s\n", strerror(err));
	}
	free(ptr);
	be_app->PostMessage(B_QUIT_REQUESTED);
	return 0;
}


static int
do_setup(
	const char * dev, const media_multi_audio_format & dacFormat)
{
	if (dev == NULL) {
		static char storage[128];
		strcpy(storage, "/dev/audio/game");
		dev = find_device(storage);
		if (dev == NULL) {
			strcpy(storage, "/dev/audio/old");
			dev = find_device(storage);
		}
		if (dev != NULL) {
			fprintf(stderr, "info: using device '%s'.\n", dev);
		}
	}

	if (dev == NULL) {
		fprintf(stderr, "error: could not find any audio devices.\n");
		return -1;
	}
	if (strstr(dev, "/dev/audio/old/")) {
		g_dev = new old_dev;
	}
	else if (strstr(dev, "/dev/audio/game/")) {
		g_dev = new game_dev;
	}
	else {
		fprintf(stderr, "error: unknown protocol for device '%s'\n", dev);
		return -1;
	}
	status_t s = g_dev->setup(dev, dacFormat, dacFormat);
	if (s == B_OK) {
		g_port = create_port(12, PORT_NAME);
		if (g_port < 0) {
			fprintf(stderr, "error: create_port(): %s\n", strerror(g_port));
			s = g_port;
		}
	}
	return s;
}


int
main(
	int argc,
	char * argv[])
{
	if (argv[1] && (argv[1][0] == '-')) {
usage:
		fprintf(stderr, "usage: snd_server [var=value ...]\n");
		fprintf(stderr, "SND_BUFSIZE=%%d; SND_DEVICE=%%s; SND_DEBUG=1/0; SND_EVENTS=1/0; SND_SWAPLR=1/0\n");
		fprintf(stderr, "'old' devices: SND_DEVICE_COPY=%%s\n");
		fprintf(stderr, "'game' devices: SND_BUF_COUNT=%%d; SND_RATE=%%d; SND_CHANNELS=%%d\n");
		return 2;
	}
	media_multi_audio_format dacFormat = media_multi_audio_format::wildcard;
	dacFormat.format = media_multi_audio_format::B_AUDIO_SHORT;
	dacFormat.channel_count = 2;
	dacFormat.byte_order = B_MEDIA_HOST_ENDIAN;
	// conservative default buffer size (512 frames)
	dacFormat.buffer_size = ((dacFormat.format & 0x0f) * dacFormat.channel_count) * 512;

	for (int ix=1; ix<argc; ix++) {
		char * eq = strchr(argv[ix], '=');
		if (eq != 0 && eq > argv[ix]) {
			g_xArgs[string(argv[ix], eq-argv[ix])] = string(eq+1);
		}
		else {
			goto usage;
		}
	}
	{
#if 0
		//	according to this code, dithering is 50% slower than non-dithering
		const char * prstr = find_xarg("SND_PROFILE");
		if (prstr != 0) {
			int cnt = atoi(prstr);
			fprintf(stderr, "info: running %d profile passes\n", cnt);
			int16 buf1[2048];
			int32 buf2[2048];
			int32 dithers[2] = { 0, 0 };
			for (int ix=0; ix<2048; ix++) {
				buf1[ix] = 0;
				buf2[ix] = ((rand()&0x7fff)-0x3fff) << 13;
			}
			thread_info ti1, ti2, ti3;
			get_thread_info(find_thread(NULL), &ti1);
			for (int ix=0; ix<cnt; ix++) {
				convert_32_16(buf2, buf1, 1024);
			}
			get_thread_info(find_thread(NULL), &ti2);
			for (int ix=0; ix<cnt; ix++) {
				convert_32_16_dither(buf2, buf1, 1024, dithers);
			}
			get_thread_info(find_thread(NULL), &ti3);
			fprintf(stderr, "no dither: %Ld us\ndither:    %Ld us\n",
				ti2.user_time-ti1.user_time, ti3.user_time-ti2.user_time);
			return 0;
		}
#endif
		const char * tstr = find_xarg("SND_BUFSIZE");
		if (tstr != 0) {
			int target = atoi(tstr);
			if (target > 0) {
				dacFormat.buffer_size = target;
			}
			else {
				fprintf(stderr, "warning: invalid argument '%s' for SND_BUFSIZE\n", tstr);
			}
		}
		const char * cstr = find_xarg("SND_CHANNELS");
		if (cstr) {
			int channels = atoi(cstr);
			if (channels >= 1 && channels <= 6) {
				dacFormat.channel_count = channels;
			}
			else {
				fprintf(stderr, "warning: invalid argument '%s' for SND_CHANNELS\n", cstr);
			}
		}
		const char * srstr = find_xarg("SND_RATE");
		if (srstr != 0) {
			dacFormat.frame_rate = atof(srstr);
			if (strchr(srstr, 'k') || strchr(srstr, 'K')) {
				dacFormat.frame_rate *= 1000.0f;
				if (fabs(dacFormat.frame_rate - 11000.0f) < 1.0f) dacFormat.frame_rate = 11025.0f;
				else if (fabs(dacFormat.frame_rate - 22000.0f) < 1.0f) dacFormat.frame_rate = 22050.0f;
				else if (fabs(dacFormat.frame_rate - 44000.0f) < 1.0f) dacFormat.frame_rate = 44100.0f;
			}
			if (dacFormat.frame_rate < 4000) {
				fprintf(stderr, "warning: bad argument '%s' for SND_RATE", srstr);
				dacFormat.frame_rate = media_multi_audio_format::wildcard.frame_rate;
			}
		}
		const char * dstr = find_xarg("SND_DEBUG");
		if (dstr && (atoi(dstr) || !strcasecmp(dstr, "yes") || !strcasecmp(dstr, "true"))) {
			g_debug = 1;
			if (atoi(dstr) > 1) g_debug = atoi(dstr);
			fprintf(stderr, "info: SND_DEBUG is %d\n", g_debug);
		}
		const char * estr = find_xarg("SND_EVENTS");
		if (estr && (!atoi(estr) || !strcasecmp(estr, "no") || !strcasecmp(estr, "false"))) {
			fprintf(stderr, "info: SND_EVENTS is false\n");
			g_events = false;
		}
		const char * device = find_xarg("SND_DEVICE");
		if (device != 0) {
			fprintf(stderr, "info: SND_DEVICE is '%s'\n", device);
		}
		if (do_setup(device, dacFormat) < 0) {
			fprintf(stderr, "error: setup(%s) failed, quitting\n", device);
			return 1;
		}
		const char * lrstr = find_xarg("SND_SWAPLR");
		if (lrstr && (atoi(lrstr) || !strcasecmp(lrstr, "yes") || !strcasecmp(lrstr, "true"))) {
			fprintf(stderr, "info: SND_SWAPLR is true\n");
			g_convert_func = convert_32_16_swap;
		}
	}

	(void)new MiniApp;

	signal(SIGINT, sig_quit);
	signal(SIGHUP, sig_quit);

	resume_thread(spawn_thread(service_loop, "service_loop", 115, 0));

	make_binder();

	be_app->Run();

	break_binder();

	teardown();
//	free(mixbuf);

	return 0;
}
