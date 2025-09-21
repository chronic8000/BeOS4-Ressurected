#ifndef _CONFIG_MANAGER_DEBUG_H_
#define _CONFIG_MANAGER_DEBUG_H_

#ifndef DEBUG
#define ASSERT(c) ((void)0)
#else
#define ASSERT(c) (!(c) ? _assert_(__FILE__,__LINE__,#c) : 0)
#endif

#define DPRINTF(a,b) do { if (verbosity > a) dprintf b; } while (0)

int _assert_(char *a, int b, char *c);
void dump_device_configuration(const struct device_configuration *c);
void dump_device_info(const struct device_info *c, 
		const struct device_configuration *current,
		const struct possible_device_configurations *possible);

#endif
