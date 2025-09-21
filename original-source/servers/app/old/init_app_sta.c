extern char __code_start__[];				/*	(defined by linker)	*/
extern char	__code_end__[];					/*	(defined by linker)	*/
extern char __data_start__[];				/*	(defined by linker)	*/
extern char __data_end__[];					/*	(defined by linker)	*/
extern char __exception_table_start__[];	/*	(defined by linker)	*/
extern char __exception_table_end__[];		/*	(defined by linker)	*/

extern int		__initialize_root_();
extern void		__initialize_be_();
extern int		__destroy_global_chain();
extern void		__teardown_be_();
extern int		__teardown_root_();
#if __POWERPC__
extern int		__register_fragment(char *, char *, char *, char *, char *, char *, char *);
extern int		__unregister_fragment(int);
extern int		__sinit();
#elif __INTEL__
extern void		_RunInit();
#endif /* __POWERPC__ */

#if __POWERPC__

static int	fragmentID;

static __asm char *__RTOC(void)
{
		mr		r3,RTOC
		blr
}

#endif /* __POWERPC__ */

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

_EXPORT
int _init_routine_(void)
{
	_kdprintf_("_init_routine_ called\n");
	/*
	 * For statically linked objects
	 */

#if __POWERPC__
	/* register the fragment */
	fragmentID = __register_fragment(__code_start__, __code_end__,
									__data_start__, __data_end__,
									__exception_table_start__, __exception_table_end__,
									__RTOC());
#endif /* __POWERPC__ */	

	_kdprintf_("_init_routine_: calling __initialize_root_\n");
	__initialize_root_();

#if __POWERPC__
	/* construct the libbe static objects */
	__sinit();
#elif __INTEL__
	_kdprintf_("_init_routine_: calling _RunInit\n");
	_RunInit();
#endif /* __POWERPC__ */	
	_kdprintf_("_init_routine_ done\n");

    return 0;
}

_EXPORT
int _term_routine_(void)
{
	/*
	 * For statically linked objects
	 */

	/* destroy the libbe static objects */
	__destroy_global_chain();

/*	__teardown_be_();*/
	__teardown_root_();

#if __POWERPC__
	/* unregister the fragment */
	__unregister_fragment(fragmentID);
#endif

	return 0;
}

