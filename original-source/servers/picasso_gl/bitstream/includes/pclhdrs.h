/*****************************************************************************
*                                                                            *
*  Copyright 1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
* $Header: //bliss/user/archive/rcs/fit/pclhdrs.h,v 6.0 97/03/17 17:36:40 shawn Release $
*
* $Log:	pclhdrs.h,v $
 * Revision 6.0  97/03/17  17:36:40  shawn
 * TDIS Version 6.00
 * 
 * Revision 4.4  97/03/11  11:27:29  shawn
 * Major modifications:
 *      Added the EMU(x, y, z) macro to control the emulation version.
 *                    x - SPEEDO
 *                    y - LJIII/LJIIIsi/LJ4
 *                    z - LJ5
 * Corrected many invalid/missing values.
 * 
 * Revision 4.3  96/07/01  09:15:45  shawn
 * Added functionality for Cyrillic support (character complements).
 * 
 * Revision 4.2  95/11/06  15:43:05  roberte
 * Corrections to Bitstream font names and IDs in comments.
 * Four corrections to font IDs in data structure for the
 * Courier fonts in the TT14 header definitions.
 * 
 * Revision 4.1  95/03/23  11:43:06  roberte
 * Corrected Typeface ID (MSB, LSB) codes for Albertus, Albertus XB
 * and Wingding entries.
 * 
 * Revision 4.0  95/02/15  16:50:01  shawn
 * Release
 * 
 * Revision 3.3  95/01/10  08:58:26  roberte
 * Updated copyright header.
 * 
*****************************************************************************/


/* Conditionals for PCL Font Character Complement Bits */
/*    based on specified printer emulation    */


#if PCL5
#define PCL_MSW 0x1FFFFFF9
#define PCL_LSW 0xFFFFFFFF
#endif

#if PCL5e
#define PCLe_MSW 0x1FFFFFF9
#define PCLe_LSW 0xFFFFFFFF
#define PCL_MSW  0x1FFFFFF9
#define PCL_LSW  0xFFFFFFFF
#endif

#if PCL6
#define PCLe_MSW 0xFFFFFFFF
#define PCLe_LSW 0x0023FFFE
#define PCL_MSW  0xFFFFFFFF
#define PCL_LSW  0x0023FFFE
#endif

/* Default values */

#ifndef PCLe_MSW
#define PCLe_MSW 0x7FFFFFF9
#endif
#ifndef PCLe_LSW
#define PCLe_LSW 0xFFFFFFFF
#endif
#ifndef PCL_MSW
#define PCL_MSW 0x7FFFFFF9
#endif
#ifndef PCL_LSW
#define PCL_LSW 0xFFFFFFFF
#endif

/* PostScript Font Character Complements */

#define PS_MSW  0x7FFFFFF9
#define PS_LSW  0xFFFFFFFF

/* TrueType Font Character Complements */

#ifdef OLDWAY
#define TT_MSW	0x1FFFFFF9
#elif PCL5 || PCL5e
#define TT_MSW	0xFFFFFFFF
#define TT_LSW	0x003FFFFE
#else   /* PCL6 */
#define TT_MSW  0xFFFFFFFF
#define TT_LSW  0x003FFFFE
#endif

/* The following was added to support Cyrillic character collections */

/*#define CYR_SUPPORT*/
#ifdef CYR_SUPPORT
#define CYR_COMPLEMENT  0xFDFFFFFF
#else
#define CYR_COMPLEMENT  0xFFFFFFFF
#endif

/* The following macro is based on the specified printer emulation. */
/*     EMU(Speedo, PCL5/5e, PCL6) */

/*#define SPEEDO_RFB 1*/

#ifdef SPEEDO_RFB
#define EMU(x, y, z)    x
#elif PCL6
#define EMU(x, y, z)    z
#else
#define EMU(x, y, z)    y
#endif



/***** Individual PCL font headers *****/

/* [font3150.spd] Dutch801 SWC -> CG-Times: */
#define PCL1Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1250, 10978, 2560),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(296, 2602, 607),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(457, 4009, 935),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  122,\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                    /* Text Width: */   EMU(411, 3608, 841),\
                    /* First Code: */   0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(666, 5850, 1364),\
                    /* Font Number: */  3150,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3151.spd] Dutch801 SWC -> CG-Times Italic: */
#define PCL2Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1334, 11715, 2732),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(296, 2602, 607),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(458, 4024, 938),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  122,\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(410, 3600, 840),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(674, 5920, 1381),\
                    /* Font Number: */  3151,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3152.spd] Dutch801 SWC -> CG-Times Bold: */
#define PCL3Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1278, 11223, 2617),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(296, 2602, 607),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(467, 4100, 956),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  122,\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(435, 3820, 891),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 2602, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(687, 6030, 1406),\
                    /* Font Number: */  3152,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3153.spd] Dutch801 SWC -> CG-Times Bold Italic: */
#define PCL4Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1382, 12137, 2830),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(296, 2602, 607),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(475, 4100, 972),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  122,\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(435, 3820, 891),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(682, 6030, 1397),\
                    /* Font Number: */  3153,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3154.spd] Swiss742 SWC -> Univers: */
#define PCL5Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1241, 10898, 2542),\
                    /* Cell Height: */  EMU(1312, 11522, 2687),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(333, 2927, 683),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(481, 4226, 986),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  52,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(470, 4128, 963),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(691, 6066, 1415),\
                    /* Font Number: */  3154,\
                      /* Font Name: */  {'S','w','i','s','s','7','4','2',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3155.spd] Swiss742 SWC -> Univers Italic: */
#define PCL6Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1247, 10951, 2554),\
                    /* Cell Height: */  EMU(1347, 11829, 2759),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(333, 2927, 683),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(481, 4221, 984),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  52,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(470, 4128, 963),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(692, 6077, 1417),\
                    /* Font Number: */  3155,\
                      /* Font Name: */  {'S','w','i','s','s','7','4','2',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  9395,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3156.spd] Swiss742 SWC -> Univers Bold: */
#define PCL7Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1241, 10898, 2542),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(333, 2927, 683),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(478, 4202, 980),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  52,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(470, 4128, 963),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(689, 6050, 1411),\
                    /* Font Number: */  3156,\
                      /* Font Name: */  {'S','w','i','s','s','7','4','2',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3157.spd] Swiss742 SWC -> Univers Bold Italic: */
#define PCL8Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1248, 10960, 2556),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(333, 2927, 683),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(478, 4202, 980),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  52,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(470, 4128, 963),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(686, 6027, 1406),\
                    /* Font Number: */  3157,\
                      /* Font Name: */  {'S','w','i','s','s','7','4','2',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  9395,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3183.spd] Swiss742 Cn SWC -> Univers Condensed: */
#define PCL9Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 1, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1151, 10108, 2357),\
                    /* Cell Height: */  EMU(1298, 11399, 2658),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(222, 1952, 455),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(476, 4181, 975),\
                     /* Width Type: */  -2,\
                    /* Style (LSB): */  EMU(40, 96, 84),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  52,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(341, 2997, 699),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(687, 6029, 1406),\
                    /* Font Number: */  3183,\
                      /* Font Name: */  {'S','w','i','s','s','7','4','2',' ','C','n',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3184.spd] Swiss742 Cn SWC -> Univers Condensed Italic: */
#define PCL10Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 1, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1226, 10767, 2511),\
                    /* Cell Height: */  EMU(1299, 11408, 2660),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(222, 1952, 455),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(478, 4202, 980),\
                     /* Width Type: */  -2,\
                    /* Style (LSB): */  EMU(41, 97, 85),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  52,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(341, 2997, 699),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(688, 6045, 1410),\
                    /* Font Number: */  3184,\
                      /* Font Name: */  {'S','w','i','s','s','7','4','2',' ','C','n',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  9395,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3185.spd] Swiss742 Cn SWC -> Univers Condensed Bold: */
#define PCL11Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 1, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1146, 10064, 2347),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(222, 1952, 455),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(481, 4223, 985),\
                     /* Width Type: */  -2,\
                    /* Style (LSB): */  EMU(40, 96, 84),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  52,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(376, 3303, 770),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(689, 6049, 1411),\
                    /* Font Number: */  3185,\
                      /* Font Name: */  {'S','w','i','s','s','7','4','2',' ','C','n',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3186.spd] Swiss742 Cn SWC -> Univers Condensed Bold Italic: */
#define PCL12Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 1, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1227, 10776, 2513),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(222, 1952, 455),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(478, 4196, 979),\
                     /* Width Type: */  -2,\
                    /* Style (LSB): */  EMU(41, 97, 85),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  52,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(376, 3303, 770),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(689, 6053, 1412),\
                    /* Font Number: */  3186,\
                      /* Font Name: */  {'S','w','i','s','s','7','4','2',' ','C','n',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  9395,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3213.spd] ZapfDingbats SWC -> ITC Zapf Dingbats: */
#define PCL13Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(999, 8773, 2046),\
                    /* Cell Height: */  EMU(1042, 9151, 2134),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(250, 2192, 512),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  45,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3213,\
                      /* Font Name: */  {'Z','a','p','f','D','i','n','g','b','a','t','s',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0xFFFFFFFE,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3163.spd] Souvenir SWC -> ITC Souvenir Light: */
#define PCL14Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1212, 10644, 2482),\
                    /* Cell Height: */  EMU(1341, 11777, 2746),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(278, 2441, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(463, 4066, 948),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  -3,\
                 /* Typeface (LSB): */  16,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(722, 6341, 1479),\
                    /* Font Number: */  3163,\
                      /* Font Name: */  {'S','o','u','v','e','n','i','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3164.spd] Souvenir SWC -> ITC Souvenir Light Italic: */
#define PCL15Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1216, 10679, 2490),\
                    /* Cell Height: */  EMU(1341, 11777, 2746),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(278, 2520, 588),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(475, 4171, 973),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  -3,\
                 /* Typeface (LSB): */  16,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(722, 6341, 1479),\
                    /* Font Number: */  3164,\
                      /* Font Name: */  {'S','o','u','v','e','n','i','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5189,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3165.spd] Souvenir SWC -> ITC Souvenir Demi: */
#define PCL16Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1201, 10547, 2460),\
                    /* Cell Height: */  EMU(1384, 12154, 2834),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(306, 2687, 627),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(463, 4066, 948),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  2,\
                 /* Typeface (LSB): */  16,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(722, 6341, 1479),\
                    /* Font Number: */  3165,\
                      /* Font Name: */  {'S','o','u','v','e','n','i','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3166.spd] Souvenir SWC -> ITC Souvenir Demi Italic: */
#define PCL17Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1226, 10767, 2511),\
                    /* Cell Height: */  EMU(1394, 12242, 2855),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(306, 2687, 627),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(475, 4171, 973),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  2,\
                 /* Typeface (MSB): */  16,\
                 /* Typeface (LSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(722, 6341, 1479),\
                    /* Font Number: */  3166,\
                      /* Font Name: */  {'S','o','u','v','e','n','i','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5189,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3167.spd] ZapfCallig SWC -> CG Palacio: */
#define PCL18Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1432, 12576, 2933),\
                    /* Cell Height: */  EMU(1298, 11399, 2658),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(250, 21955, 512),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(473, 4154, 969),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  15,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(699, 6139, 1432),\
                    /* Font Number: */  3167,\
                      /* Font Name: */  {'Z','a','p','f','C','a','l','l','i','g',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3168.spd] ZapfCallig SWC -> CG Palacio Italic: */
#define PCL19Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1430, 12558, 2929),\
                    /* Cell Height: */  EMU(1318, 11575, 2699),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(250, 2196, 512),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(475, 4171, 973),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  15,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(699, 6139, 1432),\
                    /* Font Number: */  3168,\
                      /* Font Name: */  {'Z','a','p','f','C','a','l','l','i','g',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5189,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3169.spd] ZapfCallig SWC -> CG Palacio Bold: */
#define PCL20Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1431, 12567, 2931),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(250, 2196, 512),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(475, 4171, 973),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  15,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(686, 6024, 1405),\
                    /* Font Number: */  3169,\
                      /* Font Name: */  {'Z','a','p','f','C','a','l','l','i','g',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3170.spd] ZapfCallig SWC -> CG Palacio Bold Italic: */
#define PCL21Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1459, 12813, 2988),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(250, 2196, 512),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(477, 4189, 977),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  15,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(607, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(686, 6024, 1405),\
                    /* Font Number: */  3170,\
                      /* Font Name: */  {'Z','a','p','f','C','a','l','l','i','g',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3179.spd] Incised901 SWC -> Antique Olive: */
#define PCL22Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1407, 12356, 2882),\
                    /* Cell Height: */  EMU(1356, 11908, 2777),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(296, 2602, 607),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(634, 5565, 1298),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  72,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(528, 4638, 1082),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(749, 6577, 1534),\
                    /* Font Number: */  3179,\
                      /* Font Name: */  {'I','n','c','i','s','e','d','9','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3180.spd] Incised901 SWC -> Antique Olive Italic: */
#define PCL23Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1430, 12558, 2929),\
                    /* Cell Height: */  EMU(1358, 11926, 2781),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(296, 2602, 607),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(636, 5582, 1302),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  72,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(525, 4614, 1076),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(749, 6575, 1533),\
                    /* Font Number: */  3180,\
                      /* Font Name: */  {'I','n','c','i','s','e','d','9','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5189,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3181.spd] Incised901 SWC -> Antique Olive Bold: */
#define PCL24Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1425, 12514, 2918),\
                    /* Cell Height: */  EMU(1391, 12216, 2849),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(333, 2927, 683),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(638, 5600, 1306),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  72,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(575, 5049, 1177),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(752, 6600, 1539),\
                    /* Font Number: */  3181,\
                      /* Font Name: */  {'I','n','c','i','s','e','d','9','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3182.spd] Incised901Ct SWC -> Antique Olive Bold Italic: */
#define PCL25Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1561, 13709, 3197),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(370, 3249, 758),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(627, 5506, 1284),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  72,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(743, 6525, 1522),\
                    /* Font Number: */  3182,\
                      /* Font Name: */  {'I','n','c','i','s','e','d','9','0','1','C','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3175.spd] CenturySchbk SWC -> CG Century Schoolbook: */
#define PCL26Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1376, 12084, 2818),\
                    /* Cell Height: */  EMU(1350, 11856, 2765),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(278, 2441, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(463, 4066, 948),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  23,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(722, 6341, 1479),\
                    /* Font Number: */  3175,\
                      /* Font Name: */  {'C','e','n','t','u','r','y','S','c','h','b','k',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3176.spd] CenturySchbk SWC -> CG Century Schoolbook Italic: */
#define PCL27Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1357, 11917, 2779),\
                    /* Cell Height: */  EMU(1352, 11873, 2769),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(278, 2441, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(473, 4154, 969),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  23,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(722, 6341, 1479),\
                    /* Font Number: */  3176,\
                      /* Font Name: */  {'C','e','n','t','u','r','y','S','c','h','b','k',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8169,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3177.spd] CenturySchbk SWC -> CG Century Schoolbook Bold: */
#define PCL28Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1351, 11864, 2767),\
                    /* Cell Height: */  EMU(1386, 12172, 2839),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(278, 2441, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(475, 4171, 973),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  23,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(722, 6341, 1479),\
                    /* Font Number: */  3177,\
                      /* Font Name: */  {'C','e','n','t','u','r','y','S','c','h','b','k',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3178.spd] CenturySchbk SWC -> CG Century Schoolbook Bold Italic: */
#define PCL29Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1385, 12163, 2836),\
                    /* Cell Height: */  EMU(1386, 12172, 2839),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(278, 2441, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(482, 4233, 987),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  23,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(722, 6341, 1479),\
                    /* Font Number: */  3178,\
                      /* Font Name: */  {'C','e','n','t','u','r','y','S','c','h','b','k',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3187.spd] Stymie SWC -> Stymie: */
#define PCL30Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1236, 10855, 2531),\
                    /* Cell Height: */  EMU(1346, 11821, 2757),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(296, 2599, 606),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(454, 3987, 930),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  84,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(720, 6323, 1475),\
                    /* Font Number: */  3187,\
                      /* Font Name: */  {'S','t','y','m','i','e',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3188.spd] Stymie SWC -> Stymie Italic: */
#define PCL31Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1238, 10872, 2535),\
                    /* Cell Height: */  EMU(1346, 11821, 2757),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(296, 2599, 606),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(453, 3978, 928),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  84,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(720, 6323, 1475),\
                    /* Font Number: */  3188,\
                      /* Font Name: */  {'S','t','y','m','i','e',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3189.spd] Stymie SWC -> Stymie Bold: */
#define PCL32Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1316, 11557, 2695),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(259, 2275, 530),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(468, 4268, 995),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  84,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(720, 6323, 1475),\
                    /* Font Number: */  3189,\
                      /* Font Name: */  {'S','t','y','m','i','e',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3190.spd] Stymie SWC -> Stymie Bold Italic: */
#define PCL33Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1374, 12066, 2814),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(259, 2275, 530),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(468, 4268, 995),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  84,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(720, 6323, 1475),\
                    /* Font Number: */  3190,\
                      /* Font Name: */  {'S','t','y','m','i','e',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3191.spd] ZapfHumst Dm SWC -> CG Omega: */
#define PCL34Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1379, 12110, 2824),\
                    /* Cell Height: */  EMU(1302, 11434, 2666),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(278, 2439, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(474, 4161, 970),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  17,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 160, 160),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(430, 3777, 881),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(684, 6005, 1400),\
                    /* Font Number: */  3191,\
                      /* Font Name: */  {'Z','a','p','f','H','u','m','s','t',' ','D','m',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3192.spd] ZapfHumst Dm SWC -> CG Omega Italic: */
#define PCL35Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1381, 12128, 2828),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(278, 2439, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(480, 4213, 982),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  17,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 160, 160),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(433, 3798, 886),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(689, 6050, 1411),\
                    /* Font Number: */  3192,\
                      /* Font Name: */  {'Z','a','p','f','H','u','m','s','t',' ','D','m',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5189,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3193.spd] ZapfHumst Dm SWC -> CG Omega Bold: */
#define PCL36Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1395, 12251, 2857),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(278, 2439, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(481, 4227, 986),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  17,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 160, 160),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(440, 3860, 900),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(688, 6045, 1410),\
                    /* Font Number: */  3193,\
                      /* Font Name: */  {'Z','a','p','f','H','u','m','s','t',' ','D','m',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3194.spd] ZapfHumst Dm SWC -> CG Omega Bold Italic: */
#define PCL37Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1477, 12971, 3024),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(278, 2439, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(479, 4207, 981),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  17,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 160, 160),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(439, 3856, 899),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(685, 6013, 1402),\
                    /* Font Number: */  3194,\
                      /* Font Name: */  {'Z','a','p','f','H','u','m','s','t',' ','D','m',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6369,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3195.spd] Bodoni Bk SWC -> CG Bodini: */
#define PCL38Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1344, 11803, 2753),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(269, 2362, 551),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(382, 3355, 782),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  -1,\
                 /* Typeface (LSB): */  53,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                    /* First Code: */   0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(657, 5770, 1346),\
                    /* Font Number: */  3195,\
                      /* Font Name: */  {'B','o','d','o','n','i',' ','B','k',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3196.spd] Bodoni Bk SWC -> CG Bodini Italic: */
#define PCL39Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1281, 11250, 2624),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(259, 2275, 530),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(388, 3407, 795),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  -1,\
                 /* Typeface (LSB): */  53,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(653, 5735, 1337),\
                    /* Font Number: */  3196,\
                      /* Font Name: */  {'B','o','d','o','n','i',' ','B','k',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3197.spd] Bodoni Bk SWC -> CG Bodini Bold: */
#define PCL40Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1449, 12725, 2968),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(306, 2687, 627),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(390, 3425, 799),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  53,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(649, 5700, 1329),\
                    /* Font Number: */  3197,\
                      /* Font Name: */  {'B','o','d','o','n','i',' ','B','k',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3198.spd] Bodoni Bk SWC -> CG Bodini Bold Italic: */
#define PCL41Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1427, 12532, 2922),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(296, 2599, 606),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(400, 3513, 819),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  53,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(653, 5735, 1337),\
                    /* Font Number: */  3198,\
                      /* Font Name: */  {'B','o','d','o','n','i',' ','B','k',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3199.spd] Benguiat SWC -> ITC Benguiat: */
#define PCL42Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1314, 11540, 2691),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(315, 2766, 645),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(500, 4391, 1024),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  62,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(667, 5858, 1366),\
                    /* Font Number: */  3199,\
                      /* Font Name: */  {'B','e','n','g','u','i','a','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3200.spd] Benguiat SWC -> ITC Benguiat Italic: */
#define PCL43Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1347, 11829, 2759),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(315, 2766, 645),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(514, 4514, 1053),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  62,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(667, 5858, 1366),\
                    /* Font Number: */  3200,\
                      /* Font Name: */  {'B','e','n','g','u','i','a','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3201.spd] Benguiat SWC -> ITC Benguiat Bold: */
#define PCL44Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1330, 11680, 272),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(360, 3162, 737),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(500, 4391, 1024),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  62,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(667, 5858, 1366),\
                    /* Font Number: */  3201,\
                      /* Font Name: */  {'B','e','n','g','u','i','a','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3202.spd] Benguiat SWC -> ITC Benguiat Bold Italic: */
#define PCL45Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1340, 11769, 2744),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(360, 3162, 737),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(515, 4523, 1055),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  62,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(667, 5858, 1366),\
                    /* Font Number: */  3202,\
                      /* Font Name: */  {'B','e','n','g','u','i','a','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3171.spd] Bookman SWC -> ITC Bookman Light: */
#define PCL46Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1335, 11724, 2734),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(315, 2766, 645),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(494, 4338, 1012),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  -3,\
                 /* Typeface (LSB): */  47,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(688, 6042, 1409),\
                    /* Font Number: */  3171,\
                      /* Font Name: */  {'B','o','o','k','m','a','n',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3172.spd] Bookman SWC -> ITC Bookman Light Italic: */
#define PCL47Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1401, 12304, 2869),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(315, 2766, 645),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(498, 4373, 1020),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  -3,\
                 /* Typeface (LSB): */  47,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(688, 6042, 1409),\
                    /* Font Number: */  3172,\
                      /* Font Name: */  {'B','o','o','k','m','a','n',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5777,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3173.spd] Bookman SWC -> ITC Bookman Demi: */
#define PCL48Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1354, 11891, 2773),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(360, 3162, 737),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(504, 4426, 1032),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  2,\
                 /* Typeface (LSB): */  47,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(688, 6042, 1409),\
                    /* Font Number: */  3173,\
                      /* Font Name: */  {'B','o','o','k','m','a','n',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3174.spd] Bookman SWC -> ITC Bookman Demi Italic: */
#define PCL49Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1411, 12391, 2890),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(343, 3012, 702),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(511, 4488, 1047),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  2,\
                 /* Typeface (LSB): */  47,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(688, 6042, 1409),\
                    /* Font Number: */  3174,\
                      /* Font Name: */  {'B','o','o','k','m','a','n',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5777,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3203.spd] OrigGaramond SWC -> Garamond: */
#define PCL50Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1335, 11724, 2734),\
                    /* Cell Height: */  EMU(1316, 11557, 2695),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(259, 2277, 531),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(431, 3788, 883),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  101,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(416, 3656, 853),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(698, 6132, 1430),\
                    /* Font Number: */  3203,\
                      /* Font Name: */  {'O','r','i','g','G','a','r','a','m','o','n','d',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3204.spd] OrigGaramond SWC -> Garamond Italic: */
#define PCL51Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1300, 11417, 2662),\
                    /* Cell Height: */  EMU(1318, 11575, 2699),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(241, 2114, 493),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(440, 3866, 902),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  101,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(390, 3428, 799),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(693, 6086, 1419),\
                    /* Font Number: */  3204,\
                      /* Font Name: */  {'O','r','i','g','G','a','r','a','m','o','n','d',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  9395,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3205.spd] OrigGaramond SWC -> Garamond Bold: */
#define PCL52Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1327, 11653, 2718),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(278, 2439, 569),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(463, 4070, 949),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  101,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(441, 3873, 903),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(697, 6125, 1428),\
                    /* Font Number: */  3205,\
                      /* Font Name: */  {'O','r','i','g','G','a','r','a','m','o','n','d',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3206.spd] OrigGaramond SWC -> Garamond Bold Italic: */
#define PCL53Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1311, 11513, 2685),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(259, 2277, 531),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(468, 4112, 959),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  101,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(422, 3702, 863),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(692, 6077, 1417),\
                    /* Font Number: */  3206,\
                      /* Font Name: */  {'O','r','i','g','G','a','r','a','m','o','n','d',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  9395,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3209.spd] Chianti SWC -> Shannon: */
#define PCL54Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1412, 12400, 2892),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(287, 2520, 588),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(511, 4488, 1047),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  113,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(704, 6183, 14418),\
                    /* Font Number: */  3209,\
                      /* Font Name: */  {'C','h','i','a','n','t','i',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3210.spd] Chianti SWC -> Shannon Italic: */
#define PCL55Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1366, 11996, 2798),\
                    /* Cell Height: */  EMU(1297, 11390, 2656),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(269, 2362, 551),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(511, 4488, 1047),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  113,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(704, 6183, 1442),\
                    /* Font Number: */  3210,\
                      /* Font Name: */  {'C','h','i','a','n','t','i',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  4605,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3211.spd] Chianti SWC -> Shannon Bold: */
#define PCL56Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1422, 12488, 2912),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(306, 2687, 627),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(518, 4549, 1061),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  113,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(704, 6183, 1442),\
                    /* Font Number: */  3211,\
                      /* Font Name: */  {'C','h','i','a','n','t','i',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3212.spd] Chianti XBd SWC -> Shannon Bold Italic: */
#define PCL57Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1420, 12470, 2908),\
                    /* Cell Height: */  EMU(1381, 12128, 2828),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(315, 2766, 645),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(525, 4611, 1075),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  4,\
                 /* Typeface (LSB): */  113,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(704, 6183, 1442),\
                    /* Font Number: */  3212,\
                      /* Font Name: */  {'C','h','i','a','n','t','i',' ','X','B','d',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  4605,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3208.spd] Revue Lt SWC -> Revue Light: */
#define PCL58Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1371, 12040, 2808),\
                    /* Cell Height: */  EMU(1356, 11908, 2777),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(296, 2599, 606),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(542, 4760, 1110),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  -3,\
                 /* Typeface (LSB): */  97,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(727, 6385, 1489),\
                    /* Font Number: */  3208,\
                      /* Font Name: */  {'R','e','v','u','e',' ','L','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3207.spd] Cooper Blk SWC -> Cooper Black: */
#define PCL59Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1535, 13480, 3144),\
                    /* Cell Height: */  EMU(1407, 12356, 2882),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(332, 2916, 680),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(549, 4821, 1124),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  7,\
                 /* Typeface (LSB): */  46,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 122, 122),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(741, 6507, 1518),\
                    /* Font Number: */  3207,\
                      /* Font Name: */  {'C','o','o','p','e','r',' ','B','l','k',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3220.spd] Courier SWC -> Courier Medium: */
#define PCL60Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(788, 6920, 1614),\
                    /* Cell Height: */  EMU(1151, 10108, 2357),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5291, 1234),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(434, 3815, 890),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 46, 46),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(602, 5291, 1234),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(560, 4920, 1147),\
                    /* Font Number: */  3220,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3221.spd] Courier SWC -> Courier Bold: */
#define PCL61Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(741, 6507, 1518),\
                    /* Cell Height: */  EMU(1209, 10617, 2476),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5291, 1234),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(437, 3842, 896),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 46, 46),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(602, 5291, 1234),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(560, 4920, 1147),\
                    /* Font Number: */  3221,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3222.spd] Courier SWC -> Courier Italic: */
#define PCL62Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(880, 7728, 1802),\
                    /* Cell Height: */  EMU(1106, 9713, 2265),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5291, 1234),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(434, 3815, 890),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 46, 46),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(602, 5291, 1234),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(560, 4920, 1147),\
                    /* Font Number: */  3222,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3223.spd] Courier SWC -> Courier Bold Italic: */
#define PCL63Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(794, 6973, 1626),\
                    /* Cell Height: */  EMU(1131, 9932, 2316),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5291, 1234),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(437, 3842, 896),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 103, 103),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(602, 5291, 1234),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(560, 4920, 1147),\
                    /* Font Number: */  3223,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font33220.spd] Courier SWC -> Courier Light Medium: */
#define PCL60LHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(788, 6920, 1614),\
                    /* Cell Height: */  EMU(1151, 10108, 2357),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5291, 1234),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(434, 3815, 890),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 46, 46),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(602, 5291, 1234),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(560, 4920, 1147),\
                    /* Font Number: */  3220,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','L','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(81, 711, 166),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font33221.spd] Courier SWC -> Courier Light Bold: */
#define PCL61LHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(741, 6507, 1518),\
                    /* Cell Height: */  EMU(1209, 10617, 2476),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5291, 1234),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(437, 3842, 896),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 46, 46),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(602, 5291, 1234),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(560, 4920, 1147),\
                    /* Font Number: */  3221,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','L','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(93, 817, 190),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font33222.spd] Courier SWC -> Courier Light Italic: */
#define PCL62LHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(880, 7728, 1802),\
                    /* Cell Height: */  EMU(1106, 9713, 2265),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5291, 1234),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(434, 3815, 890),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 46, 46),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(602, 5291, 1234),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(560, 4920, 1147),\
                    /* Font Number: */  3222,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','L','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(81, 711, 166),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font33223.spd] Courier SWC -> Courier Light Bold Italic: */
#define PCL63LHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(794, 6973, 1626),\
                    /* Cell Height: */  EMU(1131, 9932, 2316),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5291, 1234),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(437, 3842, 896),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 103, 103),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(602, 5291, 1234),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(560, 4920, 1147),\
                    /* Font Number: */  3223,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','L','t',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(93, 817, 190),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PCL_MSW & CYR_COMPLEMENT,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\



/* [font3224.spd] Letter Gothic -> Letter Gothic Regular: */
#define PCL64Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(662, 5814, 1356),\
                    /* Cell Height: */  EMU(1278, 11223, 2617),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(502, 4409, 1028),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(528, 4640, 1082),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  6,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(502, 4409, 1028),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(720, 6320, 1474),\
                    /* Font Number: */  3224,\
                      /* Font Name: */  {'L','e','t','t','e','r',' ','G','o','t','h','i','c'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3225.spd] Letter Gothic -> Letter Gothic Bold: */
#define PCL65Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(683, 5998, 1399),\
                    /* Cell Height: */  EMU(1344, 11803, 2753),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(502, 4409, 1028),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(536, 4705, 1097),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  6,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(502, 4409, 1028),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(721, 6335, 1477),\
                    /* Font Number: */  3225,\
                      /* Font Name: */  {'L','e','t','t','e','r',' ','G','o','t','h','i','c'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3226.spd] Letter Gothic -> Letter Gothic Italic: */
#define PCL66Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(877, 7702, 1796),\
                    /* Cell Height: */  EMU(1114, 9783, 2281),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(502, 4409, 1028),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(529, 4645, 1083),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  6,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 9, 9),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(502, 4409, 1028),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(721, 6335, 1477),\
                    /* Font Number: */  3226,\
                      /* Font Name: */  {'L','e','t','t','e','r',' ','G','o','t','h','i','c'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PCL_MSW,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3215.spd] Flareserif 821 SWC -> Albertus Medium: */
#define PCL67Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1428, 12541, 2925),\
                    /* Cell Height: */  EMU(1270, 11153, 2601),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(315, 2765, 645),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(533, 4680, 1091), \
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  1,\
                 /* Typeface (LSB): */  10,\
                 /* Typeface (MSB): */  33,\
                    /* Serif Style: */  EMU(128, 160, 160),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(429, 3764, 878),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(765, 6720, 1567),\
                    /* Font Number: */  3215,\
                      /* Font Name: */  {'F','l','a','r','e','s','e','r','i','f','8','2','1','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3216.spd] Flareserif 821 SWC -> Albertus Extrabold: */
#define PCL68Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1418, 12453, 2904),\
                    /* Cell Height: */  EMU(1286, 11294, 2634),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(370, 3253, 759),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(534, 4690, 1094),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  4,\
                 /* Typeface (LSB): */  10,\
                 /* Typeface (MSB): */  33,\
                    /* Serif Style: */  EMU(128, 160, 160),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(484, 4248, 991),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(764, 6710, 1565),\
                    /* Font Number: */  3216,\
                      /* Font Name: */  {'F','l','a','r','e','s','e','r','i','f','8','2','1','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3214.spd] Clarendon 701 SWC -> Clarendon Condensed: */
#define PCL69Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 1, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1126, 9889, 2306),\
                    /* Cell Height: */  EMU(1262, 11083, 2585),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(222, 1952, 455),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(528, 4635, 1081),\
                     /* Width Type: */  -2,\
                    /* Style (LSB): */  EMU(40, 69, 80),\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  44,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 103, 103),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(399, 3508, 818),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(694, 6095, 1421),\
                    /* Font Number: */  3214,\
                      /* Font Name: */  {'C','l','a','r','e','n','d','o','n',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3217.spd] Ribbon 131 SWC -> Coronet: */
#define PCL70Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(1385, 12163, 2836),\
                    /* Cell Height: */  EMU(1251, 10986, 2562),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(204, 1789, 417),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(230, 2017, 470),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 56, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  20,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(64, 179, 179),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(260, 2287, 533),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(749, 6581, 1535),\
                    /* Font Number: */  3217,\
                      /* Font Name: */  {'R','i','b','b','o','n',' ','1','3','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3218.spd] Audrey Two SWC -> Marigold: */
#define PCL71Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  EMU(0, 2, 0),\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(924, 8115, 1892),\
                    /* Cell Height: */  EMU(998, 8764, 2044),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(222, 1952, 455),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(516, 4530, 1056),\
                     /* Width Type: */  -1,\
                    /* Style (LSB): */  EMU(65, 57, 133),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  201,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  EMU(128, 217, 217),\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(249, 2189, 510),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(462, 4054, 945),\
                    /* Font Number: */  3218,\
                      /* Font Name: */  {'A','u','d','r','e','y',' ','T','w','o',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  2866,\
       /* Character Complement MSW :*/  PCLe_MSW,\
       /* Character Complement LSW :*/  PCLe_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3162.spd] Unnamed -> Line Printer: */
#define PCLS1Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  0,\
                    /* Cell Height: */  0,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  0,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3162,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\



/* [font3158.spd] Unnamed -> Plugin Roman Serif: */
#define PCLS2Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  0,\
                    /* Cell Height: */  0,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  0,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3158,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\



/* [font3159.spd] Unnamed -> Plugin Roman Serif Bold: */
#define PCLS3Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  0,\
                    /* Cell Height: */  0,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  0,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3159,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\



/* [font3160.spd] Unnamed -> Plugin Sanserif: */
#define PCLS4Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  0,\
                    /* Cell Height: */  0,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  0,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3160,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\



/* [font3161.spd] Unnamed -> Plugin Sanserif Bold: */
#define PCLS5Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  0,\
                    /* Cell Height: */  0,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  0,\
                 /* Typeface (MSB): */  32,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(559, 607, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3161,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\



/* [font????.spd]  -> HP Stick Font: */
#define PCLS6Hdr \
	{\
                    /* Header Size: */  0,\
                         /* Format: */  0,\
                      /* Font Type: */  0,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  0,\
                     /* Cell Width: */  0,\
                    /* Cell Height: */  0,\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  0,\
                       /* X Height: */  0,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  0,\
                 /* Typeface (MSB): */  0,\
                    /* Serif Style: */  0,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  0,\
                     /* Text Width: */  0,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  0,\
                    /* Font Number: */  0,\
                      /* Font Name: */  {'?'},\
                   /* Scale Factor: */  0,\
                   /* Master X Res: */  0,\
                   /* Master Y Res: */  0,\
             /* Underline Position: */  0,\
               /* Underline Height: */  0,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x00000000,\
       /* Character Complement LSW :*/  0x00000000,\
                      /* Data Size: */  0,\
	}\


/* [font3000.spd] Dutch SWA -> Times: */
#define PS1Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1176,\
                    /* Cell Height: */  1175,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  448,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  272,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  660,\
                    /* Font Number: */  3000,\
                      /* Font Name: */  {'D','u','t','c','h',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7467,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3001.spd] Dutch SWA -> Times Italic: */
#define PS2Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1178,\
                    /* Cell Height: */  1165,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  448,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  2,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  250,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  660,\
                    /* Font Number: */  3001,\
                      /* Font Name: */  {'D','u','t','c','h',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7472,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3002.spd] Dutch SWA -> Times Bold: */
#define PS3Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1176,\
                    /* Cell Height: */  1193,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  461,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  269,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  678,\
                    /* Font Number: */  3002,\
                      /* Font Name: */  {'D','u','t','c','h',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7477,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3003.spd] Dutch SWA -> Times Bold Italic: */
#define PS4Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1173,\
                    /* Cell Height: */  1209,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  465,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  251,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  671,\
                    /* Font Number: */  3003,\
                      /* Font Name: */  {'D','u','t','c','h',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7477,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3004.spd] Swiss SWA -> Helvetica: */
#define PS5Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1176,\
                    /* Cell Height: */  1211,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  523,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3004,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7476,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3005.spd] Swiss SWA -> Helvetica Italic: */
#define PS6Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1200,\
                    /* Cell Height: */  1211,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  523,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3005,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7476,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3006.spd] Swiss SWA -> Helvetica Bold: */
#define PS7Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1178,\
                    /* Cell Height: */  1212,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  531,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3006,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7474,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3007.spd] Swiss SWA -> Helvetica Bold Italic: */
#define PS8Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1195,\
                    /* Cell Height: */  1212,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  531,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3007,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7474,\
               /* Underline Height: */  EMU(50, 439, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3008.spd] Courier SWA -> Courier: */
#define PS9Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  707,\
                    /* Cell Height: */  1104,\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  600,\
                         /* Height: */  96,\
                       /* X Height: */  451,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  579,\
                    /* Font Number: */  3008,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7483,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3009.spd] Courier SWA -> Courier Italic: */
#define PS10Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  754,\
                    /* Cell Height: */  1104,\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  600,\
                         /* Height: */  96,\
                       /* X Height: */  451,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  579,\
                    /* Font Number: */  3009,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7483,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3010.spd] Courier SWA -> Courier Bold: */
#define PS11Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  708,\
                    /* Cell Height: */  1121,\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  600,\
                         /* Height: */  96,\
                       /* X Height: */  451,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  579,\
                    /* Font Number: */  3010,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7483,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3011.spd] Courier SWA -> Courier Bold Italic: */
#define PS12Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  742,\
                    /* Cell Height: */  1121,\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  600,\
                         /* Height: */  96,\
                       /* X Height: */  451,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  579,\
                    /* Font Number: */  3011,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7483,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3012.spd] Symbol Set SWA -> Symbol: */
#define PS13Hdr \
	{\
                    /* Header Size: */  80,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  1,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1230,\
                    /* Cell Height: */  1126,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  46,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  190,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3012,\
                      /* Font Name: */  {'S','y','m','b','o','l',' ','S','e','t',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7532,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0xFFFFFFFF,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3013.spd] ZapfCallig SWA -> Palatino: */
#define PS14Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1282,\
                    /* Cell Height: */  1240,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  473,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  15,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  699,\
                    /* Font Number: */  3013,\
                      /* Font Name: */  {'Z','a','p','f','C','a','l','l','i','g',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7444,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3014.spd] ZapfCallig SWA -> Palatino Italic: */
#define PS15Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1235,\
                    /* Cell Height: */  1239,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  475,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  15,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  204,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  699,\
                    /* Font Number: */  3014,\
                      /* Font Name: */  {'Z','a','p','f','C','a','l','l','i','g',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7443,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5189,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3015.spd] ZapfCallig SWA -> Palatino Bold: */
#define PS16Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1228,\
                    /* Cell Height: */  1227,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  475,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  15,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  686,\
                    /* Font Number: */  3015,\
                      /* Font Name: */  {'Z','a','p','f','C','a','l','l','i','g',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7449,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3016.spd] ZapfCallig SWA -> Palatino Bold Italic: */
#define PS17Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1315,\
                    /* Cell Height: */  1246,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  477,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  15,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  202,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  686,\
                    /* Font Number: */  3016,\
                      /* Font Name: */  {'Z','a','p','f','C','a','l','l','i','g',' ','S','W','A'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  7444,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3021.spd] Bookman SWA -> ITC Bookman: */
#define PS18Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1409,\
                    /* Cell Height: */  1199,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  320,\
                         /* Height: */  96,\
                       /* X Height: */  481,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  146,\
                 /* Typeface (MSB): */  81,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  671,\
                    /* Font Number: */  3021,\
                      /* Font Name: */  {'B','o','o','k','m','a','n',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7469,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3022.spd] Bookman SWA -> ITC Bookman Italic: */
#define PS19Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1463,\
                    /* Cell Height: */  1185,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  300,\
                         /* Height: */  96,\
                       /* X Height: */  486,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  146,\
                 /* Typeface (MSB): */  81,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  203,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  671,\
                    /* Font Number: */  3022,\
                      /* Font Name: */  {'B','o','o','k','m','a','n',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7469,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5777,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3023.spd] Bookman SWA -> ITC Bookman Bold: */
#define PS20Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1519,\
                    /* Cell Height: */  1241,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  340,\
                         /* Height: */  96,\
                       /* X Height: */  495,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  146,\
                 /* Typeface (MSB): */  81,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  676,\
                    /* Font Number: */  3023,\
                      /* Font Name: */  {'B','o','o','k','m','a','n',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7469,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3024.spd] Bookman SWA -> ITC Bookman Bold Italic: */
#define PS21Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1609,\
                    /* Cell Height: */  1244,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  340,\
                         /* Height: */  96,\
                       /* X Height: */  501,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  146,\
                 /* Typeface (MSB): */  81,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  215,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  676,\
                    /* Font Number: */  3024,\
                      /* Font Name: */  {'B','o','o','k','m','a','n',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7470,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5777,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3025.spd] SwissNarrow SWA -> Helvetica Narrow: */
#define PS22Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  963,\
                    /* Cell Height: */  1236,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  228,\
                         /* Height: */  96,\
                       /* X Height: */  523,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3025,\
                      /* Font Name: */  {'S','w','i','s','s','N','a','r','r','o','w',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7476,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3026.spd] SwissNarrow SWA -> Helvetica Narrow Italic: */
#define PS23Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  993,\
                    /* Cell Height: */  1235,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  228,\
                         /* Height: */  96,\
                       /* X Height: */  523,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3026,\
                      /* Font Name: */  {'S','w','i','s','s','N','a','r','r','o','w',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7476,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5777,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3027.spd] SwissNarrow SWA -> Helvetica Narrow Bold: */
#define PS24Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  972,\
                    /* Cell Height: */  1219,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  228,\
                         /* Height: */  96,\
                       /* X Height: */  531,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3027,\
                      /* Font Name: */  {'S','w','i','s','s','N','a','r','r','o','w',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7474,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3028.spd] SwissNarrow SWA -> Helvetica Narrow Bold Italic: */
#define PS25Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  979,\
                    /* Cell Height: */  1219,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  228,\
                         /* Height: */  96,\
                       /* X Height: */  531,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3028,\
                      /* Font Name: */  {'S','w','i','s','s','N','a','r','r','o','w',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7474,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5777,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3031.spd] CenturySchbk SWA -> Century Schoolbook: */
#define PS26Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1180,\
                    /* Cell Height: */  1219,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  463,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  127,\
                 /* Typeface (MSB): */  81,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  722,\
                    /* Font Number: */  3031,\
                      /* Font Name: */  {'C','e','n','t','u','r','y','S','c','h','b','k',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7476,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3032.spd] CenturySchbk SWA -> Century Schoolbook Italic: */
#define PS27Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1197,\
                    /* Cell Height: */  1220,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  471,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  127,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  204,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  722,\
                    /* Font Number: */  3032,\
                      /* Font Name: */  {'C','e','n','t','u','r','y','S','c','h','b','k',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7476,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8169,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3033.spd] CenturySchbk SWA -> Century Schoolbook Bold: */
#define PS28Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1158,\
                    /* Cell Height: */  1223,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  287,\
                         /* Height: */  96,\
                       /* X Height: */  476,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  127,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  722,\
                    /* Font Number: */  3033,\
                      /* Font Name: */  {'C','e','n','t','u','r','y','S','c','h','b','k',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7476,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3034.spd] CenturySchbk SWA -> Century Schoolbook Bold Italic: */
#define PS29Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1335,\
                    /* Cell Height: */  1234,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  287,\
                         /* Height: */  96,\
                       /* X Height: */  482,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  127,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  203,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  722,\
                    /* Font Number: */  3034,\
                      /* Font Name: */  {'C','e','n','t','u','r','y','S','c','h','b','k',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7476,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8169,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3017.spd] AvantGarde SWA -> ITC Avant Garde: */
#define PS30Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1325,\
                    /* Cell Height: */  1209,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  276,\
                         /* Height: */  96,\
                       /* X Height: */  537,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  -1,\
                 /* Typeface (LSB): */  31,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  711,\
                    /* Font Number: */  3017,\
                      /* Font Name: */  {'A','v','a','n','t','G','a','r','d','e',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7492,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3018.spd] AvantGarde SWA -> ITC Avant Garde Italic: */
#define PS31Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1335,\
                    /* Cell Height: */  1209,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  276,\
                         /* Height: */  96,\
                       /* X Height: */  537,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  -1,\
                 /* Typeface (LSB): */  31,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  711,\
                    /* Font Number: */  3018,\
                      /* Font Name: */  {'A','v','a','n','t','G','a','r','d','e',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7493,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6369,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3019.spd] AvantGarde SWA -> ITC Avant Garde Bold: */
#define PS32Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1417,\
                    /* Cell Height: */  1202,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  280,\
                         /* Height: */  96,\
                       /* X Height: */  537,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  1,\
                 /* Typeface (LSB): */  31,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  711,\
                    /* Font Number: */  3019,\
                      /* Font Name: */  {'A','v','a','n','t','G','a','r','d','e',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7493,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3020.spd] AvantGarde SWA -> ITC Avant Garde Bold Italic: */
#define PS33Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1415,\
                    /* Cell Height: */  1202,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  280,\
                         /* Height: */  96,\
                       /* X Height: */  537,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  1,\
                 /* Typeface (LSB): */  31,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  228,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  711,\
                    /* Font Number: */  3020,\
                      /* Font Name: */  {'A','v','a','n','t','G','a','r','d','e',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7493,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6369,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3029.spd] ZapfChancery SWA -> ITC Zapf Chancery: */
#define PS34Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1230,\
                    /* Cell Height: */  1069,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  220,\
                         /* Height: */  96,\
                       /* X Height: */  411,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  43,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  662,\
                    /* Font Number: */  3029,\
                      /* Font Name: */  {'Z','a','p','f','C','h','a','n','c','e','r','y',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7440,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8169,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3030.spd] ZapfDingbats SWA -> ITC Zapf Dingbats: */
#define PS35Hdr \
	{\
                    /* Header Size: */  80,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  1,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  974,\
                    /* Cell Height: */  983,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  45,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  202,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3030,\
                      /* Font Name: */  {'Z','a','p','f','D','i','n','g','b','a','t','s',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7532,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0xFFFFFFFE,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3069.spd] SwissCond SWA -> Helvetica-Condensed: */
#define PS36Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1232,\
                    /* Cell Height: */  1209,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  96,\
                       /* X Height: */  562,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  4,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  752,\
                    /* Font Number: */  3069,\
                      /* Font Name: */  {'S','w','i','s','s','C','o','n','d',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7485,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3070.spd] SwissCond SWA -> Helvetica-Condensed-Oblique: */
#define PS37Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1232,\
                    /* Cell Height: */  1209,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  96,\
                       /* X Height: */  562,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  4,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  0,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  752,\
                    /* Font Number: */  3070,\
                      /* Font Name: */  {'S','w','i','s','s','C','o','n','d',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7485,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5777,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3071.spd] SwissCond SWA -> Helvetica-Condensed-Bold: */
#define PS38Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1242,\
                    /* Cell Height: */  1209,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  96,\
                       /* X Height: */  565,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  4,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  752,\
                    /* Font Number: */  3071,\
                      /* Font Name: */  {'S','w','i','s','s','C','o','n','d',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7485,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3072.spd] SwissCond SWA -> Helvetica-Condensed-BoldObl: */
#define PS39Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1242,\
                    /* Cell Height: */  1209,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  0,\
                         /* Height: */  96,\
                       /* X Height: */  565,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  4,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  229,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  752,\
                    /* Font Number: */  3072,\
                      /* Font Name: */  {'S','w','i','s','s','C','o','n','d',' ','S','W','A'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  7485,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  5777,\
       /* Character Complement MSW :*/  PS_MSW,\
       /* Character Complement LSW :*/  PS_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3238.spd] Swiss SWM -> Arial: */
#define TT1Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1176, 2408, 2408),\
                    /* Cell Height: */  EMU(1211, 2480, 2480),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(278, 569, 569),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(519, 1062, 1062),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  EMU(64, 132, 132),\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  218,\
                 /* Typeface (MSB): */  64,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(441, 904, 904),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(716, 1466, 1466),\
                    /* Font Number: */  3238,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','M'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3239.spd] Swiss SWM -> Arial Italic: */
#define TT2Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1200, 2458, 2458),\
                    /* Cell Height: */  EMU(1211, 2480, 2480),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(278, 569, 569),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(519, 1062, 1062),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  218,\
                 /* Typeface (MSB): */  64,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(441, 904, 904),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(716, 1466, 1466),\
                    /* Font Number: */  3239,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','M'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3240.spd] Swiss SWM -> Arial Bold: */
#define TT3Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1178, 2413, 2413),\
                    /* Cell Height: */  EMU(1212, 2482, 2482),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(278, 569, 569),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(519, 1062, 1062),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  218,\
                 /* Typeface (MSB): */  64,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(479, 980, 980),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(716, 1466, 1466),\
                    /* Font Number: */  3240,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','M'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3241.spd] Swiss SWM -> Arial Bold Italic: */
#define TT4Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1195, 2447, 2447),\
                    /* Cell Height: */  EMU(1212, 2482, 2482),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(278, 569, 569),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(519, 1062, 1062),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  218,\
                 /* Typeface (MSB): */  64,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(479, 980, 980),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(716, 1466, 1466),\
                    /* Font Number: */  3241,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','M'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3234.spd] Dutch SWM -> Times New Roman: */
#define TT5Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1176, 2408, 2408),\
                    /* Cell Height: */  EMU(1175, 2406, 2406),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(447, 916, 916),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  66,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(401, 821, 821),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(662, 1356, 1356),\
                    /* Font Number: */  3234,\
                      /* Font Name: */  {'D','u','t','c','h',' ','S','W','M'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3235.spd] Dutch SWM -> Times New Roman Italic: */
#define TT6Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1178, 2413, 2413),\
                    /* Cell Height: */  EMU(1165, 2386, 2386),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(430, 881, 881),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  66,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(402, 823, 823),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(662, 1356, 1356),\
                    /* Font Number: */  3235,\
                      /* Font Name: */  {'D','u','t','c','h',' ','S','W','M'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3236.spd] Dutch SWM -> Times New Roman Bold: */
#define TT7Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1176, 2408, 2408),\
                    /* Cell Height: */  EMU(1193, 2443, 2443),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(457, 935, 935),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  66,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(427, 874, 874),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(662, 1356, 1356),\
                    /* Font Number: */  3236,\
                      /* Font Name: */  {'D','u','t','c','h',' ','S','W','M'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3237.spd] Dutch SWM -> Times New Roman Bold Italic: */
#define TT8Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1173, 2402, 2402),\
                    /* Cell Height: */  EMU(1209, 2476, 2476),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(439, 899, 899),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  66,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(412, 844, 844),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(662, 1356, 1356),\
                    /* Font Number: */  3237,\
                      /* Font Name: */  {'D','u','t','c','h',' ','S','W','M'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3220.spd] Courier SWC -> Courier New: */
#define TT9Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(707, 1448, 1448),\
                    /* Cell Height: */  EMU(1104, 2261, 2261),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(600, 1229, 1229),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(451, 924, 924),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(579, 1186, 1186),\
                    /* Font Number: */  3220,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3222.spd] Courier SWC -> Courier New Italic: */
#define TT10Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(754, 1544, 1544),\
                    /* Cell Height: */  EMU(1104, 2261, 2261),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(600, 1229, 1229),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(451, 924, 924),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(579, 1186, 1186),\
                    /* Font Number: */  3222,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3221.spd] Courier SWC -> Courier New Bold: */
#define TT11Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(708, 1450, 1450),\
                    /* Cell Height: */  EMU(1121, 2296, 2296),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(600, 1229, 1229),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(451, 924, 924),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(579, 1186, 1186),\
                    /* Font Number: */  3221,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3223.spd] Courier SWC -> Courier New Bold Italic: */
#define TT12Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(742, 1520, 1520),\
                    /* Cell Height: */  EMU(1121, 2296, 2296),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(600, 1229, 1229),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(451, 924, 924),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(579, 1186, 1186),\
                    /* Font Number: */  3223,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3246.spd] Symbol Set SWM -> Symbol: */
#define TT13Hdr \
	{\
                    /* Header Size: */  80,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  1,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1230, 2519, 2519),\
                    /* Cell Height: */  EMU(1126, 2306, 2306),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  19*32+'M'-64,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(447, 916, 916),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  46,\
                 /* Typeface (MSB): */  65,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(587, 1203, 1203),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(187, 187, 189),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(667, 1365, 1365),\
                    /* Font Number: */  3246,\
                      /* Font Name: */  {'S','y','m','b','o','l',' ','S','e','t',' ','S','W','M'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0xFFFFFFFF,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3219.spd] More Wingbats -> Wingdings: */
#define TT14Hdr \
	{\
                    /* Header Size: */  80,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  2,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1359, 2783, 2783),\
                    /* Cell Height: */  EMU(1134, 2322, 2322),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  579*32+'L'-64,\
                          /* Pitch: */  EMU(1000, 2048, 2048),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  170,\
                 /* Typeface (MSB): */  122,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(896, 1834, 1834),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(222, 222, 223),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3219,\
                      /* Font Name: */  {'M','o','r','e','W','i','n','g','b','a','t','s','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0xFFFFFFFF,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3227.spd] Unnamed -> Fixed Pitch 5291 Universal Support */
#define PCLS7Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(788, 6920, 1614),\
                    /* Cell Height: */  EMU(1151, 10108, 2357),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5287, 1233),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(451, 3961, 924),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  100,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(579, 5085, 1186),\
                    /* Font Number: */  3227,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\



/* [font3229.spd] Unnamed -> Fixed Pitch 5291 Roman Support */
#define PCLS8Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(788, 6920, 1614),\
                    /* Cell Height: */  EMU(1151, 10108, 2357),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5287, 1233),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(451, 3961, 924),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  189,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(579, 5085, 1186),\
                    /* Font Number: */  3229,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3230.spd] Unnamed -> Fixed Pitch 5291 Bold Support */
#define PCLS9Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(741, 6507, 1518),\
                    /* Cell Height: */  EMU(1209, 10617, 2476),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(602, 5287, 1233),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(451, 3961, 924),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  189,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(579, 5085, 1186),\
                    /* Font Number: */  3230,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3228.spd] Unnamed -> Fixed Pitch 4409 Universal Support */
#define PCLS10Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(662, 5814, 1642),\
                    /* Cell Height: */  EMU(1278, 11223, 2617),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(500, 4391, 1024),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(491, 4312, 1006),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  6,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  100,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(669, 5875, 1370),\
                    /* Font Number: */  3228,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3231.spd] Unnamed -> Fixed Pitch 4409 Roman Support */
#define PCLS11Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(662, 5814, 1642),\
                    /* Cell Height: */  EMU(1278, 11223, 2617),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(500, 4391, 1024),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(491, 4312, 1006),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  6,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  187,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(669, 5875, 1370),\
                    /* Font Number: */  3231,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3232.spd] Unnamed -> Fixed Pitch 4409 Bold Support */
#define PCLS12Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 11, 15),\
                      /* Font Type: */  EMU(10, 10, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 6709, 0),\
                     /* Cell Width: */  EMU(683, 5998, 1399),\
                    /* Cell Height: */  EMU(1344, 11803, 2753),\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  EMU(56, 56, 0),\
                          /* Pitch: */  EMU(500, 4391, 1024),\
                         /* Height: */  EMU(96, 843, 0),\
                       /* X Height: */  EMU(491, 4312, 1006),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  6,\
                 /* Typeface (MSB): */  16,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 10538, 2458),\
                     /* Text Width: */  EMU(500, 4391, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  187,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(669, 5875, 1370),\
                    /* Font Number: */  3232,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 8782, 2048),\
                   /* Master X Res: */  EMU(1000, 8782, 2048),\
                   /* Master Y Res: */  EMU(1000, 8782, 2048),\
             /* Underline Position: */  EMU(-224, -1967, -459),\
               /* Underline Height: */  EMU(51, 448, 104),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  PCL_MSW,\
       /* Character Complement LSW :*/  PCL_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3233.spd] Unnamed -> Unicode Universal Support */
#define TTS1Hdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1176, 2408, 2408),\
                    /* Cell Height: */  EMU(1175, 2406, 2406),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(448, 916, 916),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  66,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  187,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(660, 1352, 1352),\
                    /* Font Number: */  3233,\
                      /* Font Name: */  {'U','n','n','a','m','e','d'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


#if INCL_MFT
/* [font3234.spd] Dutch SWM -> Times New Roman: */
#define TT5MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1176, 2408, 2408),\
                    /* Cell Height: */  EMU(1175, 2406, 2406),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  EMU(56, 0, 0),\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(448, 916, 916),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  66,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(660, 1352, 1352),\
                    /* Font Number: */  3150,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3235.spd] Dutch SWM -> Times New Roman Italic: */
#define TT6MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1178, 2413. 2413),\
                    /* Cell Height: */  EMU(1165, 2386, 2386),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(448, 918, 918),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  66,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(660, 1352, 1352),\
                    /* Font Number: */  3151,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3236.spd] Dutch SWM -> Times New Roman Bold: */
#define TT7MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1176, 2408, 2408),\
                    /* Cell Height: */  EMU(1193, 2443, 2443),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(461, 944, 944),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  66,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(678, 1389, 1389),\
                    /* Font Number: */  3152,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3237.spd] Dutch SWM -> Times New Roman Bold Italic: */
#define TT8MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1173, 2402, 2402),\
                    /* Cell Height: */  EMU(1209, 2476, 2476),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(465, 952, 952),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  66,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(671, 1374, 1374),\
                    /* Font Number: */  3153,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  TT_MSW,\
       /* Character Complement LSW :*/  TT_LSW,\
                      /* Data Size: */  0,\
	}\


/* [font3000.spd] Dutch SWA -> Times: */
#define PS1MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1176, 2408, 2408),\
                    /* Cell Height: */  EMU(1175, 2406, 2406),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(448, 918, 918),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(660, 1352, 1352),\
                    /* Font Number: */  3150,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3001.spd] Dutch SWA -> Times Italic: */
#define PS2MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1178, 2413, 2413),\
                    /* Cell Height: */  EMU(1165, 2386, 2386),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(448, 918, 918),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(660, 1352, 1352),\
                    /* Font Number: */  3151,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3002.spd] Dutch SWA -> Times Bold: */
#define PS3MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1176, 2408, 2408),\
                    /* Cell Height: */  EMU(1193, 2443, 2443),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(461, 944, 944),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(678, 1389, 1389),\
                    /* Font Number: */  3152,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3003.spd] Dutch SWA -> Times Bold Italic: */
#define PS4MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  EMU(12, 15, 15),\
                      /* Font Type: */  EMU(10, 11, 11),\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  EMU(764, 0, 0),\
                     /* Cell Width: */  EMU(1173, 2402, 2402),\
                    /* Cell Height: */  EMU(1209, 2476, 2476),\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  EMU(250, 512, 512),\
                         /* Height: */  EMU(96, 0, 0),\
                       /* X Height: */  EMU(465, 952, 952),\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  5,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  EMU(1200, 2458, 2458),\
                     /* Text Width: */  EMU(500, 1024, 1024),\
                     /* First Code: */  0,\
                    /* NumContours: */  EMU(405, 405, 640),\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  EMU(671, 1374, 1374),\
                    /* Font Number: */  3153,\
                      /* Font Name: */  {'D','u','t','c','h','8','0','1',' ','S','W','C'},\
                   /* Scale Factor: */  EMU(1000, 2048, 2048),\
                   /* Master X Res: */  EMU(1000, 2048, 2048),\
                   /* Master Y Res: */  EMU(1000, 2048, 2048),\
             /* Underline Position: */  EMU(-224, -459, -459),\
               /* Underline Height: */  EMU(50, 102, 102),\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  8779,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3004.spd] Swiss SWA -> Helvetica: */
#define PS5MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  12,\
                      /* Font Type: */  10,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1176,\
                    /* Cell Height: */  1211,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  523,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3238,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','M'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  -224,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3005.spd] Swiss SWA -> Helvetica Italic: */
#define PS6MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  12,\
                      /* Font Type: */  10,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1200,\
                    /* Cell Height: */  1211,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  523,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3239,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','M'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  -224,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3006.spd] Swiss SWA -> Helvetica Bold: */
#define PS7MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  12,\
                      /* Font Type: */  10,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1178,\
                    /* Cell Height: */  1212,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  531,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3240,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','M'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  -224,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3007.spd] Swiss SWA -> Helvetica Bold Italic: */
#define PS8MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  12,\
                      /* Font Type: */  10,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1195,\
                    /* Cell Height: */  1212,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  278,\
                         /* Height: */  96,\
                       /* X Height: */  531,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  4,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  64,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  719,\
                    /* Font Number: */  3241,\
                      /* Font Name: */  {'S','w','i','s','s',' ','S','W','M'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  -224,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3008.spd] Courier SWA -> Courier: */
#define PS9MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  12,\
                      /* Font Type: */  10,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  707,\
                    /* Cell Height: */  1104,\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  600,\
                         /* Height: */  96,\
                       /* X Height: */  451,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  579,\
                    /* Font Number: */  3220,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  -224,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3009.spd] Courier SWA -> Courier Italic: */
#define PS10MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  12,\
                      /* Font Type: */  10,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  754,\
                    /* Cell Height: */  1104,\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  600,\
                         /* Height: */  96,\
                       /* X Height: */  451,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  579,\
                    /* Font Number: */  3222,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  -224,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3010.spd] Courier SWA -> Courier Bold: */
#define PS11MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  12,\
                      /* Font Type: */  10,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  708,\
                    /* Cell Height: */  1121,\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  600,\
                         /* Height: */  96,\
                       /* X Height: */  451,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  579,\
                    /* Font Number: */  3221,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  -224,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3011.spd] Courier SWA -> Courier Bold Italic: */
#define PS12MFTHdr \
	{\
                    /* Header Size: */  88,\
                         /* Format: */  12,\
                      /* Font Type: */  10,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  742,\
                    /* Cell Height: */  1121,\
                    /* Orientation: */  0,\
                        /* Spacing: */  0,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  600,\
                         /* Height: */  96,\
                       /* X Height: */  451,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  1,\
                  /* Stroke Weight: */  3,\
                 /* Typeface (LSB): */  3,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  579,\
                    /* Font Number: */  3223,\
                      /* Font Name: */  {'C','o','u','r','i','e','r',' ','S','W','C'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  1000,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  6964,\
       /* Character Complement MSW :*/  0x7FFFFFF9,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


/* [font3246.spd] Symbol Set SWM -> Symbol: */
#define PS13MFTHdr \
	{\
                    /* Header Size: */  80,\
                         /* Format: */  12,\
                      /* Font Type: */  1,\
                    /* Style (MSB): */  0,\
                       /* Baseline: */  764,\
                     /* Cell Width: */  1230,\
                    /* Cell Height: */  1126,\
                    /* Orientation: */  0,\
                        /* Spacing: */  1,\
                     /* Symbol Set: */  0,\
                          /* Pitch: */  250,\
                         /* Height: */  96,\
                       /* X Height: */  4,\
                     /* Width Type: */  0,\
                    /* Style (LSB): */  0,\
                  /* Stroke Weight: */  0,\
                 /* Typeface (LSB): */  46,\
                 /* Typeface (MSB): */  80,\
                    /* Serif Style: */  128,\
                        /* Quality: */  0,\
                      /* Placement: */  0,\
             /* Underline Distance: */  0,\
           /* Old Underline Height: */  0,\
                    /* Text Height: */  1200,\
                     /* Text Width: */  500,\
                     /* First Code: */  0,\
                    /* NumContours: */  0,\
                      /* Pitch Ext: */  0,\
                     /* Height Ext: */  0,\
                     /* Cap Height: */  4,\
                    /* Font Number: */  3246,\
                      /* Font Name: */  {'S','y','m','b','o','l',' ','S','e','t',' ','S','W','M'},\
                   /* Scale Factor: */  1000,\
                   /* Master X Res: */  1000,\
                   /* Master Y Res: */  1000,\
             /* Underline Position: */  -224,\
               /* Underline Height: */  50,\
                  /* LRE Threshold: */  0,\
                   /* Italic Angle: */  0,\
       /* Character Complement MSW :*/  0xFFFFFFFF,\
       /* Character Complement LSW :*/  0xFFFFFFFF,\
                      /* Data Size: */  0,\
	}\


#endif /*  INCL_MFT */
