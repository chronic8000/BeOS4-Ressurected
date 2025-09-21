
#include "awe_private.h"
#include <string.h>

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */


static status_t mux_open(const char *name, uint32 flags, void **cookie);
static status_t mux_close(void *cookie);
static status_t mux_free(void *cookie);
static status_t mux_control(void *cookie, uint32 op, void *data, size_t len);
static status_t mux_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t mux_write(void *cookie, off_t pos, const void *data, size_t *len);

device_hooks mux_hooks = {
    &mux_open,
    &mux_close,
    &mux_free,
    &mux_control,
    &mux_read,
    &mux_write
};


static status_t
mux_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ix;
	mux_dev * plex = NULL;

	ddprintf(CHIP_NAME ": mux_open()\n");

	*cookie = NULL;
	for (ix=0; ix<num_cards; ix++) {
		if (!strcmp(name, cards[ix].mux.name)) {
			break;
		}
	}
	if (ix == num_cards) {
		return ENODEV;
	}

	atomic_add(&cards[ix].mux.open_count, 1);
	cards[ix].mux.card = &cards[ix];
	*cookie = &cards[ix].mux;

	return B_OK;
}


static status_t
mux_close(
	void * cookie)
{
	mux_dev * plex = (mux_dev *)cookie;

	atomic_add(&plex->open_count, -1);

	return B_OK;
}


static status_t
mux_free(
	void * cookie)
{
	ddprintf(CHIP_NAME ": mux_free()\n");

	if (((mux_dev *)cookie)->open_count != 0) {
		dprintf(CHIP_NAME ": mux open_count is bad in mux_free()!\n");
	}
	return B_OK;	/* already done in close */
}


static int
get_mux_value(
	sb_card * card,
	int ix)
{
	uchar val;

	switch (ix) {
	case B_AUDIO_INPUT_SELECT:
	  val = mixer_reg(card, 0x3d);
	  if (val & 0x01)
		return B_AUDIO_INPUT_MIC;
	  if (val & 0x04)
		return B_AUDIO_INPUT_CD;
	  if (val & 0x10)
		return B_AUDIO_INPUT_LINE_IN;
	  if (val & 0x40)
		return B_AUDIO_INPUT_AUX1;
	  return 0;
	case B_AUDIO_MIC_BOOST:
	  return mixer_reg(card, 0x43) & 1;
	}

	return -1;
}


static int
gather_info(
	mux_dev * mux,
	audio_routing * data,
	int count)
{
	int ix;

	for (ix=0; ix<count; ix++) {
		data[ix].value = get_mux_value(mux->card, data[ix].selector);
		if (data[ix].value < 0) {
			break;
		}
	}

	return ix;
}


static status_t
set_mux_value(
	sb_card * card,
	int selector,
	int value)
{
	uchar bits = 0;

	switch (selector) {
	case B_AUDIO_INPUT_SELECT:
	  switch (value) {
	  case B_AUDIO_INPUT_MIC:
		bits = 0x01;
	  case B_AUDIO_INPUT_CD:
		bits = 0x06;
	  case B_AUDIO_INPUT_LINE_IN:
		bits = 0x18;
	  case B_AUDIO_INPUT_AUX1:
		bits = 0x60;
	  default:
		return EINVAL;
	  }
	  mixer_change(card, 0x3d, 0x80, bits);
	  mixer_change(card, 0x3e, 0x80, bits);
	  break;
	case B_AUDIO_MIC_BOOST:
	  mixer_change(card, 0x43, 0xfe, value & 1);
	  break;
	default:
	  return EINVAL;
	}

	return B_NO_ERROR;
}


static int
disperse_info(
	mux_dev * mux,
	audio_routing * data,
	int count)
{
	int ix;

	for (ix=0; ix<count; ix++) {
		if (set_mux_value(mux->card, data[ix].selector, data[ix].value) < B_OK) {
			break;
		}
	}

	return ix;
}


static status_t
mux_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	mux_dev * plex = (mux_dev *)cookie;
	status_t err = B_OK;

/*	ddprintf(CHIP_NAME ": mux_control()\n"); /* slow printing */

	switch (iop) {
	case B_ROUTING_GET_VALUES:
		((audio_routing_cmd *)data)->count = 
			gather_info(plex, ((audio_routing_cmd *)data)->data, 
				((audio_routing_cmd *)data)->count);
		break;
	case B_ROUTING_SET_VALUES:
		((audio_routing_cmd *)data)->count = 
			disperse_info(plex, ((audio_routing_cmd *)data)->data, 
				((audio_routing_cmd *)data)->count);
		break;
	default:
		err = B_BAD_VALUE;
		break;
	}
	return err;
}


static status_t
mux_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	return EPERM;
}


static status_t
mux_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	return EPERM;
}

