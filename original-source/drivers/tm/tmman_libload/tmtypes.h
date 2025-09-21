/*
 *  COPYRIGHT (c) 1997 by Philips Semiconductors
 *
 *   +-----------------------------------------------------------------+
 *   | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *   | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *   | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *   | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *   | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *   | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *   +-----------------------------------------------------------------+
 *
 *  Module name              : tmtypes.h    1.8
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Common definitions
 *
 *  Last update              : 13:14:07 - 98/02/21
 *
 *  Description              :  
 *
 *	This module makes basic definitions for use in shared and
 *	public header files.  Such header files should use definitions
 *	made here, or standard C types.
 *
 *	Integer and Float types fall into those selected for specific
 *	precision (for use in files, for example), and those where
 *	control of precision is sacrificed for machine efficiency.
 *
 *	String is a pointer to a string of characters taht is guaranteed
 *	to be null-terminated.  (Use char *, otherwise.)
 *
 *	Bools normally occupy machine-efficient storage.  Use :1 or
 *	:8 for more precise control of packing within structures.
 *
 *	Pointer represents a reference to an unspecified type, whereas Address
 *	is ready for use in address-arithmetic.
 *
 *	Char and Int are defined for completeness and consistency.
 *
 *	Note that this header file is platform-specific.
 */

#ifndef	_TMtypes_h
#define	_TMtypes_h

#define	False		0
#define	Null		0
#define	True		1

typedef char *		Address;	/* ready for address-arithmetic */
typedef unsigned int	Bool;
typedef char		Char;		/* machine-natural character */
typedef unsigned char   Byte;		/* raw byte */
typedef float		Float;		/* fast float */
typedef float		Float32;	/* single-precision float */
#if	!defined(__TCS__) && !defined(__MWERKS__)
typedef double		Float64;	/* double-precision float */
#endif
typedef int		Int;		/* machine-natural integer */
typedef signed char	Int8;
typedef signed short	Int16;
typedef signed long	Int32;
typedef void *		Pointer;	/* pointer to anonymous object */
typedef char *		String;		/* guaranteed null-terminated */
typedef unsigned int	UInt;		/* machine-natural unsigned integer */
typedef unsigned char	UInt8;
typedef unsigned short	UInt16;
typedef unsigned long	UInt32;


typedef Int		Endian;
#define	BigEndian	0
#define	LittleEndian	1

typedef struct {
  UInt8  majorVersion;
  UInt8  minorVersion;
  UInt16 buildVersion;
} tmVersion_t, *ptmVersion_t;

#endif
