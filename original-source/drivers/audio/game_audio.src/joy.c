/************************************************************************/
/*                                                                      */
/*                              joy.c	                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov, Oleg Mazarak							*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/
#include <string.h>

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif /* _KERNEL_EXPORT_H */

#include "device.h"
#include "mvp4_debug.h"

#include "gaudio_card.h"

//extern generic_gameport_module * gameport;

static status_t joy_open(const char *name, uint32 flags, void **cookie);
static status_t joy_close(void *cookie);
static status_t joy_free(void *cookie);
static status_t joy_control(void *cookie, uint32 op, void *data, size_t len);
static status_t joy_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t joy_write(void *cookie, off_t pos, const void *data, size_t *len);


device_hooks joy_hooks = {
    &joy_open,
    &joy_close,
    &joy_free,
    &joy_control,
    &joy_read,
    &joy_write,
    NULL,		/* select */
    NULL,		/* deselect */
    NULL,		/* readv */
    NULL		/* writev */
};


static status_t
joy_open(
	const char * name,
	uint32 flags,
	void ** cookie)
{
	int ret = B_OK;
	joy_dev* joy;
	DB_PRINTF(("joy_open()\n"));

	joy = (joy_dev*)FindDeviceCookie(name);
	*cookie = (void*)joy;
	if(!joy)
	{
		DB_PRINTF(("joy_open(): return ENODEV;\n"));
		return ENODEV;
	} 

	if(atomic_add(&joy->count, 1) == 0)(*joy->card->func->enable_gameport)(joy->card);
		
	ret = (*gameport->open_hook)(joy->driver, flags, &joy->cookie);
	if (ret != B_OK) atomic_add(&joy->count, -1); //restore value
	DB_PRINTF(("joy_open(): return %d;\n", ret));
	return ret;
}


static status_t
joy_close(
	void * cookie)
{
	status_t ret;
	joy_dev* joy = (joy_dev*)cookie;
	DB_PRINTF(("joy_close()\n"));
	ret = (*gameport->close_hook)(joy->cookie);
	DB_PRINTF(("joy_close(): return %ld;\n",ret));
	return ret;
}


static status_t
joy_free(
	void * cookie)
{
	status_t ret;
	joy_dev* joy = (joy_dev*)cookie;
	DB_PRINTF(("joy_free()\n"));
	ret = (*gameport->free_hook)(joy->cookie);
	if (atomic_add(&joy->count, -1) == 1) {
		DB_PRINTF(("joy_free(): Last!!!\n"));
		joy->cookie = NULL;
		(*joy->card->func->disable_gameport)(joy->card);
	}
	DB_PRINTF(("joy_free(): return %ld;\n",ret));
	return ret;
}


static status_t
joy_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)
{
	status_t ret;
	joy_dev* joy = (joy_dev*)cookie;
	DB_PRINTF1(("joy_control(): iop = %ld;\n",iop));
	ret = (*gameport->control_hook)(joy->cookie, iop, data, len);
	DB_PRINTF1(("joy_control(): return %ld;\n",ret));
	return ret;
}
 

static status_t
joy_read(
	void * cookie,
	off_t pos,
	void * data,
	size_t * nread)
{
	status_t ret;
	joy_dev* joy = (joy_dev*)cookie;
	DB_PRINTF1(("joy_read(): nread = %ld;\n", *nread));
	ret =(*gameport->read_hook)(joy->cookie, pos, data, nread);
	DB_PRINTF1(("joy_read(): nread = %ld, return %ld;\n", *nread, ret));
	return ret;
}


static status_t
joy_write(
	void * cookie,
	off_t pos,
	const void * data,
	size_t * nwritten)
{
	status_t ret;
	joy_dev* joy = (joy_dev*)cookie;
	DB_PRINTF1(("joy_write(): size = %ld;\n", *nwritten, ret));
	ret = (*gameport->write_hook)(joy->cookie, pos, data, nwritten);
	DB_PRINTF1(("joy_write(): nwritten = %ld, return %ld;\n", *nwritten, ret));
	return ret;
}

