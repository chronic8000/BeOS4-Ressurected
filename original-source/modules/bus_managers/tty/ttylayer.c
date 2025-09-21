

#include	<KernelExport.h>
#include	<Drivers.h>
#include	<tty/ttylayer.h>


static status_t
std_ops( int32 op, ...)
{

	switch (op) {
	case B_MODULE_INIT:
		unless (ddinit( ))
			return (B_ERROR);
		rtinit( );
		break;
	case B_MODULE_UNINIT:
		rtuninit( );
		dduninit( );
		break;
	default:
		return (B_ERROR);
	}
	return (B_OK);
}


static tty_module_info tty_module = {
	{ B_TTY_MODULE_NAME, 0, &std_ops },
	&ttyopen,
	&ttyclose,
	&ttyfree,
	&ttyread,
	&ttywrite,
	&ttycontrol,
	&ttyselect,
	&ttydeselect,
	&ttyinit,
	&ttyilock,
	&ttyhwsignal,
	&ttyin,
	&ttyout,
	ddrstart,
	ddrdone,
	ddacquire,
};

#ifdef __ZRECOVER
# include <recovery/module_registry.h>
  REGISTER_STATIC_MODULE(tty_module);
#else
_EXPORT module_info	*modules[] = {
	(module_info *)&tty_module,
	NULL
};
#endif
