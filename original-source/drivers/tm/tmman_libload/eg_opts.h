/*  CmdOpt example: options definition header.

    Normal inclusion of this header enumerates the option names for use with
    Lib_CmdOpt in referring to options.

    Defining _Eg_opts_impl, however, allows the header  to  be  included  as
    initialization data for an options table.
*/

#ifndef	_Eg_opts_impl
#ifndef	_Eg_opts_h
#undef	Def
#define	Def(name,o1,o2,typ,flags,dflt,descr)	Opt_##name,
enum {
#else
#undef	Def
#define	Def()
#endif
#endif


/*  Table of option entries.
*/
Def(args, "args", "",		CmdOpt_string, CmdOpt_verbatim,	Null, "verbatim")
Def(compile_only, "c", "",	CmdOpt_bool, 0,			False, "compile only")
Def(debug, "debug", "d",	CmdOpt_bool, 0,			True, "generate debug info")
Def(hidden, "hidden", "h",	CmdOpt_string, CmdOpt_hidden,	"<hidden", "should not be printed!")
Def(inc, "include", "I",	CmdOpt_include, 0,	Null, "include options")
Def(lib, "library", "lib",	CmdOpt_string, 0,		"<lib>", "library string")
Def(maxmem, "maxmemory", "mem",	CmdOpt_int, 0,			8388, "maximum memory")
Def(opt, "O", "",		CmdOpt_int, CmdOpt_plus,	3, "optimization level")
Def(pws, "password", "p",	CmdOpt_list, CmdOpt_plus,	Null, "passwords")

#if	!defined(_Eg_opts_impl) && !defined(_Eg_opts_h)
};					/* end of enum */
#define	_Eg_opts_h			/* prevent re-inclusion */
#endif
