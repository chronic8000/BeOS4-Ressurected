/*
 * $Log:   V:/Flite/archives/FLite/src/diskonc.H_V  $
   
      Rev 1.14   Jan 24 2000 15:02:48   vadimk
   DUB definition is removed

      Rev 1.13   Jan 13 2000 18:04:24   vadimk
   TrueFFS OSAK 4.1

      Rev 1.12   15 Nov 1999 15:18:52   dimitrys
   Add 512 Mbit Toshiba and Samsung flash support
     (big address type - 4 byte address cycle),
   Add 256 Mbit Samsung flash support

      Rev 1.11   11 Oct 1999 13:28:18   dimitrys
   CHIP_PAGE_SIZE for 2 Mb define

      Rev 1.10   10 Oct 1999 11:00:32   dimitrys
   256 Mbit Toshiba Flash support

      Rev 1.9   Jun 14 1999 18:56:48   marinak
   ifdef NFDC2148_H =>ifdef  DISKONC_H

      Rev 1.8   14 Apr 1999 14:16:40   marina
   type definition of NDOC2window was changed to volatile unsigned char FAR0*

      Rev 1.7   25 Feb 1999 11:58:30   marina
   fix include problem

      Rev 1.6   23 Feb 1999 21:12:10   marina
   separate customizable part

     Rev 1.5   21 Feb 1999 13:47:06   Dmitry
   EEprom mode writing bug fixed (alias range <= 256 bytes)
   Floors identification bug fixed

      Rev 1.4   03 Feb 1999 18:19:12   marina
   doc2map edc check bug fix

      Rev 1.3   24 Dec 1998 12:13:14   marina
   Fixed some ANSI C compilation problems
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*              Copyright (C) M-Systems Ltd. 1995-1998                  */
/*									*/
/************************************************************************/
#ifndef DISKONC_H
#define DISKONC_H

#include "flbuffer.h"
#include "flflash.h"

      /*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ.*/
      /*    Feature list            */
      /*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ.*/

/* #define MULTI_ERASE  */   /* use multiple block erase feature */
/* #define VERIFY_ERASE */   /* read after erase and verify      */
/* #define VERIFY_WRITE */   /* read back and verify write       */
/* #define WIN_FROM_SS  */   /* call Socket Services to get window location */


#define BUSY_DELAY    30000
#define START_ADR     0xC8000L
#define STOP_ADR      0xF0000L

#define EXTRA_LEN           8       /* size of extra area */
#define SYNDROM_BYTES       6       /* Number of syndrom bytes: 5 + 1 parity */
#define PAGES_PER_BLOCK     16      /* 16 pages per block on a single chip */
#define CHIP_PAGE_SIZE      0x100   /* Page Size of 2 Mbyte Flash */

     /* miscellaneous limits */

#define MAX_FLASH_DEVICES_MDOC 2
#define MAX_FLASH_DEVICES_DOC  16
#define MAX_FLOORS             4
#define CHIP_ID_DOC            0x20
#define CHIP_ID_MDOC           0x30
#define MDOC_ALIAS_RANGE       0x100
#define ALIAS_RESOLUTION       (MAX_FLASH_DEVICES_DOC + 10)

/* Flash IDs*/
#define KM29N16000_FLASH    0xec64
#define KM29N32000_FLASH    0xece5
#define KM29V64000_FLASH    0xece6
#define KM29V128000_FLASH   0xec73
#define KM29V256000_FLASH   0xec75
#define KM29V512000_FLASH   0xec76
#define NM29N16_FLASH       0x8f64
#define NM29N32_FLASH       0x8fe5
#define NM29N64_FLASH       0x8fe6
#define TC5816_FLASH        0x9864
#define TC5832_FLASH        0x98e5
#define TC5864_FLASH        0x98e6
#define TC58128_FLASH       0x9873
#define TC58256_FLASH       0x9875
#define TC58512_FLASH       0x9876

     /* Flash commands */

#define SERIAL_DATA_INPUT   0x80
#define READ_MODE           0x00
#define READ_MODE_2         0x50
#define RESET_FLASH         0xff
#define SETUP_WRITE         0x10
#define SETUP_ERASE         0x60
#define CONFIRM_ERASE       0xd0
#define READ_STATUS         0x70
#define READ_ID             0x90
#define SUSPEND_ERASE       0xb0
#define REGISTER_READ       0xe0

/* commands for moving flash pointer to areeas A,B or C of page */
typedef enum { AREA_A = READ_MODE, AREA_B = 0x1, AREA_C = READ_MODE_2 } PointerOp;

#define FAIL		0x01   /* error in block erase */

      /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
	³ Definition for writing boot image  ³
	ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

#define SPL_SIZE           0x2000 /* 8 KBytes */
#define MAX_CODE_MODULES   6      /* max number of code modules in boot area (incl. SPL) */

typedef struct tagDocIO {
  volatile unsigned char io[0x800];
} docIO;

      /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
	³ Definition of DOC 2000 memory window  ³
	ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

/*
       DOC 2000 memory window layout :

		 0000 .... 003F    IPL ROM ( part 1 )
		 0040 .... 07FF       (aliased 20H times)
		 0800 .... 083F    IPL ROM ( part 2 )
		 0840 .... 0FFF       (aliased 20H times)
			   1000    Chip Id
			   1001    DOC_Status_reg
			   1002    DOC_Control_reg
			   1003    ASIC_Control_reg
    CDSN window ----->     1004    CDSN_Control_reg
			   1005    CDSN_Device_Selector
			   1006    ECC_Config_reg
			   1007    ECC_Status_reg
		 1008 .... 100C    Test registers [5]
			   100D    CDSN_Slow_IO_reg
		 100E .... 100F    reserved ( 2 bytes )
		 1010 .... 1015    ECC_Syndrom [6]
		 1016 .... 17FF    reserved ( 2027 bytes )
		 1800 .... 1FFF    CDSN_IO (aliased 800H times)
*/

   /*-----------------------------------------
    | Definition of MDOC 2000 memory window  |
    ----------------------------------------*/

/*        MDOC 2000 memory window layout :

		 0000 .... 01FF    IPL SRAM ( part 1 )
		 0200 .... 07FF       (aliased 4 times)
		 0800 .... 0FFF    CDSN_IO (aliased 800H times)
			   1000    Chip Id
			   1001    DOC_Status_reg
			   1002    DOC_Control_reg
			   1003    ASIC_Control_reg
    CDSN window ----->     1004    CDSN_Control_reg
			   1005    CDSN_Device_Selector
			   1006    ECC_Config_reg
		 1007 .... 100C    reserved ( 6 bytes )
			   100D    CDSN_Slow_IO_reg
		 100E .... 100F    reserved ( 2 bytes )
		 1010 .... 1015    ECC_Syndrom [6]
		 1016 .... 101A    reserved ( 5 bytes )
			   101B    Alias_Resolution_reg
			   101C    Config_Input_reg
			   101D    Read_Pipeline_Init_reg
			   101E    Write_Pipeline_Term_reg
			   101F    Last_Data_Read_reg
			   1020    NOP_reg
		 1021 .... 103E    reserved ( 30 )
			   103F    Foundary_Test_reg
		 1040 .... 17FF    reserved ( 1984 bytes (7C0) )
		 1800 .... 19FF    IPL SRAM ( part 1 )
		 1A00 .... 1FFF       (aliased 4 times)
*/

#define NIPLpart1        0x0               /* read       */
#define NIPLpart2        0x800               /* read       */
#define NchipId          0x1000            /* read       */
#define NDOCstatus       0x1001            /* read       */
#define NDOCcontrol      0x1002            /*      write */
#define NASICselect      0x1003            /* read write */
#define Nsignals         0x1004            /* read write */
#define NdeviceSelector  0x1005            /* read write */
#define NECCconfig       0x1006            /*      write */
#define NECCstatus       0x1007            /* read       */
#define NslowIO          0x100d            /* read write */
#define Nsyndrom         0x1010            /* read       */
#define NaliasResolution 0x101B            /* read write MDOC only */
#define NconfigInput     0x101C            /* read write   - || -  */
#define NreadPipeInit    0x101D            /* read         - || -  */
#define NwritePipeTerm   0x101E            /*      write   - || -  */
#define NreadLastData    0x101F            /* read write   - || -  */
#define NNOPreg          0x1020            /* read write   - || -  */

#define NfoudaryTest     0x103F            /*      write */
#define Nio              0x1800            /* read write */

typedef volatile unsigned char FAR0* NDOC2window;
typedef unsigned char Reg8bitType;

     /* bits for writing to DOC2window.DOCcontrol reg */

#define  ASIC_NORMAL_MODE  0x85
#define  ASIC_RESET_MODE   0x84
#define  ASIC_CHECK_RESET  0x00

     /* bits for writing to DOC2window.signals ( CDSN_Control reg ) */

#define  CE        0x01                 /* 1 - Chip Enable          */
#define  CLE       0x02                 /* 1 - Command Latch Enable */
#define  ALE       0x04                 /* 1 - Address Latch Enable */
#define  WP        0x08                 /* 1 - Write-Protect flash  */
#define  FLASH_IO  0x10
#define  ECC_IO    0x20                 /* 1 - turn ECC on          */
#define  PWDO      0x40

     /* bits for reading from DOC2window.signals ( CDSN_Control reg ) */

#define RB         0x80                 /* 1 - ready */

     /* bits for writing to DOC2window.ECCconfig */

#define ECC_RESET               0x00
#define ECC_IGNORE              0x01
#define ECC_RESERVED            0x02    /* reserved bits  */
#define ECC_EN    (0x08 | ECC_RESERVED) /* 1 - enable ECC */
#define ECC_RW    (0x20 | ECC_RESERVED) /* 1 - write mode, 0 - read mode */

     /* bits for reading from DOC2window.ECCstatus */

#define ECC_ERROR 0x80
#define TOGGLE    0x04                  /* used for DOC 2000 detection */

#define MDOC_ASIC   0x08                /* MDOC asic */


typedef struct {
  unsigned short      	vendorID;
  unsigned short      	chipID;
  unsigned short        chipPageSize;
  unsigned short        pageSize;              /* all.................... */
  unsigned short        pageMask;              /* ...these............... */
  unsigned short        pageAreaSize;          /* .......variables....... */
  unsigned short        tailSize;              /* .............interleave */
  unsigned short        noOfBlocks;            /* total erasable blocks in flash device*/
  unsigned short      	pageAndTailSize;
  unsigned short        pagesPerBlock;
  unsigned char         totalFloors;           /*    1 .. MAX_FLOORS      */
  unsigned char         currentFloor;          /*    0 .. totalFloors-1   */
  long                  floorSize;             /*    in bytes             */

  unsigned short        flags;                 /* bitwise: BIG_PAGE, SLOW_IO etc. */
  FLBuffer              *buffer;               /* buffer for map through buffer */
  unsigned              win_io;               /* pointer to DOC CDSN_IO */
  NDOC2window           win;                  /* pointer to DOC 2000 memory window */
} NFDC21Vars;

#define NFDC21thisVars   ((NFDC21Vars *) vol.mtdVars)
#define NFDC21thisWin    (NFDC21thisVars->win)
#define NFDC21thisBuffer (NFDC21thisVars->buffer->data)
#define NFDC21thisIO     (NFDC21thisVars->win_io)



/*----------------------------------------------------------------------*/
/*      	       w i n d o w B a s e A d d r e s s		*/
/*									*/
/* Return the host base address of the window.				*/
/* If the window base address is programmable, this routine selects     */
/* where the base address will be programmed to.			*/
/*									*/
/* Parameters:                                                          */
/*      driveNo							        */
/*      lowAddress                                                      */
/*      highAddress                                                     */
/*                                                                      */
/* Returns:                                                             */
/*	Host physical address of window divided by 4 KB			*/
/*----------------------------------------------------------------------*/

extern unsigned flDocWindowBaseAddress(unsigned driveNo,
                                       unsigned long lowAddress,
                                       unsigned long highAddress);

/*----------------------------------------------------------------------*/
/*      	       w i n d o w B a s e A d d r e s s		*/
/*									*/
/* Checks if a given window is valid DOC window.			*/
/*									*/
/* Parameters:                                                          */
/*      memWinPtr host base address of the window	                */
/*                                                                      */
/* Returns:                                                             */
/*	TRUE if there is DOC FALSE otherwise			        */
/*----------------------------------------------------------------------*/

extern FLBoolean checkWinForDOC(unsigned driveNo, NDOC2window memWinPtr);

#endif /* DISKONC_H */