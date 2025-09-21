/*
*  +-------------------------------------------------------------------+
*  | Copyright (c) 1995-1999 by Philips Semiconductors.                |
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
*
*
*  Module name              : mmio.h    1.35
*
*  Last update              : 15:52:48 - 99/06/17
*
*
*  Description              :
*
*      Master TM-1 MMIO location definition & access file.
*
*        this include file is THE master reference for names and
*        addresses of TM-1 MMIO locations. The file also provides
*        an access macro for MMIO locations. This is the recommended
*        C program way to access any MMIO locations on TM-1. C-code using
*        this access mechanism will be forward portable.
*
*                USAGE OF THE MACROS
*                ===================
*
*      ---------------- example code begin -------------------
*      #include <tm1/mmio.h>
*      ...
*      unsigned value_of_vi_status, value_of_vi_control;
*      ...
*
*      value_of_vi_status = MMIO(VI_STATUS);
*      MMIO(VI_CTL    )   = value_of_vi_control;
*
*      ---------------- example code end -------------------
*      In the above code, VI_STATUS and VI_CTL     are #define's
*      defined in this file. These are the offsets of the corresponding
*      registers with respect to MMIO_base. The #define MMIO is also
*      defined in this file and does the necessary computations to add 
*      the MMIO_base value and the given offset (VI_STATUS and VI_CTL    
*      in the example).
*/

#ifndef __mmio_h__
#define __mmio_h__

#ifndef __TMAS__

#ifdef __TCS__
#include <tmlib/tmtypes.h>
#else
#include "tmtypes.h"
#endif

extern volatile UInt32 *_MMIO_base;
extern volatile UInt32 *_MMIO_bases[];

#endif

/*
* Three mmio access macros:
*   -     MMIO       accessing the 'current' mmio space (on TM1: the
*                    one of the current processor)
*                    (base stored in global variable _MMIO_base)
*   -     MMIO_M     accessing the mmio space of the specified processor,
*                    via mmio base array _MMIO_bases. Use of proper processor
*                    number is not checked at this (low) mmio level.
*   -     MMIO_B     accessing the specified mmio space directly
*
*
*  IMPORTANT NOTE: 
*
*       In case the last two macros (MMIO_M and MMIO_B) access 
*       *another* TM1 processor (e.g. in case node != the current node, resp
*       base != mmio base of current node), an endian swap of the written or read data
*       might be needed. Namely in case the current endian (PCSW.BSX) is different from
*       the endianness of the accessed MMIO space (the SE bit in its BIU_CTL)).
*
*       The problem now is that this BIU_CTL cannot be interpreted without knowing
*       its endianness on beforehand. However, one can generally assume that all the 
*       BUI_CTL.SE's in a multiprocessor cluster are set identically, so that the
*       'own' BUI_CTL.SE can be investigated for this.
*/

#define MMIO(offset)           ( ((volatile unsigned int *)_MMIO_base        )[(offset) >> 2])    
#define MMIO_M(node,offset)    ( ((volatile unsigned int *)_MMIO_bases[node] )[(offset) >> 2])
#define MMIO_B(base,offset)    ( ((volatile unsigned int *)base              )[(offset) >> 2])


/***********************************************************************/
/*
The #define's in this file are in grouped into sections.
The sections are made (roughly) according to the appearances
of the definitions in TM-1 Preliminary Data Book of
September 1995.

*/
/*
*                 Section I
*                 =========
*         General register definitions from 
*         "TM-1 Architecture"  TM-1 Data Book
*         
*/

#define DRAM_BASE               (0x100000)
#define DRAM_LIMIT              (0x100004)
#define MMIO_BASE               (0x100400)
#define EXCVEC                  (0x100800)
#define ISETTING0               (0x100810)
#define ISETTING1               (0x100814)
#define ISETTING2               (0x100818)
#define ISETTING3               (0x10081c)
#define IPENDING                (0x100820)
#define ICLEAR                  (0x100824)
#define IMASK                   (0x100828)
#define INTVEC0                 (0x100880)
#define INTVEC1                 (0x100884)
#define INTVEC2                 (0x100888)
#define INTVEC3                 (0x10088c)
#define INTVEC4                 (0x100890)
#define INTVEC5                 (0x100894)
#define INTVEC6                 (0x100898)
#define INTVEC7                 (0x10089c)
#define INTVEC8                 (0x1008a0)
#define INTVEC9                 (0x1008a4)
#define INTVEC10                (0x1008a8)
#define INTVEC11                (0x1008ac)
#define INTVEC12                (0x1008b0)
#define INTVEC13                (0x1008b4)
#define INTVEC14                (0x1008b8)
#define INTVEC15                (0x1008bc)
#define INTVEC16                (0x1008c0)
#define INTVEC17                (0x1008c4)
#define INTVEC18                (0x1008c8)
#define INTVEC19                (0x1008cc)
#define INTVEC20                (0x1008d0)
#define INTVEC21                (0x1008d4)
#define INTVEC22                (0x1008d8)
#define INTVEC23                (0x1008dc)
#define INTVEC24                (0x1008e0)
#define INTVEC25                (0x1008e4)
#define INTVEC26                (0x1008e8)
#define INTVEC27                (0x1008ec)
#define INTVEC28                (0x1008f0)
#define INTVEC29                (0x1008f4)
#define INTVEC30                (0x1008f8)
#define INTVEC31                (0x1008fc)
#define TIMER1                  (0x100C00)
#define TIMER1_TMODULUS         (0x100C00)
#define TIMER1_TVALUE           (0x100C04)
#define TIMER1_TCTL             (0x100C08)
#define TIMER2                  (0x100C20)
#define TIMER2_TMODULUS         (0x100C20)
#define TIMER2_TVALUE           (0x100C24)
#define TIMER2_TCTL             (0x100C28)
#define TIMER3                  (0x100C40)
#define TIMER3_TMODULUS         (0x100C40)
#define TIMER3_TVALUE           (0x100C44)
#define TIMER3_TCTL             (0x100C48)
#define SYSTIMER                (0x100C60)
#define SYSTIMER_TMODULUS       (0x100C60)
#define SYSTIMER_TVALUE         (0x100C64)
#define SYSTIMER_TCTL           (0x100C68)
#define BICTL                   (0x101000)
#define BINSTLOW                (0x101004)
#define BINSTHIGH               (0x101008)
#define BDCTL                   (0x101020)
#define BDATAALOW               (0x101030)
#define BDATAAHIGH              (0x101034)
#define BDATAVAL                (0x101038)
#define BDATAMASK               (0x10103c)

/*
*                   Section II
*                   ==========
*         MMIO register definitions for icache and dcache.
* 
*/

/*  See Section I  above for DRAM_BASE, DRAM_LIMIT and MMIO_BASE */

#define DRAM_CACHEABLE_LIMIT          (0x100008)
#define MEM_EVENTS                    (0x10000c)
#define DC_LOCK_CTL                   (0x100010)
#define DC_LOCK_ADDR                  (0x100014)
#define DC_LOCK_SIZE                  (0x100018)
#define DC_PARAMS                     (0x10001c)
#define IC_PARAMS                     (0x100020)
#define MM_CONFIG                     (0x100100)

#define ARB_BW_CTL                    (0x100104)
#define ARB_RAISE                     (0x10010c)

#define POWER_DOWN                    (0x100108)
#define IC_LOCK_CTL                   (0x100210)
#define IC_LOCK_ADDR                  (0x100214)
#define IC_LOCK_SIZE                  (0x100218)
#define PLL_RATIOS                    (0x100300)


/*
*                   Section III
*                   ==========
*        MMIO register definitions for Video In. 
*/

#define VI_STATUS         (0x101400)
#define VI_CTL            (0x101404)
#define VI_CLOCK          (0x101408)
#define VI_CAP_START      (0x10140c)
#define VI_CAP_SIZE       (0x101410)
#define VI_BASE1          (0x101414)
#define VI_Y_BASE_ADR     (0x101414)
#define VI_BASE2          (0x101418)
#define VI_U_BASE_ADR     (0x101418)
#define VI_SIZE           (0x10141c)
#define VI_V_BASE_ADR     (0x10141c)
#define VI_UV_DELTA       (0x101420)
#define VI_Y_DELTA        (0x101424)

#define VI1_STATUS         (0x101400)
#define VI1_CTL            (0x101404)
#define VI1_CLOCK          (0x101408)
#define VI1_CAP_START      (0x10140c)
#define VI1_CAP_SIZE       (0x101410)
#define VI1_BASE1          (0x101414)
#define VI1_Y_BASE_ADR     (0x101414)
#define VI1_BASE2          (0x101418)
#define VI1_U_BASE_ADR     (0x101418)
#define VI1_SIZE           (0x10141c)
#define VI1_V_BASE_ADR     (0x10141c)
#define VI1_UV_DELTA       (0x101420)
#define VI1_Y_DELTA        (0x101424)

#define VI2_STATUS         (0x101600)
#define VI2_CTL            (0x101604)
#define VI2_CLOCK          (0x101608)
#define VI2_CAP_START      (0x10160c)
#define VI2_CAP_SIZE       (0x101610)
#define VI2_BASE1          (0x101614)
#define VI2_Y_BASE_ADR     (0x101614)
#define VI2_BASE2          (0x101618)
#define VI2_U_BASE_ADR     (0x101618)
#define VI2_SIZE           (0x10161c)
#define VI2_V_BASE_ADR     (0x10161c)
#define VI2_UV_DELTA       (0x101620)
#define VI2_Y_DELTA        (0x101624)



/*
*                   Section IV.
*                   ==========
*       MMIO register definitions for Video Out.
*/

#define VO_STATUS          (0x101800)
#define VO_CTL             (0x101804)
#define VO_CLOCK           (0x101808)
#define VO_FRAME           (0x10180c)
#define VO_FIELD           (0x101810)
#define VO_LINE            (0x101814)
#define VO_IMAGE           (0x101818)
#define VO_YTHR            (0x10181c)
#define VO_OLSTART         (0x101820)
#define VO_OLHW            (0x101824)
#define VO_YADD            (0x101828)
#define VO_UADD            (0x10182c)
#define VO_VADD            (0x101830)
#define VO_OLADD           (0x101834)
#define VO_VUF             (0x101838)
#define VO_YOLF            (0x10183c)

/*
*                   Section V
*                   =========
*       MMIO register definitions for Audio In.
*       
*       
*/

#define AI_STATUS         (0x101c00)
#define AI_CTL            (0x101c04)
#define AI_SERIAL         (0x101c08)
#define AI_FRAMING        (0x101c0c)
#define AI_FREQ           (0x101c10)
#define AI_BASE1          (0x101c14)
#define AI_BASE2          (0x101c18)
#define AI_SIZE           (0x101c1c)

#define AI1_STATUS         (0x101c00)
#define AI1_CTL            (0x101c04)
#define AI1_SERIAL         (0x101c08)
#define AI1_FRAMING        (0x101c0c)
#define AI1_FREQ           (0x101c10)
#define AI1_BASE1          (0x101c14)
#define AI1_BASE2          (0x101c18)
#define AI1_SIZE           (0x101c1c)

#define AI2_STATUS         (0x101e00)
#define AI2_CTL            (0x101e04)
#define AI2_SERIAL         (0x101e08)
#define AI2_FRAMING        (0x101e0c)
#define AI2_FREQ           (0x101e10)
#define AI2_BASE1          (0x101e14)
#define AI2_BASE2          (0x101e18)
#define AI2_SIZE           (0x101e1c)

/*
*                   Section VI
*                   ==========
*       MMIO register definitions for Audio Out.
*       
*/

#define AO_STATUS         (0x102000)
#define AO_CTL            (0x102004)
#define AO_SERIAL         (0x102008)
#define AO_FRAMING        (0x10200c)
#define AO_FREQ           (0x102010)
#define AO_BASE1          (0x102014)
#define AO_BASE2          (0x102018)
#define AO_SIZE           (0x10201c)
#define AO_CC             (0x102020)
#define AO_CFC            (0x102024)

#define AO1_STATUS        (0x102000)
#define AO1_CTL           (0x102004)
#define AO1_SERIAL        (0x102008)
#define AO1_FRAMING       (0x10200c)
#define AO1_FREQ          (0x102010)
#define AO1_BASE1         (0x102014)
#define AO1_BASE2         (0x102018)
#define AO1_SIZE          (0x10201c)
#define AO1_CC            (0x102020)
#define AO1_CFC           (0x102024)
#define AO1_TIMESTAMP     (0x102028)

#define AO2_STATUS        (0x102200)
#define AO2_CTL           (0x102204)
#define AO2_SERIAL        (0x102208)
#define AO2_FRAMING       (0x10220c)
#define AO2_FREQ          (0x102210)
#define AO2_BASE1         (0x102214)
#define AO2_BASE2         (0x102218)
#define AO2_SIZE          (0x10221c)
#define AO2_CC            (0x102220)
#define AO2_CFC           (0x102224)
#define AO2_TIMESTAMP     (0x102228)

/*
*  SPDIF Out
*  Figure 28-4 Databook TM2 March 1998
*  Is also supported on TM1300
*
*/ 

#define SPDO_STATUS          (0x104c00)
#define SPDO_CTL             (0x104c04)
#define SPDO_FREQ            (0x104c08)
#define SPDO_BASE1           (0x104c0c)
#define SPDO_BASE2           (0x104c10)
#define SPDO_SIZE            (0x104c14)
#define SPDO_TSTAMP          (0x104c18)

/*
*                   Section VII
*                   ===========
*         PCI related definitions. 
*/

#define  BIU_STATUS   (0x103004)
#define  BIU_CTL      (0x103008)
#define  PCI_ADR      (0x10300C)
#define  PCI_DATA     (0x103010)
#define  CONFIG_ADR   (0x103014)
#define  CONFIG_DATA  (0x103018)
#define  CONFIG_CTL   (0x10301C)
#define  IO_ADR       (0x103020)
#define  IO_DATA      (0x103024)
#define  IO_CTL       (0x103028)
#define  SRC_ADR      (0x10302C)
#define  DEST_ADR     (0x103030)
#define  DMA_CTL      (0x103034)
#define  INT_CTL      (0x103038)


/*
*
*                   Section VIII
*                   ============
*
*         The SEM device. 
*/

#define  SEM          (0x100500)

/*
*
*                 Section IX
*                 ==========
*
*               JTAG registers
*/


#define  JTAG_DATA_IN      (0x103800)
#define  JTAG_DATA_OUT     (0x103804)
#define  JTAG_CTL          (0x103808)

/*
*
*                   Section X
*                   =========
*
*                ICP registers
*/

#define  ICP_MPC            (0x102400)
#define  ICP_MIR            (0x102404)
#define  ICP_DP             (0x102408)
#define  ICP_DR             (0x102410)
#define  ICP_SR             (0x102414)

/*
*
*                       Section X
*                       =========
*
*                     VLD registers
*
*/

/* Look at bottom for complete mpeg pipe of tm2  */

#define VLD_COMMAND           (0x102800)
#define VLD_SR                (0x102804)
#define VLD_QS                (0x102808)
#define VLD_PI                (0x10280c)
#define VLD_STATUS            (0x102810)
#define VLD_IMASK             (0x102814)
#define VLD_CTL               (0x102818)
#define VLD_BIT_ADR           (0x10281c)
#define VLD_BIT_CNT           (0x102820)
#define VLD_MBH_ADR           (0x102824)
#define VLD_MBH_CNT           (0x102828)
#define VLD_RL_ADR            (0x10282c)
#define VLD_RL_CNT            (0x102830)

/*
*
*                           Section XI
*                           ==========
*
*                       I2C interface registers
*
*/

#define IIC_AR                 (0x103400)
#define IIC_DR                 (0x103404)
#define IIC_STATUS             (0x103408)
#define IIC_CTL                (0x10340c)

/*
*
*                            Section XII
*                            ===========
*
*                          SSI registers
*
*/

#define SSI_CTL                  (0x102c00)
#define SSI_CSR                  (0x102c04)
#define SSI_TXDR                 (0x102c10)
#define SSI_RXDR                 (0x102c20)
#define SSI_RXACK                (0x102c24)

/*
*
*                            Section XIII
*                            ============
*
*                          TM1100 registers
*
*/

/*
EVO
*/
#define EVO_CTL     		(0x101840)
#define EVO_MASK    		(0x101844)
#define EVO_CLIP    		(0x101848)
#define EVO_KEY     		(0x10184C)
#define EVO_SLVDLY  		(0x101850)

/*
XIO
*/
#define XIO_CTL 		    (0x103060)

/*
GPI
*/
/* address range 0x104800 - 0x104BFC */
#define GPI_BASE                (0x104800)

/* Size of GPI 'virtual' MMIO space */
#define GPI_SIZE                (0x400)


/*
* ===========================================
*                          TM2x00 peripherals
* ===========================================
*/


/*
*  Internal Bus Arbiter
*/

#define ARB_RAISE0          (0x100114)
#define ARB_RAISE1          (0x100118)
#define ARB_RAISE2          (0x10011c)
#define ARB_RAISE3          (0x100120)
#define ARB_RAISE4          (0x100124)

/*
*  Powerdown
*/ 

#define BLOCK_POWER_DOWN    (0x103428)

/*
*  I2C Slave
*
*/ 
#define IICS_CTL            (0x103410)
#define IICS_SADR           (0x103414)

/*
*  Transport Stream Input
*
*/ 
#define TP1_STATUS          (0x101480)
#define TP1_CTL             (0x101484)
#define TP1_BASE1           (0x101488)
#define TP1_BASE2           (0x10148c)
#define TP1_SIZE            (0x101490)
#define TP1_PID_TABLE0      (0x101500)
#define TP1_PID_TABLE1      (0x101504)
#define TP1_PID_TABLE2      (0x101508)
#define TP1_PID_TABLE3      (0x10150c)
#define TP1_PID_TABLE4      (0x101510)
#define TP1_PID_TABLE5      (0x101514)
#define TP1_PID_TABLE6      (0x101518)
#define TP1_PID_TABLE7      (0x10151c)
#define TP1_PID_TABLE8      (0x101520)
#define TP1_PID_TABLE9      (0x101524)
#define TP1_PID_TABLE10     (0x101528)
#define TP1_PID_TABLE11     (0x10152c)
#define TP1_PID_TABLE12     (0x101530)
#define TP1_PID_TABLE13     (0x101534)
#define TP1_PID_TABLE14     (0x101538)
#define TP1_PID_TABLE15     (0x10153c)
#define TP1_PID_TABLE16     (0x101540)
#define TP1_PID_TABLE17     (0x101544)
#define TP1_PID_TABLE18     (0x101548)
#define TP1_PID_TABLE19     (0x10154c)
#define TP1_PID_TABLE20     (0x101550)
#define TP1_PID_TABLE21     (0x101554)
#define TP1_PID_TABLE22     (0x101558)
#define TP1_PID_TABLE23     (0x10155c)
#define TP1_PID_TABLE24     (0x101560)
#define TP1_PID_TABLE25     (0x101564)
#define TP1_PID_TABLE26     (0x101568)
#define TP1_PID_TABLE27     (0x10156c)
#define TP1_PID_TABLE28     (0x101570)
#define TP1_PID_TABLE29     (0x101574)
#define TP1_PID_TABLE30     (0x101578)
#define TP1_PID_TABLE31     (0x10157c)
#define TP1_PID_TABLE32     (0x101580)
#define TP1_PID_TABLE33     (0x101584)
#define TP1_PID_TABLE34     (0x101588)
#define TP1_PID_TABLE35     (0x10158c)
#define TP1_PID_TABLE36     (0x101590)
#define TP1_PID_TABLE37     (0x101594)
#define TP1_PID_TABLE38     (0x101598)
#define TP1_PID_TABLE39     (0x10159c)
#define TP1_PID_TABLE40     (0x1015a0)
#define TP1_PID_TABLE41     (0x1015a4)
#define TP1_PID_TABLE42     (0x1015a8)
#define TP1_PID_TABLE43     (0x1015ac)
#define TP1_PID_TABLE44     (0x1015b0)
#define TP1_PID_TABLE45     (0x1015b4)
#define TP1_PID_TABLE46     (0x1015b8)
#define TP1_PID_TABLE47     (0x1015bc)
#define TP1_PID_TABLE48     (0x1015c0)
#define TP1_PID_TABLE49     (0x1015c4)
#define TP1_PID_TABLE50     (0x1015c8)
#define TP1_PID_TABLE51     (0x1015cc)
#define TP1_PID_TABLE52     (0x1015d0)
#define TP1_PID_TABLE53     (0x1015d4)
#define TP1_PID_TABLE54     (0x1015d8)
#define TP1_PID_TABLE55     (0x1015dc)
#define TP1_PID_TABLE56     (0x1015e0)
#define TP1_PID_TABLE57     (0x1015e4)
#define TP1_PID_TABLE58     (0x1015e8)
#define TP1_PID_TABLE59     (0x1015ec)
#define TP1_PID_TABLE60     (0x1015f0)
#define TP1_PID_TABLE61     (0x1015f4)
#define TP1_PID_TABLE62     (0x1015f8)
#define TP1_PID_TABLE63     (0x1015fc)

#define TP2_STATUS          (0x101680)
#define TP2_CTL             (0x101684)
#define TP2_BASE1           (0x101688)
#define TP2_BASE2           (0x10168c)
#define TP2_SIZE            (0x101690)
#define TP2_PID_TABLE0      (0x101700)
#define TP2_PID_TABLE1      (0x101704)
#define TP2_PID_TABLE2      (0x101708)
#define TP2_PID_TABLE3      (0x10170c)
#define TP2_PID_TABLE4      (0x101710)
#define TP2_PID_TABLE5      (0x101714)
#define TP2_PID_TABLE6      (0x101718)
#define TP2_PID_TABLE7      (0x10171c)
#define TP2_PID_TABLE8      (0x101720)
#define TP2_PID_TABLE9      (0x101724)
#define TP2_PID_TABLE10     (0x101728)
#define TP2_PID_TABLE11     (0x10172c)
#define TP2_PID_TABLE12     (0x101730)
#define TP2_PID_TABLE13     (0x101734)
#define TP2_PID_TABLE14     (0x101738)
#define TP2_PID_TABLE15     (0x10173c)
#define TP2_PID_TABLE16     (0x101740)
#define TP2_PID_TABLE17     (0x101744)
#define TP2_PID_TABLE18     (0x101748)
#define TP2_PID_TABLE19     (0x10174c)
#define TP2_PID_TABLE20     (0x101750)
#define TP2_PID_TABLE21     (0x101754)
#define TP2_PID_TABLE22     (0x101758)
#define TP2_PID_TABLE23     (0x10175c)
#define TP2_PID_TABLE24     (0x101760)
#define TP2_PID_TABLE25     (0x101764)
#define TP2_PID_TABLE26     (0x101768)
#define TP2_PID_TABLE27     (0x10176c)
#define TP2_PID_TABLE28     (0x101770)
#define TP2_PID_TABLE29     (0x101774)
#define TP2_PID_TABLE30     (0x101778)
#define TP2_PID_TABLE31     (0x10177c)
#define TP2_PID_TABLE32     (0x101780)
#define TP2_PID_TABLE33     (0x101784)
#define TP2_PID_TABLE34     (0x101788)
#define TP2_PID_TABLE35     (0x10178c)
#define TP2_PID_TABLE36     (0x101790)
#define TP2_PID_TABLE37     (0x101794)
#define TP2_PID_TABLE38     (0x101798)
#define TP2_PID_TABLE39     (0x10179c)
#define TP2_PID_TABLE40     (0x1017a0)
#define TP2_PID_TABLE41     (0x1017a4)
#define TP2_PID_TABLE42     (0x1017a8)
#define TP2_PID_TABLE43     (0x1017ac)
#define TP2_PID_TABLE44     (0x1017b0)
#define TP2_PID_TABLE45     (0x1017b4)
#define TP2_PID_TABLE46     (0x1017b8)
#define TP2_PID_TABLE47     (0x1017bc)
#define TP2_PID_TABLE48     (0x1017c0)
#define TP2_PID_TABLE49     (0x1017c4)
#define TP2_PID_TABLE50     (0x1017c8)
#define TP2_PID_TABLE51     (0x1017cc)
#define TP2_PID_TABLE52     (0x1017d0)
#define TP2_PID_TABLE53     (0x1017d4)
#define TP2_PID_TABLE54     (0x1017d8)
#define TP2_PID_TABLE55     (0x1017dc)
#define TP2_PID_TABLE56     (0x1017e0)
#define TP2_PID_TABLE57     (0x1017e4)
#define TP2_PID_TABLE58     (0x1017e8)
#define TP2_PID_TABLE59     (0x1017ec)
#define TP2_PID_TABLE60     (0x1017f0)
#define TP2_PID_TABLE61     (0x1017f4)
#define TP2_PID_TABLE62     (0x1017f8)
#define TP2_PID_TABLE63     (0x1017fc)

/*
*  General Purpose Software I/O
*
*/ 
#define GPIO_EV1            (0x104000)
#define GPIO_TS1            (0x104004)
#define GPIO_EV2            (0x104008)
#define GPIO_TS2            (0x10400c)
#define GPIO_EV3            (0x104010)
#define GPIO_TS3            (0x104014)
#define GPIO_EV4            (0x104018)
#define GPIO_TS4            (0x10401c)
#define GPIO_EV5            (0x104020)
#define GPIO_TS5            (0x104024)
#define GPIO_EV6            (0x104028)
#define GPIO_TS6            (0x10402c)
#define GPIO_EV7            (0x104030)
#define GPIO_EV8            (0x104038)
#define GPIOA_MODE          (0x104100)
#define GPIOA_OUT           (0x104108)
#define GPIOA_IN            (0x10410c)
#define GPIOB_MODE          (0x104110)
#define GPIOB_OUT           (0x104118)
#define GPIOB_IN            (0x10411c)
#define GPIOC_MODE          (0x104120)
#define GPIOC_MOD2          (0x104124)
#define GPIOC_MOD3          (0x104128)
#define GPIOC_OUT           (0x10412c)
#define GPIOC_OUT2          (0x104130)
#define GPIOC_IN            (0x104134)
#define GPIOC_IN2           (0x104138)
#define GPIOD_MODE          (0x104140)
#define GPIOD_OUT           (0x104148)
#define GPIOD_IN            (0x10414c)
#define GPIOE_MODE          (0x104150)
#define GPIOE_OUT           (0x104158)
#define GPIOE_IN            (0x10415c)
#define GPIOF_MODE          (0x104160)
#define GPIOF_OUT           (0x104168)
#define GPIOF_IN            (0x10416c)
#define GPIOG_MODE          (0x104170)
#define GPIOG_OUT           (0x104178)
#define GPIOG_IN            (0x10417c)
#define GPIOH_MODE          (0x104180)
#define GPIOH_OUT           (0x104188)
#define GPIOH_IN            (0x10418c)
#define GPIOJ_MODE          (0x104190)
#define GPIOJ_MOD2          (0x104194)
#define GPIOJ_OUT           (0x104198)
#define GPIOJ_IN            (0x10419c)
#define GPIOK_MODE          (0x1041a0)
#define GPIOK_OUT           (0x1041a8)
#define GPIOK_IN            (0x1041ac)

/*
*  MPEG Pipe: Variable Length Decoder
*
*/ 
#define MP_VLD_COMMAND         (0x102800)
#define MP_VLD_SR              (0x102804)
#define MP_VLD_QS              (0x102808)
#define MP_VLD_PI              (0x10280c)
#define MP_VLD_MC_STATUS       (0x102810)
#define MP_VLD_IE              (0x102814)
#define MP_VLD_CTL             (0x102818)
#define MP_VLD_INP_ADR         (0x10281c)
#define MP_VLD_INP_CNT         (0x102820)
#define MP_VLD_MB_WR_ADR       (0x102824)
#define MP_VLD_MB_WR_CNT       (0x102828)
#define MP_VLD_RL_WR_ADR       (0x10282c)
#define MP_VLD_RL_WR_CNT       (0x102830)
#define MP_VLD_BIT_CNT         (0x102834)

/*
*  MPEG Pipe: RL Decode - Iscan - IQ
*
*/ 
#define MP_W_TBL0_W0           (0x102840)
#define MP_W_TBL0_W1           (0x102844)
#define MP_W_TBL0_W2           (0x102848)
#define MP_W_TBL0_W3           (0x10284c)
#define MP_W_TBL0_W4           (0x102850)
#define MP_W_TBL0_W5           (0x102854)
#define MP_W_TBL0_W6           (0x102858)
#define MP_W_TBL0_W7           (0x10285c)
#define MP_W_TBL0_W8           (0x102860)
#define MP_W_TBL0_W9           (0x102864)
#define MP_W_TBL0_W10          (0x102868)
#define MP_W_TBL0_W11          (0x10286c)
#define MP_W_TBL0_W12          (0x102870)
#define MP_W_TBL0_W13          (0x102874)
#define MP_W_TBL0_W14          (0x102878)
#define MP_W_TBL0_W15          (0x10287c)

#define MP_W_TBL1_W0           (0x102880)
#define MP_W_TBL1_W1           (0x102884)
#define MP_W_TBL1_W2           (0x102888)
#define MP_W_TBL1_W3           (0x10288c)
#define MP_W_TBL1_W4           (0x102890)
#define MP_W_TBL1_W5           (0x102894)
#define MP_W_TBL1_W6           (0x102898)
#define MP_W_TBL1_W7           (0x10289c)
#define MP_W_TBL1_W8           (0x1028a0)
#define MP_W_TBL1_W9           (0x1028a4)
#define MP_W_TBL1_W10          (0x1028a8)
#define MP_W_TBL1_W11          (0x1028ac)
#define MP_W_TBL1_W12          (0x1028b0)
#define MP_W_TBL1_W13          (0x1028b4)
#define MP_W_TBL1_W14          (0x1028b8)
#define MP_W_TBL1_W15          (0x1028bc)

#define MP_EXTRA_PIC_INFO      (0x1028c0)
#define MP_RL_STATS            (0x1028c4)
#define MP_IQ_SEL_0            (0x1028c8)
#define MP_IQ_SEL_1            (0x1028cc)

/*
*  MPEG Pipe: Motion Compensation
*
*/ 
#define MP_MC_PICINFO0         (0x102a00)
#define MP_MC_PICINFO1         (0x102a04)
#define MP_MC_PICINFO2         (0x102a08)
#define MP_MC_FREFY0           (0x102a0c)
#define MP_MC_FREFY1           (0x102a10)
#define MP_MC_FREFUV0          (0x102a14)
#define MP_MC_FREFUV1          (0x102a18)
#define MP_MC_BREFY0           (0x102a1c)
#define MP_MC_BREFY1           (0x102a20)
#define MP_MC_BREFUV0          (0x102a24)
#define MP_MC_BREFUV1          (0x102a28)
#define MP_MC_DESTY0           (0x102a2c)
#define MP_MC_DESTY1           (0x102a30)
#define MP_MC_DESTUV0          (0x102a34)
#define MP_MC_DESTUV1          (0x102a38)
#define MP_MC_COMMAND          (0x102a3c)
#define MP_MC_PFCOUNT          (0x102a40)
#define MP_MC_STATUS           (0x102a44)
#define MMI_LINE_SIZE          (0x100110)


/*
* HDVO: High Definition Video Out
*
* Only define the base address. The registers are currently
* defined in tmHDVOmmioaddr.h
*
*/
#define HDVO_BASE              (0x106000)


#endif /* __mmio_h__ */

