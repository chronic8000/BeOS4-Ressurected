#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>

#include <string.h>
#include <unistd.h>

#include "config_manager.h"
#include "config_driver.h"

#define TOUCH(x) ((void)x)

config_manager_for_driver_module_info *module;

/*********************************/

static status_t
cmopen(const char *name, uint32 mode, void **cookie)
{
	status_t error;

	TOUCH(name); TOUCH(mode); TOUCH(cookie);

	error = get_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&module);
	if (error < 0) {
		dprintf("Error getting module %s (%s)\n", B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, strerror(error));
		return error;
	}

	return B_OK;
}

static status_t
cmclose(void *cookie)
{
	TOUCH(cookie);

	return B_OK;
}

static status_t
cmfree(void *cookie)
{
	TOUCH(cookie);

	put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);

	return B_OK;
}

static status_t
cmioctl(void *cookie, uint32 op, void *data, size_t len)
{
	struct cm_ioctl_data *param = data;

	TOUCH(cookie); TOUCH(len);

	if (param->magic != op)
		return ENOSYS;

	switch (op) {
		case CM_GET_NEXT_DEVICE_INFO :
			return module->get_next_device_info(param->bus, &(param->cookie),
					param->data, param->data_len);
		case CM_GET_DEVICE_INFO_FOR :
			return module->get_device_info_for(param->cookie, param->data,
					param->data_len);
		case CM_GET_SIZE_OF_CURRENT_CONFIGURATION_FOR:
			return module->get_size_of_current_configuration_for(
					param->cookie);
		case CM_GET_CURRENT_CONFIGURATION_FOR:
			return module->get_current_configuration_for(
					param->cookie, param->data, param->data_len);
		case CM_GET_SIZE_OF_POSSIBLE_CONFIGURATIONS_FOR:
			return module->get_size_of_possible_configurations_for(
					param->cookie);
		case CM_GET_POSSIBLE_CONFIGURATIONS_FOR:
			return module->get_possible_configurations_for(
					param->cookie, param->data, param->data_len);

		case CM_COUNT_RESOURCE_DESCRIPTORS_OF_TYPE:
			return module->count_resource_descriptors_of_type(
					param->config, param->type);
		case CM_GET_NTH_RESOURCE_DESCRIPTOR_OF_TYPE:
			return module->get_nth_resource_descriptor_of_type(
					param->config, param->n, param->type,
					param->data, param->data_len);
	}

	return ENOSYS;
}

static status_t
cmread(void *cookie, off_t pos, void *buffer, size_t *len)
{
	TOUCH(buffer); TOUCH(pos); TOUCH(cookie); TOUCH(len);
	return B_ERROR;
}

static status_t
cmwrite(void *cookie, off_t pos, const void *buffer, size_t *len)
{
	TOUCH(buffer); TOUCH(pos); TOUCH(cookie); TOUCH(len);
	return B_ERROR;
}

device_hooks hooks = {
	&cmopen,
	&cmclose,
	&cmfree,
	&cmioctl,
	&cmread,
	&cmwrite
};

/***************************/

status_t init_hardware()
{
	return B_OK;
}

const char **publish_devices()
{
	static const char *devices[] = {
		CM_DEVICE_NAME, NULL
	};

	return devices;
}

device_hooks *find_device(const char *name)
{
	if (!strcmp(name, CM_DEVICE_NAME))
		return &hooks;
	return NULL;
}

status_t init_driver()
{
	return B_OK;
}

void uninit_driver()
{
}
