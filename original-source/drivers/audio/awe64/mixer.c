
#include "awe_private.h"
#include <string.h>

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */


static status_t mixer_open(const char *name, uint32 flags, void **cookie);
static status_t mixer_close(void *cookie);
static status_t mixer_free(void *cookie);
static status_t mixer_control(void *cookie, uint32 op, void *data, size_t len);
static status_t mixer_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t mixer_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks mixer_hooks = {
    &mixer_open,
    &mixer_close,
    &mixer_free,
    &mixer_control,
    &mixer_read,
    &mixer_write
};


typedef struct mixer_info {
	int port;
	float div;
	float sub;
	int minval;
	int maxval;
	int leftshift;
	int mask;
	int mutemask;
} mixer_info;

static mixer_info the_mixers[] = {
	{ 0x32, -2.0, 0.0, 0, 31, 3, 0xf8, 0x00 },
	{ 0x33, -2.0, 0.0, 0, 31, 3, 0xf8, 0x00 },
	{ 0x38, -2.0, 0.0, 0, 31, 3, 0xf8, 0x10 },
	{ 0x39, -2.0, 0.0, 0, 31, 3, 0xf8, 0x08 },
	{ 0x36, -2.0, 0.0, 0, 31, 3, 0xf8, 0x04 },
	{ 0x37, -2.0, 0.0, 0, 31, 3, 0xf8, 0x02 },
	{ 0, 0.0, 0.0, 0, 0, 0, 0, 0 },
	{ 0, 0.0, 0.0, 0, 0, 0, 0, 0 },
	{ 0x34, -2.0, 0.0, 0, 31, 3, 0xf8, 0x00 },
	{ 0x35, -2.0, 0.0, 0, 31, 3, 0xf8, 0x00 },
	{ 0, 0.0, 0.0, 0, 0, 0, 0, 0 },
	{ 0, 0.0, 0.0, 0, 0, 0, 0, 0 },
	{ 0x3b, -6.0, 0.0, 0, 3, 6, 0xc0, 0x00 },
	{ 0, 0.0, 0.0, 0, 0, 0, 0, 0 },
	{ 0x3a, -2.0, 0.0, 0, 31, 3, 0xf8, 0x01 },
	{ 0x30, -2.0, 0.0, 0, 31, 3, 0xf8, 0x00 },
	{ 0x31, -2.0, 0.0, 0, 31, 3, 0xf8, 0x00 },
	{ 0, 0.0, 0.0, 0, 0, 0, 0, 0 },
	{ 0, 0.0, 0.0, 0, 0, 0, 0, 0 },
	{ 0, 0.0, 0.0, 0, 0, 0, 0, 0 },
};
#define N_MIXERS (sizeof(the_mixers) / sizeof(the_mixers[0]))

static status_t
mixer_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;
	mixer_dev * it = NULL;

	ddprintf(CHIP_NAME ": mixer_open()\n");

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].mix.name)) {
			break;
		}
	}
	if (ix == num_cards) {
		return ENODEV;
	}

	atomic_add(&cards[ix].mix.open_count, 1);
	cards[ix].mix.card = &cards[ix];
	*cookie = &cards[ix].mix;

	return B_OK;
}


static status_t
mixer_close(
	void * cookie)
{
	mixer_dev * it = (mixer_dev *)cookie;

	atomic_add(&it->open_count, -1);

	return B_OK;
}


static status_t
mixer_free(
	void * cookie)
{
	ddprintf(CHIP_NAME ": mixer_free()\n");

	if (((mixer_dev *)cookie)->open_count != 0) {
		dprintf(CHIP_NAME ": mixer open_count is bad in mixer_free()!\n");
	}
	return B_OK;	/* already done in close */
}


static int
get_mixer_value(
	sb_card * card,
	audio_level * lev)
{
	int ix = lev->selector;
	uchar val;
	if (ix < 0) {
		return B_BAD_VALUE;
	}
	if (ix > N_MIXERS-1) {
		return B_BAD_VALUE;
	}
	val = mixer_reg(card, the_mixers[ix].port);
	lev->flags = 0;
	if (!the_mixers[ix].mutemask) {
		/* no change */
	}
	else if (!(mixer_reg(card, 0x3c) & the_mixers[ix].mutemask)) {
		lev->flags |= B_AUDIO_LEVEL_MUTED;
	}
	val &= the_mixers[ix].mask;
	val >>= the_mixers[ix].leftshift;
	lev->value = ((float)val)*the_mixers[ix].div+the_mixers[ix].sub;

	return B_OK;
}


static int
gather_info(
	mixer_dev * mixer,
	audio_level * data,
	int count)
{
	int ix;

	for (ix=0; ix<count; ix++) {
		if (get_mixer_value(mixer->card, &data[ix]) < B_OK)
			break;
	}

	return ix;
}


static status_t
set_mixer_value(
	sb_card * card,
	audio_level * lev)
{
	int selector = lev->selector;
	int value;
	int mask;
	if (selector < 0 || selector > N_MIXERS-1) {
		return EINVAL;
	}
	value = (lev->value-the_mixers[selector].sub)/the_mixers[selector].div;
	if (value < the_mixers[selector].minval) {
		value = the_mixers[selector].minval;
	}
	if (value > the_mixers[selector].maxval) {
		value = the_mixers[selector].maxval;
	}
	value <<= the_mixers[selector].leftshift;
	mask = the_mixers[selector].mutemask;
	if (mask) {
		mixer_change(card, 0x3c, ~mask,
					 (lev->flags & B_AUDIO_LEVEL_MUTED ? 0 : mask));
	}
	mixer_change(card, the_mixers[selector].port,
				 ~the_mixers[selector].mask, value);
	return B_OK;
}


static int
disperse_info(
	mixer_dev * mixer,
	audio_level * data,
	int count)
{
	int ix;

	for (ix=0; ix<count; ix++) {
		if (set_mixer_value(mixer->card, &data[ix]) < B_OK)
			break;
	}

	return ix;
}


static status_t
mixer_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	mixer_dev * it = (mixer_dev *)cookie;
	status_t err = B_OK;

	if (!data) {
		return B_BAD_VALUE;
	}

	ddprintf(CHIP_NAME ": mixer_control()\n"); /* slow printing */

	switch (iop) {
	case B_MIXER_GET_VALUES:
		((audio_level_cmd *)data)->count = 
			gather_info(it, ((audio_level_cmd *)data)->data, 
				((audio_level_cmd *)data)->count);
		break;
	case B_MIXER_SET_VALUES:
		((audio_level_cmd *)data)->count = 
			disperse_info(it, ((audio_level_cmd *)data)->data, 
				((audio_level_cmd *)data)->count);
		break;
	default:
		err = B_BAD_VALUE;
		break;
	}
	return err;
}


static status_t
mixer_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	return EPERM;
}


static status_t
mixer_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	return EPERM;
}

