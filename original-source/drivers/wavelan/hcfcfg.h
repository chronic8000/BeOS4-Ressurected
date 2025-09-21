
#ifndef HCFCFG_
#define HCFCFG_H 1

/**************************************************************************************************************
*
* FILE	 : hcfcfg.tpl // hcfcfg.h **************************** 2.0 ********************************************
*
* DATE   : $Date: 2000/05/30 21:26:08 $   $Revision: 1.1 $
*
* AUTHOR : Nico Valster
*
* DESC   : HCF Customization Macros
*
***************************************************************************************************************
* COPYRIGHT (c) 1994, 1995		 				by AT&T.	 			All Rights Reserved.
* COPYRIGHT (c) 1996, 1997, 1998, 1999, 2000	by Lucent Technologies.	All Rights Reserved.
*
***************************************************************************************************************
*
* hcfcfg.tpl list all #defines which must be specified to:
*    I:	adjust the HCF functions defined in HCF.CPP to the characteristics of a specific environment
* 		o maximum sizes for messages and notification frames, persistent configuration storage
* 		o Endianess
*
*	II:	Compiler specific macros
* 		o port I/O macros
* 		o type definitions
*
*  III:	Environment specific ASSERT macro
*
*   IV: Compiler specific
*
*    V: ;? specific
*
*
* By copying HCFCFG.TPL to HCFCFG.H and -if needed- modifying the #defines the WCI functionality can be
* tailored
*
* Supported environments:
WVLAN_41	Miniport                                NDIS 3.1
WVLAN_42	Packet                                	Microsoft Visual C 1.5
WVLAN_43	16 bits DOS ODI                      	Microsoft Visual C 1.5
WVLAN43L	16 bits DOS ODI on top of HCF-light		Microsoft Visual C 1.5
WVLAN_44	32 bits ODI (__NETWARE_386__)			WATCOM
WVLAN_45	MAC_OS									MPW?, Symantec?
WVLAN_46	Windows CE (_WIN32_WCE)					Microsoft ?
WVLAN_47	LINUX  (__LINUX__)						GCC
WVLAN_48	Miniport                                NDIS 5
WVLAN_51	Miniport                                NDIS 4
WVLAN_77	BeOS									GCC
WVLAN_81	WavePoint								BORLAND C
WCITST		Inhouse test tool						Microsoft Visual C 1.5
WSU			WaveLAN Station Update					Microsoft Visual C ??
SCO UNIX	not yet actually used ?					?
__ppc		OEM supplied							?
_AM29K		OEM supplied							?
?			OEM supplied							Microtec Research 80X86 Compiler

* T O   D O :  A D D   A   R E C I P E   H O W  T O   M O D I F Y  HCFCFG.H
*
**************************************************************************************************************/

/****************************************************************************
$Log: hcfcfg.h,v $
Revision 1.1  2000/05/30 21:26:08  baron
Initial revision

 * 
 *    Rev 1.34   11 Feb 2000 14:19:22   NVALST
 * sync with home
 * 
 *    Rev 1.33   11 Feb 2000 10:17:50   NVALST
 * roll back to HCF 4.6 to build DOS ODI 6.0 release candidate
 * 
 *    Rev 1.32   08 Feb 2000 09:28:30   NVALST
 * passed to reinier
 * 
 *    Rev 1.31   04 Feb 2000 14:44:40   NVALST
 * Intermediate, after inserting CDOC comments
 * 
 *    Rev 1.30   04 Feb 2000 14:10:10   NVALST
 * intermediate, backup before inserting CDOC comments
   
      Rev 4.6   01 Feb 2000 12:02:32   NVALST
   
      Rev 4.6   28 Jan 2000 15:39:46   NVALST
 * 
 *    Rev 1.29   28 Jan 2000 13:27:44   NVALST
 * 
 *    Rev 4.0   07 Jul 1999 10:09:22   NVALST
 * 
 *    Rev 1.169   07 Jul 1999 09:45:04   NVALST
 * 
*************************************************************************************************/


/****************************************************************************
*
* CHANGE HISTORY
*

  960702 - NV
	Original Entry - derived from HCF 2.12
*************************************************************************************************/

/*FF***************************************************************************************************************
FUNCT: IN_PORT_BYTE                                                                              2000/02/04 14:15
USERS: hcfio_string                                     
CALLS: _inp                                             
GLOBL: hcf_io                                           
LOCAL: port                                             
******************************************************************************************************************/


/*FF***************************************************************************************************************
FUNCT: IN_PORT_WORD                                                                              2000/02/04 14:15
USERS: hcf_action                                       hcf_service_nic                                  
       alloc                                            hcf_aux_cntl                                     
       calibrate                                        cmd_wait                                         
       download_data                                    get_info_mb                                      
       ini_hermes                                       isr_info                                         
       hcfio_string                                     
CALLS: _inpw                                            
GLOBL: hcf_io                                           
LOCAL: port                                             
******************************************************************************************************************/


/*FF***************************************************************************************************************
FUNCT: OUT_PORT_BYTE                                                                             2000/02/04 14:15
USERS: hcfio_string                                     
CALLS: _outp                                            
GLOBL: hcf_io                                           
LOCAL: port                                             value                                            
******************************************************************************************************************/


/*FF***************************************************************************************************************
FUNCT: OUT_PORT_WORD                                                                             2000/02/04 14:15
USERS: hcf_action                                       hcf_get_info                                     
       hcf_service_nic                                  alloc                                            
       hcf_aux_cntl                                     calibrate                                        
       cmd_wait                                         download                                         
       download_data                                    get_info_mb                                      
       ini_hermes                                       hcfio_string                                     
CALLS: _outpw                                           
GLOBL: hcf_io                                           
LOCAL: port                                             value                                            
******************************************************************************************************************/


/*FF***************************************************************************************************************
FUNCT: IN_PORT_STRING                                                                            2000/02/04 14:15
USERS: hcf_get_info                                     hcfio_string                                     
GLOBL: cx                                               di                                               
       dx                                               es                                               
       insw                                             les                                              
       mov                                              pop                                              
       push                                             rep                                              
LOCAL: dest                                             len                                              
       port                                             
******************************************************************************************************************/


/*FF***************************************************************************************************************
FUNCT: OUT_PORT_STRING                                                                           2000/02/04 14:15
USERS: download_data                                    hcfio_string                                     
GLOBL: cx                                               ds                                               
       dx                                               lds                                              
       mov                                              outsw                                            
       pop                                              push                                             
       rep                                              si                                               
LOCAL: len                                              port                                             
       src                                              
******************************************************************************************************************/


/*  * * * * * * * * * * * * * * * * * * * * * *  I * * * * * * * * * * * * * * * * * * * * * * */

//#define HCF_HSI_VAR_0				// Controlled Deployment
//#define HCF_HSI_VAR_1				// WaveLAN
//#define HCF_HSI_VAR_2				// HomeLAN

/*	Alignment
 *	Some platforms can access words on odd boundaries (with possibly an performance impact), at other
 *	platforms such an access may result in a memory access violation.
 *	It is assumed that everywhere where the HCF casts a char pointer into a word pointer, the
 *	alignment criteria are met. This put some restrictions on the MSF, which are assumed to be
 *	"automatically" fullfilled at the applicable platforms
 *	To assert this assumption, the macro HCF_ALIGN can be defined. The default vaslue is 0, meaning no
 *	alignment, a value of 2 means word alignment, other values are invalid
 */

/*  * * * * * * * * * * * * * * * * * * * * * * II * * * * * * * * * * * * * * * * * * * * * * */



/************************************************************************************************/
/******************  C O M P I L E R   S P E C I F I C   M A C R O S  ***************************/
/************************************************************************************************/
/*************************************************************************************************
*
* The platforms supported by this version are:
*	- Microsoft Visual C 1.5 (16 bits platform)
*	- Microsoft Visual C 2.0 (32 bits platform)
*	- Watcom C/C++ 9.5
*	- SCO UNIX
*
* In this version of hcfiocfg.tpl all macros except the MSVC 1.5 versions are either dependent on
* compiler/environment supplied macros (e.g. _MSC_VER or "def-ed out"
*
* By selecting the appropriate Macro definitions by means of modifying the
* "#ifdef 0/1" lines, the HCF can be adjusted for the I/O chararcteristics of
* a specific compiler
*
* If needed the macros can be modified or replaced with definitions appropriate
* for your personal platform
* If you need to make such changes it is appreciated if you inform Lucent Technologies WCND Utrecht
* That way the changes can become part of the next release of the WCI
*
*
*	The prototypes and functional description of the macros are:
*
*	hcf_8	IN_PORT_BYTE(  hcf_16 port)
*			Reads a byte (8 bits) from the specified port
*
*	hcf_16	IN_PORT_WORD(  hcf_16 port)
*			Reads a word (16 bits) from the specified port
*
*	void	OUT_PORT_BYTE( hcf_16 port, hcf_8 value)
*			Writes a byte (8 bits) to the specified port
*
*	void	OUT_PORT_WORD( hcf_16 port, hcf_16 value)
*			Writes a word (16 bits) to the specified port
*
*	void	IN_PORT_STRING( port, dest, len)
*			Reads len number of words from the specified port to the (FAR) address dest in PC-RAM
*			Note that len specifies the number of words, NOT the number of bytes
*			!!!NOTE, although len specifies the number of words, dest MUST be a char pointer NOTE!!!
*			See also the common notes for IN_PORT_STRING and OUT_PORT_STRING
*
*	void	OUT_PORT_STRING( port, src, len)
*			Writes len number of words from the (FAR) address src in PC-RAM to the specified port
*			Note that len specifies the number of words, NOT the number of bytes.
*			!!!NOTE, although len specifies the number of words, src MUST be a char pointer NOTE!!!
*
*			The peculiar combination of word-length and char pointers for IN_PORT_STRING as well as
*			OUT_PORT_STRING is justified by the assumption that it offers a more optimal algorithm
*
*			Note to the HCF-implementor:
*			Due to the passing of the parameters to compiler specific blabla.........
*			do not use "expressions" as parameters, e.g. don't use "ifbp->IFB_IOBase + HREG_AUX_DATA" but
*			assign this to a temporary variable.
*
*
*  NOTE!!	For convenience of the MSF-programmer, all {IN|OUT}_PORT_{BYTE|WORD|STRING} macros are allowed to
*			modify their parameters (although some might argue that this would constitute bad coding
*			practice). This has its implications on the HCF, e.g. as a consequence these macros should not
*			be called with parameters which have side effects, e.g auto-increment.
*
*  NOTE!!	in the Micosoft implementation of inline assembly it is O.K. to corrupt all flags except
*			the direction flag and to corrupt all registers except the segment registers and EDI, ESI,
*			ESP and EBP (or their 16 bits equivalents).
*			Other environments may have other constraints
*
*  NOTE!!	in the Intel environment it is O.K to have a word (as a 16 bits quantity) at a byte boundary,
*			hence IN_/OUT_PORT_STRING can move words between PC-memory and NIC-memory with as only
*			constraint that the the words are on a word boundary in NIC-memory. This does not hold true
*			for all conceivalble environments, e.g. an Motorola 68xxx does not allow this, in other
*			words whenever there is a move from address in 2*n in one memory type to address 2*m+1 in the
*			other type, the current templates for IN_/OUT_PORT_STRING are unsuitable. Probably the
*			boundary conditions imposed by these type of platforms prevent this case from materializing
*
*************************************************************************************************/

// Note:
// Visual C++ 1.5 : _MSC_VER ==  800
// Visual C++ 4.0 : _MSC_VER == 1000
// Visual C++ 4.2 : _MSC_VER == 1020

/* typedef unsigned char				hcf_8;
 * typedef unsigned short				hcf_16;
 * typedef unsigned long				hcf_32;
 *
 * All 16 sections defined at april 2, 1999 contained the same typedefs for hcf_8/16/32
 * This seems to warrant till other requirements pop up, to make these typedefs global
 * rather than module specific
 */

/************************************************************************************************/
/******************************  M I N I P O R T    N D I S  3.1  *******************************/
/************************************************************************************************/

#if defined WVLAN_41

#define	MSF_COMPONENT_ID			COMP_ID_MINIPORT_NDIS_31
#define HCF_STA						//station characteristics

#include <ndis.h>
#include <version.h>

#define	MSF_COMPONENT_VAR			1
#define	MSF_COMPONENT_MAJOR_VER		TPI_MAJOR_VERSION
#define	MSF_COMPONENT_MINOR_VER		TPI_MINOR_VERSION

#define	DUI_COMPAT_BOT              5
#define	DUI_COMPAT_TOP              5


//#define CNV_LITTLE_TO_INT(x)			// No endianess conversion needed										

__inline UCHAR NDIS_IN_BYTE( ULONG port )
{
    UCHAR value;
    NdisRawReadPortUchar(port , &value);
    return (value);
}

__inline USHORT NDIS_IN_WORD( ULONG port )
{
    USHORT value;
    NdisRawReadPortUshort(port , &value);
    return (value);
}

#define IN_PORT_BYTE(port)			NDIS_IN_BYTE( (ULONG) (port) )
#define IN_PORT_WORD(port)			NDIS_IN_WORD( (ULONG) (port) )
#define OUT_PORT_BYTE(port, value)	NdisRawWritePortUchar( (ULONG) (port) , (UCHAR) (value))
#define OUT_PORT_WORD(port, value)	NdisRawWritePortUshort((ULONG) (port) , (USHORT) (value))

#define IN_PORT_STRING(port, addr, len)		NdisRawReadPortBufferUshort(port, addr, (len));
#define OUT_PORT_STRING(port, addr, len)	NdisRawWritePortBufferUshort(port, addr, (len));

typedef UCHAR	hcf_8;
typedef USHORT	hcf_16;
typedef ULONG	hcf_32;

#endif	/* WVLAN_41 MINIPORT 3.1 */




/************************************************************************************************/
/******************************  M I N I P O R T    N D I S  ?.?  *******************************/
/************************************************************************************************/

#if defined WVLAN_41B

#define	MSF_COMPONENT_ID			COMP_ID_MINIPORT_NDIS_31 //  HomeLAN Driver
#define HCF_STA						//station characteristics
#define HCF_AP						//ap characteristics

#include <ndis.h>
#include <version.h>

//#define HCF_HSI_VAR_2				2

#define MSF_COMPONENT_VAR			2
#define	MSF_COMPONENT_MAJOR_VER		TPI_MAJOR_VERSION
#define	MSF_COMPONENT_MINOR_VER		TPI_MINOR_VERSION

#define DUI_COMPAT_VAR				2
#define DUI_COMPAT_BOT				1
#define DUI_COMPAT_TOP				1

// Added for STAP driver to have STA 1,7,7
#define HCF_CFG_STA_1_BOTTOM        7

//#define CNV_LITTLE_TO_INT(x)			// No endianess conversion needed										

__inline UCHAR NDIS_IN_BYTE( ULONG port )
{
    UCHAR value;
    NdisRawReadPortUchar(port , &value);
    return (value);
}

__inline USHORT NDIS_IN_WORD( ULONG port )
{
    USHORT value;
    NdisRawReadPortUshort(port , &value);
    return (value);
}

#define IN_PORT_BYTE(port)			NDIS_IN_BYTE( (ULONG) (port) )
#define IN_PORT_WORD(port)			NDIS_IN_WORD( (ULONG) (port) )
#define OUT_PORT_BYTE(port, value)	NdisRawWritePortUchar( (ULONG) (port) , (UCHAR) (value))
#define OUT_PORT_WORD(port, value)	NdisRawWritePortUshort((ULONG) (port) , (USHORT) (value))

#define IN_PORT_STRING(port, addr, len)		NdisRawReadPortBufferUshort(port, addr, (len));
#define OUT_PORT_STRING(port, addr, len)	NdisRawWritePortBufferUshort(port, addr, (len));

typedef UCHAR	hcf_8;
typedef USHORT	hcf_16;
typedef ULONG	hcf_32;

#endif	/* WVLAN_41B MINIPORT 3.1 */



/************************************************************************************************/
/****************************  P A C K E T   D R I V E R  ***************************************/
/**********************************  D O S   O D I  *********************************************/
/************************************************************************************************/

#if defined WVLAN_42 || defined WVLAN_43|| defined WVLAN43L

#pragma warning ( disable: 4001 )
										
#define HCF_STA						//station characteristics

#if defined WVLAN_43

#define HCF_TX_ERR_NOTIFY

#define	MSF_COMPONENT_ID			COMP_ID_ODI_16
#define	MSF_COMPONENT_VAR			1
#define	MSF_COMPONENT_MAJOR_VER		6
#define	MSF_COMPONENT_MINOR_VER		0

//#define HCF_CFG_STA_1_BOTTOM 2

//#define	HCF_MAX_CONFIG				0
//#define HCF_DOWNLOAD_OFF
//#define HCF_MB_OFF


#elif defined WVLAN_42
#define	MSF_COMPONENT_ID			COMP_ID_PACKET
#define	MSF_COMPONENT_VAR			1
#define	MSF_COMPONENT_MAJOR_VER		6
#define	MSF_COMPONENT_MINOR_VER		0

#elif defined WVLAN43L
#define	HCF_MAX_CONFIG				0

#define	MSF_COMPONENT_MAJOR_VER		0
#define	MSF_COMPONENT_MINOR_VER		1

#endif //WVLAN_xx

#define HCF_MAX_GROUP		16		// number of Multicast Addresses supported by ODI/Packet
										
#define FAR  __far					// segmented 16 bits mode
#if defined _M_I86TM
#define BASED __based(__segname("_CODE"))
#endif // _M_I86TM

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#include <stdio.h>
#include <conio.h>
#ifndef _DEBUG
#pragma intrinsic( _inp, _inpw, _outp, _outpw )
#endif // _DEBUG

#define IN_PORT_BYTE(port)			((hcf_8)_inp( (hcf_io)(port) ))
#define IN_PORT_WORD(port)			((hcf_16)_inpw( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	((void)_outp( (hcf_io)(port), value ))
#define OUT_PORT_WORD(port, value)	((void)_outpw( (hcf_io)(port), value ))

#if defined HCF_STRICT
#define IN_PORT_STRING( prt, dst, n)	{ ips( prt, dst, n); }
#define OUT_PORT_STRING( prt, dst, n)	{ ops( prt, dst, n); }
#elif 0		// C implementation which let the processor handle the word-at-byte-boundary problem
#define IN_PORT_STRING( prt, dst, n)	while ( n-- ) { *(hcf_16 FAR*)dst = IN_PORT_WORD( prt ); dst += 2; }
#define OUT_PORT_STRING( prt, src, n)	while ( n-- ) { OUT_PORT_WORD( prt, *(hcf_16 FAR*)src ) ; src  += 2; }
#elif 0		// C implementation which handles the word-at-byte-boundary problem 
#define IN_PORT_STRING( prt, dst, n)	while ( n-- ) { int i = IN_PORT_WORD( prt ); *dst++ = (char)i; *dst++ = (char)(i >> 8); }
#define OUT_PORT_STRING( prt, src, n)	while ( n-- ) { OUT_PORT_WORD( prt, *src | *(src+1)<<8  ) ; src  += 2; }
//;?  WHY hcf_16 FAR*)src and not unsigned char FAR*)src
#else												// Assembler implementation
#define IN_PORT_STRING( port, dest, len) __asm 		\
{													\
	__asm push di                               	\
	__asm push es                                 	\
	__asm mov cx,len                            	\
	__asm les di,dest                           	\
	__asm mov dx,port                           	\
	__asm rep insw                              	\
	__asm pop es	                            	\
	__asm pop di	                            	\
}

#define OUT_PORT_STRING( port, src, len) __asm		\
{                                               	\
	__asm push si                                 	\
	__asm push ds                                 	\
	__asm mov cx,len                              	\
	__asm lds si,src                             	\
	__asm mov dx,port                             	\
	__asm rep outsw	                            	\
	__asm pop ds                                  	\
	__asm pop si                                  	\
}

#endif	// Asm or C implementation

#endif	/* WVLAN_43, WVLAN_42 (DOS ODI, Packet Driver) */


/************************************************************************************************/
/****************************  N E T W A R E   3 8 6  *******************************************/
/************************************************************************************************/

#if defined  __NETWARE_386__	/* WVLAN_44, WATCOM */

#define	MSF_COMPONENT_ID			COMP_ID_ODI_32
#define HCF_STA						//station characteristics

#include <conio.h>

//#define CNV_LITTLE_TO_INT(x) (x)			// No endianess conversion needed

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#define IN_PORT_BYTE(port)			((hcf_8)inp( (hcf_io)(port) ))  //;?leave out cast to hcf_8;?
#define IN_PORT_WORD(port)			(inpw( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	(outp( (hcf_io)(port), value ))
#define OUT_PORT_WORD(port, value)	(outpw( (hcf_io)(port), value ))

#define IN_PORT_STRING( prt, dst, n)	while ( n-- ) { *(hcf_16*)dst = IN_PORT_WORD( prt ); dst += 2; }
#define OUT_PORT_STRING( prt, src, n)	while ( n-- ) { OUT_PORT_WORD( prt, *(hcf_16*)src ) ; src  += 2; }

#endif	// WVLAN_44, __NETWARE_386__


/************************************************************************************************/
/***********************************  M A C  O S   **********************************************/
/************************************************************************************************/

            	    	/**********/
#if defined WVLAN_45    /* MAC_OS */
                		/**********/
#include "Version.h"

#define HCF_STA                     //station characteristics
#define MSF_COMPONENT_ID            COMP_ID_MAC_OS
#define MSF_COMPONENT_VAR           VARIANT
#define MSF_COMPONENT_MAJOR_VER     VERSION_MAJOR
#define MSF_COMPONENT_MINOR_VER     VERSION_MINOR

#define MAC_OS                      1

#define HCF_BIG_ENDIAN              // selects Big Endian (a.k.a. Motorola), most significant byte first

#if defined(DEBUG)
#define HCF_ASSERT                  1
#endif // DEBUG

typedef unsigned char               hcf_8;
typedef unsigned short              hcf_16;
typedef unsigned long               hcf_32;

#ifdef  __cplusplus
extern "C" {
#endif
extern volatile unsigned char *MacIOaddr;
extern hcf_8  IN_PORT_BYTE(hcf_16 port);
extern void   OUT_PORT_BYTE(hcf_16 port, hcf_8 value);
extern hcf_16 IN_PORT_WORD(hcf_16 port);
extern void   OUT_PORT_WORD(hcf_16 port, hcf_16 value);
extern void   IN_PORT_STRING(hcf_16 port, void *dest, hcf_16 len);
extern void   OUT_PORT_STRING(hcf_16 port, void *src, hcf_16 len);

#define SwapBytes(t)    (((t) >> 8) + (((t) & 0xff) << 8))

#ifdef  __cplusplus
}
#endif

#endif  /* WVLAN_45, MAC_OS */

/************************************************************************************************/
/***********************************  W I N C E *************************************************/
/************************************************************************************************/

                  /*******************/
#ifdef _WIN32_WCE /* WVLAN_46, WINCE */
                  /*******************/

#define	MSF_COMPONENT_ID			COMP_ID_WIN_CE
#define HCF_STA						//station characteristics

#include <ndis.h>
#include <ntcompat.h>
#include "version.h"
#include "debug.h"

#define	MSF_COMPONENT_VAR			1
#define	MSF_COMPONENT_MAJOR_VER		WVLAN46_MAJOR_VERSION
#define	MSF_COMPONENT_MINOR_VER		WVLAN46_MINOR_VERSION

#define HCF_LITTLE_ENDIAN

#define HCF_MEM_IO					// overrule standard Port I/O with Memory mapped I/O

#if defined(DEBUG) || defined(_WIN32_WCE_DEBUG)
#define HCF_ASSERT                  1
#endif // DEBUG || _WIN32_WCE_DEBUG

typedef UCHAR   hcf_8 ;
typedef USHORT  hcf_16 ;
typedef ULONG   hcf_32 ;

extern hcf_8  IN_PORT_BYTE(hcf_32 port);
extern void   OUT_PORT_BYTE(hcf_32 port, hcf_8 value);
extern hcf_16 IN_PORT_WORD(hcf_32 port);
extern void   OUT_PORT_WORD(hcf_32 port, hcf_16 value);
extern void   IN_PORT_STRING(hcf_32 port, void *dest, hcf_16 len);
extern void   OUT_PORT_STRING(hcf_32 port, void *src, hcf_16 len);

#endif	/* WVLAN_46, _WIN32_WCE */


/************************************************************************************************/
/******************************************  L I N U X  *****************************************/
/************************************************************************************************/

#if defined WVLAN_47 || defined WVLAN_49

#define HCF_STA						//station characteristics
#if defined WVLAN_47
#define	MSF_COMPONENT_ID	COMP_ID_LINUX_PD
#endif // WVLAN_47
#if defined WVLAN_49
#define	MSF_COMPONENT_ID	COMP_ID_LINUX_LIB
#endif // WVLAN_49

#define	MSF_COMPONENT_VAR			1

#define __NO_VERSION__				//define kernel version only in MSF47 / MSF
#include <linux/module.h>
#include <asm/io.h>
#include <linux/types.h>

//#define CNV_LITTLE_TO_INT(x)			// No endianess conversion needed										

typedef __u8						hcf_8;
typedef __u16						hcf_16;
typedef __u32						hcf_32;

#define IN_PORT_BYTE(port)			((hcf_8)inb( (hcf_io)(port) ))
#define IN_PORT_WORD(port)			((hcf_16)inw( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	(outb( (hcf_8) (value), (hcf_io)(port) ))
#define OUT_PORT_WORD(port, value)	(outw( (hcf_16) (value), (hcf_io)(port) ))
#define IN_PORT_STRING(port, dst, n)	(insw((hcf_io)(port), dst, n))
#define OUT_PORT_STRING(port, src, n)	(outsw((hcf_io)(port), src, n))

#endif	/* LINUX */

/************************************************************************************************/
/******************************  M I N I P O R T    N D I S  5.0  *******************************/
/************************************************************************************************/

#if defined WVLAN_48

#define	MSF_COMPONENT_ID			COMP_ID_MINIPORT_NDIS_50
#define HCF_STA						//station characteristics

#include <ndis.h>
#include <version.h>

#define	MSF_COMPONENT_VAR			1
#define	MSF_COMPONENT_MAJOR_VER		TPI_MAJOR_VERSION
#define	MSF_COMPONENT_MINOR_VER		TPI_MINOR_VERSION

#define	DUI_COMPAT_BOT              5
#define	DUI_COMPAT_TOP              5


//#define CNV_LITTLE_TO_INT(x)			// No endianess conversion needed										

__inline UCHAR NDIS_IN_BYTE( ULONG port )
{
    UCHAR value;
    NdisRawReadPortUchar(port , &value);
    return (value);
}

__inline USHORT NDIS_IN_WORD( ULONG port )
{
    USHORT value;
    NdisRawReadPortUshort(port , &value);
    return (value);
}

#define IN_PORT_BYTE(port)			NDIS_IN_BYTE( (ULONG) (port) )
#define IN_PORT_WORD(port)			NDIS_IN_WORD( (ULONG) (port) )
#define OUT_PORT_BYTE(port, value)	NdisRawWritePortUchar( (ULONG) (port) , (UCHAR) (value))
#define OUT_PORT_WORD(port, value)	NdisRawWritePortUshort((ULONG) (port) , (USHORT) (value))

#define IN_PORT_STRING(port, addr, len)		NdisRawReadPortBufferUshort(port, addr, (len));
#define OUT_PORT_STRING(port, addr, len)	NdisRawWritePortBufferUshort(port, addr, (len));

typedef UCHAR	hcf_8;
typedef USHORT	hcf_16;
typedef ULONG	hcf_32;

#endif	/* WVLAN_48 MINIPORT 5.0 */

/************************************************************************************************/
/*********************************************  Q N X  ******************************************/
/************************************************************************************************/

#if defined  __QNX__ || defined WVLAN_50

#define HCF_STA						//station characteristics

#define MSF_COMPONENT_ID		0   //Although there is no DUI support, we need this to get ...
#define MSF_COMPONENT_VAR		0	//...compatibilty check to function
#define MSF_COMPONENT_MAJOR_VER	0	//...;?this is worth looking into to make this a more
#define MSF_COMPONENT_MINOR_VER	0	//..."defined" I/F so OEMers can figure out what to do

#include <conio.h>

//#define CNV_LITTLE_TO_INT(x)		// No endianess conversion needed										

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#define IN_PORT_BYTE(port)			((hcf_8)inp( (hcf_io)(port) ))
#define IN_PORT_WORD(port)			((hcf_16)inpw( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	(outp((hcf_io)(port), (hcf_8) (value)))
#define OUT_PORT_WORD(port, value)	(outpw( (hcf_io)(port), (hcf_16) (value) ))
#define IN_PORT_STRING( prt, dst, n)	while ( n-- ) { *(hcf_16*)dst = IN_PORT_WORD( prt ); dst += 2; }
#define OUT_PORT_STRING( prt, src, n)	while ( n-- ) { OUT_PORT_WORD( prt, *(hcf_16*)src ) ; src  += 2; }

#endif	/* QNX || WVLAN_50 */



/************************************************************************************************/
/*********************************************  B  E O S  ***************************************/
/************************************************************************************************/

#if defined  __BEOS__ 

#define HCF_STA						//station characteristics

#define MSF_COMPONENT_ID		0 //COMP_ID_BEOS   //Although there is no DUI support, we need this to get ...
#define MSF_COMPONENT_VAR		0	//...compatibilty check to function
#define MSF_COMPONENT_MAJOR_VER	0	//...;?this is worth looking into to make this a more
#define MSF_COMPONENT_MINOR_VER	0	//..."defined" I/F so OEMers can figure out what to do

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>

uint8 read_io_8 (int);
void write_io_8 (int, uint8);
uint16 read_io_16 (int);
void write_io_16 (int, uint16);


//#define CNV_LITTLE_TO_INT(x)		// No endianess conversion needed										

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#define IN_PORT_BYTE(port)			((hcf_8)read_io_8( (hcf_io)(port) ))
#define IN_PORT_WORD(port)			((hcf_16)read_io_16( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	(write_io_8((hcf_io)(port), (hcf_8) (value)))
#define OUT_PORT_WORD(port, value)	(write_io_16( (hcf_io)(port), (hcf_16) (value) ))
//#define OUT_PORT_BYTE(port, value)	dprintf( "OPB %04x %04x", port, value)//(write_io_8((hcf_io)(port), (hcf_8) (value)))
//#define OUT_PORT_WORD(port, value)	dprintf( "OPW %04x %04x", port, value)//(write_io_16( (hcf_io)(port), (hcf_16) (value) ))
#define IN_PORT_STRING( prt, dst, n)	while ( n-- ) { *(hcf_16*)dst = IN_PORT_WORD( prt ); dst += 2; }
#define OUT_PORT_STRING( prt, src, n)	while ( n-- ) { OUT_PORT_WORD( prt, *(hcf_16*)src ) ; src  += 2; }

#endif	/* BEOS */



/************************************************************************************************/
/******************************  M I N I P O R T    N D I S  4    *******************************/
/************************************************************************************************/

#if defined WVLAN_51

#define	MSF_COMPONENT_ID			COMP_ID_MINIPORT_NDIS_31
#define HCF_STA						//station characteristics

#include <ndis.h>
#include <version.h>

#define	MSF_COMPONENT_VAR			1
#define	MSF_COMPONENT_MAJOR_VER		TPI_MAJOR_VERSION
#define	MSF_COMPONENT_MINOR_VER		TPI_MINOR_VERSION

#define	DUI_COMPAT_BOT              5
#define	DUI_COMPAT_TOP              5


//#define CNV_LITTLE_TO_INT(x)			// No endianess conversion needed										

__inline UCHAR NDIS_IN_BYTE( ULONG port )
{
    UCHAR value;
    NdisRawReadPortUchar(port , &value);
    return (value);
}

__inline USHORT NDIS_IN_WORD( ULONG port )
{
    USHORT value;
    NdisRawReadPortUshort(port , &value);
    return (value);
}

#define IN_PORT_BYTE(port)			NDIS_IN_BYTE( (ULONG) (port) )
#define IN_PORT_WORD(port)			NDIS_IN_WORD( (ULONG) (port) )
#define OUT_PORT_BYTE(port, value)	NdisRawWritePortUchar( (ULONG) (port) , (UCHAR) (value))
#define OUT_PORT_WORD(port, value)	NdisRawWritePortUshort((ULONG) (port) , (USHORT) (value))

#define IN_PORT_STRING(port, addr, len)		NdisRawReadPortBufferUshort(port, addr, (len));
#define OUT_PORT_STRING(port, addr, len)	NdisRawWritePortBufferUshort(port, addr, (len));

typedef UCHAR	hcf_8;
typedef USHORT	hcf_16;
typedef ULONG	hcf_32;

#endif	/* WVLAN_51 MINIPORT 4 */

/************************************************************************************************/
/******************************************  FreeBSD  *******************************************/
/************************************************************************************************/

#if defined __FREE_BSD__

#define HCF_STA										//station characteristics
#define	MSF_COMPONENT_ID	        COMP_ID_FreeBSD
#define	MSF_COMPONENT_VAR			1
#define	MSF_COMPONENT_MAJOR_VER		1
#define	MSF_COMPONENT_MINOR_VER		0

#define HCF_MAX_MSG					2048			// overrule standard WaveLAN Pakket Size of 1514 with 2048
#define HCF_MEM_IO									// overrule standard Port I/O with Memory mapped I/O

#include <machine/cpufunc.h>

//#define CNV_LITTLE_TO_INT(x)						// No endianess conversion needed

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#define IN_PORT_BYTE(port)			((hcf_8)inb( (hcf_io)(port) ))
#define IN_PORT_WORD(port)			((hcf_16)inw( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	(outb((hcf_io)(port), (hcf_8)(value)))
#define OUT_PORT_WORD(port, value)	(outw((hcf_io)(port), (hcf_16)(value)))

#define IN_PORT_STRING( prt, dst, n)	while ( n-- ) { *(hcf_16*)dst = IN_PORT_WORD( prt ); dst += 2; }
#define OUT_PORT_STRING( prt, src, n)	while ( n-- ) { OUT_PORT_WORD( prt, *(hcf_16*)src ) ; src  += 2; }

#endif	/* FreeBSD */


/************************************************************************************************/
/*********************************  W A V E P O I N T  ******************************************/
/************************************************************************************************/

#if defined WVLAN_81	/* BORLANDC */

#define HCF_AP							//access point characteristics
#define HCF_IFB_SECURE				1	//IFB is secure in WavePOINT, HCF can restore WEP keys
#define	MSF_COMPONENT_ID	COMP_ID_AP1
#define	MSF_COMPONENT_VAR			1
#define	MSF_COMPONENT_MAJOR_VER		4
#define	MSF_COMPONENT_MINOR_VER		0

#define HCF_PROT_TIME				49	//49*10240 microseconds H/W failure protection timer				

#include <dos.h>

//#define CNV_LITTLE_TO_INT(x)			// No endianess conversion needed										

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

//#define HCF_ASSERT					0  /* debug build only */

#if !defined FAR
#define FAR  far					// segmented 16 bits mode
#endif //!defined FAR


#define IN_PORT_BYTE(port)					(inportb( (hcf_io)(port) ))
#define IN_PORT_WORD(port)					(inport( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)      	(outportb( (hcf_io)(port), value ))
#define OUT_PORT_WORD(port, value)      	(outport( (hcf_io)(port), value ))

#define IN_PORT_STRING(port, addr, len) 	\
	asm { push di; push es; mov cx,len; les di,addr; mov dx,port; rep insw; pop es; pop di }

#define OUT_PORT_STRING(port, addr, len)	\
	asm { push si; push ds; mov cx,len; lds si,addr; mov dx,port; rep outsw; pop ds; pop si }

#endif /* WVLAN_81 WavePoint */


/************************************************************************************************/
/********************************  W A V E L A U N C H  *****************************************/
/************************************************************************************************/

#if defined WVLAUNCH

#include "DriverX.h"
extern HWDEVICE*	g_pDevice;

//#define	MSF_COMPONENT_ID			0  //;? to get around browser problem
#define HCF_STA						//station characteristics

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;



#define IN_PORT_WORD(port)	HwInpw( g_pDevice, port )
#define OUT_PORT_WORD(port, value)	HwOutpw( g_pDevice, port, value )
#define IN_PORT_BYTE(port)	HwInp( g_pDevice, port )
#define OUT_PORT_BYTE(port, value)	HwOutp( g_pDevice, port, value )


// C implementation which let the processor handle the word-at-byte-boundary problem
#define IN_PORT_STRING( prt, dst, n)	while ( n-- ) { *(hcf_16 FAR*)dst = IN_PORT_WORD( prt ); dst += 2; }
#define OUT_PORT_STRING( prt, src, n)	while ( n-- ) { OUT_PORT_WORD( prt, *(hcf_16 FAR*)src ) ; src  += 2; }

#endif //WVLAUNCH

/************************************************************************************************/
/*************************************  W C I T S T *********************************************/
/************************************************************************************************/

#if defined WCITST
#define MSF_COMPONENT_ID		0   //Although there is no DUI support, we need this to get ...
#define MSF_COMPONENT_VAR		0	//...compatibilty check to function
#define MSF_COMPONENT_MAJOR_VER	0	//...;?this is worth looking into to make this a more
#define MSF_COMPONENT_MINOR_VER	0	//..."defined" I/F so OEMers can figure out what to do

#pragma warning ( disable: 4001 )
										
#define HCF_STA						//station characteristics
#define HCF_AP						//AccesPoint characteristics
#define HCF_TX_ERR_NOTIFY			//log erronuous transmissions in Mailbox

//#define HCF_DOWNLOAD_OFF
//#define HCF_MB_OFF

#if !defined _CONSOLE
#define FAR  __far					// segmented 16 bits mode
#if defined _M_I86TM
#define BASED __based(__segname("_CODE"))
#endif // _M_I86TM
#endif  //_CONSOLE

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#include <stdio.h>
#include <conio.h>
#ifndef _DEBUG
#pragma intrinsic( _inp, _inpw, _outp, _outpw )
#endif // _DEBUG

#ifdef LOG
extern FILE* utm_logfile;
hcf_16	ipw( hcf_16 port );
hcf_8	ipb( hcf_16 port );
void	opw( hcf_16 port, hcf_16 value );
void	opb( hcf_16 port, hcf_8 value );

#define IN_PORT_BYTE(port)			ipb( (hcf_io)(port) )
#define IN_PORT_WORD(port)			ipw( (hcf_io)(port) )
#define OUT_PORT_BYTE(port, value)	opb( (hcf_io)(port), (hcf_8)(value) )
#define OUT_PORT_WORD(port, value)	opw( (hcf_io)(port), (hcf_16)(value) )
#else //LOG
#define IN_PORT_BYTE(port)			((hcf_8)_inp( (hcf_io)(port) ))
#define IN_PORT_WORD(port)			((hcf_16)_inpw( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	((void)_outp( (hcf_io)(port), value ))
#define OUT_PORT_WORD(port, value)	((void)_outpw( (hcf_io)(port), value ))
#endif //LOG

#define	toch_maar_geen_asm
#if defined(toch_maar_asm)  && !defined(__DA_C__)  //;? temporary solution to satisfy DA-C
#define IN_PORT_STRING( port, dest, len) __asm 		\
{													\
	__asm push di                               	\
	__asm push es                                 	\
	__asm mov cx,len                            	\
	__asm les di,dest                           	\
	__asm mov dx,port                           	\
	__asm rep insw                              	\
	__asm pop es	                            	\
	__asm pop di	                            	\
}

#define OUT_PORT_STRING( port, src, len) __asm		\
{                                               	\
	__asm push si                                 	\
	__asm push ds                                 	\
	__asm mov cx,len                              	\
	__asm lds si,src                             	\
	__asm mov dx,port                             	\
	__asm rep outsw	                            	\
	__asm pop ds                                  	\
	__asm pop si                                  	\
}

#else	//toch_maar_asm  && !__DA_C__
#define IN_PORT_STRING( prt, dst, n)	while ( n-- ) { *(hcf_16 FAR*)dst = IN_PORT_WORD( prt ); dst += 2; }
#define OUT_PORT_STRING( prt, src, n)	while ( n-- ) { OUT_PORT_WORD( prt, *(hcf_16 FAR*)src ) ; src  += 2; }
//;?  WHY hcf_16 FAR*)src and not unsigned char FAR*)src
#endif	//toch_maar_asm  && !__DA_C__

#endif	/* WCITST */

/************************************************************************************************/
/********************************************  W S U  *******************************************/
/************************************************************************************************/

#if 0 //;? conflicts with WIN_CE _MSC_VER >= 1000 	/* Microsoft Visual C ++ 4.x, 5.x */

// Note:
// Visual C++ 4.0 : _MSC_VER == 1000
// Visual C++ 4.2 : _MSC_VER == 1020

										
#pragma warning ( disable: 4001 )
										
#define HCF_STA						//station characteristics

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#include <stdio.h>
#include <conio.h>

#define IN_PORT_BYTE(port)			((hcf_8)_inp( (hcf_io)(port) ))
#define IN_PORT_WORD(port)			((hcf_16)_inpw( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	((void)_outp( (hcf_io)(port), value ))
#define OUT_PORT_WORD(port, value)	((void)_outpw( (hcf_io)(port), value ))

#define	toch_maar_geen_asm
#if defined(toch_maar_asm)
#define IN_PORT_STRING( port, dest, len) __asm 		\
{													\
	__asm push di                               	\
	__asm push es                                 	\
	__asm mov cx,len                            	\
	__asm les di,dest                           	\
	__asm mov dx,port                           	\
	__asm rep insw                              	\
	__asm pop es	                            	\
	__asm pop di	                            	\
}

#define OUT_PORT_STRING( port, src, len) __asm		\
{                                               	\
	__asm push si                                 	\
	__asm push ds                                 	\
	__asm mov cx,len                              	\
	__asm lds si,src                             	\
	__asm mov dx,port                             	\
	__asm rep outsw	                            	\
	__asm pop ds                                  	\
	__asm pop si                                  	\
}

#else	//toch_maar_asm
#define IN_PORT_STRING( prt, dst, n)	while ( n-- ) { *(hcf_16*)dst = IN_PORT_WORD( prt ); dst += 2; }
#define OUT_PORT_STRING( prt, src, n)	while ( n-- ) { OUT_PORT_WORD( prt, *(hcf_16*)src ) ; src  += 2; }
#endif	//toch_maar_asm

#endif	/* _MSC_VER >= 1000 (Microsoft Visual C++ 4.0 ) */




/************************************************************************************************/
/********************************** S C O   U N I X  ********************************************/
/************************************************************************************************/

#if 0

#define HCF_STA						//station characteristics
#define	MSF_COMPONENT_ID

//#define CNV_LITTLE_TO_INT(x)			// No endianess conversion needed										

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#define IN_PORT_BYTE(port)			((hcf_8)inb( (hcf_io)(port) ))
#define IN_PORT_WORD(port)			((hcf_16)inw( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	(outb( (hcf_io)(port), (hcf_8) (value) ))
#define OUT_PORT_WORD(port, value)	(outw( (hcf_io)(port), (hcf_16) (value) ))

#endif	/* SCO UNIX */


/************************************************************************************************/
/**********************************   Diab or High C 29K   **************************************/
/************************************************************************************************/
/* known users: GK, JW
 */

#if defined(__ppc) || defined(_AM29K)

#define HCF_AP						//AccesPoint characteristics
#define MSF_COMPONENT_VAR       0
#define MSF_COMPONENT_ID        0
#define MSF_COMPONENT_MAJOR_VER 1
#define MSF_COMPONENT_MINOR_VER 0

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#define SwapBytes(t)    /*lint -e572*/(((t) >> 8) + (((t) & 0xff) << 8))/*lint +e572*/

#if defined(__ppc)
    #ifndef __GNUC__
        #define __asm__     asm
    #endif

    #if !defined(_lint)
        #define EIEIO()     __asm__(" eieio")
    #else
        #define EIEIO()
    #endif

    static hcf_8 IN_PORT_BYTE(int port) {
        hcf_8 value = *(volatile hcf_8 *)(port); EIEIO();
        return value;
    }

    static hcf_16 IN_PORT_WORD(int port) {
        hcf_16 value = *(volatile hcf_16 *)(port); EIEIO();
        value = SwapBytes(value);
        return value;
    }

    #define OUT_PORT_BYTE(port, value) { *(volatile hcf_8 *)(port) = (value); EIEIO(); }
    #define OUT_PORT_WORD(port, value)      \
            { *(volatile hcf_16 *)(port) = SwapBytes(value); EIEIO(); }
#else
    #define IN_PORT_BYTE(port) (*(volatile hcf_8 *)(port))
    #define IN_PORT_WORD(port) (*(volatile hcf_16 *)(port))
    #define OUT_PORT_BYTE(port, value) (*(volatile hcf_8 *)(port) = (value))
    #define OUT_PORT_WORD(port, value) (*(volatile hcf_16 *)(port) = (value))
#endif

/***************************************************************************/

#define IN_PORT_STRING( port, dest, len)        {                       \
                        unsigned l = (len);                             \
                        hcf_16 t, *d = (volatile hcf_16 *)(dest);       \
                        while (l--) {                                   \
                            t = IN_PORT_WORD(port);                     \
                            *d++ = SwapBytes(t);                        \
                        }                                               \
                                                }

#define OUT_PORT_STRING( port, src, len)        {                       \
                        unsigned l = (len);                             \
                        hcf_16 t, *s = (volatile hcf_16 *)(src);        \
                        while (l--) {                                   \
                            t = *s++;                                   \
                            OUT_PORT_WORD(port, SwapBytes(t));          \
                        }                                               \
                                                }

#if PRODUCT == 9150
    #define HCF_AP
    #define HCF_ASSERT
    #undef MSF_COMPONENT_ID
#endif

#endif	/* Diab or High C 29K */


/************************************************************************************************/
/*****************************************  MPC860 **********************************************/
/************************************************************************************************/
/* known users: RR
 */

#if defined CPU && CPU == PPC860

#define HCF_AP						//AccesPoint characteristics
#define MSF_COMPONENT_VAR       0
#define MSF_COMPONENT_ID        0
#define MSF_COMPONENT_MAJOR_VER 1
#define MSF_COMPONENT_MINOR_VER 0

#define HCF_BIG_ENDIAN
#define HCF_MEM_IO

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#define SwapBytes(t)    /*lint -e572*/(((t) >> 8) + (((t) & 0xff) << 8))/*lint +e572*/

#ifndef __GNUC__
    #define __asm__     asm
#endif

#if !defined(_lint)
    #define EIEIO()     __asm__(" eieio")
#else
    #define EIEIO()
#endif

static hcf_8 IN_PORT_BYTE(int port) {
    hcf_8 value = *(volatile hcf_8 *)(port); EIEIO();
    return value;
}

static hcf_16 IN_PORT_WORD(int port) {
    hcf_16 value = *(volatile hcf_16 *)(port); EIEIO();
    value = SwapBytes(value);
    return value;
    #ifdef __GNUC__
        /* the following serves to avoid the compiler warnings that
         * IN_PORT_BYTE() or IN_PORT_WORD() is not used in some files */
        (void)IN_PORT_BYTE;
        (void)IN_PORT_WORD;
    #endif
}

#define OUT_PORT_BYTE(port, value) { *(volatile hcf_8 *)(port) = (value); EIEIO(); }
#define OUT_PORT_WORD(port, value)      \
        { *(volatile hcf_16 *)(port) = SwapBytes(value); EIEIO(); }

/***************************************************************************/

#define IN_PORT_STRING( port, dest, len)        {                       \
                        unsigned l = (len);                             \
                        hcf_16 t;                                       \
                        volatile hcf_16 *d = (volatile hcf_16 *)(dest); \
                        while (l--) {                                   \
                            t = IN_PORT_WORD(port);                     \
                            *d++ = SwapBytes(t);                        \
                        }                                               \
                                                }

#define OUT_PORT_STRING( port, src, len)        {                       \
                        unsigned l = (len);                             \
                        hcf_16 t;                                       \
                        volatile hcf_16 *s = (volatile hcf_16 *)(src);  \
                        while (l--) {                                   \
                            t = *s++;                                   \
                            OUT_PORT_WORD(port, SwapBytes(t));          \
                        }                                               \
                                                }

#if PRODUCT == 9150
//?    #define HCF_AP
    #define HCF_ASSERT
    #undef MSF_COMPONENT_ID
#endif

#endif	/* PPC860 */

/************************************************************************************************/
/****************************  Microtec Research 80X86 Compiler *********************************/
/************************************************************************************************/

#if 0

#define HCF_STA						/*station characteristics*/

#define MSF_COMPONENT_VAR       0
#define MSF_COMPONENT_ID        0
#define MSF_COMPONENT_MAJOR_VER 1
#define MSF_COMPONENT_MINOR_VER 0

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

extern int far inp( int );
extern void far outp( int, int );
extern int far inpw( int );
extern void far outpw( int, int );

#define IN_PORT_BYTE(port)		((hcf_8)inp( (hcf_io)(port) ))
#define IN_PORT_WORD(port)		((hcf_16)inpw( (hcf_io)(port) ))
#define OUT_PORT_BYTE(port, value)	((void)outp( (hcf_io)(port), value ))
#define OUT_PORT_WORD(port, value)	((void)outpw( (hcf_io)(port), value ))

#define IN_PORT_STRING( port, dest, len)        {                       \
                        unsigned l = (len);                             \
                        hcf_16 *d = (hcf_16 *)(dest);                   \
                        while (l--) *d++ =  IN_PORT_WORD(port);         \
                                                }

#define OUT_PORT_STRING( port, src, len)        {                       \
                        unsigned l = (len);                             \
                        hcf_16 *s = (hcf_16 *)(src);                    \
                        while (l--) OUT_PORT_WORD(port, *s++);          \
                                                }
#endif	/* Microtec 80X86 C Compiler */



/************************************************************************************************/
/******************************  W A V E L A N  E C  ********************************************/
/************************************************************************************************/
/* known users: KM
 */

			   /*********/
#ifdef mc68302 /* LC302 */
			   /*********/

#define	MSF_COMPONENT_ID			COMP_ID_EC
#define HCF_STA						//station characteristics

#include <version.h>

#define	MSF_COMPONENT_VAR			1
#define	MSF_COMPONENT_MAJOR_VER		MAJOR_VERSION
#define	MSF_COMPONENT_MINOR_VER		MINOR_VERSION

#define HCF_BIG_ENDIAN
#define HCF_MEM_IO

typedef unsigned char				hcf_8;
typedef unsigned short				hcf_16;
typedef unsigned long				hcf_32;

#define SwapBytes(t)	/*lint -e572*/(((t) >> 8) + (((t) & 0xff) << 8))/*lint +e572*/

#define PCMCIA_ADDRESS 0xc80000UL

#define IN_PORT_BYTE(port)				 (*(hcf_8 *)(port))
#define IN_PORT_2BYTES(port)			 (*(hcf_16 *)(port))
#if 0
static hcf_16 IN_PORT_WORD(hcf_32 port) // should be hcf_io, not hcf_32
{
  hcf_16 word = IN_PORT_2BYTES(port);
  return SwapBytes(word);
}
#else
static hcf_16 swap_var;
#define IN_PORT_WORD(port) \
  (((swap_var = IN_PORT_2BYTES(port)) >> 8) + (((swap_var) & 0xff) << 8))
#endif
#define OUT_PORT_BYTE(port, value)		 (*(hcf_8 *)(port) = (hcf_8)(value))
#define OUT_PORT_2BYTES(port, value)	 (*(hcf_16 *)(port) = (hcf_16)(value))
#define OUT_PORT_WORD(port, value)		 OUT_PORT_2BYTES(port, SwapBytes(value))

#define IN_PORT_STRING(port, dest, len)	  while ((len)--) {*(hcf_16 *)(dest) = IN_PORT_2BYTES(port); (dest) += 2; }
#define OUT_PORT_STRING(port, src, len)	  while ((len)--) {OUT_PORT_2BYTES((port), *(hcf_16 *)(src)) ; (src)  += 2; }

#endif	/* mc68302 */


/************************************************************************************************/
/******************************** WaveLAN Utilities  ********************************************/
/************************************************************************************************/
/* known users: SK
 */

				   	/**************************/
#ifdef WVLAN_UTIL	/* WaveLAN Utilities      */
			   		/**************************/

typedef UCHAR	hcf_8;
typedef USHORT	hcf_16;
typedef ULONG	hcf_32;

#endif // SJAAK


/*  * * * * * * * * * * * * * * * * * * * * * *  IV  * * * * * * * * * * * * * * * * * * * * * * */

/***************************************Compiler specific ****************************************/

#if !defined EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif //__cplusplus
#endif //EXTERN_C


/************************************************************************************************/
/********** M A C R O S derived of C O M P I L E R   S P E C I F I C   M A C R O S  *************/
/************************************************************************************************/



#if !defined FAR
#define FAR							// default to flat 32-bits code
#endif //!defined FAR

typedef hcf_8  FAR *wci_bufp;			 // segmented 16-bits or flat 32-bits pointer to 8 bits unit
typedef hcf_16 FAR *wci_recordp;		 // segmented 16-bits or flat 32-bits pointer to 16 bits unit

typedef hcf_8  FAR *hcf_8fp;			 // segmented 16-bits or flat 32-bits pointer to 8 bits unit
typedef hcf_16 FAR *hcf_16fp;			 // segmented 16-bits or flat 32-bits pointer to 16 bits unit
typedef hcf_32 FAR *hcf_32fp;			 // segmented 16-bits or flat 32-bits pointer to 8 bits unit


#if ! defined HCF_STA && ! defined HCF_AP
error; define at least one of these terms. Note that most users need to define exactly one!!;
#endif

/*  * * * * * * * * * * * * * * * * * * * * * *  V  * * * * * * * * * * * * * * * * * * * * * * */

#if !defined HCF_PORT_IO && !defined HCF_MEM_IO
#define HCF_PORT_IO			// default to Port I/O
#endif

#if defined HCF_PORT_IO && defined HCF_MEM_IO
error;								// you should define at most 1 of the two IO-types. Default is Port I/O
#endif

#if defined HCF_PORT_IO
typedef hcf_16 hcf_io;
#endif //HCF_PORT_IO

#if defined HCF_MEM_IO
typedef hcf_32 hcf_io;
#endif //HCF_MEM_IO



/* MSF_COMPONENT_ID is used to define the CFG_IDENTITY_STRCT in HCF.C
 * CFG_IDENTITY_STRCT is defined in HCF.C purely based on convenience arguments
 * The HCF can not have the knowledge to determine the ComponentId field of the
 * Identity record (aka as Version Record), therefore the MSF part of the Drivers
 * must supply this value via the System Constant MSF_COMPONENT_ID
 * There is a set of values predefined in MDD.H (format COMP_ID_.....)
 *
 * Note that taking MSF_COMPONENT_ID as a default value for DUI_COMPAT_VAR is
 * purely an implementation convenience, the numerical values of these two
 * quantities have none functional relationship whatsoever.
 * Originally there was only a single DUI-variant with value 1. 
 * Then it was considered advantaguous to have unique DUI-variant per driver and 
 * - as said before - out of convenience these variants where choosen to have 
 * the same numerical value as the component id of the driver.
 * Again later, it was decided that there would be two Miniport 3.1 drivers,
 * the old one continuing to have DUI-varaint and component ID 41, the new 
 * one (the STAP or HomeLAN drivr) having component ID 41 but DUI-variant 2 (since
 * 1 was already used in the past)
 */

#if defined	MSF_COMPONENT_ID
#if !defined DUI_COMPAT_VAR
#define	DUI_COMPAT_VAR				MSF_COMPONENT_ID
#endif //!defined DUI_COMPAT_VAR

#if ! defined DUI_COMPAT_BOT		//;?this way utilities can lower as well raise the bottom
#define	DUI_COMPAT_BOT              6
#endif // DUI_COMPAT_BOT

#if ! defined DUI_COMPAT_TOP		//;?this way utilities can lower as well raise the top
#define	DUI_COMPAT_TOP              6
#endif // DUI_COMPAT_TOP

#endif // MSF_COMPONENT_ID


#if ! defined(HCF_HSI_VAR_0) && ! defined(HCF_HSI_VAR_1) && ! defined(HCF_HSI_VAR_2)
#define HCF_HSI_VAR_1				// WaveLAN
#endif


/************************************************************************************************/
/******  M S F    S U P P O R T    F U N C T I O N S    P R O T O T Y P E S   *******************/
/************************************************************************************************/

EXTERN_C void /*FAR*/ msf_assert ( wci_bufp file_namep, hcf_16 line_number, hcf_16 trace, int qual ); //<HCFL>
//MSF_ASSERT_RTN msf_assert;			//<HCFL>

//******************************************* A L I G N M E N T  **********************************************
#if !defined HCF_ALIGN
#define HCF_ALIGN 1			//default to no alignment
#endif // HCF_ALIGN


#if HCF_ALIGN != 1 && HCF_ALIGN != 2 && HCF_ALIGN != 4 && HCF_ALIGN != 8
	error;
#endif // HCF_ALIGN != 0 && HCF_ALIGN != 2 etc




/*  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *	
 *	The routines ips and ops (short for InPutString and OutPutString) are created to use the
 *	compiler to do the type checking. It turned out that it is too easy to accidentally pass
 *	a word pointer to the the macros IN_PORT_STRING and OUT_PORT_STRING rather than a byte pointer.
 *	The "+2" as some macro implementations use, does not have the intended effect in those cases.
 *	The HCF_STRICT business can be ignored by MSF programmers.
 *	
 *  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#if defined HCF_STRICT
void ips( hcf_io prt, wci_bufp dst, int n);
void ops( hcf_io prt, wci_bufp src, int n);
#endif //HCF_STRICT

#if !defined HCF_MAX_GROUP
#define HCF_MAX_GROUP		16		/* historical determined number of Multicast Addresses
									 * up to Station Supplier 4, Hermes supported a maximum of 16 addresses
									 * to prevent side effects on existing MSFs and to prevent the need
									 * for (messy) adaptation code, the HCF default is 16, making the
									 * HCF default compatible with all station vesions (as far as the
									 * Multicast aspect is concerned. It is suggested that MSFs which
									 * want 32 addresses, limit the Station Actor range to a bottom of
									 * at least 5														*/
#elif HCF_MAX_GROUP > 32
error;								// Hermes supports up to 32 Multicast Addresses
#endif

#if !defined HCF_MAX_CONFIG
#define HCF_MAX_CONFIG		(256 + HCF_MAX_GROUP*MAC_ADDR_SIZE)	/* maximum accumulated size in hcf_16 of LTV
														 		 * records used in hcf_put_config			*/
#endif

#if !defined HCF_MAX_MSG
#define HCF_MAX_MSG			1514	// WaveLAN Pakket Size
#endif

#if !defined HCF_MAX_NOTIFY		
#define HCF_MAX_NOTIFY		6		// maximum size in bytes of "real" data in Notify command
#endif

#if !defined HCF_PROT_TIME
#define HCF_PROT_TIME		256		// number of 10K microseconds protection timer against H/W malfunction
#elif HCF_PROT_TIME < 19 || HCF_PROT_TIME >	256
error;								// below the minimum .5 second required by Hermes or above the hcf_32 capacity
#endif


/*	I/O Address size
 *	Platforms which use port mapped I/O will (in general) have a 64k I/O space, conveniently expressed in
 *	a 16-bits quantity
 *	Platforms which use memory mapped I/O will (in general) have an I/O space much larger than 64k,
 *	and need a 32-bits quantity to express the I/O base
 *	To accomodate this the macros HCF_PORT_IO and HCF_MEM_IO are available. Exactly 1 of these must be
 *	defined. If HCF_PORT_IO is defined, the HCF will use an hcf_16 to express I/O base and store in the
 *	IFB. If HCF_MEM_IO, an hcf_32 is used for this purpose. The default is HCF_PORT_IO
 */
#if !defined HCF_PORT_IO && !defined HCF_MEM_IO
#define HCF_PORT_IO					//default to port I/O
#endif


/*	Endianess
 *	Default: HCF_LITTLE_ENDIAN
 *	Little Endian (a.k.a. Intel), least significant byte first
 *	Big Endian (a.k.a. Motorola), most significant byte first
 *
 * The following macros are supplied
 *  o CNV_LITTLE_TO_INT(w)			interprets the 16-bits input value as Little Endian, returns an hcf_16
 * 	o CNV_BIG_TO_INT(w)				interprets the 16-bits input value as Big Endian, returns an hcf_16
 * 	o CNV_INT_TO_BIG_NP(addr)		converts in place the 16-bit value addressed by a near pointer from hcf_16
 * 									to Big Endian
 * 	o CNV_LITTLE_TO_INT_NP(addr)	converts in place the 16-bit value addressed by a near pointer from
 *									Little endian to hcf_16
 *
 * At a number of places in the HCF code, the CNV_INT_TO_BIG_NP macro is used. While it does have the desired
 * effect on all platforms, it's naming is misleading, so revisit all places where these CNV macros are used
 * to assure the right name is used at the right place.
 * Hint: introduce CNV_HOST_TO_NETWORK names if appropriate
 *
 */
#if !defined HCF_LITTLE_ENDIAN && !defined HCF_BIG_ENDIAN
#define HCF_LITTLE_ENDIAN			// default to Little Endian
#endif

#if defined HCF_LITTLE_ENDIAN && defined HCF_BIG_ENDIAN
error;								// you should define at most 1 of the two Endian-types. Default is Little Endian
#endif

/* To increase portability, use unsigned char and unsigned char * when accessing parts of larger
 * types to convert their Endianess
 */

#if defined HCF_BIG_ENDIAN
//******************************************** B I G   E N D I A N *******************************************
#define CNV_LITTLE_TO_INT(w)    ( ((hcf_16)(w) & 0x00ff) << 8 | ((hcf_16)(w) & 0xff00) >> 8 )
#define CNV_BIG_TO_INT(w)		(w)		// No endianess conversion needed

#define CNV_INT_TO_BIG_NP(addr)
#define CNV_LITTLE_TO_INT_NP(addr) {							\
	hcf_8 temp;													\
	temp = ((hcf_8 FAR *)(addr))[0];							\
	((hcf_8 FAR *)(addr))[0] = ((hcf_8 FAR *)(addr))[1];		\
	((hcf_8 FAR *)(addr))[1] = temp;							\
}

#endif // HCF_BIG_ENDIAN

#if defined HCF_LITTLE_ENDIAN
//****************************************** L I T T L E   E N D I A N ****************************************
#define CNV_LITTLE_TO_INT(w) 	(w)		// No endianess conversion needed
#define CNV_BIG_TO_INT(w)       ( ((hcf_16)(w) & 0x00ff) << 8 | ((hcf_16)(w) & 0xff00) >> 8 )

#if ! defined __BEOS__
#define CNV_INT_TO_BIG_NP(addr) {							\
hcf_8 temp;												\
temp = ((hcf_8 FAR *)(addr))[0];						\
	((hcf_8 FAR *)(addr))[0] = ((hcf_8 FAR *)(addr))[1];	\
	((hcf_8 FAR *)(addr))[1] = temp;						\
}
#endif // __BEOS__
#define CNV_LITTLE_TO_INT_NP(addr)

#endif // HCF_LITTLE_ENDIAN

// conversion macros which can be expressed in other macros
#define CNV_INT_TO_LITTLE(w)	CNV_LITTLE_TO_INT(w)
#define CNV_INT_TO_BIG(w)		CNV_BIG_TO_INT(w)



/*************************************************************************************************************/
/******************************************** . . . . . . . . .  *********************************************/
/*************************************************************************************************************/


/* The BASED customization macro is used to resolves the SS!=DS conflict for the Interrupt Service
 * logic in DOS Drivers. Due to the cumbersomeness of mixing C and assembler local BASED variables
 * still end up in the wrong segment. The workaround is that the HCF uses only global BASED
 * variables or IFB-based variables
 * The "BASED" construction (supposedly) only amounts to something in the small memory model.
 *
 * Note that the whole BASED riggamarole is needlessly complicated because both the Microsoft Compiler and
 * Linker are unnecessary restrictive in what far pointer manipulation they allow
 */

#if !defined BASED
#define BASED
#endif

#if !defined NULL
#define NULL ((void *) 0)
#endif


#endif //HCFCFG_H
