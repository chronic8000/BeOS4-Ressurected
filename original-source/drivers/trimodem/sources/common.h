/*========================================================================
   File:    COMMON.H
   Purpose: This file contains the constants commonly used 
            so it is included in all C files

   Author:  Richard Poirier , 1993-04-08
   Modifications:   Alain Clermont , 1999-08-19
                        Remove OS specific header files (vdw.h , 
                        vtoolscp.h , ... )

                    Copyright (C) 1991 by Trisignal communications Inc.
                                     All Rights Reserved
  =========================================================================*/
#ifndef M_COMMON_H
#define M_COMMON_H

/*
 *  TsType.h contains the definitions of all the data types 
 *  used by TS modem code.
 */
#include "TsType.h"

/*
 * Bit-field ordering is compiler dependant. 
 * Normally compilers for little-endian processors order bits in bit field 
 * structures with the least significant bits at the top of the structure 
 * and vice-versa for compilers targeting big-endian processors. 
 * However this is only a convention and there are exceptions. 
 * Therefore, to be flexible we can define the BIT_FIELD_ORDERING switch to 
 * either BIT_FIELD_LOW_TO_HIGH for compilers placing the least significant 
 * bits first in the structure and we can define to BIT_FIELD_HIGH_TO_LOW 
 * otherwise. 
 *
 * See DevelopEnvironDef.h
 */
#define BIT_FIELD_LOW_TO_HIGH     1
#define BIT_FIELD_HIGH_TO_LOW     2
#include "DevelopEnvironDef.h"

/* Define new/delete functions */
#include "TsMemoryUtil.hpp"

#ifndef NULL
#ifdef  __cplusplus
/* With C++ tighter type checking, the type of 0 will
   be determined by context */
#define NULL    0
#else
/* In ISO C, NULL is a void pointer */
#define NULL    ((void *)0)
#endif
#endif

#ifndef TRUE
#define TRUE            1
#endif

#ifndef FALSE
#define FALSE           0
#endif

#ifndef ON
#define ON              (BYTE)1
#endif

#ifndef OFF
#define OFF             (BYTE)0
#endif

#ifndef HIGH
#define HIGH            (BYTE)1
#endif

#ifndef LOW
#define LOW             (BYTE)0
#endif

#define SWITCH_ON          1
#define SWITCH_OFF         0

#define ON_HOOK         (BYTE)1
#define OFF_HOOK        (BYTE)0

#define BREAK           (BYTE)1
#define MAKE            (BYTE)0

#define INACTIVE        (BYTE)0
#define ACTIVE          (BYTE)1

#define NONE            (BYTE)0
#define DISABLED        (BYTE)0
#define ENABLED         (BYTE)1

#undef  DSP56K

#ifndef BIT_0
#define BIT_0           0x00000001                      /* bit position 0                                           */
#define BIT_1           0x00000002                      /* bit position 1                                           */
#define BIT_2           0x00000004                      /* bit position 2                                           */
#define BIT_3           0x00000008                      /* bit position 3                                           */
#define BIT_4           0x00000010                      /* bit position 4                                           */
#define BIT_5           0x00000020                      /* bit position 5                                           */
#define BIT_6           0x00000040                      /* bit position 6                                           */
#define BIT_7           0x00000080                      /* bit position 7                                           */
#define BIT_8           0x00000100                      /* bit position 8                                           */
#define BIT_9           0x00000200                      /* bit position 9                                           */
#define BIT_10          0x00000400                      /* bit position 10                                          */
#define BIT_11          0x00000800                      /* bit position 11                                          */
#define BIT_12          0x00001000                      /* bit position 12                                          */
#define BIT_13          0x00002000                      /* bit position 13                                          */
#define BIT_14          0x00004000                      /* bit position 14                                          */
#define BIT_15          0x00008000                      /* bit position 15                                          */
#define BIT_16          0x00010000                      /* bit position 16                                          */
#define BIT_17          0x00020000                      /* bit position 17                                          */
#define BIT_18          0x00040000                      /* bit position 18                                          */
#define BIT_19          0x00080000                      /* bit position 19                                          */
#define BIT_20          0x00100000                      /* bit position 20                                          */
#define BIT_21          0x00200000                      /* bit position 21                                          */
#define BIT_22          0x00400000                      /* bit position 22                                          */
#define BIT_23          0x00800000                      /* bit position 23                                          */
#define BIT_24          0x01000000                      /* bit position 24                                          */
#define BIT_25          0x02000000                      /* bit position 25                                          */
#define BIT_26          0x04000000                      /* bit position 26                                          */
#define BIT_27          0x08000000                      /* bit position 27                                          */
#define BIT_28          0x10000000                      /* bit position 28                                          */
#define BIT_29          0x20000000                      /* bit position 29                                          */
#define BIT_30          0x40000000                      /* bit position 30                                          */
#define BIT_31          0x80000000                      /* bit position 31                                          */
#endif

#define T20MS           (UINT32)2                       /* timer value (in increment of 10ms  this is 20  ms        */
#define T30MS           (UINT32)3                       /* timer value (in increment of 10ms  this is 30  ms        */
#define T40MS           (UINT32)4                       /* timer value (in increment of 10ms  this is 40  ms        */
#define T50MS           (UINT32)5                       /* timer value (in increment of 10ms  this is 50  ms        */
#define T60MS           (UINT32)6                       /* timer value (in increment of 10ms  this is 60  ms        */
#define T70MS           (UINT32)7                       /* timer value (in increment of 10ms  this is 70  ms        */
#define T75MS           (UINT32)8                       /* timer value (in increment of 10ms  this is 75  ms        */
#define T80MS           (UINT32)8                       /* timer value (in increment of 10ms  this is 80  ms        */
#define T90MS           (UINT32)9                       /* timer value (in increment of 10ms  this is 90  ms        */
#define T100MS          (UINT32)10                      /* timer value (in increment of 10ms  this is 100 ms        */
#define T110MS          (UINT32)11                      /* timer value (in increment of 10ms  this is 110 ms        */
#define T120MS          (UINT32)12                      /* timer value (in increment of 10ms  this is 120 ms        */
#define T130MS          (UINT32)13                      /* timer value (in increment of 10ms  this is 130 ms        */
#define T150MS          (UINT32)15                      /* timer value (in increment of 10ms  this is 150 ms        */
#define T190MS          (UINT32)19                      /* timer value (in increment of 10ms  this is 190 ms        */
#define T200MS          (UINT32)20                      /* timer value (in increment of 10ms  this is 200 ms        */
#define T220MS          (UINT32)22                      /* timer value (in increment of 10ms  this is 200 ms        */
#define T250MS          (UINT32)25                      /* timer value (in increment of 10ms  this is 250 ms        */
#define T300MS          (UINT32)30                      /* timer value (in increment of 10ms  this is 300 ms        */
#define T350MS          (UINT32)35                      /* timer value (in increment of 10ms  this is 350 ms        */
#define T400MS          (UINT32)40                      /* timer value (in increment of 10ms  this is 400 ms        */
#define T500MS          (UINT32)50                      /* timer value (in increment of 10ms  this is 500 ms        */
#define T550MS          (UINT32)55                      /* timer value (in increment of 10ms  this is 550 ms        */
#define T600MS          (UINT32)60                      /* timer value (in increment of 10ms  this is 600 ms        */
#define T640MS          (UINT32)64                      /* timer value (in increment of 10ms  this is 640 ms        */
#define T660MS          (UINT32)66                      /* timer value (in increment of 10ms  this is 660 ms        */
#define T700MS          (UINT32)70                      /* timer value (in increment of 10ms  this is 700 ms        */
#define T750MS          (UINT32)75                      /* timer value (in increment of 10ms  this is 750 ms        */
#define T800MS          (UINT32)80                      /* timer value (in increment of 10ms  this is 800 ms        */
#define T900MS          (UINT32)90                      /* timer value (in increment of 10ms  this is 900 ms        */
#define T1030MS         (UINT32)103                     /* timer value (in increment of 10ms  this is 1.03 seconds  */
#define T1200MS         (UINT32)120                     /* timer value (in increment of 10ms  this is 1.2 seconds   */
#define T1300MS         (UINT32)130                     /* timer value (in increment of 10ms  this is 1.3 seconds   */
#define T1500MS         (UINT32)150                     /* timer value (in increment of 10ms  this is 1.5 seconds   */
#define T1600MS         (UINT32)160                     /* timer value (in increment of 10ms  this is 1.6 seconds   */
#define T1700MS         (UINT32)170                     /* timer value (in increment of 10ms  this is 1.7 seconds   */
#define T1750MS         (UINT32)175                     /* timer value (in increment of 10ms  this is 1.75 seconds  */
#define T1800MS         (UINT32)180                     /* timer value (in increment of 10ms  this is 1.8 seconds   */
#define T2500MS         (UINT32)250                     /* timer value (in increment of 10ms  this is 2.5 seconds   */
#define T3400MS         (UINT32)340                     /* timer value (in increment of 10ms  this is 3.8 seconds   */
#define T3800MS         (UINT32)380                     /* timer value (in increment of 10ms  this is 3.8 seconds   */
#define T4500MS         (UINT32)450                     /* timer value (in increment of 10ms  this is 4.5 seconds   */
#define T1SEC           (UINT32)100                     /* timer value (in increment of 10ms  this is 1 seconds     */
#define T2SEC           (UINT32)200                     /* timer value (in increment of 10ms  this is 2 seconds     */
#define T3SEC           (UINT32)300                     /* timer value (in increment of 10ms  this is 3 seconds     */
#define T4SEC           (UINT32)400                     /* timer value (in increment of 10ms  this is 4 seconds     */
#define T5SEC           (UINT32)500                     /* timer value (in increment of 10ms  this is 5 seconds     */
#define T6SEC           (UINT32)600                     /* timer value (in increment of 10ms  this is 6 seconds     */
#define T7SEC           (UINT32)700                     /* timer value (in increment of 10ms  this is 7 seconds     */
#define T8SEC           (UINT32)800                     /* timer value (in increment of 10ms  this is 8 seconds     */
#define T9SEC           (UINT32)900                     /* timer value (in increment of 10ms  this is 9 seconds     */
#define T10SEC          (UINT32)1000                    /* timer value (in increment of 10ms  this is 10 seconds    */
#define T12SEC          (UINT32)1200                    /* timer value (in increment of 10ms  this is 12 seconds    */
#define T15SEC          (UINT32)1500                    /* timer value (in increment of 10ms  this is 15 seconds    */
#define T20SEC          (UINT32)2000                    /* timer value (in increment of 10ms  this is 20 seconds    */
#define T25SEC          (UINT32)2500                    /* timer value (in increment of 10ms  this is 25 seconds    */
#define T30SEC          (UINT32)3000                    /* timer value (in increment of 10ms  this is 30 seconds    */
#define T35SEC          (UINT32)3500                    /* timer value (in increment of 10ms  this is 35 seconds    */
#define T1MIN           (UINT32)6000                    /* timer value (in increment of 10ms  this is 1 min         */
#define T1_5MIN         (UINT32)9000                    /* timer value (in increment of 10ms  this is 1.5 min       */
#define T2MIN           (UINT32)12000                   /* timer value (in increment of 10ms  this is 2 min         */
#define T3MIN           (UINT32)18000                   /* timer value (in increment of 10ms  this is 3 min         */
#define T7MIN           (UINT32)42000                   /* timer value (in increment of 10ms  this is 7 min         */
#define T32MIN          (UINT32)192000                  /* timer value (in increment of 10ms  this is 32min         */
#define T1HR            (UINT32)360000                  /* timer value (in increment of 10ms  this is 1 hour        */

/********************************\
 * serial interface definitions *
\********************************/

/* configuration type */
#define ASYNC_DATA              0
#define DIRECT_ASYNC_BASIC      1
#define DIRECT_ASYNC_EXTEND     2
#define DIRECT_SYNC_DATA        3
#define HDLC                    4
#define HDLC_NRZI               5
#define ASYNC_CMD               6
#define IDLE                    7
#define HDLC16_DATA             8
#define HDLC32_DATA             9
#define BSC_V25                 10
#define ASYNC_AUTOBAUD          11
#define SYNC_DATA               12 // lsb first onto non-multiplexed bus (to data pump)
#define REMOTE_DIALOG           13
#define DIRECT_SYNC_HALF        14
#define ASYNC_V80               15 
#define SYNC_DATA_MSB           16 // msb first onto multiplexed bus (IDL or GCI)
#define SYNC_DATA_MULTIPLEXED   17 // lsb first onto multiplexed bus (IDL or GCI)
#define LOOPBACK_ASYNC_BASIC    18 // DIRECT_ASYNC_BASIC applied to local digital loopback
#define PIAFS_DATA              19 // PIAFS interface
#define SYNC_DATA_CODEC         21 // Sync data used for codec interface (Fsync, Clk, Rx, Tx)
#define V110_ASYNC_DATA             22
#define V110_DIRECT_ASYNC_BASIC     23
#define V110_DIRECT_ASYNC_EXTEND    24
#define V110_DIRECT_SYNC_DATA       25
#define GSM_TRANSP_FAX          26
#define GSM_TRANSP_DATA         27
#define SYNC_DATA_SOFTMODEM     28 // Sync data used for software modem over ISDN
#define HDLC00_DATA             29

enum FramingMode
{
    FRM_V14,
    FRM_HDLC,
    FRM_TRANSPARENT,
    FRM_V110_ASYNC_DATA,
    FRM_V110_DIRECT_ASYNC_BASIC,
    FRM_V110_DIRECT_ASYNC_EXTEND,
    FRM_V110_DIRECT_SYNC_DATA,
    FRM_ASYNC,
    FRM_PIAFS,
    FRM_PDC_DATA
};

enum CrcLength
{
    CRC_LENGTH_16,
    CRC_LENGTH_32
};


/* these next configurations are here only for help to configure_dce() routine.
 * it is in fact ASYNC_DATA with format FMT8N1_9
 */
#define ODP_ADP             100 /* ASYNC_DATA with format FMT8N1_9 */
#define ASYNC_MNP           101 /* ASYNC_DATA with format FMT8N1_MNP */

/* these next 2 were added by the DTE module for internal use */
#define UNCHANGED           254
#define SCC_RESET           255

/* speed */
/* When modifying this list don't forget to modify the convertion table in v25ter */
#define BPS75           0
#define BPS300          1
#define BPS1200         2
#define BPS2400         3
#define BPS4800         4
#define BPS7200         5
#define BPS9600         6
#define BPS12000        7
#define BPS14400        8
#define BPS16800        9
#define BPS19200        10
#define BPS21600        11
#define BPS24000        12
#define BPS26400        13
#define BPS28000        14
#define BPS28800        15
#define BPS29333        16
#define BPS30667        17
#define BPS31200        18
#define BPS32000        19
#define BPS33333        20
#define BPS33600        21
#define BPS34000        22
#define BPS34667        23
#define BPS36000        24
#define BPS37333        25
#define BPS38000        26
#define BPS38400        27
#define BPS38667        28
#define BPS40000        29
#define BPS41333        30
#define BPS42000        31
#define BPS42667        32
#define BPS44000        33
#define BPS45333        34
#define BPS46000        35
#define BPS46667        36
#define BPS48000        37
#define BPS49333        38
#define BPS50000        39
#define BPS50667        40
#define BPS52000        41
#define BPS53333        42
#define BPS54000        43
#define BPS54667        44
#define BPS56000        45
#define BPS57600        46
#define BPS64000        47
#define BPS72000        48
#define BPS76800        49
#define BPS96000        50
#define BPS115200       51
#define BPS128000       52
#define BPS230400       53
#define BPS460800       54
#define BPSreserved0    55
#define BPS691200       56
#define BPSreserved1    57
#define BPSreserved2    58
#define BPS921600       59

/* format
 *
 * Since there is so many combinations of the data format argument it
 * is described as a bit mapped value.
 *
 *  Bit 5       number of stop bits
 *              0:  1
 *              1:  2
 *  Bits 4, 3   number of data bits
 *              00: 6
 *              01: 7
 *              10: 8
 *              11: 9
 *  Bit 2       parity
 *              0:  disabled
 *              1:  enabled
 *  Bits 1, 0   parity type
 *              00: even
 *              01: odd
 *              10: mark
 *              11: space
 */


                        /*  stop  | data         |  parity| parity/type
                         *  Bit5  | Bit4    Bit3 |  Bit2  | Bit1    Bit0 */

#define FMT7N1  0x08    /*  0       0       1       0       0       0   */
#define FMT7N2  0x28    /*  1       0       1       0       0       0   */
#define FMT7E1  0x0c    /*  0       0       1       1       0       0   */
#define FMT7E2  0x2c    /*  1       0       1       1       0       0   */
#define FMT7O1  0x0d    /*  0       0       1       1       0       1   */
#define FMT7O2  0x2d    /*  1       0       1       1       0       1   */
#define FMT8N1  0x10    /*  0       1       0       0       0       0   */
#define FMT8N2  0x30    /*  1       1       0       0       0       0   */
#define FMT8E1  0x14    /*  0       1       0       1       0       0   */
#define FMT8E2  0x34    /*  1       1       0       1       0       0   */
#define FMT8O1  0x15    /*  0       1       0       1       0       1   */
#define FMT8O2  0x35    /*  1       1       0       1       0       1   */
#define FMT6N1  0x00    /*  0       0       0       0       0       0   */
#define FMT9N1  0x18    /*  0       1       1       0       0       0   */

#define FMT8N1_MNP 0xfe /*  irrelevent */
#define FMT8N1_9 0xff   /*  irrelevent */

/* flow control */
#define FLOW_NONE       0       /* no flow control */
#define RTS_CTS         1       /* bi-directionnal hardware */
#define SOFT_BID        2       /* bi-directionnal software */
#define SOFT_UNI        3       /* unidirectionnal software */
#define SOFT_BID_TRANS  4       /* bi-directionnal software transparent */
#define CTS             5       /* unidirectionnal hardware */
#define UNI_HARD_SOFT   6       /* unidirectionnal hardware and software */
#define BID_HARD_SOFT_TRANS 7   /* bi-directionnal hardware and software transparent */

/* Definitions used with the DCE interface while in HDLC mode (V.80) */
#define KEEP_BAD_FRAME          BIT_0
#define TX_MARK_ON_IDLE         BIT_1
#define TX_FLAG_ON_FRAME_ABORT  BIT_2
#define UNLIMITED_FRAME_LENGTH  BIT_3

/* Definitions used with the DCE interface while in SYNC mode (V.80) */
#define EIGTH_BITS_SYNC_NO_HUNT  0
#define EIGTH_BITS_SYNC_HUNT     1
#define SIXTEEN_BITS_SYNC_HUNT   2

/* break type */
#define EXPEDITED       0
#define NON_EXPEDITED   1                                           

/* bit stuffing for direct mode operation */
#define STUFF_16BITS    0
#define STUFF_80BITS    1

/*==================================================
   Identificator for all Trisignal entities.
  ================================================== */
#define RESERVED_ID             0
#define ISDN_SIGNALING_ENTITY   0x01
#define V110_ENTITY             0x02
#define V120_ENTITY             0x03
/* #define V120_COMPRESSION ??? */
#define PPP1_ENTITY             0x04
#define PPP2_ENTITY             0x05
/* MLPPP ???    */
/* #define X25_ENTITY */
#define X75_ENTITY              0x06
#define PIAFS_ENTITY            0x07

/* #define VOICE_ENTITY */
#define HEADSET_ENTITY          0x10
#define POTS1_ENTITY            0x11
#define POTS2_ENTITY            0x12
#define POTS3_ENTITY            0x13
#define POTS4_ENTITY            0x14
#define POTS5_ENTITY            0x15

#define SOFT_DTMF_ENTITY        0x20
                            
#define MODEM_ENTITY            0x30
#define V42_ENTITY              0x31
#define V42BIS_ENTITY           0x32
#define MNP_ENTITY              0x33
#define MNP10_ENTITY            0x34
#define FAX_ENTITY              0x35

#define DSVD_ENTITY             0x36
#define V80_ENTITY              0x37
#define VV_ENTITY               0x38

#define PHASE_START             0
#define PHASE_STOP              1
#define PHASE_DATA              2
#define PHASE_BREAK             3
#define PHASE_ABORT             4 
#define PHASE_FLAG              5 
#define PHASE_CRC               6 
#define PHASE_SYNC              7 
#define PHASE_HUNT              8
#define PHASE_INCOMPLETE        9
#endif 
