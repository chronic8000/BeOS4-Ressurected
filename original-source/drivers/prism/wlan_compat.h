/* src/include/wlan/wlan_compat.h
*
* Types and macros to aid in portability
*
*/

#ifndef _WLAN_COMPAT_H
#define _WLAN_COMPAT_H

/*=============================================================*/
/*------ Establish Platform Identity --------------------------*/
/*=============================================================*/
/* Key macros: */
/* WLAN_CPU_FAMILY */
	#define WLAN_Ix86			1
	#define WLAN_PPC			2
	#define WLAN_Ix96			3
/* WLAN_CPU_CORE */
	#define WLAN_I386CORE			1
	#define WLAN_PPCCORE			2
	#define WLAN_I296			3
/* WLAN_CPU_PART */
	#define WLAN_I386PART			1
	#define WLAN_MPC860			2
	#define WLAN_MPC823			3
	#define WLAN_I296SA			4
	#define WLAN_PPCPART			5
/* WLAN_SYSARCH */
	#define WLAN_PCAT			1
	#define WLAN_MBX			2
	#define WLAN_RPX			3
	#define WLAN_LWARCH			4
	#define WLAN_PMAC			5
/* WLAN_OS */
	#define WLAN_LINUX_KERNEL		1
	#define WLAN_LINUX_USER			2
	#define WLAN_LWOS			3
	#define WLAN_QNX4			4
	#define WLAN_BEOS			5
/* WLAN_COMPILER */
	#define WLAN_GNUC			1
	#define WLAN_DIAB			2
/* WLAN_HOSTIF (generally set on the command line, not detected) */
	#define WLAN_PCMCIA			1
	#define WLAN_ISA			2
	#define WLAN_PCI			3
	#define WLAN_USB			4

/* LinuxPPC users! uncomment the following line to build for LinuxPPC */
/*   a header file change has created a small problem that will be fixed */
/*   in the next release.  For the time being we can't automagically */
/*   detect LinuxPPC */

/* #define CONFIG_PPC 1 */


/* Lets try to figure out what we've got. */
#if defined(__LINUX_WLAN__) && defined(__KERNEL__)
	#define WLAN_OS				WLAN_LINUX_KERNEL
	#define WLAN_COMPILER			WLAN_GNUC
	#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
		#define WLAN_CPU_FAMILY		WLAN_Ix86
		#define WLAN_CPU_CORE		WLAN_I386CORE
		#define WLAN_CPU_PART		WLAN_I386PART
		#define WLAN_SYSARCH		WLAN_PCAT
	#elif defined(CONFIG_PPC)
		#define WLAN_CPU_FAMILY		WLAN_PPC
		#define WLAN_CPU_CORE		WLAN_PPCCORE
		#if defined(CONFIG_MBX)
			#define WLAN_CPU_PART	WLAN_MPC860
			#define WLAN_SYSARCH	WLAN_MBX
		#elif defined(CONFIG_RPXLITE)
			#define WLAN_CPU_PART	WLAN_MPC823
			#define WLAN_SYSARCH	WLAN_RPX
		#elif defined(CONFIG_RPXCLASSIC)
			#define WLAN_CPU_PART	WLAN_MPC860
			#define WLAN_SYSARCH	WLAN_RPX
		#else
			#define WLAN_CPU_PART	WLAN_PPCPART
			#define WLAN_SYSARCH	WLAN_PMAC
		#endif
	#endif
#elif defined(__LINUX_WLAN__) && !defined(__KERNEL__)
	#define WLAN_OS				WLAN_LINUX_USER
	#define WLAN_COMPILER			WLAN_GNUC
	#if defined(__I386__)
		#define WLAN_CPU_FAMILY		WLAN_Ix86
		#define WLAN_CPU_CORE		WLAN_I386CORE
		#define WLAN_CPU_PART		WLAN_I386PART
		#define WLAN_SYSARCH		WLAN_PCAT
	#elif defined(__WLAN_PPC__)
		#define WLAN_CPU_FAMILY		WLAN_PPC
		#define WLAN_CPU_CORE		WLAN_PPCCORE
		#if defined(CONFIG_MBX)
			#define WLAN_CPU_PART	WLAN_MPC860
			#define WLAN_SYSARCH	WLAN_MBX
		#elif defined(CONFIG_RPX)
			#define WLAN_CPU_PART	WLAN_MPC850
			#define WLAN_SYSARCH	WLAN_RPX
		#else
			#define WLAN_CPU_PART	WLAN_PPCPART
			#define WLAN_SYSARCH	WLAN_PMAC
		#endif
	#else
		#error "No CPU identified!"
	#endif
#elif defined(LW) || defined(LW_HDW_ISDN) || defined(LW_HDW_SERIAL)
	#define WLAN_OS				WLAN_LWOS
	#define WLAN_COMPILER			WLAN_DIAB
	#define WLAN_CPU_FAMILY			WLAN_PPC
	#define WLAN_CPU_CORE			WLAN_PPCCORE
	#define 			WLAN_MPC860
	#define WLAN_SYSARCH			WLAN_LWARCH
#elif defined(MBX)
	#define WLAN_OS				WLAN_LWOS
	#define WLAN_COMPILER			WLAN_DIAB
	#define WLAN_CPU_FAMILY			WLAN_PPC
	#define WLAN_CPU_CORE			WLAN_PPCCORE
	#define WLAN_CPU_PART			WLAN_MPC860
	# WLAN_SYSARCH			WLAN_MBX
#elif defined(__BEOS__)
	#define WLAN_OS			WLAN_BEOS
	#define WLAN_COMPILER		WLAN_GNUC
	#define WLAN_CPU_FAMILY		WLAN_Ix86
	#define WLAN_CPU_CORE		WLAN_I386CORE
	#define WLAN_CPU_PART		WLAN_I386PART
	#define WLAN_SYSARCH		WLAN_PCAT
#endif

/*=============================================================*/
/*------ Bit settings -----------------------------------------*/
/*=============================================================*/

#define BIT0	0x00000001
#define BIT1	0x00000002
#define BIT2	0x00000004
#define BIT3	0x00000008
#define BIT4	0x00000010
#define BIT5	0x00000020
#define BIT6	0x00000040
#define BIT7	0x00000080
#define BIT8	0x00000100
#define BIT9	0x00000200
#define BIT10	0x00000400
#define BIT11	0x00000800
#define BIT12	0x00001000
#define BIT13	0x00002000
#define BIT14	0x00004000
#define BIT15	0x00008000
#define BIT16	0x00010000
#define BIT17	0x00020000
#define BIT18	0x00040000
#define BIT19	0x00080000
#define BIT20	0x00100000
#define BIT21	0x00200000
#define BIT22	0x00400000
#define BIT23	0x00800000
#define BIT24	0x01000000
#define BIT25	0x02000000
#define BIT26	0x04000000
#define BIT27	0x08000000
#define BIT28	0x10000000
#define BIT29	0x20000000
#define BIT30	0x40000000
#define BIT31	0x80000000

#define MOTO_BIT31	0x00000001
#define MOTO_BIT30	0x00000002
#define MOTO_BIT29	0x00000004
#define MOTO_BIT28	0x00000008
#define MOTO_BIT27	0x00000010
#define MOTO_BIT26	0x00000020
#define MOTO_BIT25	0x00000040
#define MOTO_BIT24	0x00000080
#define MOTO_BIT23	0x00000100
#define MOTO_BIT22	0x00000200
#define MOTO_BIT21	0x00000400
#define MOTO_BIT20	0x00000800
#define MOTO_BIT19	0x00001000
#define MOTO_BIT18	0x00002000
#define MOTO_BIT17	0x00004000
#define MOTO_BIT16	0x00008000
#define MOTO_BIT15	0x00010000
#define MOTO_BIT14	0x00020000
#define MOTO_BIT13	0x00040000
#define MOTO_BIT12	0x00080000
#define MOTO_BIT11	0x00100000
#define MOTO_BIT10	0x00200000
#define MOTO_BIT9	0x00400000
#define MOTO_BIT8	0x00800000
#define MOTO_BIT7	0x01000000
#define MOTO_BIT6	0x02000000
#define MOTO_BIT5	0x04000000
#define MOTO_BIT4	0x08000000
#define MOTO_BIT3	0x10000000
#define MOTO_BIT2	0x20000000
#define MOTO_BIT1	0x40000000
#define MOTO_BIT0	0x80000000

typedef unsigned char   UINT8;
typedef unsigned short  UINT16;
typedef unsigned long   UINT32;

typedef signed char     INT8;
typedef signed short    INT16;
typedef signed long     INT32;

typedef unsigned int    UINT;
typedef signed int      INT;

#if (WLAN_COMPILER == WLAN_GNUC)
	typedef unsigned long long	UINT64;
	typedef signed long long	INT64;
#else
	typedef UINT8			UINT64[8];
#endif

#define UINT8_MAX	(0xffUL)
#define UINT16_MAX	(0xffffUL)
#define UINT32_MAX	(0xffffffffUL)

#define INT8_MAX	(0x7fL)
#define INT16_MAX	(0x7fffL)
#define INT32_MAX	(0x7fffffffL)

/*=============================================================*/
/*------ Compiler Portability Macros --------------------------*/
/*=============================================================*/
#if (WLAN_COMPILER == WLAN_GNUC)
	#define __WLAN_ATTRIB_PACK__		__attribute__ ((packed))
	#define __WLAN_PRAGMA_PACK1__
	#define __WLAN_PRAGMA_PACKDFLT__
	#define __WLAN_INLINE__			inline
	#define WLAN_MIN_ARRAY			0
#elif (WLAN_COMPILER == WLAN_DIAB)
	#define __WLAN_ATTRIB_PACK__	
	#define __WLAN_PRAGMA_PACK1__		#pragma pack
	#define __WLAN_PRAGMA_PACKDFLT__	#pragma pack()
	#define __WLAN_INLINE__			inline
	#define WLAN_MIN_ARRAY			1
#else
	#error "Unknown compiler"
#endif

/*=============================================================*/
/*------ OS Portability Macros --------------------------------*/
/*=============================================================*/

#ifndef WLAN_DBVAR
#define WLAN_DBVAR	wlan_debug
#endif

#if (WLAN_OS == WLAN_LINUX_KERNEL)
	#define WLAN_LOG_ERROR0(s) printk(KERN_ERR __FUNCTION__ ": " s);
	#define WLAN_LOG_ERROR1(s,n) printk(KERN_ERR __FUNCTION__ ": " s, (n));
	#define WLAN_LOG_ERROR2(s,n1,n2) printk(KERN_ERR __FUNCTION__ ": " s, (n1), (n2));
	#define WLAN_LOG_ERROR3(s,n1,n2,n3) printk(KERN_ERR __FUNCTION__ ": " s, (n1), (n2), (n3));
	#define WLAN_LOG_ERROR4(s,n1,n2,n3,n4) printk(KERN_ERR __FUNCTION__ ": " s, (n1), (n2), (n3), (n4));

	#define WLAN_LOG_WARNING0(s) printk(KERN_WARNING __FUNCTION__ ": " s);
	#define WLAN_LOG_WARNING1(s,n) printk(KERN_WARNING __FUNCTION__ ": " s, (n));
	#define WLAN_LOG_WARNING2(s,n1,n2) printk(KERN_WARNING __FUNCTION__ ": " s, (n1), (n2));
	#define WLAN_LOG_WARNING3(s,n1,n2,n3) printk(KERN_WARNING __FUNCTION__ ": " s, (n1), (n2), (n3));
	#define WLAN_LOG_WARNING4(s,n1,n2,n3,n4) printk(KERN_WARNING __FUNCTION__ ": " ": " s, (n1), (n2), (n3), (n4));

	#define WLAN_LOG_NOTICE0(s) printk(KERN_NOTICE __FUNCTION__ ": " s);
	#define WLAN_LOG_NOTICE1(s,n) printk(KERN_NOTICE __FUNCTION__ ": " s, (n));
	#define WLAN_LOG_NOTICE2(s,n1,n2) printk(KERN_NOTICE __FUNCTION__ ": " s, (n1), (n2));
	#define WLAN_LOG_NOTICE3(s,n1,n2,n3) printk(KERN_NOTICE __FUNCTION__ ": " s, (n1), (n2), (n3));
	#define WLAN_LOG_NOTICE4(s,n1,n2,n3,n4) printk(KERN_NOTICE __FUNCTION__ ": " s, (n1), (n2), (n3), (n4));

	#define WLAN_LOG_INFO0(s) printk(KERN_INFO s);
	#define WLAN_LOG_INFO1(s,n) printk(KERN_INFO s, (n));
	#define WLAN_LOG_INFO2(s,n1,n2) printk(KERN_INFO s, (n1), (n2));
	#define WLAN_LOG_INFO3(s,n1,n2,n3) printk(KERN_INFO s, (n1), (n2), (n3));
	#define WLAN_LOG_INFO4(s,n1,n2,n3,n4) printk(KERN_INFO s, (n1), (n2), (n3), (n4));
	#define WLAN_LOG_INFO5(s,n1,n2,n3,n4,n5) printk(KERN_INFO s, (n1), (n2), (n3), (n4), (n5));

	#if defined(WLAN_INCLUDE_DEBUG)
		#define WLAN_ASSERT(c) if ((!(c)) && WLAN_DBVAR >= 1) { \
			WLAN_LOG_DEBUG0(1, "Assertion failure!\n"); }
		#define WLAN_HEX_DUMP( l, s, p, n)	if( WLAN_DBVAR >= (l) ){ \
			int __i__; \
			printk(KERN_DEBUG s ":"); \
			for( __i__=0; __i__ < (n); __i__++) \
				printk( " %02x", ((UINT8*)(p))[__i__]); \
			printk("\n"); }

		#define DBFENTER { if ( WLAN_DBVAR >= 4 ){ WLAN_LOG_DEBUG0(3,"Enter\n"); } }
		#define DBFEXIT  { if ( WLAN_DBVAR >= 4 ){ WLAN_LOG_DEBUG0(3,"Exit\n"); } }

		#define WLAN_LOG_DEBUG0(l, s) if ( WLAN_DBVAR >= (l)) printk(KERN_DEBUG __FUNCTION__": " s);
		#define WLAN_LOG_DEBUG1(l, s,n) if ( WLAN_DBVAR >= (l)) printk(KERN_DEBUG __FUNCTION__": " s, (n));
		#define WLAN_LOG_DEBUG2(l, s,n1,n2) if ( WLAN_DBVAR >= (l)) printk(KERN_DEBUG __FUNCTION__": " s, (n1), (n2));
		#define WLAN_LOG_DEBUG3(l, s,n1,n2,n3) if ( WLAN_DBVAR >= (l)) printk(KERN_DEBUG __FUNCTION__": " s, (n1), (n2), (n3));
		#define WLAN_LOG_DEBUG4(l, s,n1,n2,n3,n4) if ( WLAN_DBVAR >= (l)) printk(KERN_DEBUG __FUNCTION__": " s, (n1), (n2), (n3), (n4));
		#define WLAN_LOG_DEBUG5(l, s,n1,n2,n3,n4,n5) if ( WLAN_DBVAR >= (l)) printk(KERN_DEBUG __FUNCTION__": " s, (n1), (n2), (n3), (n4), (n5));
	#else
		#define WLAN_ASSERT(c) 
		#define WLAN_HEX_DUMP( l, s, p, n)

		#define DBFENTER 
		#define DBFEXIT 

		#define WLAN_LOG_DEBUG0(l, s)
		#define WLAN_LOG_DEBUG1(l, s,n)
		#define WLAN_LOG_DEBUG2(l, s,n1,n2)
		#define WLAN_LOG_DEBUG3(l, s,n1,n2,n3)
		#define WLAN_LOG_DEBUG4(l, s,n1,n2,n3,n4)
		#define WLAN_LOG_DEBUG5(l, s,n1,n2,n3,n4,n5)
	#endif
#elif (WLAN_OS == WLAN_LWOS)
	#define KERN_ERR	
	#define KERN_WARNING
	#define KERN_NOTICE
	#define KERN_INFO
	#define KERN_DEBUG
	#define __FUNCTION__

	#define WLAN_LOG_ERROR0(s) kprintf(KERN_ERR s);
	#define WLAN_LOG_ERROR1(s,n) kprintf(KERN_ERR s, (n));
	#define WLAN_LOG_ERROR2(s,n1,n2) kprintf(KERN_ERR s, (n1), (n2));
	#define WLAN_LOG_ERROR3(s,n1,n2,n3) kprintf(KERN_ERR s, (n1), (n2), (n3));
	#define WLAN_LOG_ERROR4(s,n1,n2,n3,n4) kprintf(KERN_ERR s, (n1), (n2), (n3), (n4));

	#define WLAN_LOG_WARNING0(s) kprintf(KERN_WARNING s);
	#define WLAN_LOG_WARNING1(s,n) kprintf(KERN_WARNING s, (n));
	#define WLAN_LOG_WARNING2(s,n1,n2) kprintf(KERN_WARNING s, (n1), (n2));
	#define WLAN_LOG_WARNING3(s,n1,n2,n3) kprintf(KERN_WARNING s, (n1), (n2), (n3));
	#define WLAN_LOG_WARNING4(s,n1,n2,n3,n4) kprintf(KERN_WARNING s, (n1), (n2), (n3), (n4));

	#define WLAN_LOG_NOTICE0(s) kprintf(KERN_NOTICE s);
	#define WLAN_LOG_NOTICE1(s,n) kprintf(KERN_NOTICE s, (n));
	#define WLAN_LOG_NOTICE2(s,n1,n2) kprintf(KERN_NOTICE s, (n1), (n2));
	#define WLAN_LOG_NOTICE3(s,n1,n2,n3) kprintf(KERN_NOTICE s, (n1), (n2), (n3));
	#define WLAN_LOG_NOTICE4(s,n1,n2,n3,n4) kprintf(KERN_NOTICE s, (n1), (n2), (n3), (n4));

	#if defined(WLAN_INCLUDE_DEBUG)
		#define WLAN_ASSERT(c) if ((!(c)) && WLAN_DBVAR >= 1) \
			{WLAN_LOG_DEBUG0(1, "Assertion failure!\n");}
		#define WLAN_HEX_DUMP( l, s, p, n)	if( WLAN_DBVAR >= (l) ){ \
			int __i__; \
			kprintf(KERN_DEBUG s ":"); \
			for( __i__=0; __i__ < (n); __i__++) \
				kprintf( " %02x", ((UINT8*)(p))[__i__]); \
			kprintf("\n"); }

		#define DBFENTER { if ( WLAN_DBVAR >= 4 ){ WLAN_LOG_DEBUG0(3,"Enter\n"); } }
		#define DBFEXIT  { if ( WLAN_DBVAR >= 4 ){ WLAN_LOG_DEBUG0(3,"Exit\n"); } }

		#define WLAN_LOG_INFO0(s) kprintf(KERN_INFO __FUNCTION__": " s);
		#define WLAN_LOG_INFO1(s,n) kprintf(KERN_INFO __FUNCTION__": " s, (n));
		#define WLAN_LOG_INFO2(s,n1,n2) kprintf(KERN_INFO __FUNCTION__": " s, (n1), (n2));
		#define WLAN_LOG_INFO3(s,n1,n2,n3) kprintf(KERN_INFO __FUNCTION__": " s, (n1), (n2), (n3));
		#define WLAN_LOG_INFO4(s,n1,n2,n3,n4) kprintf(KERN_INFO __FUNCTION__": " s, (n1), (n2), (n3), (n4));
		#define WLAN_LOG_INFO4(s,n1,n2,n3,n4,n5) kprintf(KERN_INFO __FUNCTION__": " s, (n1), (n2), (n3), (n4),(n5));

		#define WLAN_LOG_DEBUG0(l, s) if ( WLAN_DBVAR >= (l)) kprintf(KERN_DEBUG __FUNCTION__": " s);
		#define WLAN_LOG_DEBUG1(l, s,n) if ( WLAN_DBVAR >= (l)) kprintf(KERN_DEBUG __FUNCTION__": " s, (n));
		#define WLAN_LOG_DEBUG2(l, s,n1,n2) if ( WLAN_DBVAR >= (l)) kprintf(KERN_DEBUG __FUNCTION__": " s, (n1), (n2));
		#define WLAN_LOG_DEBUG3(l, s,n1,n2,n3) if ( WLAN_DBVAR >= (l)) kprintf(KERN_DEBUG __FUNCTION__": " s, (n1), (n2), (n3));
		#define WLAN_LOG_DEBUG4(l, s,n1,n2,n3,n4) if ( WLAN_DBVAR >= (l)) kprintf(KERN_DEBUG __FUNCTION__": " s, (n1), (n2), (n3), (n4));
		#define WLAN_LOG_DEBUG5(l, s,n1,n2,n3,n4,n5) if ( WLAN_DBVAR >= (l)) kprintf(KERN_DEBUG __FUNCTION__": " s, (n1), (n2), (n3), (n4), (n5));
	#else
		#define WLAN_ASSERT(c) 
		#define WLAN_HEX_DUMP( l, s, p, n)

		#define DBFENTER 
		#define DBFEXIT 

		#define WLAN_LOG_INFO0(s)
		#define WLAN_LOG_INFO1(s,n)
		#define WLAN_LOG_INFO2(s,n1,n2)
		#define WLAN_LOG_INFO3(s,n1,n2,n3)
		#define WLAN_LOG_INFO4(s,n1,n2,n3,n4)
		#define WLAN_LOG_INFO5(s,n1,n2,n3,n4,n5)

		#define WLAN_LOG_DEBUG0(l, s)
		#define WLAN_LOG_DEBUG1(l, s,n)
		#define WLAN_LOG_DEBUG2(l, s,n1,n2)
		#define WLAN_LOG_DEBUG3(l, s,n1,n2,n3)
		#define WLAN_LOG_DEBUG4(l, s,n1,n2,n3,n4)
		#define WLAN_LOG_DEBUG5(l, s,n1,n2,n3,n4,n5)
	#endif

#elif (WLAN_OS == WLAN_BEOS)
	#if 1
		#define DBFENTER 
		#define DBFEXIT 
	#else
		#define DBFENTER dprintf("Enter %s\n",__FUNCTION__)
		#define DBFEXIT  dprintf("Exit  %s\n",__FUNCTION__)
	#endif
	#define WLAN_LOG_ERROR0(s) 				dprintf("prism ERR: " s)
	#define WLAN_LOG_ERROR1(s,n) 			dprintf("prism ERR: " s,n)
	#define WLAN_LOG_ERROR2(s,n1,n2)		dprintf("prism ERR: " s,n1,n2) 		
	#define WLAN_LOG_ERROR3(s,n1,n2,n3)		dprintf("prism ERR: " s,n1,n2,n3) 	
	#define WLAN_LOG_ERROR4(s,n1,n2,n3,n4)	dprintf("prism ERR: " s,n1,n2,n3,n4)

	#define WLAN_LOG_WARNING0(s)				dprintf("prism WARN: " s)
	#define WLAN_LOG_WARNING1(s,n)				dprintf("prism WARN: " s,n)
	#define WLAN_LOG_WARNING2(s,n1,n2)			dprintf("prism WARN: " s,n1,n2)
	#define WLAN_LOG_WARNING3(s,n1,n2,n3)		dprintf("prism WARN: " s,n1,n2,n3)
	#define WLAN_LOG_WARNING4(s,n1,n2,n3,n4)	dprintf("prism WARN: " s,n1,n2,n3,n4)

	#define WLAN_LOG_NOTICE0(s)					dprintf("prism NOTICE: " s)
	#define WLAN_LOG_NOTICE1(s,n)				dprintf("prism NOTICE: " s,n)
	#define WLAN_LOG_NOTICE2(s,n1,n2)			dprintf("prism NOTICE: " s,n1,n2)
	#define WLAN_LOG_NOTICE3(s,n1,n2,n3)		dprintf("prism NOTICE: " s,n1,n2,n3)
	#define WLAN_LOG_NOTICE4(s,n1,n2,n3,n4)		dprintf("prism NOTICE: " s,n1,n2,n3,n4)


	#define WLAN_LOG_INFO0(s)					dprintf("prism INFO: " s)
	#define WLAN_LOG_INFO1(s,n)					dprintf("prism INFO: " s,n)
	#define WLAN_LOG_INFO2(s,n1,n2)				dprintf("prism INFO: " s,n1,n2)
	#define WLAN_LOG_INFO3(s,n1,n2,n3)			dprintf("prism INFO: " s,n1,n2,n3)
	#define WLAN_LOG_INFO4(s,n1,n2,n3,n4)		dprintf("prism INFO: " s,n1,n2,n3,n4)
	#define WLAN_LOG_INFO5(s,n1,n2,n3,n4,n5)	dprintf("prism INFO: " s,n1,n2,n3,n4,n5)

	#define WLAN_LOG_DEBUG0(s)					dprintf("prism DEBUG: " s)
	#define WLAN_LOG_DEBUG1(s,n)				dprintf("prism DEBUG: " s,n)
	#define WLAN_LOG_DEBUG2(s,n1,n2)			dprintf("prism DEBUG: " s,n1,n2)
	#define WLAN_LOG_DEBUG3(s,n1,n2,n3)			dprintf("prism DEBUG: " s,n1,n2,n3)
	#define WLAN_LOG_DEBUG4(s,n1,n2,n3,n4)		dprintf("prism DEBUG: " s,n1,n2,n3,n4)
	#define WLAN_LOG_DEBUG5(s,n1,n2,n3,n4,n5)	dprintf("prism DEBUG: " s,n1,n2,n3,n4,n5)

#else
	#define WLAN_LOG_ERROR0(s)
	#define WLAN_LOG_ERROR1(s,n)
	#define WLAN_LOG_ERROR2(s,n1,n2)
	#define WLAN_LOG_ERROR3(s,n1,n2,n3)
	#define WLAN_LOG_ERROR4(s,n1,n2,n3,n4)

	#define WLAN_LOG_WARNING0(s)
	#define WLAN_LOG_WARNING1(s,n)
	#define WLAN_LOG_WARNING2(s,n1,n2)
	#define WLAN_LOG_WARNING3(s,n1,n2,n3)
	#define WLAN_LOG_WARNING4(s,n1,n2,n3,n4)

	#define WLAN_LOG_NOTICE0(s)
	#define WLAN_LOG_NOTICE1(s,n)
	#define WLAN_LOG_NOTICE2(s,n1,n2)
	#define WLAN_LOG_NOTICE3(s,n1,n2,n3)
	#define WLAN_LOG_NOTICE4(s,n1,n2,n3,n4)

		#define WLAN_ASSERT(c) 
		#define WLAN_HEX_DUMP( l, s, p, n)

		#define DBFENTER 
		#define DBFEXIT 

		#define WLAN_LOG_INFO0(s)
		#define WLAN_LOG_INFO1(s,n)
		#define WLAN_LOG_INFO2(s,n1,n2)
		#define WLAN_LOG_INFO3(s,n1,n2,n3)
		#define WLAN_LOG_INFO4(s,n1,n2,n3,n4)
		#define WLAN_LOG_INFO5(s,n1,n2,n3,n4,n5)

		#define WLAN_LOG_DEBUG0(l, s)
		#define WLAN_LOG_DEBUG1(l, s,n)
		#define WLAN_LOG_DEBUG2(l, s,n1,n2)
		#define WLAN_LOG_DEBUG3(l, s,n1,n2,n3)
		#define WLAN_LOG_DEBUG4(l, s,n1,n2,n3,n4)
		#define WLAN_LOG_DEBUG5(l, s,n1,n2,n3,n4,n5)
#endif

#if (WLAN_OS == WLAN_LINUX_KERNEL)
	#define wlan_ticks_per_sec		HZ
	#define wlan_ms_per_tick		(1000UL / (wlan_ticks_per_sec))
	#define wlan_ms_to_ticks(n)		( (n) / (wlan_ms_per_tick))
	#define wlan_tu2ticks(n)		( (n) / (wlan_ms_per_tick))
	#define WLAN_INT_DISABLE(n)		{ save_flags((n)); cli(); }
	#define WLAN_INT_ENABLE(n)		{ sti(); restore_flags((n)); }
	#define am930shim_pballoc_p80211( pb, len)	am930llc_pballoc_p80211((pb),(len))
	#define am930shim_pballoc		am930llc_pballoc
	#define am930shim_pbfree		am930llc_pbfree
	#define am930shim_malloc(l,d)		kmalloc((l), (d))
	#define am930shim_free(p,l)		kfree_s((p), (l))
	#define am930shim_rxframe(o,p)		am930llc_rxframe((o), (p))
	#define am930shim_ontxcomplete(o,r)	am930llc_ontxcomplete((o), (r))
	typedef	UINT32				wlan_flags_t;
	#ifdef CONFIG_MODVERSIONS
		#define MODVERSIONS		1
		#include <linux/modversions.h>
	#endif

	#ifdef CONFIG_SMP
		#define __SMP__			1
	#endif	

#elif (WLAN_OS == WLAN_LWOS)
	#define atomic_t							UINT32
	#define jiffies				(sNumTicks)
	#define wlan_ticks_per_sec		((kTicksPerTenth) * 10UL)
	#define HZ				(wlan_ticks_per_sec)
	#define wlan_ms_per_tick		(1000UL / (wlan_ticks_per_sec))
	#define wlan_ms_to_ticks(n)		( (n) / (wlan_ms_per_tick))
	#define wlan_tu2ticks(n)		( (n) / (wlan_ms_per_tick))
	#define udelay(n) 			{ int i, j=0; for( i = (n)*10; i > 0; i--) j++; }
	#define outb_p(v, a)			(*((UINT8*)(a))) = (v);
	#define outb(v, a)			(*((UINT8*)(a))) = (v);
	#define inb_p(a)			(*((UINT8*)(a)))
	#define inb(a)				(*((UINT8*)(a)))
	#define readb(a)			(*((UINT8*)(a)))
	#define writeb(byte, dest)		((*((UINT8*)(dest))) = ((UINT8)(byte)))
	#define test_and_set_bit(b, p)	\
		(((*(UINT32*)(p)) & ((BIT0) << (b))) ? \
		(1) : (((*(UINT32*)(p)) | ((BIT0) << (b))), (0)) )
	#define clear_bit(b, p)			((*((UINT32*)(p))) |= ((BIT0) << (b)))

	#define kmalloc(l,d)			malloc((l))
	#define kfree_s(p,l)			free((p))
	#define GFP_KERNEL			0
	#define GFP_ATOMIC			0
	#define WLAN_INT_DISABLE(n)		{ IntDisable(&(n)); }
	#define WLAN_INT_ENABLE(n)		{ IntRestore(&(n)); }
	#define spin_lock_irqsave(a,b)		WLAN_INT_DISABLE((b));
	#define spin_unlock_irqrestore(a,b)	WLAN_INT_ENABLE((b));
	#define am930shim_pballoc_p80211( pb, len)	am930lw_pballoc_p80211((pb),(len))
	#define am930shim_pballoc		am930lw_pballoc
	#define am930shim_pbfree		am930lw_pbfree
	#define am930shim_malloc(l,d)		am930lw_bufalloc((l))
	#define am930shim_free(p,l)		am930lw_buffree((p), (l))
	#define am930shim_rxframe(o,p)		am930lw_rxframe((o), (p))
	#define am930shim_ontxcomplete(o,r)	am930lw_ontxcomplete((o), (r))
	#define SPIN_LOCK_UNLOCKED		0
	#define time_after(a,b)			((UINT32)(b) - (UINT32)(a) < 0)
	#define time_before(a,b)		time_after(b,a)
	#define time_after_eq(a,b)		((UINT32)(a) - (UINT32)(b) >= 0)
	#define time_before_eq(a,b)		time_after_eq(b,a)
	typedef UINT32				spinlock_t;
	typedef	UINT32				wlan_flags_t;
#endif

#define wlan_minutes2ticks(a) ((a)*(wlan_ticks_per_sec *  60))
#define wlan_seconds2ticks(a) ((a)*(wlan_ticks_per_sec))

/*=============================================================*/
/*------ Hardware Portability Macros --------------------------*/
/*=============================================================*/

#if (WLAN_OS == WLAN_LINUX_KERNEL || WLAN_OS == WLAN_LINUX_USER)
	#define ieee2host16(n)	__le16_to_cpu(n)
	#define ieee2host32(n)	__le32_to_cpu(n)
	#define host2ieee16(n)	__cpu_to_le16(n)
	#define host2ieee32(n)	__cpu_to_le32(n)
#elif (WLAN_OS == WLAN_LWOS)
	#define __swab16(n) \
		((UINT16) ( \
			((((UINT16)(n)) & ((UINT16)0x00ffU)) << 8 ) | \
			((((UINT16)(n)) & ((UINT16)0xff00U)) >> 8 )      ))
	#define __swab32(n) \
		((UINT32) ( \
			((((UINT32)(n)) & ((UINT32)0x000000ffUL)) << 24 ) | \
			((((UINT32)(n)) & ((UINT32)0x0000ff00UL)) <<  8 ) | \
			((((UINT32)(n)) & ((UINT32)0x00ff0000UL)) >>  8 ) | \
			((((UINT32)(n)) & ((UINT32)0xff000000UL)) >> 24 ) )) 

	#ifndef htons
		#define htons(n) (n)
	#endif

	#ifndef ntohs
		#define ntohs(n) (n)
	#endif
#elif (WLAN_OS == WLAN_BEOS)

	#define wlan_outb(byte, io_addr) 		(*pci->write_io_8)((io_addr), (byte))
	#define wlan_outw(word, io_addr) 		(*pci->write_io_16)((io_addr), (word))
	#define wlan_outl(lword, io_addr) 		(*pci->write_io_32)((io_addr), (lword))
	#define wlan_inb(io_addr) 				(*pci->read_io_8)(io_addr)
	#define wlan_inw(io_addr) 				(*pci->read_io_16)(io_addr)
	#define wlan_inl(io_addr) 				(*pci->read_io_32)(io_addr)

	#define wlan_inw_le16_to_cpu(a)			(B_LENDIAN_TO_HOST_INT16(wlan_inw(a)))
	#define wlan_inw_be16_to_cpu(a)         (B_BENDIAN_TO_HOST_INT16(wlan_inw(a)))
	#define wlan_inl_le32_to_cpu(a)         (B_LENDIAN_TO_HOST_INT32(wlan_inl(a)))
	#define wlan_inl_be32_to_cpu(a)         (B_BENDIAN_TO_HOST_INT32(wlan_inl(a)))

	#define wlan_outw_cpu_to_le16(v,a)      wlan_outw(B_HOST_TO_LENDIAN_INT16((v)), (a))
	#define wlan_outw_cpu_to_be16(v,a)      wlan_outw(B_HOST_TO_BENDIAN_INT16((v)), (a))
	#define wlan_outl_cpu_to_le32(v,a)      wlan_outl(B_HOST_TO_LENDIAN_INT32((v)), (a))
	#define wlan_outl_cpu_to_be32(v,a)      wlan_outl(B_HOST_TO_BENDIAN_INT32((v)), (a))

	#define ieee2host16(n)	B_LENDIAN_TO_HOST_INT16(n)
	#define ieee2host32(n)	B_LENDIAN_TO_HOST_INT32(n)
	#define host2ieee16(n)	B_HOST_TO_LENDIAN_INT16(n)
	#define host2ieee32(n)	B_HOST_TO_LENDIAN_INT32(n)


#elif (WLAN_OS == WLAN_LINUX_KERNEL && WLAN_CPU_FAMILY == WLAN_Ix86)
	#define wlan_inb(a)			inb((a))
	#define wlan_inw(a)			inw((a))
	#define wlan_inw_le16_to_cpu(a)		inw((a))
	#define wlan_inw_be16_to_cpu(a)		(__be16_to_cpu(inw((a))))
	#define wlan_inl(a)			inl((a))
	#define wlan_inl_le32_to_cpu(a)		inl((a))
	#define wlan_inl_be32_to_cpu(a)		(__be32_to_cpu(inl((a))))

	#define wlan_outb(v,a)			outb((v),(a))
	#define wlan_outw(v,a)			outw((v),(a))
	#define wlan_outw_cpu_to_le16(v,a)	outw((v),(a))
	#define wlan_outw_cpu_to_be16(v,a)	outw(__cpu_to_be16((v)), (a))
	#define wlan_outl(v,a)			outl((v),(a))
	#define wlan_outl_cpu_to_le32(v,a)	outl((v),(a))
	#define wlan_outl_cpu_to_be32(v,a)	outl(__cpu_to_be32((v)), (a))
#elif (WLAN_OS == WLAN_LINUX_KERNEL && WLAN_CPU_FAMILY == WLAN_PPC) 
	#define wlan_inb(a)			inb((a))
	#define wlan_inw(a)			in_be16((unsigned short *)((a)+_IO_BASE))
	#define wlan_inw_le16_to_cpu(a)		inw((a))
	#define wlan_inw_be16_to_cpu(a)		in_be16((unsigned short *)((a)+_IO_BASE))
	#define wlan_inl(a)			in_be32((unsigned short *)((a)+_IO_BASE))
	#define wlan_inl_le32_to_cpu(a)		inwl((a))
	#define wlan_inl_be32_to_cpu(a)		in_be32((unsigned short *)((a)+_IO_BASE))

	#define wlan_outb(v,a)			outb((v),(a))
	#define wlan_outw(v,a)			out_be16((unsigned short *)((a)+_IO_BASE), (v))
	#define wlan_outw_cpu_to_le16(v,a)	outw((v),(a))
	#define wlan_outw_cpu_to_be16(v,a)	out_be16((unsigned short *)((a)+_IO_BASE), (v))
	#define wlan_outl(v,a)			out_be32((unsigned short *)((a)+_IO_BASE), (v))
	#define wlan_outl_cpu_to_le32(v,a)	outl((v),(a))
	#define wlan_outl_cpu_to_be32(v,a)	out_be32((unsigned short *)((a)+_IO_BASE), (v))
#endif

/*=============================================================*/
/*--- General Macros ------------------------------------------*/
/*=============================================================*/

#define wlan_max(a, b) (((a) > (b)) ? (a) : (b))
#define wlan_min(a, b) (((a) < (b)) ? (a) : (b))

#define WLAN_KVERSION(a,b,c)	(((a)<<16)+((b)<<8)+(c))

/*=============================================================*/
/*--- Variables -----------------------------------------------*/
/*=============================================================*/

extern int wlan_debug;
extern int wlan_ethconv;		/* What's the default ethconv? */

/*=============================================================*/
/*--- Functions -----------------------------------------------*/
/*=============================================================*/

#endif /* _WLAN_COMPAT_H */


/*
* Copyright (C) 1999 AbsoluteValue Systems, Inc.  All Rights Reserved.
* --------------------------------------------------------------------
*
* linux-wlan
*
*   The contents of this file are subject to the Mozilla Public
*   License Version 1.1 (the "License"); you may not use this file
*   except in compliance with the License. You may obtain a copy of
*   the License at http://www.mozilla.org/MPL/
*
*   Software distributed under the License is distributed on an "AS
*   IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
*   implied. See the License for the specific language governing
*   rights and limitations under the License.
*
*   Alternatively, the contents of this file may be used under the
*   terms of the GNU Public License version 2 (the "GPL"), in which
*   case the provisions of the GPL are applicable instead of the
*   above.  If you wish to allow the use of your version of this file
*   only under the terms of the GPL and not to allow others to use
*   your version of this file under the MPL, indicate your decision
*   by deleting the provisions above and replace them with the notice
*   and other provisions required by the GPL.  If you do not delete
*   the provisions above, a recipient may use your version of this
*   file under either the MPL or the GPL.
*
* --------------------------------------------------------------------
*
* Inquiries regarding the linux-wlan Open Source project can be
* made directly to:
*
* AbsoluteValue Systems Inc.
* info@linux-wlan.com
* http://www.linux-wlan.com
*
* --------------------------------------------------------------------
*
* Portions of the development of this software were funded by 
* Intersil Corporation as part of PRISM(R) chipset product development.
*
* --------------------------------------------------------------------
*/

