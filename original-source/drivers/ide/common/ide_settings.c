#include "devicelist.h"
#include "ide_settings.h"
#include <KernelExport.h>
#include <string.h>
#include <IDE.h>
#include <driver_settings.h>
#include <driver_settings_p.h>
#include <stdlib.h>
#include <debugoutput.h>

static bool
match_string(driver_parameter *p, const char *string, int size)
{
	const char *debugprefix = DEBUGPREFIX "read_settings_device match_string:";
	int slen;
	DI(3) dprintf("%s %s %s %s\n", debugprefix, p->name, p->values[0], string);
	if(p->parameter_count != 0) {
		DE(3) dprintf("%s %s takes no sub parameters\n", debugprefix, p->name);
		return false;
	}
	if(p->value_count != 1) {
		DE(3) dprintf("%s %s takes two arguments\n", debugprefix, p->name);
		return false;
	}
	slen = strlen(p->values[0]);
	if(slen > size) {
		DE(3) dprintf("%s %s string too long\n", debugprefix, p->name);
		return false;
	}
	if(strncmp(string, p->values[0], slen) != 0) {
		DI(3) dprintf("%s %s %s, does not match\n", debugprefix, p->name, p->values[0]);
		return false;
	}
	while(slen < size) {
		if(string[slen] != '\0' && string[slen] != ' ') {
			DI(1) dprintf("%s %s %s, does not fully match\n", debugprefix, p->name, p->values[0]);
			return false;
		}
		slen++;
	}
	DI(3) dprintf("%s %s %s, matches\n", debugprefix, p->name, p->values[0]);
	return true;
}

static bool
match_int(driver_parameter *p, int val)
{
	const char *debugprefix = DEBUGPREFIX "read_settings_device:";
	if(p->parameter_count != 0) {
		DE(3) dprintf("%s %s takes no sub parameters\n", debugprefix, p->name);
		return false;
	}
	if(p->value_count != 1) {
		DE(3) dprintf("%s %s takes two arguments\n", debugprefix, p->name);
		return false;
	}
	if(strtol(p->values[0], NULL, 10) == val)
		return true;
	else
		return false;
}

static bool
matchall(device_entry *d, driver_parameter *s)
{
	int i;
	for(i = 0; i < s->parameter_count; i++) {
		driver_parameter *p = &s->parameters[i];
		if(strcmp(p->name, "bus") == 0) {
			if(!match_int(p, d->busnum))
				return false;
		}
		else if(strcmp(p->name, "device") == 0) {
			if(!match_int(p, d->devicenum))
				return false;
		}
		else if(strcmp(p->name, "model") == 0) {
			if(!match_string(p, d->di->model_number, 40))
				return false;
		}
		else if(strcmp(p->name, "serial") == 0) {
			if(!match_string(p, d->di->serial_number, 20))
				return false;
		}
		else if(strcmp(p->name, "firmware") == 0) {
			if(!match_string(p, d->di->firmware_revision, 8))
				return false;
		}
		else {
			return false;
		}
	}
	return true;
}

static status_t
match_device(device_entry *d, driver_parameter *s)
{
	const char *debugprefix = DEBUGPREFIX "match_device:";
	int i;
	bool match = false;
	for(i = 0; i < s->parameter_count; i++) {
		driver_parameter *p = &s->parameters[i];
		if(strcmp(p->name, "match") == 0) {
			if(matchall(d, p))
				match = true;
		}
		else if(strcmp(p->name, "use") == 0) {
		}
		else {
			DE(3) dprintf("%s unknown parameter, %s\n", debugprefix, p->name);
		}
	}
	if(!match)
		return false;
	for(i = 0; i < s->parameter_count; i++) {
		driver_parameter *p = &s->parameters[i];
		if(strcmp(p->name, "match") == 0) {
		}
		else if(strcmp(p->name, "use") == 0) {
			int j;
			for(j = 0; j < p->parameter_count; j++) {
				use_settings_device(d, &p->parameters[j]);
			}
		}
		else {
			DE(3) dprintf("%s unknown parameter, %s\n", debugprefix, p->name);
		}
	}
	
	return B_NO_ERROR;
}

status_t
read_settings_device(device_entry *d, const char *drivername)
{
	const char *debugprefix = DEBUGPREFIX "read_settings_device:";
	void *settingshandle;
	const driver_settings *s;

	settingshandle = load_driver_settings(drivername);
	if(settingshandle == 0)
		return B_ERROR;
	s = get_driver_settings(settingshandle);
	if(s) {
		int i;
		for(i = 0; i < s->parameter_count; i++) {
			driver_parameter *p = &s->parameters[i];
			if(strcmp(p->name, "deviceat") == 0) {
				if(p->value_count != 2) {
					DE(2) dprintf("%s deviceat takes two arguments\n", debugprefix);
					continue;
				}
				if(strtol(p->values[0], NULL, 10) == d->busnum &&
				   strtol(p->values[1], NULL, 10) == d->devicenum) {
					int j;
					for(j = 0; j < p->parameter_count; j++) {
						use_settings_device(d, &p->parameters[j]);
					}
				}
			}
			else if(strcmp(p->name, "matchdevices") == 0) {
				match_device(d, p);
			}
			else if(strcmp(p->name, "global") == 0) {
			}
			else {
				DE(3) dprintf("%s unknown parameter, %s\n", debugprefix, p->name);
			}
		}
	}
	unload_driver_settings(settingshandle);
	return B_NO_ERROR;
}

bool ide_dma_disabled(void)
{
	void *handle;
	bool result = FALSE;

	handle = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	result = get_driver_boolean_parameter(handle, SAFEMODE_DISABLE_IDE_DMA,
			false, true);
	unload_driver_settings(handle);

	return result;
}
