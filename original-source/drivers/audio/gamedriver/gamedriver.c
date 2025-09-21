//	TODO
//	Implement rest of ioctl()s
//	MIDI
//	Gameport
//	Possibly "beep" (gunshot) support


#include <KernelExport.h>
#include <OS.h>
#include <Drivers.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <MediaDefs.h>	//	for B_MEDIA_BAD_FORMAT
#include <math.h>
#include <driver_settings.h>
#include <config_manager.h>

#include "gamedriver.h"
#include "debug.h"


#define DEBUG_ACTIVE_LOCK 0
#define DEBUG_ACTIVE_SPINLOCK 0
#define DEBUG_GLOBAL_LOCK 0


int32 api_version = B_CUR_DRIVER_API_VERSION;
int g_debug = 0;

status_t dev_open(const char * i_name, uint32 i_flags, void ** o_cookie);
status_t dev_close(void * i_cookie);
status_t dev_free(void * i_cookie);
status_t dev_control(void * i_cookie, uint32 i_code, void * io_data, size_t i_size);
status_t dev_read(void * i_cookie, off_t i_pos, void * o_buf, size_t * io_size);
status_t dev_write(void * i_cookie, off_t i_pos, const void * i_buf, size_t * io_size);

extern device_hooks m_hooks;
extern device_hooks j_hooks;

device_hooks s_hooks = {
	dev_open,
	dev_close,
	dev_free,
	dev_control,
	dev_read,
	dev_write
};

pci_module_info * g_pci;
isa_module_info * g_isa;
usb_module_info * g_usb;
config_manager_for_driver_module_info * g_pnp;
generic_mpu401_module * g_mpu401;
int32 g_mpu401_count;
generic_gameport_module * g_gameport;
int32 g_gameport_count;

static struct _ctrl_info
{
	const char *	name;
	size_t			size;
}
ctrl_info[] =
{
	{ "GAME_GET_INFO", sizeof(game_get_info) },
/* get info on specific inputs/outputs, and configure them */
	{ "GAME_GET_CODEC_INFOS", sizeof(game_get_codec_infos) },
	{ "_GAME_UNUSED_OPCODE_0_", 0 },
	{ "GAME_SET_CODEC_FORMATS", sizeof(game_set_codec_formats) },
/* mixer information */
	{ "GAME_GET_MIXER_INFOS", sizeof(game_get_mixer_infos) },
	{ "GAME_GET_MIXER_CONTROLS", sizeof(game_get_mixer_controls) },
	{ "GAME_GET_MIXER_LEVEL_INFO", sizeof(game_get_mixer_level_info) },
	{ "GAME_GET_MIXER_MUX_INFO", sizeof(game_get_mixer_mux_info) },
	{ "GAME_GET_MIXER_ENABLE_INFO", sizeof(game_get_mixer_enable_info) },
/*	we specifically omit specific control connection info for now	*/
	{ "GAME_MIXER_UNUSED_OPCODE_1_", 0 },
/* mixer control values */
	{ "GAME_GET_MIXER_CONTROL_VALUES", sizeof(game_get_mixer_control_values) },
	{ "GAME_SET_MIXER_CONTROL_VALUES", sizeof(game_set_mixer_control_values) },
/* stream management */
	{ "GAME_OPEN_STREAM", sizeof(game_open_stream) },
	{ "GAME_GET_TIMING", sizeof(game_get_timing) },
	{ "_GAME_UNUSED_OPCODE_2_", 0 },
	{ "GAME_GET_STREAM_CONTROLS", sizeof(game_get_stream_controls) },
	{ "GAME_SET_STREAM_CONTROLS", sizeof(game_set_stream_controls) },
	{ "_GAME_UNUSED_OPCODE_3_", 0 },
	{ "GAME_SET_STREAM_BUFFER", sizeof(game_set_stream_buffer) },
	{ "GAME_RUN_STREAMS", sizeof(game_run_streams) },
	{ "GAME_CLOSE_STREAM", sizeof(game_close_stream) },
/* buffers tied to streams (looping buffer, typically) */
	{ "GAME_OPEN_STREAM_BUFFERS", sizeof(game_open_stream_buffers) },
	{ "GAME_CLOSE_STREAM_BUFFERS", sizeof(game_close_stream_buffers) },
/* save/load state */
	{ "GAME_GET_DEVICE_STATE", sizeof(game_get_device_state) },
	{ "GAME_SET_DEVICE_STATE", sizeof(game_set_device_state) },
/* extensions (interfaces) */
	{ "GAME_GET_INTERFACE_INFO", sizeof(game_get_interface_info) },
	{ "GAME_GET_INTERFACES", sizeof(game_get_interfaces) },
};


#define IGNORE(x) (void)(x)

/*	g_loaded_modules are locked as they are referenced	*/
#define LOCK_ACTIVE(ap) CHECK_NO_LOCK((ap)->locked) \
	for (benlock_active(ap DEBUG_ARGS2), (ap)->locked=LOCKVAL; \
		(ap)->locked || unbenlock_active(ap DEBUG_ARGS2); \
		(ap)->locked=0)

/*	don't call into plug-in while holding this spinlock!	*/
#define SPINLOCK_ACTIVE(ap) \
	for (spinlock_active((ap) DEBUG_ARGS2), (ap)->spinlocked=LOCKVAL; \
		(ap)->spinlocked || unspinlock_active(ap DEBUG_ARGS2); \
		(ap)->spinlocked=0)


/*	plugs/names are only accessed from normal threads	*/
static ga_loaded_module * g_loaded_modules;
static const char ** g_device_names;

static sem_id g_globals_sem;
static int32 g_globals_ben;
static int32 g_globals_temp;

#define LOCK_GLOBALS CHECK_NO_LOCK(g_globals_temp) \
	for (benlock_globals(DEBUG_ARGS), g_globals_temp=LOCKVAL; \
		g_globals_temp || unbenlock_globals(DEBUG_ARGS); \
		g_globals_temp=0)


static void
benlock_globals(
	DEBUG_ARGS_DECL)
{
#if DEBUG_GLOBAL_LOCK
	lddprintf(("%ld \033[36m>>> GLOBALS\033[0m %s %d\n", find_thread(NULL), file, line));
#endif
	if (atomic_add(&g_globals_ben, -1) < 1) {
		(void)acquire_sem(g_globals_sem);	/* no CAN_INTERRUPT because we ALWAYS release the lock going out */
	}
}

static bool
unbenlock_globals(
	DEBUG_ARGS_DECL)
{
#if DEBUG_GLOBAL_LOCK
	lddprintf(("%ld \033[36m<<< GLOBALS\033[0m %s %d\n", find_thread(NULL), file, line));
#endif
	if (atomic_add(&g_globals_ben, 1) < 0) {
		(void)release_sem_etc(g_globals_sem, 1, B_DO_NOT_RESCHEDULE);
	}
	return false;
}

int32 alcnt;

static void
benlock_active(
	ga_loaded_module * ap
	DEBUG_ARGS_DECL2)
{
#if DEBUG_ACTIVE_LOCK
	lddprintf(("%ld %ld \033[32m>>> COOKIE\033[0m %p %s %d\n", atomic_add(&alcnt, 1), find_thread(NULL), ap, file, line));
#endif
	if (atomic_add(&ap->ben, -1) < 1) {
		(void)acquire_sem(ap->sem);	/*	no CAN_INTERRUPT because we ALWAYS release the sem going out */
	}
}

static bool
unbenlock_active(
	ga_loaded_module * ap
	DEBUG_ARGS_DECL2)
{
#if DEBUG_ACTIVE_LOCK
	lddprintf(("%ld %ld \033[32m<<< COOKIE\033[0m %p %s %d\n", atomic_add(&alcnt, 1), find_thread(NULL), ap, file, line));
#endif
	if (atomic_add(&ap->ben, 1) < 0) {
		(void)release_sem_etc(ap->sem, 1, B_DO_NOT_RESCHEDULE);
	}
	return false;
}

static void
spinlock_active(
	ga_loaded_module * ap
	DEBUG_ARGS_DECL2)
{
	cpu_status cp;
#if DEBUG_ACTIVE_SPINLOCK
	lddprintf(("\033[35m>>> COOKIESPIN\033[0m %p %s %d\n", ap, file, line));
#endif
	cp = disable_interrupts();
	acquire_spinlock(&ap->spinlock);
	ap->cpu = cp;
}

static bool
unspinlock_active(
	ga_loaded_module * ap
	DEBUG_ARGS_DECL2)
{
	cpu_status cp = ap->cpu;
	release_spinlock(&ap->spinlock);
	restore_interrupts(cp);
	/*	sequencing may be off, because we print outside the lock to avoid	*/
	/*	delayed dprintf()-ing	*/
#if DEBUG_ACTIVE_SPINLOCK
	lddprintf(("\033[35m<<< COOKIESPIN\033[0m %p %s %d\n", ap, file, line));
#endif
	return false;
}


static ga_active_stream_info *
find_stream(
	ga_cookie * pc,
	int32 stream)
{
	int ordinal = GAME_STREAM_ORDINAL(stream);
	int ix;

	if (!GAME_IS_STREAM(stream)) {
		return 0;
	}

	ASSERT_LOCKED(pc->module);

	for (ix=0; ix<pc->module->adc_count; ix++) {
		if (ordinal < pc->module->adc_infos[ix].max_stream_count) {
			return pc->module->adc_streams[ix]+ordinal;
		}
		ordinal -= pc->module->adc_infos[ix].max_stream_count;
	}
	for (ix=0; ix<pc->module->dac_count; ix++) {
		if (ordinal < pc->module->dac_infos[ix].max_stream_count) {
			return pc->module->dac_streams[ix]+ordinal;
		}
		ordinal -= pc->module->dac_infos[ix].max_stream_count;
	}
	eprintf(("\033[31mgamedriver: find_stream() on bad stream id %ld\033[0m\n", stream));
	return NULL;
}

static game_codec_info *
find_codec(
	ga_cookie * pc,
	int codec)
{
	int ordinal = GAME_DAC_ORDINAL(codec);

	if (codec == GAME_NO_ID) {
		return NULL;
	}

	// assert_locked_somehow
	ASSERT(
		pc->module->locked == find_thread(NULL) ||
		pc->module->spinlocked == find_thread(NULL));

	if (GAME_IS_ADC(codec)) {
		if (ordinal < 0 || ordinal >= pc->module->adc_count) {
			return NULL;
		}
		return pc->module->adc_infos+ordinal;
	}
	else if (GAME_IS_DAC(codec)) {
		if (ordinal < 0 || ordinal >= pc->module->dac_count) {
			return NULL;
		}
		return pc->module->dac_infos+ordinal;
	}
	eprintf(("\033[31mgamedriver: find_codec(%d) with bad ID\033[0m\n", codec));
	return NULL;
}

static ga_mixer_info *
find_mixer(
	ga_cookie * pc,
	int mixer)
{
	int ordinal = GAME_MIXER_ORDINAL(mixer);

	if (!GAME_IS_MIXER(mixer) || ordinal >= pc->module->mixer_count)
	{
		eprintf(("\033[31mgamedriver: find_mixer(%d) with bad ID\033[0m\n", mixer));
		return NULL;
	}
	ASSERT(pc->module->mixer_infos[ordinal].info.mixer_id == mixer);
	return &pc->module->mixer_infos[ordinal];
}

static uint32
find_sample_size(
	uint32 fmt)
{
	fmt &= ~(B_FMT_IS_GLOBAL | B_FMT_SAME_AS_INPUT);
	if (!fmt) return 0;
	if (fmt <= B_FMT_8BIT_U) return 1;
	if (fmt <= B_FMT_16BIT) return 2;
	if (fmt <= B_FMT_FLOAT) return 4;
	if (fmt == B_FMT_DOUBLE) return 8;
	if (fmt == B_FMT_EXTENDED) return 10;
	eprintf(("\033[31mgamedriver: find_sample_size(0x%lx) failed\033[0m\n", fmt));
	return 0;
}

static struct
{
	uint32		flag;
	float		rate;
}
fixed_rates[] =
{
	{ B_SR_8000, 8000. },
	{ B_SR_11025, 11025. },
	{ B_SR_12000, 12000. },
	{ B_SR_16000, 16000. },
	{ B_SR_22050, 22050. },
	{ B_SR_24000, 24000. },
	{ B_SR_32000, 32000. },
	{ B_SR_44100, 44100. },
	{ B_SR_48000, 48000. },
	{ B_SR_64000, 64000. },
	{ B_SR_88200, 88200. },
	{ B_SR_96000, 96000. },
	{ B_SR_176400, 176400. },
	{ B_SR_192000, 192000. },
	{ B_SR_384000, 384000. },
	{ B_SR_1536000, 1536000. },
};

static float
find_frame_rate(
	uint32 flags)
{
	int ix;

	for (ix=0; ix<sizeof(fixed_rates)/sizeof(fixed_rates[0]); ix++)
	{
		if (flags & fixed_rates[ix].flag)
		{
			return fixed_rates[ix].rate;
		}
	}
	eprintf(("\033[31mgamedriver: find_frame_rate(0x%lx) failed\033[0m\n", flags));
	return 0.;
}

static uint32
find_frame_rate_code(
	float rate,
	float diff)
{
	int ix;

	for (ix=0; ix<sizeof(fixed_rates)/sizeof(fixed_rates[0]); ix++)
	{
		if (fabs(rate-fixed_rates[ix].rate) <= diff)
		{
			return fixed_rates[ix].flag;
		}
	}
	return B_SR_CVSR;
}


static void
ga_update_timing(
	void * cookie,
	size_t frames,
	bigtime_t at_time)
{
	ga_active_stream_info * asip = (ga_active_stream_info *)cookie;

	ASSERT(asip->feed.cookie == asip);

	SPINLOCK_ACTIVE(asip->owner->module)
	{
		//	calculate timing info
		size_t next_frame;
		size_t page_frames = asip->page_frame_count;
		float rate = frames*1000000.0/(at_time-asip->timing.at_time);
		if (rate < asip->timing.cur_frame_rate/1.4)
		{
			rate = asip->timing.cur_frame_rate/1.4;
		}
		else if (rate > asip->timing.cur_frame_rate*1.4)
		{
			rate = asip->timing.cur_frame_rate*1.4;
		}
		asip->timing.cur_frame_rate = (asip->timing.cur_frame_rate*0.98+rate*0.02);
		asip->timing.at_time = at_time;

		if (page_frames)
		{
			while (frames > 0) {
				next_frame = page_frames - (asip->timing.frames_played % page_frames);
				if (next_frame > frames)
					next_frame = frames;
				else
					asip->release_count += 1;
				asip->timing.frames_played += next_frame;
				frames -= next_frame;
			}
		}
		else
		{
			asip->timing.frames_played += frames;
		}
	}
}

static long
ga_int_done(
	void * cookie)
{
	ga_active_stream_info * asip = (ga_active_stream_info *)cookie;
	long ret = B_HANDLED_INTERRUPT;

	ASSERT(asip->feed.cookie == asip);
	//	if the stream has any release count, release the semaphore
	if (asip->release_count > 0) {
//kprintf("%ld\n", asip->release_count);
		(void)release_sem_etc(asip->info.stream_sem, asip->release_count,
				B_DO_NOT_RESCHEDULE);
		asip->release_count = 0;
		ret = B_INVOKE_SCHEDULER;
	}
	return ret;
}

static ga_active_buffer_info *
find_buffer(
	ga_cookie * pc,
	int32 buffer)
{
	int ordinal;

	if (!GAME_IS_BUFFER(buffer)) {
		ddprintf(("gamedriver: %ld is not a buffer ID\n", buffer));
		return NULL;
	}
	ordinal = GAME_BUFFER_ORDINAL(buffer);
	if (ordinal < 0 || ordinal >= pc->module->buffer_count) {
		ddprintf(("gamedriver: ordinal %d, buffer_count %d\n", ordinal, pc->module->buffer_count));
		return NULL;
	}
	if (pc->module->buffers[ordinal>>BLOCK_SHIFT][ordinal&BLOCK_MASK].owner != pc) {
		eprintf(("gamedriver: find_buffer(%ld) finds buffer with other owner (%p != %p)\n",
				buffer, pc->module->buffers[ordinal>>BLOCK_SHIFT][ordinal&BLOCK_MASK].owner, pc));
		return NULL;
	}
	return &pc->module->buffers[ordinal>>BLOCK_SHIFT][ordinal&BLOCK_MASK];
}

static ga_utility_funcs g_utility =
{
	sizeof(ga_utility_funcs),
	find_sample_size,
	find_frame_rate,
	find_frame_rate_code,
};

static void
push_loaded_module(
	plug_api * gdi,
	char * path)
{
	char devpath[36];
	ga_loaded_module * ap;
	int cnt = 1;

	if (strlen(gdi->short_name) > 19)
	{
		eprintf(("\033[35mgamedriver: short_name '%s' is too long (19 chars max)\033[0m\n", gdi->short_name));
		//fixme:	fix it by clobbering his string storage (which could be bad)
		((char *)gdi->short_name)[19] = 0;
	}
	sprintf(devpath, "audio/game/%s", gdi->short_name);
	ASSERT(strlen(devpath) < sizeof(ap->devpath) - 2);
	LOCK_GLOBALS
	{
		for (ap = g_loaded_modules; ap != NULL; ap = ap->next)
		{
			if (!strncmp(devpath, ap->devpath, strlen(devpath)))
				cnt++;
		}
		sprintf(devpath, "audio/game/%s/%d", gdi->short_name, cnt);
		ddprintf(("gamedriver: device name is %s\n", devpath));
		ap = (ga_loaded_module *)calloc(1, sizeof(ga_loaded_module));
		strncpy(ap->module, path, sizeof(ap->module));
		ap->module[sizeof(ap->module)-1] = 0;
		strncpy(ap->devpath, devpath, sizeof(ap->devpath));
		ap->devpath[sizeof(ap->devpath)-1] = 0;
		sprintf(devpath, "midi/%s/%d", gdi->short_name, cnt);
		strncpy(ap->midipath, devpath, sizeof(ap->midipath));
		ap->midipath[sizeof(ap->midipath)-1] = 0;
		sprintf(devpath, "joystick/%s/%d", gdi->short_name, cnt);
		strncpy(ap->joypath, devpath, sizeof(ap->joypath));
		ap->joypath[sizeof(ap->joypath)-1] = 0;

		ap->dev = gdi;
		ap->sem = create_sem(0, devpath+11);
		ap->ben = 1;
		ap->locked = 0;
		set_sem_owner(ap->sem, B_SYSTEM_TEAM);
		ap->next = g_loaded_modules;
		g_loaded_modules = ap;
	}
}

static int
discover_pci_device(
	pci_info * pci)
{
	char path[44];
	int ix;
	sprintf(path, GAPLUG_PCI_MODULE_NAME_PREFIX "%04x_%04x_%04x_%04x_",
			pci->vendor_id, pci->device_id,
			pci->u.h0.subsystem_vendor_id, pci->u.h0.subsystem_id);
	ASSERT(strlen(path) < 42);
	for (ix=0; ix<4; ix++)
	{
		char * ptr = strrchr(path, '_');
		plug_module_info * gpi;
		plug_api * gdi;
		strcpy(ptr, GAPLUG_PCI_MODULE_NAME_SUFFIX);
//	The problem is that the kernel uses the file name as found, no the
//	actual file name to unique-ify the loaded module; thus two different
//	loaders would get different copies of the module, which is bad when
//	we start talking MIDI, gameport etc.
#warning fixme: should use readlink() and resolve manually here
		if (get_module(path, (module_info **)&gpi) >= 0)
		{
			ddprintf(("gamedriver: found module '%s'\n", path));
			if ((gdi = gpi->accept_pci(pci, g_pci, &g_utility)) != NULL)
			{
				eprintf(("\033[32mgamedriver: found pci handler at %p for %s\033[0m\n", gdi, path));
				if (gdi->info_size == sizeof(plug_api))
				{
					push_loaded_module(gdi, path);
					return 1;
				}
				eprintf(("\033[35mgamedriver: pci handler has size 0x%lx, should be 0x%lx\033[0m\n", gdi->info_size, sizeof(plug_api)));
			}
			put_module(path);
		}
		*ptr = 0;
	}
	d2printf(("gamedriver: got no module for %04x_%04x_%04x_%04x\n",
			pci->vendor_id, pci->device_id, pci->u.h0.subsystem_vendor_id,
			pci->u.h0.subsystem_id));
	return 0;
}

static status_t
discover_pci_devices()
{
	status_t err = g_pci ? B_WHATEVER : get_module(B_PCI_MODULE_NAME, (module_info **)&g_pci);
	int32 cookie = 0;
	int count = 0;
	pci_info pci;

	if (err < 0) {
		return err;
	}
	while (g_pci->get_nth_pci_info(cookie, &pci) >= B_WHATEVER) {
		count += discover_pci_device(&pci);
		cookie++;
	}
	ddprintf(("tried %ld PCI devices\n", cookie));
	return count > 0 ? B_WHATEVER : B_BLAME_THE_HARDWARE;
}

static int
discover_isa_device(uint32 id, struct isa_device_info* isa, uint64 cookie)
{
	EISA_PRODUCT_ID pid;
	char code[16];
	char path[44];
	int ix;

	pid.id = id;
	sprintf(code, "%c%c%c_%03x_%01x",
			((pid.b[0] >> 2) & 0x1f) + 'A' - 1,
			((pid.b[1] & 0xe0) >> 5) | ((pid.b[0] & 0x3) << 3) + 'A' - 1,
			(pid.b[1] & 0x1f) + 'A' - 1,
			((uint32) pid.b[2] << 4) + (pid.b[3] >> 4),
			pid.b[3] & 0xf);
	/* VVV_PPP_R  == Vendor Product Revision */
	ASSERT(strlen(code) < 16);
	sprintf(path, GAPLUG_ISA_MODULE_NAME_PREFIX "%s_", code);
	ASSERT(strlen(path) < 42);
	for (ix=0; ix<3; ix++)
	{
		char * ptr = strrchr(path, '_');
		plug_module_info * gpi;
		plug_api * gdi;
		strcpy(ptr, GAPLUG_ISA_MODULE_NAME_SUFFIX);
//	The problem is that the kernel uses the file name as found, no the
//	actual file name to unique-ify the loaded module; thus two different
//	loaders would get different copies of the module, which is bad when
//	we start talking MIDI, gameport etc.
#warning fixme: should use readlink() and resolve manually here
		if (get_module(path, (module_info **)&gpi) >= 0)
		{
			ddprintf(("gamedriver: found module '%s'\n", path));
			if ((gdi = gpi->accept_isa(isa, g_isa, &g_utility, g_pnp, cookie)) != NULL)
			{
				eprintf(("\033[32mgamedriver: found isa handler at %p for %s\033[0m\n", gdi, path));
				if (gdi->info_size == sizeof(plug_api))
				{
					push_loaded_module(gdi, path);
					return 1;
				}
				eprintf(("\033[35mgamedriver: isa handler has size 0x%lx, should be 0x%lx\033[0m\n", gdi->info_size, sizeof(plug_api)));
			}
			put_module(path);
		}
		*ptr = 0;
	}
	d2printf(("gamedriver: got no module for %s\n", code));
	return 0;
}

static status_t
discover_isa_devices()
{
	status_t err = g_isa ? B_WHATEVER : get_module(B_ISA_MODULE_NAME, (module_info **)&g_isa);
	struct isa_device_info isa;
	uint64 cookie = 0;
	int tried = 0;
	int found = 0;
	int i;

	if (err < 0) {
		return err;
	}
	err = g_pnp ? B_WHATEVER : get_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&g_pnp);
	if (err < 0) {
		return err;
	}
	while (g_pnp->get_next_device_info(B_ISA_BUS, &cookie, &isa.info, sizeof(isa)) == B_OK) {
		if (discover_isa_device(isa.vendor_id, &isa, cookie))
			++found;
		else if (discover_isa_device(isa.logical_device_id, &isa, cookie))
			++found;
		else for (i = 0; i < isa.num_compatible_ids; i++)
			if (discover_isa_device(isa.compatible_ids[i], &isa, cookie)) {
				++found;
				break;
			}
		++tried;
	}
	ddprintf(("tried %ld ISA devices, found %ld\n", tried, found));
	return found > 0 ? B_WHATEVER : B_BLAME_THE_HARDWARE;
}

static status_t
discover_usb_devices()
{
	status_t err = g_usb ? B_WHATEVER : get_module(B_USB_MODULE_NAME, (module_info **)&g_usb);

	if (err < 0) {
		return err;
	}
	eprintf(("\033[32mgamedriver: USB devices not supported\033[0m\n"));
	return B_BLAME_THE_HARDWARE;
}

static status_t
discover_devices()
{
	status_t pci_err = discover_pci_devices();
	status_t isa_err = discover_isa_devices();
	status_t usb_err = discover_usb_devices();

	if (!pci_err || !isa_err || !usb_err) {
		ddprintf(("gameaudio: found a reason to live\n"));
		return B_WHATEVER;
	}
	eprintf(("\033[31mgameaudio: pci_err 0x%lx  isa_err 0x%lx  usb_err 0x%lx\033[0m\n",
			pci_err, isa_err, usb_err));
	return B_BLAME_THE_HARDWARE;
}

static void
undiscover_devices()
{
	ga_loaded_module * ap, * del;

	LOCK_GLOBALS {
		ap = g_loaded_modules;
		while (ap != NULL) {
			ASSERT(ap->cookies == NULL);
			ddprintf(("gamedriver: calling ap->dev->uninit %p\n", ap->dev->uninit));
			if (ap->dev->uninit) {
				ap->dev->uninit(ap->dev);
			}
			del = ap;
			ap = ap->next;
			ddprintf(("gamedriver: put module '%s'\n", del->module));
			put_module(del->module);
			free(del);
		}
	}
}

static int
dbg_inb(
	int argc,
	char * argv[])
{
	uint32 addr;
	uint32 val;
	if (argc != 1)
	{
		kprintf("usage: inb ioaddr\n");
		return 1;
	}
	addr = parse_expression(argv[0]);
	if (addr & 0xffff0000)
	{
		kprintf("ioaddr must be smaller than 0x10000\n");
		return 1;
	}
	val = g_pci->read_io_8(addr);
	kprintf("outb 0x%04lx 0x%02lx\n", addr, val);
	return 0;
}

static int
dbg_inw(
	int argc,
	char * argv[])
{
	uint32 addr;
	uint32 val;
	if (argc != 1)
	{
		kprintf("usage: inw ioaddr\n");
		return 1;
	}
	addr = parse_expression(argv[0]);
	if (addr & 0xffff0000)
	{
		kprintf("ioaddr must be smaller than 0x10000\n");
		return 1;
	}
	val = g_pci->read_io_16(addr);
	kprintf("outw 0x%04lx 0x%04lx\n", addr, val);
	return 0;
}

static int
dbg_inl(
	int argc,
	char * argv[])
{
	uint32 addr;
	uint32 val;
	if (argc != 1)
	{
		kprintf("usage: inl ioaddr\n");
		return 1;
	}
	addr = parse_expression(argv[0]);
	if (addr & 0xffff0000)
	{
		kprintf("ioaddr must be smaller than 0x10000\n");
		return 1;
	}
	val = g_pci->read_io_32(addr);
	kprintf("outl 0x%04lx 0x%08lx\n", addr, val);
	return 0;
}

static int
dbg_outb(
	int argc,
	char * argv[])
{
	uint32 addr;
	uint32 val;
	if (argc != 2)
	{
		kprintf("usage: outb ioaddr value\n");
		return 1;
	}
	addr = parse_expression(argv[0]);
	if (addr & 0xffff0000)
	{
		kprintf("ioaddr must be smaller than 0x10000\n");
		return 1;
	}
	val = parse_expression(argv[1]);
	if (val & 0xffffff00)
	{
		kprintf("value must be smaller than 0x100\n");
		return 1;
	}
	g_pci->write_io_8(addr, val);
	kprintf("inb 0x%04lx\n", addr);
	return 0;
}

static int
dbg_outw(
	int argc,
	char * argv[])
{
	uint32 addr;
	uint32 val;
	if (argc != 2)
	{
		kprintf("usage: outw ioaddr value\n");
		return 1;
	}
	addr = parse_expression(argv[0]);
	if (addr & 0xffff0000)
	{
		kprintf("ioaddr must be smaller than 0x10000\n");
		return 1;
	}
	val = parse_expression(argv[1]);
	if (val & 0xffff0000)
	{
		kprintf("value must be smaller than 0x10000\n");
		return 1;
	}
	g_pci->write_io_16(addr, val);
	kprintf("inw 0x%04lx\n", addr);
	return 0;
}

static int
dbg_outl(
	int argc,
	char * argv[])
{
	uint32 addr;
	uint32 val;
	if (argc != 2)
	{
		kprintf("usage: outl ioaddr value\n");
		return 1;
	}
	addr = parse_expression(argv[0]);
	if (addr & 0xffff0000)
	{
		kprintf("ioaddr must be smaller than 0x10000\n");
		return 1;
	}
	val = parse_expression(argv[1]);
	g_pci->write_io_32(addr, val);
	kprintf("inl 0x%04lx\n", addr);
	return 0;
}

static void
driver_cleanup(
	void)
{
	delete_sem(g_globals_sem);

	remove_debugger_command("inb", dbg_inb);
	remove_debugger_command("inw", dbg_inw);
	remove_debugger_command("inl", dbg_inl);
	remove_debugger_command("outh", dbg_outb);
	remove_debugger_command("outw", dbg_outw);
	remove_debugger_command("outl", dbg_outl);
}

status_t
init_driver()
{
	status_t status;
#if DEBUG
	void * settings = load_driver_settings("game_audio");
	g_debug = atoi(get_driver_parameter(settings, "debug", "0", "1"));
	unload_driver_settings(settings);
	dprintf("gamedriver: debug level is %d (set in game_audio settings file with keyword 'debug')\n", g_debug);
#endif

	add_debugger_command("inb", dbg_inb, "read I/O byte");
	add_debugger_command("inw", dbg_inw, "read I/O word");
	add_debugger_command("inl", dbg_inl, "read I/O longword");
	add_debugger_command("outh", dbg_outb, "write I/O byte");
	add_debugger_command("outw", dbg_outw, "write I/O word");
	add_debugger_command("outl", dbg_outl, "write I/O longword");

	//	Do not remove this check! It's important we catch and fix this right away in the future.
	if ((GAME_FIRST_UNIMPLEMENTED_CODE - GAME_GET_INFO) != (sizeof(ctrl_info)/sizeof(ctrl_info[0])))
	{
		kernel_debugger("\033[33m\ngamedriver: Please update ctrl_info to match enum game_audio_opcode!\033[0m\n");
	}

	g_globals_ben = 1;
	g_globals_sem = create_sem(0, "gamedriver globals");
	set_sem_owner(g_globals_sem, B_SYSTEM_TEAM);
	status = discover_devices();
	if (status < B_OK)
	{
		driver_cleanup();
	}
	return status;
}

void
uninit_driver()
{
	ddprintf(("gameaudio: uninit_driver()\n"));
	undiscover_devices();

	/*	When all devices have been undiscovered, no interrupt handlers
	 *	will run, so we don't need a lock.	*/

	if (g_pci) put_module(B_PCI_MODULE_NAME);
	if (g_isa) put_module(B_ISA_MODULE_NAME);
	if (g_usb) put_module(B_USB_MODULE_NAME);
	if (g_pnp) put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
	if (g_mpu401) put_module(B_MPU_401_MODULE_NAME);
	if (g_gameport) put_module(B_GAMEPORT_MODULE_NAME);

	driver_cleanup();
}

const char **
publish_devices()
{
	int cnt = 0;
	ga_loaded_module * ap;

	LOCK_GLOBALS {
		free(g_device_names);
		g_device_names = NULL;
		ap = g_loaded_modules;
	 	while (ap != NULL) {
			ap = ap->next;
			cnt++;
		}
		if (cnt > 0) {
			g_device_names = (const char **)calloc(2*cnt+1, sizeof(char *));
			ap = g_loaded_modules;
			cnt = 0;
			while (ap != NULL) {
				g_device_names[cnt] = ap->devpath;
				cnt++;
				if (ap->dev->get_mpu401_info) {
					g_device_names[cnt] = ap->midipath;
					cnt++;
				}			
				if (ap->dev->get_gameport_info) {
					g_device_names[cnt] = ap->joypath;
					cnt++;
				}			
				ap = ap->next;
			}
		}
	}
	return g_device_names;
}

device_hooks *
find_device(
	const char * name)
{
	if (!strncmp(name, "midi/", 5))
	  return &m_hooks;
	if (!strncmp(name, "joystick/", 9))
	  return &j_hooks;
	return &s_hooks;
}

static void
default_adc_info(
	plug_api * dev,
	game_codec_info * info,
	int ix)
{
	info->codec_id = GAME_MAKE_ADC_ID(ix);
	info->linked_codec_id = GAME_NO_ID;
	info->linked_mixer_id = GAME_NO_ID;
	info->max_stream_count = 1;
	info->cur_stream_count = 0;
	info->min_chunk_frame_count = 512;
	info->chunk_frame_count_increment = 0;
	info->max_chunk_frame_count = 512;
	info->cur_chunk_frame_count = 512;
	sprintf(info->name, "%s ADC %d", dev->short_name, ix+1);
	if ((info->name[0] >= 'a') && (info->name[0] <= 'z')) {
		info->name[0] &= ~0x20;	//	make upper-case
	}
	info->frame_rates = B_SR_44100;
	info->cvsr_min = info->cvsr_max = 0.0;
	info->formats = B_FMT_16BIT;
	info->designations = 0;
	info->channel_counts = 0x2;
	info->cur_frame_rate = B_SR_44100;
	info->cur_cvsr = 44100.0;
	info->cur_format = B_FMT_16BIT;
	info->cur_channel_count = 2;
}

static void
default_dac_info(
	plug_api * dev,
	game_codec_info * info,
	int ix)
{
	info->codec_id = GAME_MAKE_DAC_ID(ix);
	info->linked_codec_id = GAME_NO_ID;
	info->linked_mixer_id = GAME_NO_ID;
	info->max_stream_count = 1;
	info->cur_stream_count = 0;
	info->min_chunk_frame_count = 512;
	info->chunk_frame_count_increment = 0;
	info->max_chunk_frame_count = 512;
	info->cur_chunk_frame_count = 512;
	sprintf(info->name, "%s DAC %d", dev->short_name, ix+1);
	if ((info->name[0] >= 'a') && (info->name[0] <= 'z')) {
		info->name[0] &= ~0x20;	//	make upper-case
	}
	info->frame_rates = B_SR_44100;
	info->cvsr_min = info->cvsr_max = 0.0;
	info->formats = B_FMT_16BIT;
	info->designations = 0;
	info->channel_counts = 0x2;
	info->cur_frame_rate = B_SR_44100;
	info->cur_cvsr = 44100.0;
	info->cur_format = B_FMT_16BIT;
	info->cur_channel_count = 2;
}

static void
default_stream(
	ga_active_stream_info * strm,
	game_codec_info * codec,
	int index)
{
	strm->info.info_size = sizeof(strm->info);
	strm->info.codec_id = codec->codec_id;
	strm->info.stream_sem = -1;
	strm->info.out_stream_id = GAME_MAKE_STREAM_ID(index);
}

static void
load_mixer_info(
	ga_loaded_module * pc,
	int ix)
{
	ga_mixer_info * mi = &pc->mixer_infos[ix];
	gaplug_get_mixer_controls * gmc = &mi->controls;
	gaplug_mixer_control_values * gmcv = &mi->values;

	//	get mixer info
	pc->dev->get_mixer_description(pc->dev, ix, &mi->info);
	ddprintf(("gamedriver: %s mixer %d: %s\n", pc->dev->short_name, ix, mi->info.name));
	//	validate mixer info
	if (!GAME_IS_MIXER(mi->info.mixer_id))
	{
		eprintf(("\033[35mgamedriver: %s returns non-mixer ID %ld for mixer %d\n\033[0m",
			pc->dev->short_name, mi->info.mixer_id, ix));
	}
	if (GAME_MIXER_ORDINAL(mi->info.mixer_id) != ix)
	{
		eprintf(("\033[35mgamedriver: %s returns mixer ID out of order (ordinal %ld at index %d)\033[0m\n",
			pc->dev->short_name, GAME_MIXER_ORDINAL(mi->info.mixer_id), ix));
	}
	if ((mi->info.linked_codec_id != GAME_NO_ID) && !GAME_IS_DAC(mi->info.linked_codec_id) &&
		!GAME_IS_ADC(mi->info.linked_codec_id))
	{
		eprintf(("\033[35mgamedriver: %s returns non-codec ID %ld for linked_codec_id for mixer %d\n\033[0m",
			pc->dev->short_name, mi->info.linked_codec_id, ix));
	}
#if DEBUG
	if ((mi->info.control_count == 0) || (mi->info.control_count > 30))
	{
		ddprintf(("\033[35mgamedriver: %s has suspicious control count %ld for mixer %d\033[0m\n",
			pc->dev->short_name, mi->info.control_count, ix));
	}
#endif

	//	allocate control space
	gmc->i_mixer = &mi->info;
	mi->hidden_ctl_count = 0;
	if (mi->info.control_count > 0)
	{ 
		int errs = 0;
		int iy;
		int32 ctls = mi->info.control_count;
		
		//  allocating one additional bucket in o-cntls,o-infos and o-cookies for sorting purposes
		gmc->o_controls = (game_mixer_control *)calloc(mi->info.control_count + 1, sizeof(game_mixer_control));
		gmc->o_infos = (gaplug_control_info *)calloc(mi->info.control_count + 1, sizeof(gaplug_control_info *));
		gmc->o_cookies = (void **)calloc(mi->info.control_count + 1, sizeof(void *));
		//	get controls
		pc->dev->get_mixer_controls(pc->dev, gmc);

		//  move the hidden controls to the end of table	
		for (ix=0, iy=0; iy < ctls; iy++)
		{
			if (gmc->o_controls[ix].flags & GAME_MIXER_CONTROL_HIDDEN)
			{
				mi->hidden_ctl_count++;
				//move hidden ctls to end of table and shift rest of ctls up 
				gmc->o_controls[ctls] = gmc->o_controls[ix];	//move to extra bucket
				memmove(&gmc->o_controls[ix],&gmc->o_controls[ix+1], (ctls-ix)*sizeof(game_mixer_control)); //shift remaining ctls up
				// keep infos in sync 
				gmc->o_infos[ctls] = gmc->o_infos[ix];	//move to extra bucket
				memmove(&gmc->o_infos[ix],&gmc->o_infos[ix+1],(ctls-ix)*sizeof(gaplug_control_info *)); //shift remaining ctls up
				// keep cookies in sync 
				gmc->o_cookies[ctls] = gmc->o_cookies[ix];	//move to extra bucket
				memmove(&gmc->o_cookies[ix],&gmc->o_cookies[ix+1], (ctls-ix)*sizeof(void *)); //shift remaining ctls up
			}	
			else {
				ix ++;		//nothing got shifted so just bump index 	
			}	
		}
		
		memset(&gmc->o_controls[ctls],0, sizeof(game_mixer_control));  //clear out tmp bucket to avoid any possible confusion 
		memset(&gmc->o_infos[ctls], 0, sizeof(gaplug_control_info *)); 
		memset(&gmc->o_cookies[ctls], 0, sizeof(void *));		
		
		//	validate controls
		ddprintf(("gamedriver: testing %s returned control info\n", pc->dev->short_name));
		for (ix = 0; ix < gmc->i_mixer->control_count; ix++)
		{
			if (!gmc->o_infos[ix].other)
			{
				errs++;
				eprintf(("\033[35mgamedriver: %s returns NULL gaplug_control_info for control id %ld\033[0m\n",
					pc->dev->short_name, gmc->o_controls[ix].control_id));
			}
			//	fixme:	assuming control_id is in the same place for all the structs
			if (gmc->o_infos[ix].level->control_id != gmc->o_controls[ix].control_id)
			{
				errs++;
				eprintf(("\033[35mgamedriver: %s is inconsistent in giving control IDs (%ld == %ld)\033[0m\n",
					pc->dev->short_name, gmc->o_infos[ix].level->control_id, gmc->o_controls[ix].control_id));
			}
			//	fixme:	assuming mixer_id is in the same place for all the structs
			if (gmc->o_infos[ix].level->mixer_id != gmc->o_controls[ix].mixer_id)
			{
				errs++;
				eprintf(("\033[35mgamedriver: %s is inconsistent in giving mixer IDs (%ld == %ld)\033[0m\n",
					pc->dev->short_name, gmc->o_infos[ix].level->mixer_id, gmc->o_controls[ix].mixer_id));
			}
			switch (gmc->o_controls[ix].kind)
			{
			case GAME_MIXER_CONTROL_IS_LEVEL:
				if (gmc->o_infos[ix].level->info_size != sizeof(game_get_mixer_level_info))
				{
					errs++;
					eprintf(("\033[35mgamedriver: %s claims game_get_mixer_level_info is %ld bytes in size.\033[0m\n",
						pc->dev->short_name, gmc->o_infos[ix].level->info_size));
				}
				break;
			case GAME_MIXER_CONTROL_IS_MUX:
				if (gmc->o_infos[ix].mux->info_size != sizeof(game_get_mixer_mux_info))
				{
					errs++;
					eprintf(("\033[35mgamedriver: %s claims game_get_mixer_mux_info is %ld bytes in size.\033[0m\n",
						pc->dev->short_name, gmc->o_infos[ix].mux->info_size));
				}
				if (gmc->o_infos[ix].mux->items == NULL)
				{
					errs++;
					eprintf(("\033[35mgamedriver: %s has a mux with NULL items! (id %ld)\033[0m\n",
						pc->dev->short_name, gmc->o_infos[ix].mux->control_id));
				}
				break;
			case GAME_MIXER_CONTROL_IS_ENABLE:
				if (gmc->o_infos[ix].enable->info_size != sizeof(game_get_mixer_enable_info))
				{
					errs++;
					eprintf(("\033[35mgamedriver: %s claims game_get_mixer_enable_info is %ld bytes in size.\033[0m\n",
						pc->dev->short_name, gmc->o_infos[ix].enable->info_size));
				}
				break;
			default:
				errs++;
				eprintf(("\033[35mgamedriver: what is control type %ld? I'm only asking because %s claims to have one.\033[0m\n",
					gmc->o_controls[ix].kind, pc->dev->short_name));
			}
		}
		if (errs > 0)
		{
			ddprintf(("\033[31mgamedriver: %s gets %d thing%s wrong!\033[0m\n", pc->dev->short_name, errs, errs > 1 ? "s" : ""));
		}
		else
		{
			ddprintf(("\033[32mgamedriver: %s passes with flying colors!\033[0m\n", pc->dev->short_name));
		}
	
		//	allocate control values (for use in ioctl)
		gmcv->i_count = mi->info.control_count;
		gmcv->i_values = (const gaplug_mixer_control_value *)calloc(gmcv->i_count, sizeof(gaplug_mixer_control_value));
	
		// hide hidden controls from user 
		mi->info.control_count = mi->info.control_count - mi->hidden_ctl_count;  
	}
}

status_t
dev_open(
	const char * i_name,
	uint32 i_flags,
	void ** o_cookie)
{
	int iy;
	int ix;
	ga_loaded_module * ap = NULL;
	plug_api * dev = NULL;
	ga_cookie * pc = NULL;
	status_t err = B_WHATEVER;

	ddprintf(("dev_open(%s, 0x%lx)\n", i_name, i_flags));

	LOCK_GLOBALS
	{
		for (ap = g_loaded_modules; ap != NULL; ap = ap->next)
		{
			if (!strcmp(i_name, ap->devpath))
			{
				dev = ap->dev;
				break;
			}
		}
		if (dev == NULL)
		{
			eprintf(("\033[31mgamedriver: dev_open(%s) wasn't found\033[0m\n", i_name));
			err = B_SHUCKS;
		}
		//	do sanity check on the module
		if ((dev != NULL) && (!dev->uninit))
		{
			eprintf(("\033[35mgamedriver: %s must implement uninit()\033[0m\n", i_name));
			err = B_SHUCKS;
		}
		if ((dev != NULL) && (!dev->init_codecs || !dev->get_codec_infos))
		{
			eprintf(("\033[35mgamedriver: %s must implement init_codecs and get_codec_infos\033[0m\n", i_name));
			err = B_SHUCKS;
		}
		if ((dev != NULL) && (!dev->init_stream || !dev->run_streams))
		{
			eprintf(("\033[35mgamedriver: %s must implement init_stream and run_streams\033[0m\n", i_name));
			err = B_SHUCKS;
		}
		//	initialize device/cookie here
		if (!err)
		{
			pc = (ga_cookie *)calloc(1, sizeof(ga_cookie));
			if (pc == NULL)
			{
				err = B_OH_OH;
			}
		}
		if (!err && dev->open)
		{
			err = dev->open(dev, i_name, i_flags, pc);
			if (err < 0)
			{
				free(pc);
				pc = NULL;
				ddprintf(("gamedriver: dev->open() returns 0x%lx\n", err));
			}
		}
		if (pc != NULL)
		{
			ddprintf(("gamedriver: plug cookie is %p (previous %p)\n", pc, ap->cookies));
			pc->module = ap;
			pc->dev = dev;
			pc->next = ap->cookies;
			pc->write_ok = (((i_flags & 3) == O_WRONLY) || ((i_flags & 3) == O_RDWR));
			ap->cookies = pc;
			if (!ap->inited)
			{
				int cnt;
				dev->init_codecs(dev, &ap->adc_count, &ap->dac_count, &ap->mixer_count);
				ap->adc_infos = (game_codec_info *)calloc(ap->adc_count, sizeof(game_codec_info));
				ap->dac_infos = (game_codec_info *)calloc(ap->dac_count, sizeof(game_codec_info));
				for (ix=0; ix<ap->adc_count; ix++)
				{
					default_adc_info(dev, ap->adc_infos+ix, ix);
				}
				for (ix=0; ix<ap->dac_count; ix++)
				{
					default_dac_info(dev, ap->dac_infos+ix, ix);
				}
				dev->get_codec_infos(dev, ap->adc_infos, ap->dac_infos);
				//fixme:	we could get away with fewer block allocations if we merged all
				//	this info into one array of stream infos, and pre- (or post-) pended the
				//	array pointers.
				cnt = 0;
				ap->adc_streams = (ga_active_stream_info **)calloc(ap->adc_count, sizeof(ga_active_stream_info *));
				for (ix=0; ix<ap->adc_count; ix++)
				{
					ap->adc_streams[ix] = (ga_active_stream_info *)calloc(ap->adc_infos[ix].max_stream_count,
							sizeof(ga_active_stream_info));
					for (iy=0; iy<ap->adc_infos[ix].max_stream_count; iy++)
					{
						default_stream(ap->adc_streams[ix]+iy, (game_codec_info *)ap->adc_infos+ix, cnt);
						cnt++;
					}
				}
				ap->dac_streams = (ga_active_stream_info **)calloc(ap->dac_count, sizeof(ga_active_stream_info *));
				for (ix=0; ix<ap->dac_count; ix++)
				{
					ap->dac_streams[ix] = (ga_active_stream_info *)calloc(ap->dac_infos[ix].max_stream_count,
							sizeof(ga_active_stream_info));
					for (iy=0; iy<ap->dac_infos[ix].max_stream_count; iy++)
					{
						default_stream(ap->dac_streams[ix]+iy, ap->dac_infos+ix, cnt);
						cnt++;
					}
				}
				if (ap->mixer_count > 0)
				{
					if (!dev->get_mixer_description || !dev->get_mixer_controls ||
						!dev->get_mixer_control_values || !dev->set_mixer_control_values)
					{
						eprintf(("\033[35mgamedriver: %s returns %d mixers, but doesn't implement mixer hooks!\033[0m\n",
							dev->short_name, ap->mixer_count));
						ap->mixer_count = 0;
					}
					else
					{
						ap->mixer_infos = (ga_mixer_info *)calloc(ap->mixer_count, sizeof(ga_mixer_info));
						for (ix=0; ix<ap->mixer_count; ix++)
						{
							load_mixer_info(ap, ix);
						}
					}
				}
				ap->inited = true;
			}
			*o_cookie = pc;
		}
	}
	return err;
}

static void
remove_buffer(
	ga_cookie * pc,
	ga_active_buffer_info * pbuf)
{
	status_t err = B_WHATEVER;

	ddprintf(("gamedriver: remove_buffer(%p, %ld) stream %ld\n", pc, pbuf->id, pbuf->stream));

#if DEBUG
	if (pbuf->count > 0)
	{
		ddprintf(("\033[33mgamedriver: warning: removing buffer %ld with refcount %d\033[0m\n", pbuf->id, pbuf->count));
	}
#endif
	if (pc->dev->close_stream_buffer) {
		game_open_stream_buffer gosb;
		gosb.stream = pbuf->stream;
		gosb.byte_size = pbuf->size;
		gosb.area = pbuf->area;
		gosb.offset = pbuf->offset;
		gosb.buffer = pbuf->id;
		err = pc->dev->close_stream_buffer(pc->dev, &gosb);
	}
	pbuf->id = GAME_NO_ID;
	pbuf->stream = GAME_NO_ID;
	pbuf->owner = 0;
	if (err < 0) {
		return;
	}
	delete_area(pbuf->area);
	pbuf->area = 0;
	pbuf->base = 0;
}

static void
terminate_stream(
	ga_cookie * pc,
	ga_active_stream_info * pstrm)
{
	game_run_stream grs;
	game_run_streams grss;
	gaplug_run_streams gprs;
	gaplug_run_stream_info gprsi;

	memset(&grs, 0, sizeof(grs));
	memset(&grss, 0, sizeof(grss));
	memset(&gprs, 0, sizeof(gprs));
	memset(&gprsi, 0, sizeof(gprsi));

	grs.stream = pstrm->info.out_stream_id;
	grs.state = GAME_STREAM_STOPPED;
	grss.info_size = sizeof(grs);
	grss.in_stream_count = 1;
	grss.out_actual_count = 1;
	grss.streams = &grs;
	gprs.total_slots = 1;
	gprs.io_request = &grss;
	gprs.info = &gprsi;
	gprsi.request = &grs;
	gprsi.stream = &pstrm->info;
	gprsi.stream_cookie = pstrm->stream_cookie;

	pc->dev->run_streams(pc->dev, &gprs);
	pstrm->state = GAME_STREAM_STOPPED;
}

static void
remove_stream(
	ga_cookie * pc,
	ga_active_stream_info * pstrm)
{
	int ix;
	game_codec_info * gdi;

	ddprintf(("gamedriver: remove_stream(%p, %ld) codec %ld\n", pc,
			pstrm->info.out_stream_id, pstrm->info.codec_id));

	ASSERT_LOCKED(pc->module);

	//	synchronize with the add-on so that it won't call us anymore
	terminate_stream(pc, pstrm);

	SPINLOCK_ACTIVE(pc->module)
	{
		//	remove all the queue slots
		if (pstrm->buffer)
		{
			pstrm->buffer->count--;
			pstrm->buffer = NULL;
			pstrm->page_frame_count = 0;
		}
		// should this be outside the spinlock?
		gdi = find_codec(pc, pstrm->info.codec_id);
		ASSERT(gdi != NULL);
		gdi->cur_stream_count--;
	}

	//	remove bound buffers
	for (ix=0; ix<pc->module->buffer_count; ix++)
	{
		if (pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK].stream == pstrm->info.out_stream_id)
		{
			ASSERT(pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK].id == GAME_MAKE_BUFFER_ID(ix));
			remove_buffer(pc, &pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK]);
		}
	}
	if (pc->dev->remove_stream)
	{
		pc->dev->remove_stream(pc->dev, &pstrm->info, pstrm->stream_cookie);
	}
	pstrm->owner = NULL;
	pstrm->stream_cookie = NULL;
}

status_t
dev_close(
	void * i_cookie)
{
	ga_cookie * pc = (ga_cookie *)i_cookie;
	status_t err = B_WHATEVER;

	ddprintf(("dev_close(%p)\n", i_cookie));

	LOCK_ACTIVE(pc->module)
	{
		ga_cookie ** ppc = &(pc->module->cookies);

		//	unlink cookie

		while ((*ppc != NULL) && (*ppc != pc)) {
			ppc = &(*ppc)->next;
		}
		if (!*ppc) {
			eprintf(("\033[31mgamedriver: bad cookie %p passed to dev_close\033[0m\n", i_cookie));
			err = B_SHUCKS;
		}
		else
		{
			if (pc->dev->close) {
				pc->dev->close(pc->dev, pc);
			}
			*ppc = (*ppc)->next;
		}
	}
	return err;
}

status_t
dev_free(
	void * i_cookie)
{
	ga_cookie * pc = (ga_cookie *)i_cookie;
	int ix;
	int iy;

	ddprintf(("dev_free(%p)\n", i_cookie));

	LOCK_ACTIVE(pc->module)
	{
		for (ix=0; ix<pc->module->adc_count; ix++) {
			for (iy=0; iy<pc->module->adc_infos[ix].max_stream_count; iy++) {
				if (pc->module->adc_streams[ix][iy].owner == pc) {
					remove_stream(pc, pc->module->adc_streams[ix]+iy);
				}
			}
		}
		for (ix=0; ix<pc->module->dac_count; ix++) {
			for (iy=0; iy<pc->module->dac_infos[ix].max_stream_count; iy++) {
				if (pc->module->dac_streams[ix][iy].owner == pc) {
					remove_stream(pc, pc->module->dac_streams[ix]+iy);
				}
			}
		}
		for (ix=0; ix<pc->module->buffer_count; ix++) {
			if (pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK].owner == pc) {
				ASSERT(pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK].id == GAME_MAKE_BUFFER_ID(ix));
				ASSERT(pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK].stream == GAME_NO_ID);
				//ddprintf(("gamedriver: pc->module->buffers[%ld].stream == %ld\n", ix, pc->module->buffers[ix].stream));
				remove_buffer(pc, &pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK]);
			}
		}
		if (pc->dev->free) {
			pc->dev->free(pc->dev, pc);
		}
	}
	LOCK_GLOBALS
	{
		if (pc->module->cookies == 0) {
			ddprintf(("gamedriver: disposing info about %s\n", pc->module->devpath));
	
			free(pc->module->adc_infos);
			pc->module->adc_count = 0;
			pc->module->adc_infos = NULL;
			free(pc->module->dac_infos);
			pc->module->dac_count = 0;
			pc->module->dac_infos = NULL;

#if DEBUG
			for (ix=0; ix<pc->module->buffer_count; ix++) {
				ASSERT(pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK].owner == NULL);
			}
#endif

			for (ix=0; ix<pc->module->adc_count; ix++) {
				free(pc->module->adc_streams[ix]);
			}
			free(pc->module->adc_streams);
			pc->module->adc_streams = NULL;

			for (ix=0; ix<pc->module->dac_count; ix++) {
				free(pc->module->dac_streams[ix]);
			}
			free(pc->module->dac_streams);
			pc->module->dac_streams = NULL;

			for (ix=0; ix<pc->module->buffer_count>>BLOCK_SHIFT; ix++)
			{
				free(pc->module->buffers[ix]);
			}
			free(pc->module->buffers);
			pc->module->buffers = NULL;
			pc->module->buffer_count = 0;

			for (ix=0; ix<pc->module->mixer_count; ix++)
			{
				free(pc->module->mixer_infos[ix].controls.o_controls);
				free(pc->module->mixer_infos[ix].controls.o_infos);
				free(pc->module->mixer_infos[ix].controls.o_cookies);
				free((void *)pc->module->mixer_infos[ix].values.i_values);
			}
			free(pc->module->mixer_infos);
			pc->module->mixer_infos = NULL;
			pc->module->mixer_count = 0;

			delete_sem(pc->module->sem);
			pc->module->sem = B_BAD_SEM_ID;

			pc->module->inited = false;
		}
	}

	free(pc);

	return B_WHATEVER;
}

static status_t
get_info(
	ga_cookie * pc,
	game_get_info * info)
{
	game_get_info out;
	ga_loaded_module * ap;
	status_t err = B_WHATEVER;

	memset(&out, 0, sizeof(out));
	out.info_size = sizeof(out);
	sprintf(out.name, "%s; gamedriver", pc->dev->short_name);
	if (pc->dev->vendor_name)
	{
		strncpy(out.vendor, pc->dev->vendor_name, sizeof(out.vendor));
		out.vendor[sizeof(out.vendor)-1] = 0;
	}
	else
	{
		strcpy(out.vendor, "Be, Incorporated");
	}
	out.ordinal = 0;
	out.last_opcode = GAME_FIRST_UNIMPLEMENTED_CODE;

	LOCK_GLOBALS {
		for (ap = g_loaded_modules; (ap != NULL) && (ap != pc->module); ap = ap->next) {
			out.ordinal++;
		}
		if (ap == NULL)
		{
			eprintf(("\033[33mgamedriver: cookie 0x%08lx is not found in list for get_info()!\033[0m\n", (uint32)pc->module));
		}
		ap = pc->module;
		out.version = GAME_CURRENT_API_VERSION;
		out.dac_count = ap->dac_count;
		out.adc_count = ap->adc_count;
		out.mixer_count = ap->mixer_count;
	}
	if (pc->dev->get_info) {
		err = pc->dev->get_info(pc->dev, &out);
	}
	if (err == B_WHATEVER) {
		if (info->info_size < sizeof(out)) {
			memcpy(info, &out, info->info_size);
		}
		else {
			memcpy(info, &out, sizeof(out));
		}
	}
	return err;
}

static status_t
get_codec_infos(
	ga_cookie * pc,
	game_get_codec_infos * request)
{
	int ix;

	if (request->in_request_count == 0) {
		request->out_actual_count = pc->module->dac_count+pc->module->adc_count;
		return B_WHATEVER;
	}
	request->out_actual_count = 0;
	for (ix=0; ix<request->in_request_count; ix++) {
		int ordinal = GAME_DAC_ORDINAL(request->info[ix].codec_id);
		if (!GAME_IS_DAC(request->info[ix].codec_id)) {
			if (!GAME_IS_ADC(request->info[ix].codec_id))
			{
				eprintf(("gamedriver: bad CODEC ID %ld passed to get_codec_infos()\n",
						request->info[ix].codec_id));
				request->info[ix].codec_id = GAME_NO_ID;
			}
			else if ((ordinal < 0) || (ordinal >= pc->module->adc_count))
			{
				eprintf(("gamedriver: out-of-range ADC_ID %ld passed to get_codec_infos()\n",
						request->info[ix].codec_id));
			}
			else
			{
				if (pc->module->adc_infos[ordinal].codec_id != request->info[ix].codec_id) {
					eprintf(("gamedriver: plug-in '%s' returned bad DAC_ID %ld for ordinal %d\n",
							pc->dev->short_name, pc->module->adc_infos[ordinal].codec_id, ordinal));
				}
				memcpy(request->info+ix, pc->module->adc_infos+ordinal, sizeof(game_codec_info));
				request->out_actual_count++;
			}
		}
		else if ((ordinal < 0) || (ordinal >= pc->module->dac_count)) {
			eprintf(("gamedriver: out-of-range DAC_ID %ld passed to get_codec_infos()\n",
					request->info[ix].codec_id));
		}
		else {
			if (pc->module->dac_infos[ordinal].codec_id != request->info[ix].codec_id) {
				eprintf(("gamedriver: plug-in '%s' returned bad DAC_ID %ld for ordinal %d\n",
						pc->dev->short_name, pc->module->dac_infos[ordinal].codec_id, ordinal));
			}
			memcpy(request->info+ix, pc->module->dac_infos+ordinal, sizeof(game_codec_info));
			request->out_actual_count++;
		}
	}
	return (request->out_actual_count > 0) ? B_WHATEVER : B_DONT_DO_THAT;
}

static status_t
set_codec_format(
	ga_cookie * pc,
	game_codec_format * format)
{
	game_codec_info * info;
	status_t err = B_WHATEVER;

	if (GAME_IS_DAC(format->codec)) {
		int ordinal = GAME_DAC_ORDINAL(format->codec);
		if ((ordinal < 0) || (ordinal >= pc->module->dac_count)) {
			goto bad_id;
		}
		info = pc->module->dac_infos+ordinal;
	}
	else if (GAME_IS_ADC(format->codec)) {
		int ordinal = GAME_ADC_ORDINAL(format->codec);
		if ((ordinal < 0) || (ordinal >= pc->module->adc_count)) {
			goto bad_id;
		}
		info = (game_codec_info *)(pc->module->adc_infos+ordinal);
	}
	else {
bad_id:
		eprintf(("gamedriver: set_codec_format() for bad ID %ld\n", format->codec));
		format->out_status = B_DONT_DO_THAT;
		return B_DONT_DO_THAT;
	}
	if (format->flags & ~(GAME_CODEC_FAIL_IF_DESTRUCTIVE|
			GAME_CODEC_CLOSEST_OK|GAME_CODEC_SET_ALL)) {
		eprintf(("gamedriver: bad format change request flags: 0x%lx\n",
				format->flags));
		if (!(format->flags & GAME_CODEC_CLOSEST_OK)) {
			format->out_status = B_MEDIA_BAD_FORMAT;
			return B_MEDIA_BAD_FORMAT;
		}
	}
	if (format->flags & GAME_CODEC_SET_CHANNELS) {
		if ((1<<(format->channels-1)) == info->cur_channel_count) {
			goto ok_channels;
		}
		if (!(info->channel_counts & (1<<(format->channels-1)))) {
			ddprintf(("gamedriver: channel_counts %ld is bad\n", format->channels));
			err = B_MEDIA_BAD_FORMAT;
			goto ok_channels;
		}
	}
	else {
ok_channels:
		format->flags &= ~GAME_CODEC_SET_CHANNELS;
		format->channels = info->cur_channel_count;
	}
	if (format->flags & GAME_CODEC_SET_CHUNK_FRAME_COUNT) {
		if (format->chunk_frame_count == info->cur_chunk_frame_count) {
			goto ok_chunk_frame_count;
		}
		if ((format->chunk_frame_count < info->min_chunk_frame_count) ||
				(format->chunk_frame_count > info->max_chunk_frame_count)) {
			eprintf(("gamedriver: chunk_frame_count %ld is bad (min %ld max %ld)\n",
					format->chunk_frame_count, info->min_chunk_frame_count,
					info->max_chunk_frame_count));
			err = B_MEDIA_BAD_FORMAT;
			goto ok_chunk_frame_count;
		}
		if (info->chunk_frame_count_increment == 0) {
			eprintf(("gamedriver: %s sets chunk_frame_count_increment to 0, which is "
					"illegal when the current chunk frame count is not the same as min and max.\n",
					pc->dev->short_name));
		}
		if (info->chunk_frame_count_increment <= 0) {
			size_t temp = format->chunk_frame_count / info->min_chunk_frame_count;
			if ((temp*info->min_chunk_frame_count != format->chunk_frame_count) ||
					((temp & (temp-1)) != 0)) {
				eprintf(("gamedriver: chunk_frame_count %ld is bad (min %ld max %ld leftshift)\n",
						format->chunk_frame_count, info->min_chunk_frame_count,
						info->max_chunk_frame_count));
				err = B_MEDIA_BAD_FORMAT;
				goto ok_chunk_frame_count;
			}
		}
		else if ((format->chunk_frame_count-info->min_chunk_frame_count)%
				(info->chunk_frame_count_increment)) {
			eprintf(("gamedriver: chunk_frame_count %ld is bad (min %ld max %ld inc %ld)\n",
					format->chunk_frame_count, info->min_chunk_frame_count,
					info->max_chunk_frame_count, info->chunk_frame_count_increment));
			err = B_MEDIA_BAD_FORMAT;
			goto ok_chunk_frame_count;
		}
	}
	else {
ok_chunk_frame_count:
		format->flags &= ~GAME_CODEC_SET_CHUNK_FRAME_COUNT;
		format->chunk_frame_count = info->cur_chunk_frame_count;
	}
	if (format->flags & GAME_CODEC_SET_FORMAT) {
		if (format->format == info->cur_format) {
			goto ok_format;
		}
		if (!(format->format & info->formats)) {
			ddprintf(("gamedriver: format 0x%lx is bad\n", format->format));
			err = B_MEDIA_BAD_FORMAT;
			goto ok_format;
		}
	}
	else {
ok_format:
		format->flags &= ~GAME_CODEC_SET_FORMAT;
		format->format = info->cur_format;
	}
	if (format->flags & GAME_CODEC_SET_FRAME_RATE) {
		if (!(format->frame_rate & info->frame_rates)) {
			if ((format->frame_rate == B_SR_CVSR) || !(B_SR_CVSR & info->frame_rates))
			{
				ddprintf(("gamedriver: frame_rate 0x%lx is bad\n", format->frame_rate));
				err = B_MEDIA_BAD_FORMAT;
				goto ok_frame_rate;
			}
			else
			{
				format->cvsr = find_frame_rate(format->frame_rate);
				format->frame_rate = B_SR_CVSR;
			}
		}
		if ((format->frame_rate == info->cur_frame_rate) &&
				((info->cur_frame_rate != B_SR_CVSR) || (info->cur_cvsr == format->cvsr))) {
			goto ok_frame_rate;
		}
		if ((format->frame_rate == B_SR_CVSR) && ((format->cvsr < info->cvsr_min) ||
				(format->cvsr > info->cvsr_max))) {
			ddprintf(("gamedriver: cvsr %g is bad\n", format->cvsr));
			err = B_MEDIA_BAD_FORMAT;
			goto ok_frame_rate;
		}
	}
	else {
ok_frame_rate:
		format->flags &= ~GAME_CODEC_SET_FRAME_RATE;
		format->frame_rate = info->cur_frame_rate;
		format->cvsr = info->cur_cvsr;
	}

	//fixme:	we need better handling of FAIL_IF_DESTRUCTIVE

	if ((!err || (format->flags & GAME_CODEC_CLOSEST_OK)) && (format->flags & GAME_CODEC_SET_ALL)) {
		if (!pc->dev->set_codec_format) {
			eprintf(("\033[31mgamedriver: format change reqeust with no hook! (flags 0x%lx)\033[0m\n",
					format->flags));
			err = B_SHOULDNT_HAVE_DONE_THAT;
		}
		else {
			if ((format->flags & GAME_CODEC_SET_FRAME_RATE) && !(format->frame_rate & B_SR_CVSR))
			{
				format->cvsr = find_frame_rate(format->frame_rate);
			}
			err = pc->dev->set_codec_format(pc->dev, format);
			info->cur_channel_count = format->channels;
			info->cur_chunk_frame_count = format->chunk_frame_count;
			info->cur_format = format->format;
			info->cur_frame_rate = format->frame_rate;
			info->cur_cvsr = format->cvsr;
		}
	}
	format->out_status = err;
	return err;
}

static status_t
set_codec_formats(
	ga_cookie * pc,
	game_set_codec_formats * request)
{
	int ix;
	status_t err = B_WHATEVER;

	request->out_actual_count = 0;
	for (ix=0; ix<request->in_request_count; ix++) {
		status_t temp = set_codec_format(pc, &request->formats[ix]);
		if (!err) {
			err = temp;
		}
		if (!temp) {
			request->out_actual_count++;
		}
	}
	return (request->out_actual_count > 0) ? B_WHATEVER : err;
}

static status_t
get_mixer_info(
	ga_cookie * pc,
	game_mixer_info * o_info)
{
	ga_mixer_info * gmi = find_mixer(pc, o_info->mixer_id);
	if (!gmi)
	{
		o_info->mixer_id = GAME_NO_ID;
		return B_DONT_DO_THAT;
	}
	*o_info = gmi->info;
	d2printf(("gamedriver: mixer id %ld name %s\n", o_info->mixer_id, o_info->name));
	return B_OK;
}

static status_t
get_mixer_infos(
	ga_cookie * pc,
	game_get_mixer_infos * request)
{
	int ix;
	status_t err = B_WHATEVER;

	ddprintf(("gamedriver: get_mixer_infos(%s, count %ld)\n", pc->dev->short_name, request->in_request_count));

	request->out_actual_count = 0;
	for (ix=0; ix<request->in_request_count; ix++)
	{
		status_t temp = get_mixer_info(pc, &request->info[ix]);
		if (!err) err = temp;
		if (!temp) request->out_actual_count++;
	}
	return (request->out_actual_count > 0) ? B_WHATEVER : err;
}

static status_t
get_mixer_control(
	ga_cookie * pc,
	ga_mixer_info * info,
	int ix,
	game_mixer_control * o_control)
{
	if (ix < 0 || ix >= info->info.control_count)
	{
		o_control->control_id = GAME_NO_ID;
		return B_BAD_INDEX;
	}
	*o_control = info->controls.o_controls[ix];
	d2printf(("gamedriver: control ix %d id %ld\n", ix, o_control->control_id));
	return B_OK;
}

static status_t
get_mixer_controls(
	ga_cookie * pc,
	game_get_mixer_controls * request)
{
	int ix;
	status_t err = B_WHATEVER;
	ga_mixer_info * gmi = find_mixer(pc, request->mixer_id);

	ddprintf(("gamedriver: get_mixer_controls(%s, count %ld)\n", pc->dev->short_name, request->in_request_count));

	if (gmi == NULL)
	{
		eprintf(("\033[33mgamedriver: get_mixer_controls() with bad mixer id %ld\033[0m\n",
			request->mixer_id));
		return B_DONT_DO_THAT;
	}
	request->out_actual_count = 0;
	for (ix=0; ix<request->in_request_count; ix++)
	{
		status_t temp = get_mixer_control(pc, gmi, ix+request->from_ordinal, &request->control[ix]);
		if (!err) err = temp;
		if (!temp) request->out_actual_count++;
	}
	return (request->out_actual_count > 0) ? B_WHATEVER : err;
}

static status_t
get_mixer_level_info(
	ga_cookie * pc,
	game_get_mixer_level_info * level)
{
	ga_mixer_info * gmi = find_mixer(pc, level->mixer_id);
	int ix;

	ddprintf(("gamedriver: get_mixer_level_info(%s, mixer %ld, control %ld)\n",
		pc->dev->short_name, level->mixer_id, level->control_id));

	if (gmi == NULL)
	{
		eprintf(("\033[33mgamedriver: get_mixer_level_info() with bad mixer id %ld\033[0m\n",
			level->mixer_id));
		return B_DONT_DO_THAT;
	}
	for (ix = 0; ix < gmi->info.control_count; ix++)
	{
		if (gmi->controls.o_controls[ix].control_id == level->control_id)
		{
			if (gmi->controls.o_controls[ix].kind != GAME_MIXER_CONTROL_IS_LEVEL)
			{
				eprintf(("\033[33mgamedriver: request for level id %ld which is of kind %ld\033[0m\n",
					level->control_id, gmi->controls.o_controls[ix].kind));
				return B_WE_CANT_GO_ON_LIKE_THIS;
			}
			*level = *gmi->controls.o_infos[ix].level;
			return B_WHATEVER;
		}
	}
	eprintf(("\033[33mgamedriver: request for level id %ld which doesn't exist in mixer %ld\033[0m\n",
		level->control_id, level->mixer_id));
	return B_DONT_DO_THAT;
}

static status_t
get_mixer_mux_info(
	ga_cookie * pc,
	game_get_mixer_mux_info * mux)
{
	ga_mixer_info * gmi = find_mixer(pc, mux->mixer_id);
	int ix;

	ddprintf(("gamedriver: get_mixer_mux_info(%s, mixer %ld, control %ld)\n",
		pc->dev->short_name, mux->mixer_id, mux->control_id));

	if (gmi == NULL)
	{
		eprintf(("\033[33mgamedriver: get_mixer_mux_info() with bad mixer id %ld\033[0m\n",
			mux->mixer_id));
		return B_DONT_DO_THAT;
	}
	for (ix = 0; ix < gmi->info.control_count; ix++)
	{
		if (gmi->controls.o_controls[ix].control_id == mux->control_id)
		{
			game_mux_item * in_ptr = mux->items;
			int32 req_cnt = mux->in_request_count;

			if (req_cnt && !in_ptr)
			{
				eprintf(("\033[31mgamedriver: game_get_mixer_mux_info() with request count %ld and NULL items\033[0m\n",
					req_cnt));
			}
			if (gmi->controls.o_controls[ix].kind != GAME_MIXER_CONTROL_IS_MUX)
			{
				eprintf(("\033[33mgamedriver: request for mux id %ld which is of kind %ld\033[0m\n",
					mux->control_id, gmi->controls.o_controls[ix].kind));
				return B_WE_CANT_GO_ON_LIKE_THIS;
			}
			*mux = *gmi->controls.o_infos[ix].mux;
			if (in_ptr)
			{
				int act_count = min(mux->out_actual_count, req_cnt);
				if (act_count > 0 && in_ptr)
				{
					memcpy(in_ptr, mux->items, sizeof(game_mux_item)*act_count);
				}
			}
			mux->in_request_count = req_cnt;
			mux->items = in_ptr;
			return B_WHATEVER;
		}
	}
	eprintf(("\033[33mgamedriver: request for mux id %ld which doesn't exist in mixer %ld\033[0m\n",
		mux->control_id, mux->mixer_id));
	return B_DONT_DO_THAT;
}

static status_t
get_mixer_enable_info(
	ga_cookie * pc,
	game_get_mixer_enable_info * enable)
{
	ga_mixer_info * gmi = find_mixer(pc, enable->mixer_id);
	int ix;

	ddprintf(("gamedriver: get_mixer_enable_info(%s, mixer %ld, control %ld)\n",
		pc->dev->short_name, enable->mixer_id, enable->control_id));

	if (gmi == NULL)
	{
		eprintf(("\033[33mgamedriver: get_mixer_enable_info() with bad mixer id %ld\033[0m\n",
			enable->mixer_id));
		return B_DONT_DO_THAT;
	}
	for (ix = 0; ix < gmi->info.control_count; ix++)
	{
		if (gmi->controls.o_controls[ix].control_id == enable->control_id)
		{
			if (gmi->controls.o_controls[ix].kind != GAME_MIXER_CONTROL_IS_ENABLE)
			{
				eprintf(("\033[33mgamedriver: request for enable id %ld which is of kind %ld\033[0m\n",
					enable->control_id, gmi->controls.o_controls[ix].kind));
				return B_WE_CANT_GO_ON_LIKE_THIS;
			}
			*enable = *gmi->controls.o_infos[ix].enable;
			return B_WHATEVER;
		}
	}
	eprintf(("\033[33mgamedriver: request for enable id %ld which doesn't exist in mixer %ld\033[0m\n",
		enable->control_id, enable->mixer_id));
	return B_DONT_DO_THAT;
}

static int
find_mixer_control(
	gaplug_get_mixer_controls * ggmc,
	int id)
{
	int ix;
	int count = ggmc->i_mixer->control_count;

	for (ix = 0; ix < count; ix++)
	{
		if (id == ggmc->o_controls[ix].control_id)
			return ix;
	}
	return -1;
}

status_t
get_mixer_control_values(
	ga_cookie * pc,
	game_get_mixer_control_values * values)
{
	int ix;
	status_t err = B_WHATEVER;
	ga_mixer_info * mi = find_mixer(pc, values->mixer_id);
	gaplug_mixer_control_values * gmcv;
	gaplug_get_mixer_controls * ggmc;

	ddprintf(("gamedriver: get_mixer_control_values(%s, count %ld)\n", pc->dev->short_name, values->in_request_count));
	if (mi == NULL)
	{
		eprintf(("\033[33mgamedriver: get_mixer_control_values() for bad mixer ID %ld\033[0m\n",
			values->mixer_id));
		return B_DONT_DO_THAT;
	}
	if (values->values == NULL)
	{
		eprintf(("\033[33mgamedriver: get_mixer_control_values() with NULL values!\033[0m\n"));
		return B_DONT_DO_THAT;
	}
	gmcv = &mi->values;
	ggmc = &mi->controls;
	/*	collate each reqeusted control with actual request information to pass to the plug-in	*/
	gmcv->i_count = 0;
	for (ix=0; ix<values->in_request_count; ix++)
	{
		int cix = find_mixer_control(ggmc, values->values[ix].control_id);
		if (cix < 0)
		{
			eprintf(("\033[33mgamedriver: get_mixer_control_values() for bad control ID %ld\033[0m\n",
				values->values[ix].control_id));
			if (!err) err = B_DONT_DO_THAT;
			values->values[ix].control_id = GAME_NO_ID;
		}
		else
		{
			gaplug_mixer_control_value * cv = (gaplug_mixer_control_value *)&gmcv->i_values[gmcv->i_count];
			cv->i_cookie = ggmc->o_cookies[cix];
			cv->i_info = ggmc->o_infos[cix];
			cv->io_value = &values->values[ix];
			cv->io_value->mixer_id = ggmc->o_controls[cix].mixer_id;
			cv->io_value->kind = ggmc->o_controls[cix].kind;
			if (ggmc->o_controls[cix].kind == GAME_MIXER_CONTROL_IS_LEVEL)
			{
				values->values[ix].u.level.value_count = cv->i_info.level->value_count;
			}
			gmcv->i_count++; 
		}
	}
	ddprintf(("gamedriver: got %d control values\n", gmcv->i_count));
	if ((values->out_actual_count = gmcv->i_count) <= 0)
		return err;
	/*	pass request to the plug-in */
	pc->dev->get_mixer_control_values(pc->dev, gmcv);
	return B_WHATEVER;
}

status_t
set_mixer_control_values(
	ga_cookie * pc,
	game_set_mixer_control_values * values)
{
	int ix;
	status_t err = B_WHATEVER;
	ga_mixer_info * mi = find_mixer(pc, values->mixer_id);
	gaplug_mixer_control_values * gmcv;
	gaplug_get_mixer_controls * ggmc;

	ddprintf(("gamedriver: set_mixer_control_values(%s, mixer %ld, count %ld)\n",
		pc->dev->short_name, values->mixer_id, values->in_request_count));

	if (mi == NULL)
	{
		eprintf(("\033[33mgamedriver: set_mixer_control_values() for bad mixer ID %ld\033[0m\n",
			values->mixer_id));
		return B_DONT_DO_THAT;
	}
	if (values->values == NULL)
	{
		eprintf(("\033[33mgamedriver: set_mixer_control_values() with NULL values!\033[0m\n"));
		return B_DONT_DO_THAT;
	}
	gmcv = &mi->values;
	ggmc = &mi->controls;
	/*	collate each reqeusted control with actual request information to pass to the plug-in	*/
	gmcv->i_count = 0;
	for (ix=0; ix<values->in_request_count; ix++)
	{
		int cix = find_mixer_control(ggmc, values->values[ix].control_id);
		if (cix < 0)
		{
			eprintf(("\033[33mgamedriver: set_mixer_control_values() for bad control ID %ld\033[0m\n",
				values->values[ix].control_id));
			if (!err) err = B_DONT_DO_THAT;
			values->values[ix].control_id = GAME_NO_ID;
		}
		else
		{
			gaplug_mixer_control_value * cv = (gaplug_mixer_control_value *)&gmcv->i_values[gmcv->i_count];
			cv->i_cookie = ggmc->o_cookies[cix];
			cv->i_info = ggmc->o_infos[cix];
			cv->io_value = &values->values[ix];
			gmcv->i_count++; 
		}
	}
	ddprintf(("gamedriver: set %d control values\n", gmcv->i_count));
	if ((values->out_actual_count = gmcv->i_count) <= 0)
		return err;
	/*	pass request to the plug-in */
	pc->dev->set_mixer_control_values(pc->dev, gmcv);
	return B_WHATEVER;
}

status_t
open_stream(
	ga_cookie * pc,
	game_open_stream * request)
{
	//fixme:	this code assumes dac_info == adc_info
	int ordinal = GAME_DAC_ORDINAL(request->codec_id);
	game_codec_info *di;
	ga_active_stream_info * asip;
	int ix;
	status_t err = B_OK;
	game_stream_capabilities * caps;

	ddprintf(("gamedriver: open_stream(codec %ld)\n", request->codec_id));

	if (GAME_IS_DAC(request->codec_id))
	{
		if ((ordinal < 0) || (ordinal >= pc->module->dac_count))
		{
			eprintf(("gamedriver: open_stream() for bad DAC ID %ld\n",
					request->codec_id));
			return B_DONT_DO_THAT;
		}
		di = pc->module->dac_infos+ordinal;
		asip = pc->module->dac_streams[ordinal];
		caps = &di->stream_capabilities;
	}
	else if (GAME_IS_ADC(request->codec_id))
	{
		if ((ordinal < 0) || (ordinal >= pc->module->adc_count))
		{
			eprintf(("gamedriver: open_stream() for bad ADC ID %ld\n",
					request->codec_id));
			return B_DONT_DO_THAT;
		}
		di = (game_codec_info *)(pc->module->adc_infos+ordinal);
		asip = pc->module->adc_streams[ordinal];
		caps = &di->stream_capabilities;
	}
	else
	{
		eprintf(("gamedriver: open_stream() for non-codec ID %ld\n",
				request->codec_id));
		return B_DONT_DO_THAT;
	}
	if (request->request & ~(caps->capabilities))
	{
		eprintf(("gamedriver: open_stream() requesting flags 0x%lx when only 0x%lx available\n",
				request->request, caps->capabilities));
		return B_MEDIA_BAD_FORMAT;
	}
	if (di->cur_stream_count >= di->max_stream_count)
	{
		ddprintf(("gamedriver: codec already has %ld streams\n",
				di->cur_stream_count));
		return B_IM_WAY_AHEAD_OF_YOU;
	}
	for (ix=0; ix<di->max_stream_count; ix++)
	{
		if (asip[ix].owner == NULL)
		{
			asip += ix;
			goto found_stream;
		}
	}
	ASSERT((ix == -1));	/*	really a trespass -- shouldn't be here!	*/
	return B_BLAME_THE_HARDWARE;

found_stream:
	ASSERT(asip->info.codec_id == di->codec_id);
	if (!(request->request & GAME_STREAM_FR))
	{
		request->frame_rate = di->cur_frame_rate;
		request->cvsr_rate = di->cur_cvsr;
	}
	else if (!(request->frame_rate & caps->frame_rates))
	{
		if (caps->frame_rates & B_SR_CVSR)
		{
			request->cvsr_rate = find_frame_rate(request->frame_rate);
			request->frame_rate = B_SR_CVSR;
		}
		if (!(request->frame_rate & caps->frame_rates) ||
			((request->frame_rate == B_SR_CVSR) && ((request->cvsr_rate < caps->cvsr_min) ||
				(request->cvsr_rate > caps->cvsr_max))))
		{
			eprintf(("gamedriver: open_stream() requested frame rate 0x%lx (cvsr %d) is bad\n", request->frame_rate, (int)request->cvsr_rate));
			ddprintf(("gamedriver: caps = 0x%lx  cvsr_min = %d  cvsr_max = %d\n", caps->frame_rates, (int)caps->cvsr_min, (int)caps->cvsr_max));
			err = B_DONT_DO_THAT;
		}
	}
	else if (!(request->frame_rate & B_SR_CVSR))
	{
		request->cvsr_rate = find_frame_rate(request->frame_rate);
	}	
	if (!(request->request & GAME_STREAM_FORMAT))
	{
		request->format = di->cur_format;
	}
	else if (!(request->format & caps->formats))
	{
		eprintf(("gamedriver: open_stream() requested format 0x%lx is bad\n", request->format));
		err = B_DONT_DO_THAT;
	}
	if (!(request->request & GAME_STREAM_CHANNEL_COUNT))
	{
		request->channel_count = di->cur_channel_count;
	}
	else if (!((1<<(request->channel_count-1)) & caps->channel_counts))
	{
		eprintf(("gamedriver: open_stream() requested channel_count %ld is bad\n", request->channel_count));
		err = B_DONT_DO_THAT;
	}
	if (err < 0)
	{
		return err;
	}
	asip->stream_cookie = NULL;
	asip->feed.cookie = asip;
	asip->feed.int_done = ga_int_done;
	asip->feed.update_timing = ga_update_timing;
	ASSERT(sizeof(asip->info) == sizeof(*request));
	request->out_stream_id = asip->info.out_stream_id;
	err = pc->dev->init_stream(pc->dev, request, &asip->feed, &asip->stream_cookie);
	if (err < 0)
	{
		eprintf(("gamedriver: %s returns 0x%lx from init_stream()\n",
				pc->dev->short_name, err));
		request->out_stream_id = GAME_NO_ID;
		return err;
	}
	memcpy(&asip->info, request, sizeof(*request));
	asip->frame_size = 1;
	asip->frame_size *= find_sample_size(request->format);
	asip->frame_size *= request->channel_count;
	ASSERT((asip->frame_size > 0));
	if (asip->frame_size == 0)
	{
		eprintf(("gamedriver: %s messed with open_stream() request so frame_size is 0!\n", pc->dev->short_name));
		return B_BLAME_THE_HARDWARE;
	}
	asip->owner = pc;
	di->cur_stream_count++;
	asip->release_count = 0;
	asip->next_closing = 0;
	asip->controls.stream = request->out_stream_id;
	asip->controls.caps = request->request;
	asip->controls.volume = caps->normal_point;
	asip->controls.lr_pan = 0;
	asip->controls.fb_pan = 0;
	asip->controls.frame_rate = request->cvsr_rate;
	asip->capabilities = caps;
	ddprintf(("gamedriver: open_stream() returns stream %ld (which is %ld of %ld)\n",
		request->out_stream_id, di->cur_stream_count, di->max_stream_count));
	return B_WHATEVER;
}

static status_t
get_stream_timing(
	ga_cookie * pc,
	game_get_timing * timing)
{
	ga_active_stream_info * gasi = find_stream(pc, timing->source);
	status_t err = B_OK;

	if (gasi == NULL)
	{
		eprintf(("\033[31mgamedriver: get_timing() on bad stream %ld\033[0m\n", timing->source));
		return B_DONT_DO_THAT;
	}
	ASSERT_LOCKED(pc->module);
	if (timing->flags & GAME_ACQUIRE_STREAM_SEM)
	{
		sem_id sem = gasi->info.stream_sem;
		unbenlock_active(pc->module DEBUG_ARGS2);
		pc->module->locked = 0;
		
		err = acquire_sem_etc(sem, 1, B_CAN_INTERRUPT |
			(timing->timeout_at ? B_ABSOLUTE_TIMEOUT : 0), timing->timeout_at);
			
		benlock_active(pc->module DEBUG_ARGS2);
		pc->module->locked = LOCKVAL;
		gasi = find_stream(pc, timing->source);
		if (gasi == NULL)
		{
			eprintf(("\033[31mgamedriver: get_timing(): stream %ld expired while blocked\033[0m\n", timing->source));
			return B_DONT_DO_THAT;
		}
	}
	if (err == B_OK)
	{
		SPINLOCK_ACTIVE(pc->module)
		{
			timing->state = gasi->state;
			timing->timing = gasi->timing;
		}
	}
	return err;
}

static status_t
get_codec_timing(
	ga_cookie * pc,
	game_get_timing * timing)
{
	game_codec_info * gci = find_codec(pc, timing->source);

	if (gci == NULL)
	{
		eprintf(("\033[31mgamedriver: get_timing() on bad codec %ld\033[0m\n", timing->source));
		return B_DONT_DO_THAT;
	}
	ASSERT_LOCKED(pc->module);
#warning implement get_codec_timing
	return B_OK;
}

static status_t
get_stream_controls(
	ga_cookie * pc,
	game_get_stream_controls * controls)
{
	int ix;
	status_t err = B_OK;

	if (controls->controls == NULL)
	{
		eprintf(("\033[31mgamedriver: controls->stream is NULL in get_stream_controls()\033[0m\n"));
		return B_DONT_DO_THAT;
	}
	controls->out_actual_count = 0;

	for (ix = 0; ix < controls->in_request_count; ix++)
	{
		ga_active_stream_info * gasi = find_stream(pc, controls->controls[ix].stream);
		if (gasi == NULL)
		{
			eprintf(("\033[31mgamedriver: get_stream_controls() on bad stream %ld\033[0m\n", controls->controls[ix].stream));
			err = B_DONT_DO_THAT;
			controls->controls[ix].stream = GAME_NO_ID;
		}
		else
		{
			controls->controls[ix] = gasi->controls;
			controls->out_actual_count++;
		}
	}

	return err;
}

static status_t
set_stream_controls(
	ga_cookie * pc,
	game_set_stream_controls * controls)
{
	int ix;
	status_t err = B_OK;
	void * temp_cookies[8];
	game_open_stream * temp_infos[8];
	void ** cookies;
	game_open_stream ** infos;
	int total = 0;

	if (controls->controls == NULL)
	{
		eprintf(("\033[31mgamedriver: controls->stream is NULL in set_stream_controls()\033[0m\n"));
		return B_DONT_DO_THAT;
	}
	controls->out_actual_count = 0;
	if (controls->in_request_count <= 8)
	{
		cookies = temp_cookies;
		infos = temp_infos;
	}
	else
	{
		cookies = (void **)calloc(controls->in_request_count, sizeof(void *));
		infos = (game_open_stream **)calloc(controls->in_request_count, sizeof(game_open_stream *));
	}

	for (ix = 0; ix < controls->in_request_count; ix++)
	{
		ga_active_stream_info * gasi = find_stream(pc, controls->controls[ix].stream);
		if (gasi == NULL)
		{
			eprintf(("\033[31mgamedriver: get_stream_controls() on bad stream %ld\033[0m\n", controls->controls[ix].stream));
its_an_error:
			err = B_DONT_DO_THAT;
			controls->controls[ix].stream = GAME_NO_ID;
			cookies[ix] = 0;
			infos[ix] = 0;
		}
		else if (controls->controls[ix].caps & ((~gasi->info.request)|GAME_STREAM_FORMAT|GAME_STREAM_CHANNEL_COUNT))
		{
			eprintf(("\033[31mgamedriver: get_stream_controls() on stream %ld with bad caps 0x%lx\033[0m\n", controls->controls[ix].stream, controls->controls[ix].caps));
			goto its_an_error;
		}
		else
		{
			if ((controls->controls[ix].caps & GAME_STREAM_VOLUME) == 0)
			{
				controls->controls[ix].volume = gasi->controls.volume;
			}
			else if ((controls->controls[ix].volume < gasi->capabilities->min_volume) ||
				(controls->controls[ix].volume > gasi->capabilities->max_volume))
			{
				eprintf(("\033[31mgamedriver: volume %ld is beyond acceptable values (%ld - %ld)\033[0m\n",
					controls->controls[ix].volume, gasi->capabilities->min_volume, gasi->capabilities->max_volume));
				goto its_an_error;
			}
			if ((controls->controls[ix].caps & GAME_STREAM_PAN) == 0)
			{
				controls->controls[ix].lr_pan = gasi->controls.lr_pan;
				controls->controls[ix].fb_pan = gasi->controls.fb_pan;
			}
			else if ((controls->controls[ix].lr_pan == -32768) || (controls->controls[ix].fb_pan == -32768))
			{
				eprintf(("\033[31mgamedriver: allowed panning range is -32767 to +32767\033[0m\n"));
				goto its_an_error;
			}
			if ((controls->controls[ix].caps & GAME_STREAM_FR) == 0)
			{
				controls->controls[ix].frame_rate = gasi->controls.frame_rate;
			}
			else if ((controls->controls[ix].frame_rate < gasi->capabilities->cvsr_min) ||
				(controls->controls[ix].volume > gasi->capabilities->cvsr_max))
			{
				eprintf(("\033[31mgamedriver: volume %ld is beyond acceptable values\033[0m\n", controls->controls[ix].volume));
				goto its_an_error;
			}
			cookies[ix] = gasi->stream_cookie;
			infos[ix] = &gasi->info;
			total++;
		}
	}
	if ((total > 0) && (pc->dev->set_stream_controls))
	{
		status_t err2 = pc->dev->set_stream_controls(pc->dev, controls, cookies, infos);
		if (err2 < 0)
		{
			eprintf(("\033[31mgamedriver: %s returns %s from set_stream_controls()\033[0m\n",
				pc->dev->short_name, strerror(err2)));
			err = err2;
		}
	}
	for (ix = 0; ix < controls->in_request_count; ix++)
	{
		if (controls->controls[ix].stream != GAME_NO_ID)
		{
			ga_active_stream_info * gasi;
			ASSERT((&((ga_active_stream_info *)0)->info) == NULL);
			gasi = (ga_active_stream_info *)infos[ix];
			if (gasi->controls.stream != controls->controls[ix].stream)
			{
				eprintf(("gamedriver: %s messed with the 'stream' member of game_set_stream_controls\n", pc->dev->short_name));
			}
			if (gasi->controls.caps != controls->controls[ix].caps)
			{
				eprintf(("gamedriver: %s messed with the 'caps' member of game_set_stream_controls\n", pc->dev->short_name));
			}
			gasi->controls = controls->controls[ix];
		}
	}
	if (cookies != temp_cookies)
	{
		free(cookies);
		free(infos);
	}

	return err;
}

static status_t
close_stream(
	ga_cookie * pc,
	game_close_stream * request)
{
	ga_active_stream_info * gasi = find_stream(pc, request->stream);
	status_t err = B_OK;
	sem_id stream_sem;

	if (!gasi)
	{
		eprintf(("\033[31mgamedriver: close_stream() called on bad stream %ld\033[0m\n", request->stream));
		return B_DONT_DO_THAT;
	}
	if (gasi->owner != pc)
	{
		eprintf(("\033[31mgamedriver: close_stream() by 0x%08lx on stream %ld owned by 0x%08lx\033[0m\n",
			(uint32)pc, request->stream, (uint32)gasi->owner));
		return B_IM_AFRAID_I_CANT_LET_YOU_DO_THAT;
	}

	stream_sem = gasi->info.stream_sem;
	remove_stream(pc, gasi);
	if ((request->flags & GAME_CLOSE_DELETE_SEM_WHEN_DONE) != 0)
	{
		(void)delete_sem(stream_sem);
	}
	return err;
}

static status_t
open_stream_buffer(
	ga_cookie * pc,
	game_open_stream_buffer * buf)
{
	status_t err = B_WHATEVER;
	gaplug_open_stream_buffer gosb;
	bool owner = false;
	void * addr;
	size_t round = B_PAGE_SIZE;
	area_info ai;
	int ix;
	char name[32];
	ga_active_buffer_info * gab;
	ga_active_stream_info * gas;

	ddprintf(("gamedriver: open_stream_buffer(%p, %p)\n", pc, buf));

	buf->area = 0;
	buf->buffer = GAME_NO_ID;
	memset(&gosb, 0, sizeof(gosb));
	gosb.io_request = buf;
	//fixme:	beautify the handling of active stream/open stream structs
	if (buf->stream == GAME_NO_ID)
	{
		gas = NULL;
	}
	else
	{
		gas = find_stream(pc, buf->stream);
		if (gas == NULL)
		{
			eprintf(("\033[31mgamedriver: open_stream_buffer on unknown stream %ld\033[0m\n", buf->stream));
			return B_DONT_DO_THAT;
		}
	}
	if (!gas)
	{
		eprintf(("\033[31mgamedriver: open_stream_buffer() on no stream is illegal.\033[0m\n"));
		return B_WE_CANT_GO_ON_LIKE_THIS;
	}
	gosb.in_stream = &gas->info;
	gosb.in_codec = find_codec(pc, gosb.in_stream->codec_id);
	ASSERT(gosb.in_codec != NULL);
	if (!gosb.in_codec)
	{
		eprintf(("\033[31mgamedriver: find_codec(%ld) returns NULL?\033[0m\n", gosb.in_stream->codec_id));
		return B_BLAME_THE_HARDWARE;
	}
	//	we round up to a whole chunk, and then again up to a page
	round = gosb.in_codec->cur_chunk_frame_count * gosb.in_codec->cur_channel_count *
			find_sample_size(gosb.in_codec->cur_format);
	if (round < B_PAGE_SIZE)
	{
		round = B_PAGE_SIZE;
	}
	gosb.in_stream_cookie = ((ga_active_stream_info *)gosb.in_stream)->stream_cookie;
	gosb.io_size = (buf->byte_size + round-1);
	gosb.io_size -= gosb.io_size % round;
	if (round != B_PAGE_SIZE)
	{
		gosb.io_size = (gosb.io_size + B_PAGE_SIZE-1) & -B_PAGE_SIZE;
	}

	ddprintf(("gamedriver: size %ld rounds to %ld\n", buf->byte_size, gosb.io_size));

	gosb.io_lock = B_FULL_LOCK;
	gosb.io_protection = 0;
	if (pc->dev->open_stream_buffer)
	{
		err = pc->dev->open_stream_buffer(pc->dev, &gosb);
		if (err < 0)
		{
			ddprintf(("gamedriver: %s returns 0x%lx from open_stream_buffer\n",
					pc->dev->short_name, err));
			return err;
		}
		if (buf->area > 0)
		{
			goto allocated;
		}
	}
	owner = true;
	sprintf(name, "gameaudio %s", pc->dev->short_name);
	buf->area = create_area(name, &addr, B_ANY_KERNEL_ADDRESS, gosb.io_size,
			gosb.io_lock, gosb.io_protection);

	ddprintf(("gamedriver: create_area(%ld) returns 0x%lx\n", gosb.io_size, buf->area));

	if (buf->area < 0)
	{
		eprintf(("gamedriver: open_buffer(%ld) returns 0x%lx\n", buf->byte_size, err));
		return buf->area;
	}
	buf->offset = 0;

allocated:
	err = get_area_info(buf->area, &ai);
	if (err < 0)
	{
		eprintf(("gamedriver: get_area_info(%ld) returns 0x%lx\n", buf->area, err));
		return err;
	}
	for (ix=0; ix<pc->module->buffer_count; ix++)
	{
		if (pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK].id == GAME_NO_ID)
		{
			goto got_slot;
		}
	}
	//	we're out of buffer slots; make some more!
	ddprintf(("gamedriver: realloc buffer slots (exhausted count %d)\n", pc->module->buffer_count));
	addr = calloc(((pc->module->buffer_count>>BLOCK_SHIFT)+1), sizeof(ga_active_buffer_info*));
	if (addr == NULL)
	{
		eprintf(("\033[31mgamedriver: no space to reallocate buffer array\033[0m\n"));
nomem:
		if (owner)
		{
			delete_area(buf->area);
		}
		else
		{
			pc->dev->close_stream_buffer(pc->dev, buf);
		}
		return B_OH_OH;
	}
	{
		void * tofree = 0;
		SPINLOCK_ACTIVE(pc->module)
		{
			if (pc->module->buffers)
			{
				memcpy(addr, pc->module->buffers, (pc->module->buffer_count>>BLOCK_SHIFT)*
					sizeof(ga_active_buffer_info*));
				tofree = pc->module->buffers;
			}
			pc->module->buffers = (ga_active_buffer_info **)addr;
		}
		pc->module->buffers[pc->module->buffer_count>>BLOCK_SHIFT] = (ga_active_buffer_info *)
			calloc(BUFFERS_IN_A_BLOCK, sizeof(ga_active_buffer_info));
		free(tofree);
	}
	if (!pc->module->buffers[pc->module->buffer_count>>BLOCK_SHIFT])
	{
		eprintf(("\033[31mgamedriver: no space for new buffer block\033[0m\n"));
		goto nomem;
	}
	gab = pc->module->buffers[ix>>BLOCK_SHIFT];
	for (ix = 0; ix < BUFFERS_IN_A_BLOCK; ix++)
	{
		gab[ix].id = GAME_NO_ID;
	}
	ix = pc->module->buffer_count;
	pc->module->buffer_count += BUFFERS_IN_A_BLOCK;

got_slot:
	gab = &pc->module->buffers[ix>>BLOCK_SHIFT][ix&BLOCK_MASK];
	memset(gab, 0, sizeof(ga_active_buffer_info));
	gab->id = GAME_MAKE_BUFFER_ID(ix);
	buf->buffer = GAME_MAKE_BUFFER_ID(ix);
	gab->stream = buf->stream;
	gab->area = buf->area;
	gab->base = ai.address;
	gab->offset = buf->offset;
	gab->size = ai.size;
	gab->owner = pc;
	if (pc->dev->bind_stream_buffer)
	{
		pc->dev->bind_stream_buffer(pc->dev, &gosb, gab->id);
	}
	return B_WHATEVER;
}

static status_t
open_stream_buffers(
	ga_cookie * pc,
	game_open_stream_buffers * i_bufs)
{
	status_t err = B_OK;
	int ix;

	if (!i_bufs->buffers)
	{
		eprintf(("\033[31mgamedriver: open_stream_buffers() with NULL buffers\033[0m\n"));
		return B_DONT_DO_THAT;
	}
	ddprintf(("gamedriver: open_stream_buffers(%s, count %ld)\n", pc->dev->short_name, i_bufs->in_request_count));
	i_bufs->out_actual_count = 0;
	for (ix=0; ix<i_bufs->in_request_count; ix++)
	{
		if ((i_bufs->buffers[ix].out_status = open_stream_buffer(pc, i_bufs->buffers+ix)) < 0)
		{
			if (!err)
			{
				err = i_bufs->buffers[ix].out_status;
			}
		}
		else
		{
			i_bufs->out_actual_count++;
		}
	}
	return (i_bufs->out_actual_count > 0) ? B_OK : err;
}

static status_t
close_stream_buffer(
	ga_cookie * pc,
	game_close_stream_buffer * i_buf)
{
	status_t err = B_WHATEVER;

	ddprintf(("gamedriver: close_stream_buffer(0x%08lx, buffer %ld)\n", (uint32)pc, i_buf->buffer));

	if (i_buf->buffer == GAME_NO_ID)
	{
		eprintf(("\033[31mgamedriver: close_stream_buffer(GAME_NO_ID)\033[0m\n"));
		return B_DONT_DO_THAT;
	}

	LOCK_GLOBALS
	{
		ga_active_buffer_info * buffer = find_buffer(pc, i_buf->buffer);
		if (!buffer)
		{
			eprintf(("\033[31mgamedriver: close_stream_buffer() on unknown buffer id %ld\033[0m\n", i_buf->buffer));
			i_buf->buffer = GAME_NO_ID;
			err = B_DONT_DO_THAT;
		}
		else if (buffer->owner != pc)
		{
			eprintf(("\033[31mgamedriver: close_stream_buffer() on buffer %ld (owner 0x%08lx != 0x%08lx)\033[0m\n", i_buf->buffer, (uint32)buffer->owner, (uint32)pc));
			err = B_IM_AFRAID_I_CANT_LET_YOU_DO_THAT;
		}
		else if (buffer->count > 0)
		{
			eprintf(("\033[31mgamedriver: close_stream_buffer() on buffer %ld with refcount %d\033[0m\n", i_buf->buffer, buffer->count));
			err = B_IM_WAY_AHEAD_OF_YOU;
		}
		else
		{
			remove_buffer(pc, buffer);
		}
	}
	return err;
}

static status_t
close_stream_buffers(
	ga_cookie * pc,
	game_close_stream_buffers * i_bufs)
{
	status_t err = B_OK;
	int ix;

	if (!i_bufs->buffers)
	{
		eprintf(("\033[31mgamedriver: close_stream_buffers() with NULL buffers\033[0m\n"));
		return B_DONT_DO_THAT;
	}
	ddprintf(("gamedriver: close_stream_buffers(%s, count %ld)\n", pc->dev->short_name, i_bufs->in_request_count));
	i_bufs->out_actual_count = 0;
	for (ix=0; ix<i_bufs->in_request_count; ix++)
	{
		status_t temp;
		if ((temp = close_stream_buffer(pc, i_bufs->buffers+ix)) < 0)
		{
			if (!err)
			{
				err = temp;
			}
		}
		else
		{
			i_bufs->out_actual_count++;
		}
	}
	return (i_bufs->out_actual_count > 0) ? B_OK : err;
}

static status_t
set_stream_buffer(
	ga_cookie * pc,
	game_set_stream_buffer * i_buf)
{
	ga_active_stream_info * as;
	status_t err = B_WHATEVER;
	gaplug_set_buffer gsb;

	d2printf(("gamedriver: set_stream_buffer(%p, buffer %ld)\n", pc, i_buf->buffer));

	as = find_stream(pc, i_buf->stream);
	if (!as)
	{
		eprintf(("\033[31mgamedriver: set_stream_buffer() on bad stream %ld\033[0m\n", i_buf->stream));
		return B_DONT_DO_THAT;
	}
	if (as->state != GAME_STREAM_STOPPED)
	{
		eprintf(("\033[31mgamedriver: set_stream_buffer() on running stream %ld\033[0m\n", i_buf->stream));
		return B_DONT_DO_THAT;
	}
	if (i_buf->buffer == GAME_NO_ID)
	{
		SPINLOCK_ACTIVE(pc->module)
		{
			if (as->buffer != NULL)
			{
				as->buffer->count--;
				d2printf(("gamedriver: decrementing count for buffer %ld to %d\n",
					as->buffer->id, as->buffer->count));
			}
			as->buffer = NULL;
		}
		if (pc->dev->set_stream_buffer != NULL)
		{
			memset(&gsb, 0, sizeof(gsb));
			gsb.io_request = i_buf;
			gsb.in_stream = &as->info;
			gsb.in_stream_cookie = as->stream_cookie;
			gsb.in_area = -1;
			gsb.in_base = NULL;
			gsb.in_offset = 0;
			gsb.in_size = 0;
			err = pc->dev->set_stream_buffer(pc->dev, &gsb);
			if (err != B_OK)
			{
				ddprintf(("gamedriver: plug-in set_stream_buffer(NULL) returns 0x%lx\n", err));
				return err;
			}
		}
	}
	else
	{
		ga_active_buffer_info * ab;

		ab = find_buffer(pc, i_buf->buffer);
		if (!ab)
		{
			eprintf(("\033[31mgamedriver: set_stream_buffer() on bad buffer %ld\033[0m\n", i_buf->buffer));
			return B_DONT_DO_THAT;
		}
		ASSERT(as->frame_size > 0);
		if (!as->frame_size)
		{
			eprintf(("\033[31mgamedriver: internal error (frame_size is %ld in set_stream_buffer)\033[0m\n", as->frame_size));
			return B_BLAME_THE_HARDWARE;
		}
		if ((i_buf->frame_count <= 0) || (i_buf->frame_count > ab->size/as->frame_size))
		{
			eprintf(("\033[31mgamedriver: set_stream_buffer() with bad frame_count %ld\033[0m\n", i_buf->frame_count));
			return B_DONT_DO_THAT;
		}
		if ((i_buf->flags & GAME_BUFFER_PING_PONG) &&
			(!i_buf->page_frame_count || (i_buf->frame_count % i_buf->page_frame_count)))
		{
			eprintf(("\033[31mgamedriver: set_stream_buffer() with PING_PONG and bad page_frame_count %ld for frame_count %ld\033[0m\n",
				i_buf->page_frame_count, i_buf->frame_count));
			return B_WE_CANT_GO_ON_LIKE_THIS;
		}
		SPINLOCK_ACTIVE(pc->module)
		{
			if (as->buffer != NULL)
			{
				as->buffer->count--;
				d2printf(("gamedriver: decrementing count for buffer %ld to %d\n",
					as->buffer->id, as->buffer->count));
			}
			ab->count++;
			d2printf(("gamedriver: incrementing count for buffer %ld to %d\n", ab->id, ab->count));
			as->page_frame_count = (i_buf->flags & GAME_BUFFER_PING_PONG) ? i_buf->page_frame_count : 0;
			as->buffer = ab;
		}
		if (pc->dev->set_stream_buffer != NULL)
		{
			memset(&gsb, 0, sizeof(gsb));
			gsb.io_request = i_buf;
			gsb.in_stream = &as->info;
			gsb.in_stream_cookie = as->stream_cookie;
			gsb.in_area = ab->area;
			gsb.in_base = ab->base;
			gsb.in_offset = ab->offset;
			gsb.in_size = ab->size;
			err = pc->dev->set_stream_buffer(pc->dev, &gsb);
			if (err != B_OK)
			{
				ddprintf(("gamedriver: plug-in set_stream_buffer(%ld) returns 0x%lx\n", ab->area, err));
				return err;
			}
		}
	}
	return B_WHATEVER;
}

static status_t
run_streams(
	ga_cookie * pc,
	game_run_streams * run)
{
	gaplug_run_streams grs;
	int ix, iy;
	status_t err = B_WHATEVER;
	gaplug_run_stream_info temp_infos[8];
	gaplug_run_stream_info * a_infos = NULL;

	if ((run->in_stream_count < 0) || (run->streams == NULL))
	{
		eprintf(("\033[31mgamedriver: run_streams(%ld, %p) bad arguments\033[0m\n", run->in_stream_count, run->streams));
		return B_DONT_DO_THAT;
	}

	if (run->in_stream_count <= 8)
	{
		a_infos = temp_infos;
		memset(a_infos, 0, sizeof(temp_infos));
	}
	else
	{
		a_infos = (gaplug_run_stream_info *)calloc(run->in_stream_count, sizeof(gaplug_run_stream_info));
	}
	LOCK_GLOBALS
	{
		int req_count = 0;
		run->out_actual_count = 0;
		for (ix=0; ix<run->in_stream_count; ix++)
		{
			ga_active_stream_info *asi;
			if (((asi = find_stream(pc, run->streams[ix].stream)) == NULL) ||
					(asi->owner != pc) || (asi->buffer == NULL))
			{
				eprintf(("\033[31mgamedriver: run_streams() on bad or unconfigured stream %ld\033[0m\n", run->streams[ix].stream));
bad_stream:
				run->streams[ix].stream = GAME_NO_ID;
			}
			else
			{
				/*	We introduce O(n2) here to do some extra sanity checking. That may or may not
				 *	be the right thing. I think the number of streams run at the same time will
				 *	always be low enough that it's OK.	*/
				for (iy=0; iy<run->out_actual_count; iy++)
				{
					if (grs.info[iy].stream == &asi->info)
					{
						eprintf(("\033[33mgamedriver: run_streams() on the same stream (%ld) more than once in the same request\033[0m\n",
								grs.info[iy].stream->out_stream_id));
						goto bad_stream;
					}
				}
				a_infos[req_count].request = &run->streams[ix];
				a_infos[req_count].stream = &asi->info;
				a_infos[req_count].stream_cookie = asi->stream_cookie;
				switch (run->streams[ix].state)
				{
				case GAME_STREAM_RUNNING:
					if (asi->state == GAME_STREAM_STOPPED)
					{
						//	reset the running counter
						asi->timing.cur_frame_rate = asi->info.cvsr_rate;
						asi->timing.frames_played = 0;
					}
					asi->timing.at_time = system_time();
					run->streams[ix].out_timing = asi->timing;
					a_infos[req_count].looping.address = asi->buffer->base;
					a_infos[req_count].looping.frame_count = asi->buffer->size/asi->frame_size;
					a_infos[req_count].looping.page_frame_count = asi->page_frame_count;
					a_infos[req_count].looping.buffer = asi->buffer->id;
					break;
				case GAME_STREAM_STOPPED:
					SPINLOCK_ACTIVE(pc->module)
					{
						ASSERT(asi->timing.cur_frame_rate > 3.e3 && asi->timing.cur_frame_rate < 4.e6);
						if (!(asi->timing.cur_frame_rate > 3.e3 && asi->timing.cur_frame_rate < 4.e6))
						{
							eprintf(("\033[33mgamedriver: why is cur_frame_rate %g for stream %ld?\033[0m\n", asi->timing.cur_frame_rate, asi->info.out_stream_id));
							asi->timing.cur_frame_rate = 44100.0;
						}
						run->streams[ix].out_timing = asi->timing;
					}
					break;
				default:
					eprintf(("\033[31mgamedriver: run_streams() on stream %ld with bad state %ld\033[0m\n", run->streams[ix].stream, run->streams[ix].state));
					goto bad_stream;
				}
				req_count++;
			}
		}
d2printf(("gamedriver: req_count is %d\n", req_count));
		if (req_count > 0)
		{
			run->out_actual_count = req_count;
			grs.total_slots = req_count;
			grs.io_request = run;
			grs.info = a_infos;
			err = pc->dev->run_streams(pc->dev, &grs);
			SPINLOCK_ACTIVE(pc->module)
			{
				if (err == B_WHATEVER) for (ix=0; ix<grs.total_slots; ix++)
				{
					if (run->streams[ix].stream != GAME_NO_ID)
					{
						game_stream_state st = run->streams[ix].state;
						((ga_active_stream_info *)grs.info[ix].stream)->state = st;
					}
				}
			}
		}
		else
		{
			eprintf(("\033[31mgamedriver: can't run any streams!\033[0m\n"));
		}
	}
	if (a_infos != temp_infos)
	{
		free(a_infos);
	}

	return err;
}

static const char *
ctrl_name(
	uint32 i_code)
{
	if ((i_code < GAME_GET_INFO) || (i_code >= GAME_GET_INFO+sizeof(ctrl_info)/sizeof(ctrl_info[0])))
	{
		return "unknown";
	}
	return ctrl_info[i_code-GAME_GET_INFO].name;
}

static size_t
get_size(
	uint32 i_code)
{
	if ((i_code < GAME_GET_INFO) || (i_code >= GAME_GET_INFO+sizeof(ctrl_info)/sizeof(ctrl_info[0])))
	{
		return 0;	//	gotta assume it's OK
	}
	return ctrl_info[i_code-GAME_GET_INFO].size;
}

static int
check_size(
	uint32 i_code,
	size_t size,
	size_t *o_size)
{
	size_t realSize = get_size(i_code);
	if ((i_code < GAME_GET_INFO) || (i_code >= GAME_GET_INFO+sizeof(ctrl_info)/sizeof(ctrl_info[0])))
	{
		return 1;	//	gotta assume it's OK
	}
	*o_size = realSize;
	if (realSize > size)
	{
		return 0;
	}
	if (size > 0x10000UL)	//	detect some cases of garbage data
	{
		return 0;
	}
	return 1;
}

status_t
dev_control(
	void * i_cookie,
	uint32 i_code,
	void * io_data,
	size_t i_size)
{
	ga_cookie * pc = (ga_cookie *)i_cookie;
	status_t err = B_IM_SORRY_DAVE;
	size_t r_size = 0;

	d2printf(("dev_control(%p, 0x%lx, %p) [%s]\n", i_cookie, i_code, io_data, ctrl_name(i_code)));

	if (io_data == NULL) {
		eprintf(("\033[31mgamedriver: dev_control() called with NULL io_data\033[0m\n"));
		return B_DONT_DO_THAT;
	}
	if (!check_size(i_code, *(size_t *)io_data, &r_size))
	{
		eprintf(("\033[31mgamedriver: dev_control(%s) with info_size %ld (should be >= %ld)\033[0m\n",
				ctrl_name(i_code), *(size_t *)io_data, get_size(i_code)));
		*(size_t *)io_data = r_size;
		return B_DONT_DO_THAT;
	}

#if DEBUG
	LOCK_GLOBALS
	{
		ga_loaded_module * ap = g_loaded_modules;
		while (ap != NULL)
		{
			ga_cookie * ck = ap->cookies;
			while (ck != NULL)
			{
				if (ck == pc)
				{
					goto cookie_ok;
				}
				ck = ck->next;
			}
			ap = ap->next;
		}
		eprintf(("\n\033[31mgamedriver: dev_control() on unknown cookie %p !!!\033[0m\n\n", i_cookie));
		err = B_SHUCKS;
cookie_ok:
		;
	}
	if (err == B_SHUCKS)
	{
		return err;
	}
#endif

#define WRONLY(x) if (!pc->write_ok) err = B_IM_AFRAID_I_CANT_LET_YOU_DO_THAT; else x

	LOCK_ACTIVE(pc->module) {
		if (pc->dev->control && ((err = pc->dev->control(pc->dev, i_code, io_data, pc)) !=
				B_IM_SORRY_DAVE)) {
			ddprintf(("gamedriver: device ioctl(0x%lx) returns 0x%lx\n", i_code, err));
		}
		else switch (i_code) {
		case GAME_GET_INFO:
			err = get_info(pc, (game_get_info *)io_data);
			break;
		case GAME_GET_CODEC_INFOS:
			err = get_codec_infos(pc, (game_get_codec_infos *)io_data);
			break;
		case _GAME_UNUSED_OPCODE_0_:
			break;
		case GAME_SET_CODEC_FORMATS:
			WRONLY(err = set_codec_formats(pc, (game_set_codec_formats *)io_data));
			break;
		case GAME_GET_MIXER_INFOS:
			err = get_mixer_infos(pc, (game_get_mixer_infos *)io_data);
			break;
		case GAME_GET_MIXER_CONTROLS:
			err = get_mixer_controls(pc, (game_get_mixer_controls *)io_data);
			break;
		case GAME_GET_MIXER_LEVEL_INFO:
			err = get_mixer_level_info(pc, (game_get_mixer_level_info *)io_data);
			break;
		case GAME_GET_MIXER_MUX_INFO:
			err = get_mixer_mux_info(pc, (game_get_mixer_mux_info *)io_data);
			break;
		case GAME_GET_MIXER_ENABLE_INFO:
			err = get_mixer_enable_info(pc, (game_get_mixer_enable_info *)io_data);
			break;
		case GAME_MIXER_UNUSED_OPCODE_1_:
			break;
		case GAME_GET_MIXER_CONTROL_VALUES:
			err = get_mixer_control_values(pc, (game_get_mixer_control_values *)io_data);
			break;
		case GAME_SET_MIXER_CONTROL_VALUES:
			WRONLY(err = set_mixer_control_values(pc, (game_set_mixer_control_values *)io_data));
			break;
		case GAME_OPEN_STREAM:
			WRONLY(err = open_stream(pc, (game_open_stream *)io_data));
			break;
		case GAME_GET_TIMING:
			if (GAME_IS_STREAM(((game_get_timing *)io_data)->source))
			{
				err = get_stream_timing(pc, (game_get_timing *)io_data);
			}
			else if (GAME_IS_DAC(((game_get_timing *)io_data)->source) ||
				GAME_IS_ADC(((game_get_timing *)io_data)->source))
			{
				err = get_codec_timing(pc, (game_get_timing *)io_data);
			}
			else
			{
				eprintf(("\033[31mgamedriver: unknown source %ld in GAME_GET_TIMING.\033[0m\n",
					((game_get_timing *)io_data)->source));
				err = B_BAD_VALUE;
			}
			break;
		case _GAME_UNUSED_OPCODE_2_:
			break;
		case GAME_GET_STREAM_CONTROLS:
			err = get_stream_controls(pc, (game_get_stream_controls *)io_data);
			break;
		case GAME_SET_STREAM_CONTROLS:
			err = set_stream_controls(pc, (game_set_stream_controls *)io_data);
			break;
		case _GAME_UNUSED_OPCODE_3_:
			break;
		case GAME_SET_STREAM_BUFFER:
			WRONLY(err = set_stream_buffer(pc, (game_set_stream_buffer *)io_data));
			break;
		case GAME_RUN_STREAMS:
			WRONLY(err = run_streams(pc, (game_run_streams *)io_data));
			break;
		case GAME_CLOSE_STREAM:
			WRONLY(err = close_stream(pc, (game_close_stream *)io_data));
			break;
		case GAME_OPEN_STREAM_BUFFERS:
			WRONLY(err = open_stream_buffers(pc, (game_open_stream_buffers *)io_data));
			break;
		case GAME_CLOSE_STREAM_BUFFERS:
			WRONLY(err = close_stream_buffers(pc, (game_close_stream_buffers *)io_data));
			break;
		case GAME_GET_DEVICE_STATE:
#warning implement GAME_GET_DEVICE_STATE
			break;
		case GAME_SET_DEVICE_STATE:
#warning implement GAME_SET_DEVICE_STATE
			break;
		case GAME_GET_INTERFACE_INFO:
#warning implement GAME_GET_INTERFACE_INFO
			break;
		case GAME_GET_INTERFACES:
#warning implement GAME_GET_INTERFACES
			break;
		default:
			eprintf(("\033[35mgamedriver: unknown ioctl(%ld)\033[0m\n", i_code));
			break;
		}
	}
#if DEBUG
	if (err < B_WHATEVER) {
		ddprintf(("gamedriver: ioctl(0x%lx) returns 0x%lx\n", i_code, err));
	}
#endif
	*(size_t *)io_data = r_size;
	return err;
}

status_t
dev_read(
	void * i_cookie,
	off_t i_pos,
	void * o_buf,
	size_t * io_size)
{
	*io_size = 0;
	return B_IM_AFRAID_I_CANT_LET_YOU_DO_THAT;
}

status_t
dev_write(
	void * i_cookie,
	off_t i_pos,
	const void * i_buf,
	size_t * io_size)
{
	*io_size = 0;
	return B_IM_AFRAID_I_CANT_LET_YOU_DO_THAT;
}


static void
midi_interrupt_op(int32 op, void * data)
{
	plug_mpu401_info* info = (plug_mpu401_info*) data;

	if (op == B_MPU_401_ENABLE_CARD_INT)
		info->enable_mpu401_int(info->cookie);
	else if (op == B_MPU_401_DISABLE_CARD_INT)
		info->disable_mpu401_int(info->cookie);
}

static status_t
midi_open(const char * i_name, uint32 i_flags, void ** o_cookie)
{
	ga_loaded_module * ap = NULL;
	plug_api * dev = NULL;
	status_t err = B_OK;

	ddprintf(("midi_open(%s, 0x%lx)\n", i_name, i_flags));

	LOCK_GLOBALS
	{
		for (ap = g_loaded_modules; ap != NULL; ap = ap->next)
		{
			if (!strcmp(i_name, ap->midipath))
			{
				dev = ap->dev;
				break;
			}
		}
	}

	if (dev == NULL)
	{
		eprintf(("\033[31mgamedriver: midi_open(%s) wasn't found\033[0m\n", i_name));
		return B_SHUCKS;
	}
	else if (!dev->get_mpu401_info || !dev->set_mpu401_callback)
	{
		eprintf(("\033[35mgamedriver: %s must implement get_mpu401_info and set_mpu401_callback\033[0m\n", i_name));
		return B_SHUCKS;
	}

	LOCK_GLOBALS
	{
		// For the first open load the mpu401 module
		// and create a new mpu401_info struct

		if (g_mpu401_count == 0)
			err = get_module(B_MPU_401_MODULE_NAME, (module_info**) &g_mpu401);

		if (err == B_OK)
		{
			++g_mpu401_count;
			if (ap->mpu401_open_count == 0)
			{
				err = dev->get_mpu401_info(dev, &ap->mpu401_info);
				if (err == B_OK)
				{
					err = g_mpu401->create_device_alt(ap->mpu401_info.port,
													  ap->mpu401_info.cookie,
													  ap->mpu401_info.read_func,
													  ap->mpu401_info.write_func,
													  &ap->mpu401_device_id,
													  0,
													  midi_interrupt_op,
													  &ap->mpu401_info);
					if (err == B_OK)
					{
						dev->set_mpu401_callback(dev, g_mpu401->interrupt_hook,
												 ap->mpu401_device_id);
					}
				}
			}
		}

		if (err == B_OK)
		{
			void* cookie;
			err = g_mpu401->open_hook(ap->mpu401_device_id, i_flags, &cookie);
			if (err == B_OK)
			{
				++ap->mpu401_open_count;
				*o_cookie = malloc(sizeof(ga_midi_cookie));
				((ga_midi_cookie*) *o_cookie)->inner_cookie = cookie;
				((ga_midi_cookie*) *o_cookie)->module = ap;
			}
			else
			{
				ap->dev->set_mpu401_callback(ap->dev, NULL, NULL);
				g_mpu401->delete_device(ap->mpu401_device_id);
			}
		}

		if (err != B_OK && g_mpu401_count > 0 && --g_mpu401_count == 0)
		{
			g_mpu401 = NULL;
			put_module(B_MPU_401_MODULE_NAME);
		}
	}

	return err;
}

static status_t
midi_close(void * i_cookie)
{
	ga_midi_cookie* cookie = (ga_midi_cookie*) i_cookie;
	return g_mpu401->close_hook(cookie->inner_cookie);
}

static status_t
midi_free(void * i_cookie)
{
	ga_midi_cookie* cookie = (ga_midi_cookie*) i_cookie;
	ga_loaded_module* ap = cookie->module;
	status_t err = g_mpu401->free_hook(cookie->inner_cookie);

	if (err == B_OK)
	{
		LOCK_GLOBALS
		{
			if (--ap->mpu401_open_count == 0)
			{
				ap->dev->set_mpu401_callback(ap->dev, NULL, NULL);
				g_mpu401->delete_device(ap->mpu401_device_id);
				if (--g_mpu401_count == 0)
				{
				  g_mpu401 = NULL;
				  put_module(B_MPU_401_MODULE_NAME);
				}
			}
		}
		free(cookie);
	}

	return err;
}

static status_t
midi_control(void * i_cookie, uint32 i_code, void * io_data, size_t i_size)
{
	ga_midi_cookie* cookie = (ga_midi_cookie*) i_cookie;
	return g_mpu401->control_hook(cookie->inner_cookie, i_code, io_data, i_size);
}

static status_t
midi_read(void * i_cookie, off_t i_pos, void * o_buf, size_t * io_size)
{
	ga_midi_cookie* cookie = (ga_midi_cookie*) i_cookie;
	return g_mpu401->read_hook(cookie->inner_cookie, i_pos, o_buf, io_size);
}

static status_t
midi_write(void * i_cookie, off_t i_pos, const void * i_buf, size_t * io_size)
{
	ga_midi_cookie* cookie = (ga_midi_cookie*) i_cookie;
	return g_mpu401->write_hook(cookie->inner_cookie, i_pos, i_buf, io_size);
}

device_hooks m_hooks = {
	midi_open,
	midi_close,
	midi_free,
	midi_control,
	midi_read,
	midi_write
};

static status_t
joy_open(const char * i_name, uint32 i_flags, void ** o_cookie)
{
	ga_loaded_module * ap = NULL;
	plug_api * dev = NULL;
	status_t err = B_OK;

	ddprintf(("joy_open(%s, 0x%lx)\n", i_name, i_flags));

	LOCK_GLOBALS
	{
		for (ap = g_loaded_modules; ap != NULL; ap = ap->next)
		{
			if (!strcmp(i_name, ap->joypath))
			{
				dev = ap->dev;
				break;
			}
		}
	}

	if (dev == NULL)
	{
		eprintf(("\033[31mgamedriver: joy_open(%s) wasn't found\033[0m\n", i_name));
		return B_SHUCKS;
	}
	else if (!dev->get_gameport_info)
	{
		eprintf(("\033[35mgamedriver: %s must implement get_gameport_info\033[0m\n", i_name));
		return B_SHUCKS;
	}

	LOCK_GLOBALS
	{
		// For the first open load the gameport module
		// and create a new gameport_info struct

		if (g_gameport_count == 0)
			err = get_module(B_GAMEPORT_MODULE_NAME, (module_info**) &g_gameport);

		if (err == B_OK)
		{
			++g_gameport_count;
			if (ap->gameport_open_count == 0)
			{
				err = dev->get_gameport_info(dev, &ap->gameport_info);
				if (err == B_OK)
				{
					err = g_gameport->create_device_alt(ap->gameport_info.cookie,
														ap->gameport_info.read_func,
														ap->gameport_info.write_func,
														&ap->gameport_device_id);
				}
			}
		}

		if (err == B_OK)
		{
			void* cookie;
			err = g_gameport->open_hook(ap->gameport_device_id, i_flags, &cookie);
			if (err == B_OK)
			{
				++ap->gameport_open_count;
				*o_cookie = malloc(sizeof(ga_joy_cookie));
				((ga_joy_cookie*) *o_cookie)->inner_cookie = cookie;
				((ga_joy_cookie*) *o_cookie)->module = ap;
			}
			else
				g_gameport->delete_device(ap->gameport_device_id);
		}

		if (err != B_OK && g_gameport_count > 0 && --g_gameport_count == 0)
		{
			g_gameport = NULL;
			put_module(B_MPU_401_MODULE_NAME);
		}
	}

	return err;
}

static status_t
joy_close(void * i_cookie)
{
	ga_joy_cookie* cookie = (ga_joy_cookie*) i_cookie;
	return g_gameport->close_hook(cookie->inner_cookie);
}

static status_t
joy_free(void * i_cookie)
{
	ga_joy_cookie* cookie = (ga_joy_cookie*) i_cookie;
	ga_loaded_module* ap = cookie->module;
	status_t err = g_gameport->free_hook(cookie->inner_cookie);

	if (err == B_OK)
	{
		LOCK_GLOBALS
		{
			if (--ap->gameport_open_count == 0)
			{
				g_gameport->delete_device(ap->gameport_device_id);
				if (--g_gameport_count == 0)
				{
				  g_gameport = NULL;
				  put_module(B_GAMEPORT_MODULE_NAME);
				}
			}
		}
		free(cookie);
	}

	return err;
}

static status_t
joy_control(void * i_cookie, uint32 i_code, void * io_data, size_t i_size)
{
	ga_joy_cookie* cookie = (ga_joy_cookie*) i_cookie;
	return g_gameport->control_hook(cookie->inner_cookie, i_code, io_data, i_size);
}

static status_t
joy_read(void * i_cookie, off_t i_pos, void * o_buf, size_t * io_size)
{
	ga_joy_cookie* cookie = (ga_joy_cookie*) i_cookie;
	return g_gameport->read_hook(cookie->inner_cookie, i_pos, o_buf, io_size);
}

static status_t
joy_write(void * i_cookie, off_t i_pos, const void * i_buf, size_t * io_size)
{
	ga_joy_cookie* cookie = (ga_joy_cookie*) i_cookie;
	return g_gameport->write_hook(cookie->inner_cookie, i_pos, i_buf, io_size);
}

device_hooks j_hooks = {
	joy_open,
	joy_close,
	joy_free,
	joy_control,
	joy_read,
	joy_write
};
