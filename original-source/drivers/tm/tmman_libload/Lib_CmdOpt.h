/*
 *  +-------------------------------------------------------------------+
 *  | Copyright (c) 1995,1996,1997 by Philips Semiconductors.           |
 *  |                                                                   |
 *  | This software  is furnished under a license  and may only be used |
 *  | and copied in accordance with the terms  and conditions of such a |
 *  | license  and with  the inclusion of this  copyright notice.  This |
 *  | software or any other copies of this software may not be provided |
 *  | or otherwise  made available  to any other person.  The ownership |
 *  | and title of this software is not transferred.                    |
 *  |                                                                   |
 *  | The information  in this software  is subject  to change  without |
 *  | any  prior notice  and should not be construed as a commitment by |
 *  | Philips Semiconductors.                                           |
 *  |                                                                   |
 *  | This  code  and  information  is  provided  "as is"  without  any |
 *  | warranty of any kind,  either expressed or implied, including but |
 *  | not limited  to the implied warranties  of merchantability and/or |
 *  | fitness for any particular purpose.                               |
 *  +-------------------------------------------------------------------+
  
    Module name              : Lib_CmdOpt.h 1.22
  
    Module type              : SPECIFICATION
  
    Title                    : Command-line option processing.
  
    Last update              : 17:28:17 - 97/06/11
  
    Description:

    This package parses Unix-style command-lines.  Since no rigid convention
    applies, this specification attempts to employ a useful distillation  of
    the common or most reasonable usages.  It is possible to tailor  certain
    choices.  Rare or baroque forms are not catered for.

    Please note that this specification is NOT  fully  implemented  at  this
    time!

    This package starts with the well-known argc/argv parse of the  command-
    line,  and  with  a  caller-provided   definition   of   option   names/
    characteristics as well as general parameters.

    The rules of the syntax employed follow:

    Command-line items are classified either as "arguments" or "options".

    Options are distinguished by a leading '-'.  They may also  be  disting-
    uished by a leading '+', if the caller chooses to allow this.

    Arguments are positional and are obtained by index  (ignoring  options).
    So, for example, in: "verb -i -v foo -o bar", arg 1 is "foo" and  arg  2
    is "bar".  Argument "0" is the program name acoording to  the  argc/argv
    convention, so in this example, it would be "verb".

    Options are named.  The caller can choose whether the  names  are  case-
    sensitive or not.  Each option may have a second name which is merely  a
    synonym.

    The caller can select whether options may be mixed  with  arguments,  or
    whether they must all appear before the first argument.

    An option name is treated differently  depending  on  whether  it  is  a
    single character only,  or  several.   Multi-letter  options  appear  by
    themselves.  For example, "-trace".  Single-letter options  may  usually
    be concatenated.  Thus, "-a -b -c" could be written as "-abc",  as  long
    as there is no option named "abc".

    Some programs allow the first argument to  be  processed  as  a  set  of
    single letter options whether or not the leading '-' is specified.   The
    caller selects whether this is the case.

    Options may be defined to have type: boolean, integer, float, string, or
    list.  A default value (of the appropriate type) is  also  provided  for
    each option.

    A boolean option is set true if it appears and left as its default if it
    does not appear.  The caller may further define that a boolean option is
    negatable -- it can then be negated by prefixing  its  name  with  "no".
    Note that a single-letter option can  not  be  concatenated  with  other
    options when it appears in the negative.

    All other types of options take values that are specified in addition to
    the option name.  There are several forms of specification:

	-opt value	"next arg" form
	-opt=value	"equals" form
	-Ovalue		single-letter form

    The last form has two variants which can be defined  by  the  caller  as
    attributes of each option.  In the usual case, the single-letter  option
    may be appended to other  letter  options,  as  in  "-mnPxy".   This  is
    equivalent to "-m -n -Pxy", even if options "x" and "y" are defined.

    In the other variant (seen in tools such as ar and tar), the  value  for
    the letter option appears in "next arg" style.   In  this  example,  "f"
    takes a string value: "-cfv uhaul.tar".  Note that no other  option  may
    take a value [exception -O3].

    The caller defines an acceptable range for each integer option.  Integer
    values may be expressed in decimal, hex, or  octal  notation.   A  value
    restricted to a single-digit in range may be specified  "inline"  for  a
    letter option: "-cO3g".

    String values may be enclosed in  '"'  and  may  contain  embedded  "\""
    escape codes.

    A list value is a set of strings separated by either ":" or  ",".   List
    values  accumulate  so  that,  "-DDEBUG -DVER=12"   is   equivalent   to
    "-DDEBUG,VER=12".

    The caller can detect whether  an  option  has  been  specified  on  the
    command-line (useful for toggles).  The caller can also  detect  whether
    an option was specified with '-' or '+'.

    Options can be thought of as  global  (their  effect  applies  to  every
    argument) or positional (they effect only arguments following  and  only
    until overridden).  This package does not  support  positional  options,
    however.

    The caller may define an option to be an "inclusion" option.   It  takes
    a string value that specifies a file to  continue  parsing  command-line
    arguments from.  If specified at the end  of  the  line  with  no  value
    available, stdin is read.

    The caller may define an option  to  indicate  that  verbatim  arguments
    follow (up until end-of-line or the null option, "--").   Alternatively,
    the caller can specify that all arguments after the nth are to  be  left
    unprocessed for verbatim use elsewhere.

    Some existing tools use the concept of hidden options.   These  are  not
    supported.  Definition of  such  options  should  be  made  compile-time
    conditional.


    Interface:

    The user provides a table of CmdOption entries, each entry describing an
    option legal on the command-line.  The user refers to each option by its
    table index.

    The command-line is parsed according to the table information,  combined
    with the rules above.  The position of each command-line element is noted

*/

#ifndef	_Lib_CmdOpt_h
#define	_Lib_CmdOpt_h

#include <stdio.h>
#include "tmtypes.h"

/*  User enumerates options from 0.  Arguments are designated by CmdOpt_arg.
    CmdOpt_no_arg indicates neither an argument or option is present (occurs
    when position index is out of range).
*/
#define	CmdOpt_arg		(-1)
#define	CmdOpt_no_arg		(-2)

/*  These are option flags that may be specified in the flags field of  each
    CmdOption table entry:

    hidden	option is recognized but not listed by cmdopt_print.
    plus	option may be introduced by '+'.
    verbatim	option is followed by a list of string values taken verbatim
		until end of command-line or the null option, "--".
*/
#define	CmdOpt_hidden		1	/* don't reveal option in help */
#define	CmdOpt_plus		2	/* +option allowed */
#define	CmdOpt_verbatim		4	/* args to -- are verbatim */
#define	CmdOpt__last_flag	CmdOpt_verbatim

/*  These are flags that may be used in the call to cmdopt_flags to vary the
    behavior of Lib_CmdOpt.
*/
#define	CmdOpt_equals		1	/* [dflt] allow -option=value */
#define	CmdOpt_first		2	/* options must precede args */

typedef struct _CmdOpt_value CmdOpt_value; /* private type */

/*  These are the  various  types  of  options.   The  type  field  of  each
    CmdOption table entry uses one of these values to specify  the  expected
    type of value provided by the corresponding option.
*/
typedef enum {
  CmdOpt_bool,
  CmdOpt_float,				/* [unimpl] */
  CmdOpt_include,			/* include options from file */
  CmdOpt_int,
  CmdOpt_uint,
  CmdOpt_list,				/* list (of strings) */
  CmdOpt_string,
} CmdOpt_type;

/*  Prototype for the user's error-handling call-back function.
*/
typedef Bool	(CmdOpt_fail)(int code, char *name, char *reason);

/*  This private structure is  only  visible  here  to  allow  the  user  to
    construct an option definition table.
*/
typedef struct _CmdOption {
  char *	cmdopt_name;		/* option spelling */
  char *	cmdopt_alias;		/* synonym */
  CmdOpt_type	cmdopt_type;		/* type of value provided */
  int		cmdopt_flags;
  void *	cmdopt_default;		/* default value */
  char *	cmdopt_descr;		/* description for help message */
  CmdOpt_value *cmdopt_value;		/* initially Null */
} CmdOption;

/*  Functions:

    arg		obtain the value of the nth argument.
    arg_pos	return the position of the nth argument.
    argopt_pos	return the arg/option and its value at the position.
    dump	dump the internal state after parsing.
    flags	specify flags to modify behavior.
		(flags are ORed with current)
    nargs	return the number of arguments.
    opt		obtain the (1st or default) value of the indicated option.
    opt_after	obtain the value of the indicated option after the position
		specified.  returns the position found, or 0 if not found.
    opt_plus	returns true if option was specified as +option.
    parse	parse the command-line and assemble its information
    print	print a helpful description of the arguments.
*/
char *		cmdopt_arg(int ax);
int		cmdopt_arg_pos(int ax);
int		cmdopt_argopt_pos(int pos, int *option, void **value);
void		cmdopt_dump(void);
int		cmdopt_flags(int flags);
int		cmdopt_nargs(void);
void *		cmdopt_opt(int opt);
int		cmdopt_opt_after(int opt, int pos, void **value);
Bool	                                                                                                                                                                                                                         