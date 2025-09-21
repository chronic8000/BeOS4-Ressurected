/*****************************************************************************
*                                                                            *
*  Copyright 1995, as an unpublished work by Bitstream Inc., Cambridge, MA   *
*                                                                            *
*         These programs are the sole property of Bitstream Inc. and         *
*           contain its proprietary and confidential information.            *
*                                                                            *
* $Header: //bliss/user/archive/rcs/fit/finfotbl.c,v 6.0 97/03/17 17:36:07 shawn Release $
*
* $Log:	finfotbl.c,v $
 * Revision 6.0  97/03/17  17:36:07  shawn
 * TDIS Version 6.00
 * 
 * Revision 4.9  97/03/13  11:17:27  shawn
 * Moved #include <stddef.h> above #include "speedo.h".
 * 
 * Revision 4.8  97/03/13  10:50:12  shawn
 * Moved '#ifdef INTEL' below #include "speedo.h" where INTEL is defined.
 * 
 * Revision 4.7  97/03/11  11:20:09  shawn
 * Removed HPLJ6L from menu (Temporary).
 * 
 * Revision 4.6  96/12/05  11:07:16  shawn
 * Added support for the LJ5/LJ5M/LJ6L printers.
 * 
 * Revision 4.5  96/09/19  13:42:00  shawn
 * Removed the non-standard PostScript (full) 35 font FIT entry.
 * Set gCompressedPFR to not include mutable PostScript fonts for the 35, 39 and 35+TT14.
 * 
 * Revision 4.4  96/09/17  10:04:50  shawn
 * Added support for LJ5/5L/5M printers.
 * 
 * Revision 4.3  96/07/02  12:58:03  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 4.2  96/07/01  09:20:09  shawn
 * Added support for LJ4L and cleaned up selection menu.
 * 
 * Revision 4.1  96/04/30  19:02:06  roberte
 * worked on the AddPostScript function. Added param, bracketed the
 * index testing when gCompressedPFR is TRUE. New parameter is doInsertion.
 * 
 * Revision 4.0  95/02/15  16:49:06  shawn
 * Release
 * 
 * Revision 3.4  95/02/13  10:30:46  roberte
 * Upgraded to ANSI.
 * Added menu choices and table entries for Compressed/Combined TrueDoc 80
 * font set PFR (LJ4 + PS35).
 * 
 * Revision 3.3  95/01/10  08:57:33  roberte
 * Updated copyright header.
 * 
 * Revision 3.2  95/01/04  16:29:51  roberte
 * Release
 * 
 * Revision 3.1  95/01/04  11:44:56  roberte
 * Release
 * 
 * Revision 2.5  94/12/28  09:20:22  roberte
 * Added support for compressed PFR for LaserJet 4 FIT.
 * Insertion of pseudo-italic support font entries for
 * the compressed PFR architecture.
 * Based on conditional compile PROC_TRUEDOC and COMPRESSED_PFR
 * both non-zero.
 * 
 * Revision 2.4  94/11/14  13:35:01  roberte
 * No more stdef.h, use btstypes.h for data types.
 * 
 * Revision 2.3  94/11/11  11:00:15  roberte
 * Corrected init of local base variable in 2 subroutines.
 * 
 * Revision 2.2  94/07/20  08:51:17  roberte
 * Initial INCL_MFT Version. Addition of numerous Support font names,
 * FIT table entries and modifications to Add??() functions.
 * 
 * Revision 2.1  94/06/01  10:58:46  roberte
 * Added new support fonts for fixed pitch 5291 and 4409, and universal for SWMs.
 * 
 * Revision 2.0  94/05/04  09:24:26  roberte
 * Release
 * 
 * Revision 1.22  94/04/08  11:27:43  roberte
 * Release
 * 
 * Revision 1.21  94/02/23  11:33:54  roberte
 * Rewrite of some code for more flexibility for newly defined font sets.
 * Added menu choice for 4L font set.
 * Changed bucketing of support fonts so sans serif supports direct to
 * serif supports. This eliminated a set of duplicate characters. Saves
 * significant data space.
 * 
 * Revision 1.20  93/07/29  12:07:46  roberte
 * Added Helvetica-Condensed fonts and menu selection for making PS39 FIT table.
 * Made all PS root fonts have no nextSearchEncode path. The buck stops here, for now.
 * 
 * Revision 1.19  93/07/02  14:27:21  roberte
 * Added menu item for 4L 26 font FIT table. Broke out
 * several new "Add" functions to ease that process, and make it
 * easier to define new future configurations.
 * 
 * Revision 1.18  93/05/13  08:58:03  roberte
 * Corrected bucketing encoding for Albertus fonts, left a commented out #define for
 * recovering former bucketing encode. We don't think the old one should have worked,
 * but our proofs were OK!
 * 
 * Revision 1.17  93/04/23  17:43:56  roberte
 * Changed all functions declarations to standard K&R type
 * 
 * Revision 1.16  93/04/16  16:13:43  roberte
 * Changed all mis-spellings of Bodoni (was Bodini).
 * 
 * Revision 1.15  93/04/06  10:06:16  roberte
 * Removed curly brackets from all font alias name #defines.
 * Removed duplicate definitions of PCLS1Name - PCLS6Name
 * 
 * Revision 1.14  93/03/16  10:20:52  roberte
 * Changed comments of 8 TrueType emulators from SWA to SWM.
 * 
 * Revision 1.13  93/03/15  12:40:24  roberte
 * Release
 * 
 * Revision 1.12  93/02/12  16:26:53  roberte
 * Added #define of O_BINARY for non-INTEL builds, and use of
 * this bit flag in the "open" call.
 * 
 * Revision 1.11  93/02/01  14:22:25  roberte
 * Removed OLDWAY block fro Courier New, and left Courier
 * New having NEXT_NONE for support instead of SERIF.
 * 
 * Revision 1.10  93/01/12  12:50:36  roberte
 * Support typefaces now tagged as pdlSupport.
 * Changes for Arial and Times New Roman. New font IDs.
 * 
 * Revision 1.9  92/11/19  09:04:20  roberte
 * Corrected spelling of Helvetica-Narrow typeface industry names.
 * 
 * Revision 1.8  92/11/02  09:16:38  roberte
 * Changes within the nextSearchEncode field for the Couriers and Letter Gothics.
 * The contain all support within themselves.
 * 
 * Revision 1.7  92/10/29  15:34:06  roberte
 * Corrected nextSearchEncode for Coronet. 
 * 
 * Revision 1.6  92/10/29  09:19:16  roberte
 * Corrected number of fonts in menu.
 * 
 * Revision 1.5  92/10/28  08:55:58  roberte
 * Changes reflecting actual internal fonts in the LJIV printer.
 * No dingbats and no antique olive bold italic.
 * 
 * Revision 1.4  92/10/01  20:00:12  laurar
 * take out errno which is an illegal variable name on PC;
 * variable was not referenced anyway.
 * 
 * Revision 1.3  92/09/29  16:27:25  laurar
 * add header files for INTEL
 * 
 * Revision 1.2  92/09/26  14:13:18  roberte
 * Added copyright header and RCS markers.
 * 
*****************************************************************************/
/*****************************************************************************
    FILE:        FINFOTBL.C
    PROJECT:    4-in-1
    PURPOSE:    Builds a utility program to write out binary
                files (Font Information Tables) for various
                font configurations for the 4-in-1 project.
    METHOD:        Prompt the user with a menu of configuration
                possiblities, build the appropriate set
                from a "library" of all possibilities, and
                write out that custom set as a binary image file.
    AUTHOR:        RJE @ Bitstream Inc.
    CREATED:    08-14-92
******************************************************************************/

#include <stdio.h>
#include <fcntl.h>                /* get O_???? types */
#include <sys/types.h>            /* get size_t, S_I???? */
#include <sys/stat.h>

#ifdef INTEL
#include <stddef.h>
#endif

#include "speedo.h"
#include "pclhdrs.h"
#include "finfotbl.h"            /* data structure of Font Info Table */

#ifndef INTEL
#define O_BINARY 0
#endif


int gCurFontIdx;

#define PCLS1Name       "Line Printer"

#define PCLS2Name       "Plugin Roman Serif"
#define PCLS3Name       "Plugin Roman Serif Bold"
#define PCLS4Name       "Plugin Sanserif"
#define PCLS5Name       "Plugin Sanserif Bold"

#ifdef HAVE_STICK_FONT
#define PCLS6Name       "HP Stick Font"
#endif

#define PCLS7Name       "Fixed Pitch 5291 Universal"
#define PCLS8Name       "Fixed Pitch 5291 Roman"
#define PCLS9Name       "Fixed Pitch 5291 Bold"

#define PCLS10Name      "Fixed Pitch 4409 Universal"
#define PCLS11Name      "Fixed Pitch 4409 Roman"
#define PCLS12Name      "Fixed Pitch 4409 Bold"

#define TTS1Name        "Unicode Universal"

#define PCL1Name        "CG-Times"
#define PCL2Name        "CG-Times Italic"
#define PCL3Name        "CG-Times Bold"
#define PCL4Name        "CG-Times Bold Italic"

#define PCL5Name        "Univers"
#define PCL6Name        "Univers Italic"
#define PCL7Name        "Univers Bold"
#define PCL8Name        "Univers Bold Italic"

#define PCL9Name        "Univers Condensed"
#define PCL10Name       "Univers Condensed Italic"
#define PCL11Name       "Univers Condensed Bold"
#define PCL12Name       "Univers Condensed Bold Italic"

#define PCL13Name       "ITC Zapf Dingbats"

#define PCL14Name       "ITC Souvenir Light"
#define PCL15Name       "ITC Souvenir Light Italic"
#define PCL16Name       "ITC Souvenir Demi"
#define PCL17Name       "ITC Souvenir Demi Italic"

#define PCL18Name       "CG Palacio"
#define PCL19Name       "CG Palacio Italic"
#define PCL20Name       "CG Palacio Bold"
#define PCL21Name       "CG Palacio Bold Italic"

#define PCL22Name       "Antique Olive"
#define PCL23Name       "Antique Olive Italic"
#define PCL24Name       "Antique Olive Bold"
#define PCL25Name       "Antique Olive Bold Italic"

#define PCL26Name       "CG Century Schoolbook"
#define PCL27Name       "CG Century Schoolbook Italic"
#define PCL28Name       "CG Century Schoolbook Bold"
#define PCL29Name       "CG Century Schoolbook Bold Italic"

#define PCL30Name       "Stymie"
#define PCL31Name       "Stymie Italic"
#define PCL32Name       "Stymie Bold"
#define PCL33Name       "Stymie Bold Italic"

#define PCL34Name       "CG Omega"
#define PCL35Name       "CG Omega Italic"
#define PCL36Name       "CG Omega Bold"
#define PCL37Name       "CG Omega Bold Italic"

#define PCL38Name       "CG Bodoni"
#define PCL39Name       "CG Bodoni Italic"
#define PCL40Name       "CG Bodoni Bold"
#define PCL41Name       "CG Bodoni Bold Italic"

#define PCL42Name       "ITC Benguiat"
#define PCL43Name       "ITC Benguiat Italic"
#define PCL44Name       "ITC Benguiat Bold"
#define PCL45Name       "ITC Benguiat Bold Italic"

#define PCL46Name       "ITC Bookman Light"
#define PCL47Name       "ITC Bookman Light Italic"
#define PCL48Name       "ITC Bookman Demi"
#define PCL49Name       "ITC Bookman Demi Italic"

#define PCL50Name       "Garamond"
#define PCL51Name       "Garamond Italic"
#define PCL52Name       "Garamond Bold"
#define PCL53Name       "Garamond Bold Italic"

#define PCL54Name       "Shannon"
#define PCL55Name       "Shannon Italic"
#define PCL56Name       "Shannon Bold"
#define PCL57Name       "Shannon Bold Italic"

#define PCL58Name       "Revue Light"

#define PCL59Name       "Cooper Black"

#define PCL60Name       "Courier Medium"
#define PCL61Name       "Courier Bold"
#define PCL62Name       "Courier Italic"
#define PCL63Name       "Courier Bold Italic"

#define PCL64Name       "Letter Gothic Regular"
#define PCL65Name       "Letter Gothic Bold"
#define PCL66Name       "Letter Gothic Italic"

#define PCL67Name       "Albertus Medium"
#define PCL68Name       "Albertus Extrabold"

#define PCL69Name       "Clarendon Condensed"

#define PCL70Name       "Coronet"

#define PCL71Name       "Marigold"

#define PS1Name         "Times-Roman"
#define PS2Name         "Times-Italic"
#define PS3Name         "Times-Bold"
#define PS4Name         "Times-BoldItalic"

#define PS5Name         "Helvetica"
#define PS6Name         "Helvetica-Oblique"
#define PS7Name         "Helvetica-Bold"
#define PS8Name         "Helvetica-BoldOblique"

#define PS9Name         "Courier"
#define PS10Name        "Courier-Oblique"
#define PS11Name        "Courier-Bold"
#define PS12Name        "Courier-BoldOblique"

#define PS13Name        "Symbol"

#define PS14Name        "Palatino-Roman"
#define PS15Name        "Palatino-Italic"
#define PS16Name        "Palatino-Bold"
#define PS17Name        "Palatino-BoldItalic"

#define PS18Name        "Bookman-Light"
#define PS19Name        "Bookman-LightItalic"
#define PS20Name        "Bookman-Demi"
#define PS21Name        "Bookman-DemiItalic"

#define PS22Name        "Helvetica-Narrow"
#define PS23Name        "Helvetica-Narrow-Oblique"
#define PS24Name        "Helvetica-Narrow-Bold"
#define PS25Name        "Helvetica-Narrow-BoldOblique"

#define PS26Name        "NewCenturySchlbk-Roman"
#define PS27Name        "NewCenturySchlbk-Italic"
#define PS28Name        "NewCenturySchlbk-Bold"
#define PS29Name        "NewCenturySchlbk-BoldItalic"

#define PS30Name        "AvantGarde-Book"
#define PS31Name        "AvantGarde-BookOblique"
#define PS32Name        "AvantGarde-Demi"
#define PS33Name        "AvantGarde-DemiOblique"

#define PS34Name        "ZapfChancery-MediumItalic"

#define PS35Name        "ZapfDingbats"

#define PS36Name        "Helvetica-Condensed"
#define PS37Name        "Helvetica-Condensed-Oblique"
#define PS38Name        "Helvetica-Condensed-Bold"
#define PS39Name        "Helvetica-Condensed-BoldObl"

#define TT1Name         "Arial"
#define TT2Name         "Arial Italic"
#define TT3Name         "Arial Bold"
#define TT4Name         "Arial Bold Italic"

#define TT5Name         "Times New Roman"
#define TT6Name         "Times New Roman Italic"
#define TT7Name         "Times New Roman Bold"
#define TT8Name         "Times New Roman Bold Italic"

#if INCL_MFT
#define PS1SName	"Times Skeletal"
#define PS2SName	"Times Skeletal Italic"
#define PS3SName	"Times Skeletal Bold"
#define PS4SName	"Times Skeletal Bold Italic"

#define PS5SName	"HelveticaSkeletal"
#define PS6SName	"HelveticaSkeletal-Oblique"
#define PS7SName	"HelveticaSkeletal-Bold"
#define PS8SName	"HelveticaSkeletal-BoldObl"

#define PS9SName	"CourierSkeletal"
#define PS10SName	"CourierSkeletal-Oblique"
#define PS11SName	"CourierSkeletal-Bold"
#define PS12SName	"CourierSkeletal-BoldObl"

#define PS13SName	"SymbolSkeletal"

#define TT5SName        "Times Skeletal Roman"
#define TT6SName        "Times Skeletal Roman Italic"
#define TT7SName        "Times Skeletal Roman Bold"
#define TT8SName        "Times Skeletal Roman Bold Italic"
#endif

#define TT9Name         "Courier New"
#define TT10Name        "Courier New Italic"
#define TT11Name        "Courier New Bold"
#define TT12Name        "Courier New Bold Italic"

#define TT13Name        "Symbol"

#define TT14Name        "Wingdings"

#define PseudoPCL23Name         "Pseudo Ant Ol It"

#define PseudoPCL10Name         "Pseudo Unvrs Cd It"
#define PseudoPCL12Name         "Pseudo Unvrs Cd Bd It"

#define PseudoPCL35Name         "Pseudo CG Omg It"
#define PseudoPCL37Name         "Pseudo CG Omg Bd It"

#define PseudoPCL51Name         "Pseudo Grmnd It"
#define PseudoPCL53Name         "Pseudo Grmnd Bd It"

#define PseudoPCL2Name          "Pseudo CG-Tms It"
#define PseudoPCL4Name          "Pseudo CG-Tms Bd It"

#define PseudoPCL6Name          "Pseudo Unvrs It"
#define PseudoPCL8Name          "Pseudo Unvrs Bd It"

#define PseudoPCL61Name         "Drawn Cour Bd"
#define PseudoPCL63Name         "Drawn Cour Bd It"

#define PseudoPCL66Name         "Pseudo Lt Goth It"

#define PseudoTT2Name           "Pseudo Arl It"
#define PseudoTT4Name           "Pseudo Arl Bd It"

#define PseudoTT6Name           "Pseudo Tms Nw It"
#define PseudoTT8Name           "Pseudo Tms Nw Bd It"

#define PseudoPCL3230Name	"Pseudo Bd 5291"

#define PseudoPS11Name          "Drawn PS Cour Bd"
                                            
#define PseudoLt1Name           "5291 Light Roman"
#define PseudoLt2Name           "Courier Light"
#define PseudoLt3Name           "Courier Light Italic"
#define PseudoLt4Name           "5291 Light Bold"
#define PseudoLt5Name           "5291 Light pseudo/Bold"
#define PseudoLt6Name           "Courier Light pseudo/Bold"
#define PseudoLt7Name           "Courier Light Bold"
#define PseudoLt8Name           "Courier Light pseudo/(Bold Italic)"
#define PseudoLt9Name           "Courier Light Bold Italic"

FontInfoType gPseudoFontInfoTable[] =
{
    /* [font 13180] Incised901 SWC -> PSEUDO Antique Olive Italic */
    {PseudoPCL23Name, PCL23Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13184] Swiss742 Cn SWC -> PSEUDO Univers Condensed Italic */
    {PseudoPCL10Name, PCL10Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13186] Swiss742 Cn SWC -> PSEUDO Univers Condensed Bold Italic */
    {PseudoPCL12Name, PCL12Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13192] ZapfHumst Dm SWC -> PSEUDO CG Omega Italic */
    {PseudoPCL35Name, PCL35Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13194] ZapfHumst Dm SWC -> PSEUDO CG Omega Bold Italic */
    {PseudoPCL37Name, PCL37Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13204] OrigGaramond SWC -> PSEUDO Garamond Italic */
    {PseudoPCL51Name, PCL51Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13206] OrigGaramond SWC -> PSEUDO Garamond Bold Italic */
    {PseudoPCL53Name, PCL53Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13155] Swiss742 SWC -> PSEUDO Univers Italic */
    {PseudoPCL6Name, PCL6Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13157] Swiss742 SWC -> PSEUDO Univers Bold Italic */
    {PseudoPCL8Name, PCL8Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 23230] -> PSEUDO-BOLD Fixed Pitch 5291 support */
    {PseudoPCL3230Name, PCLS9Hdr, pdlSupport, NULLCHARPTR, UP_FOUR, RES_NATIVE},
    /* [font 13221]  -> DRAWN Courier Bold */
    {PseudoPCL61Name, PCL61Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13223]  -> DRAWN Courier Bold Italic */
    {PseudoPCL63Name, PCL63Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 13226]  -> PSEUDO Letter Gothic Italic */
    {PseudoPCL66Name, PCL66Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 23010.spd] Courier SWA -> DRAWN Courier Bold */
    {PseudoPS11Name, PS11Hdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font 33229.spd] PSEUDO-LIGHT Fixed Pitch 5291 Roman */
    {PseudoLt1Name, PCLS8Hdr, pdlSupport, NULLCHARPTR, UP_NINE, RES_NATIVE},
    /* [font 33220.spd] PSEUDO-LIGHT Courier Medium */
    {PseudoLt2Name, PCL60LHdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 33222.spd] PSEUDO-LIGHT Courier pseudo/Italic */
    {PseudoLt3Name, PCL62LHdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 33230.spd] PSUEDO-LIGHT Fixed Pitch 5291 Bold */
    {PseudoLt4Name, PCLS9Hdr, pdlSupport, NULLCHARPTR, UP_FOUR, RES_NATIVE},
    /* [font 43230.spd] PSUEDO-LIGHT Fixed Pitch 5291 pseudo/Bold */
    {PseudoLt5Name, PCLS9Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 43221.spd] PSEUDO-LIGHT Courier pseudo/Bold */
    {PseudoLt6Name, PCL61LHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 33221.spd] PSEUDO-LIGHT Courier Bold */
    {PseudoLt7Name, PCL61LHdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 43223.spd] PSEUDO_LIGHT Courier pseudo/(Bold Italic) */
    {PseudoLt8Name, PCL63LHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font 33223.spd] PSEUDO_LIGHT Courier Bold Italic */
    {PseudoLt9Name, PCL63LHdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE}
};


FontInfoType gSrcFontInfoTable[] =
{
/*------------ Auxilliary or Background font support ----------------*/
    /* [font3162.spd]  -> Line Printer */
    {PCLS1Name, PCLS1Hdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3158.spd]  -> Plugin Roman Serif */
    {PCLS2Name, PCLS2Hdr, pdlSupport, NULLCHARPTR, BUCKET, RES_NATIVE},
    /* [font3159.spd]  -> Plugin Roman Serif Bold */
    {PCLS3Name, PCLS3Hdr, pdlSupport, NULLCHARPTR, BUCKET, RES_NATIVE},
    /* [font3160.spd]  -> Plugin Sanserif */
    {PCLS4Name, PCLS4Hdr, pdlSupport, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3161.spd]  -> Plugin Sanserif Bold */
    {PCLS5Name, PCLS5Hdr, pdlSupport, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
#ifdef HAVE_STICK_FONT
    /* [font????.spd]  -> HP Stick Font */
    {PCLS6Name, PCLS6Hdr, pdlSupport, NULLCHARPTR, BUCKET, RES_NATIVE},
#endif

/*------------ HP PCL emulators ----------------*/
    /* [font3150.spd] Dutch801 SWC -> CG-Times */
    {PCL1Name, PCL1Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3151.spd] Dutch801 SWC -> CG-Times Italic */
    {PCL2Name, PCL2Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3152.spd] Dutch801 SWC -> CG-Times Bold */
    {PCL3Name, PCL3Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3153.spd] Dutch801 SWC -> CG-Times Bold Italic */
    {PCL4Name, PCL4Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3154.spd] Swiss742 SWC -> Univers */
    {PCL5Name, PCL5Hdr, pdlPCL, NULLCHARPTR, SANS, RES_NATIVE},
    /* [font3155.spd] Swiss742 SWC -> Univers Italic */
    {PCL6Name, PCL6Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3156.spd] Swiss742 SWC -> Univers Bold */
    {PCL7Name, PCL7Hdr, pdlPCL, NULLCHARPTR, SANS_BOLD, RES_NATIVE},
    /* [font3157.spd] Swiss742 SWC -> Univers Bold Italic */
    {PCL8Name, PCL8Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3183.spd] Swiss742 Cn SWC -> Univers Condensed */
    {PCL9Name, PCL9Hdr, pdlPCL, NULLCHARPTR, SANS, RES_NATIVE},
    /* [font3184.spd] Swiss742 Cn SWC -> Univers Condensed Italic */
    {PCL10Name, PCL10Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3185.spd] Swiss742 Cn SWC -> Univers Condensed Bold */
    {PCL11Name, PCL11Hdr, pdlPCL, NULLCHARPTR, SANS_BOLD, RES_NATIVE},
    /* [font3186.spd] Swiss742 Cn SWC -> Univers Condensed Bold Italic */
    {PCL12Name, PCL12Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3213.spd] ZapfDingbats SWC -> ITC Zapf Dingbats */
    {PCL13Name, PCL13Hdr, pdlPCL, NULLCHARPTR, BUCKET, RES_NATIVE},

    /* [font3163.spd] Souvenir SWC -> ITC Souvenir Light */
    {PCL14Name, PCL14Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3164.spd] Souvenir SWC -> ITC Souvenir Light Italic */
    {PCL15Name, PCL15Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3165.spd] Souvenir SWC -> ITC Souvenir Demi */
    {PCL16Name, PCL16Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3166.spd] Souvenir SWC -> ITC Souvenir Demi Italic */
    {PCL17Name, PCL17Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3167.spd] ZapfCallig SWC -> CG Palacio */
    {PCL18Name, PCL18Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3168.spd] ZapfCallig SWC -> CG Palacio Italic */
    {PCL19Name, PCL19Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3169.spd] ZapfCallig SWC -> CG Palacio Bold */
    {PCL20Name, PCL20Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3170.spd] ZapfCallig SWC -> CG Palacio Bold Italic */
    {PCL21Name, PCL21Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3179.spd] Incised901 SWC -> Antique Olive */
    {PCL22Name, PCL22Hdr, pdlPCL, NULLCHARPTR, SANS, RES_NATIVE},
    /* [font3180.spd] Incised901 SWC -> Antique Olive Italic */
    {PCL23Name, PCL23Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3181.spd] Incised901 SWC -> Antique Olive Bold */
    {PCL24Name, PCL24Hdr, pdlPCL, NULLCHARPTR, SANS_BOLD, RES_NATIVE},
    /* [font3182.spd] Incised901Ct SWC -> Antique Olive Bold Italic */
    {PCL25Name, PCL25Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3175.spd] CenturySchbk SWC -> CG Century Schoolbook */
    {PCL26Name, PCL26Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3176.spd] CenturySchbk SWC -> CG Century Schoolbook Italic */
    {PCL27Name, PCL27Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3177.spd] CenturySchbk SWC -> CG Century Schoolbook Bold */
    {PCL28Name, PCL28Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3178.spd] CenturySchbk SWC -> CG Century Schoolbook Bold Italic */
    {PCL29Name, PCL29Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3187.spd] Stymie SWC -> Stymie */
    {PCL30Name, PCL30Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3188.spd] Stymie SWC -> Stymie Italic */
    {PCL31Name, PCL31Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3189.spd] Stymie SWC -> Stymie Bold */
    {PCL32Name, PCL32Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3190.spd] Stymie SWC -> Stymie Bold Italic */
    {PCL33Name, PCL33Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3191.spd] ZapfHumst Dm SWC -> CG Omega */
    {PCL34Name, PCL34Hdr, pdlPCL, NULLCHARPTR, 	SANS, RES_NATIVE},
    /* [font3192.spd] ZapfHumst Dm SWC -> CG Omega Italic */
    {PCL35Name, PCL35Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3193.spd] ZapfHumst Dm SWC -> CG Omega Bold */
    {PCL36Name, PCL36Hdr, pdlPCL, NULLCHARPTR, SANS_BOLD, RES_NATIVE},
    /* [font3194.spd] ZapfHumst Dm SWC -> CG Omega Bold Italic */
    {PCL37Name, PCL37Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3195.spd] Bodoni Bk SWC -> CG Bodoni */
    {PCL38Name, PCL38Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3196.spd] Bodoni Bk SWC -> CG Bodoni Italic */
    {PCL39Name, PCL39Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3197.spd] Bodoni Bk SWC -> CG Bodoni Bold */
    {PCL40Name, PCL40Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3198.spd] Bodoni Bk SWC -> CG Bodoni Bold Italic */
    {PCL41Name, PCL41Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3199.spd] Benguiat SWC -> ITC Benguiat */
    {PCL42Name, PCL42Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3200.spd] Benguiat SWC -> ITC Benguiat Italic */
    {PCL43Name, PCL43Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3201.spd] Benguiat SWC -> ITC Benguiat Bold */
    {PCL44Name, PCL44Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3202.spd] Benguiat SWC -> ITC Benguiat Bold Italic */
    {PCL45Name, PCL45Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3171.spd] Bookman SWC -> ITC Bookman Light */
    {PCL46Name, PCL46Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3172.spd] Bookman SWC -> ITC Bookman Light Italic */
    {PCL47Name, PCL47Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3173.spd] Bookman SWC -> ITC Bookman Demi */
    {PCL48Name, PCL48Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3174.spd] Bookman SWC -> ITC Bookman Demi Italic */
    {PCL49Name, PCL49Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3203.spd] OrigGaramond SWC -> Garamond */
    {PCL50Name, PCL50Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3204.spd] OrigGaramond SWC -> Garamond Italic */
    {PCL51Name, PCL51Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3205.spd] OrigGaramond SWC -> Garamond Bold */
    {PCL52Name, PCL52Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3206.spd] OrigGaramond SWC -> Garamond Bold Italic */
    {PCL53Name, PCL53Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3209.spd] Chianti SWC -> Shannon */
    {PCL54Name, PCL54Hdr, pdlPCL, NULLCHARPTR, SANS, RES_NATIVE},
    /* [font3210.spd] Chianti SWC -> Shannon Italic */
    {PCL55Name, PCL55Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3211.spd] Chianti SWC -> Shannon Bold */
    {PCL56Name, PCL56Hdr, pdlPCL, NULLCHARPTR, SANS_BOLD, RES_NATIVE},
    /* [font3212.spd] Chianti XBd SWC -> Shannon Bold Italic */
    {PCL57Name, PCL57Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3208.spd] Revue Lt SWC -> Revue Light */
    {PCL58Name, PCL58Hdr, pdlPCL, NULLCHARPTR, SANS, RES_NATIVE},
    /* [font3207.spd] Cooper Blk SWC -> Cooper Black */
    {PCL59Name, PCL59Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},

	/* Fixed Pitch 5291 Roman Support */
    /* [font3227.spd]  -> Fixed Pitch 5291 Universal */
    {PCLS7Name, PCLS7Hdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3229.spd]  -> Fixed Pitch 5291 Roman */
    {PCLS8Name, PCLS8Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3220.spd]  -> Courier Medium */
    {PCL60Name, PCL60Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3222.spd]  -> Courier Italic */
    {PCL62Name, PCL62Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

	/* Fixed Pitch 5291 Bold support: */
    /* [font3230.spd]  -> Fixed Pitch 5291 Bold */
    {PCLS9Name, PCLS9Hdr, pdlSupport, NULLCHARPTR, UP_THREE, RES_NATIVE},
    /* [font3221.spd]  -> Courier Bold */
    {PCL61Name, PCL61Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3223.spd]  -> Courier Bold Italic */
    {PCL63Name, PCL63Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},


	/* Fixed Pitch 4409 Roman Support */
    /* [font3228.spd]  -> Fixed Pitch 4409 Universal */
    {PCLS10Name, PCLS10Hdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3231.spd]  -> Fixed Pitch 4409 Roman */
    {PCLS11Name, PCLS11Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3224.spd]  -> Letter Gothic Regular */
    {PCL64Name, PCL64Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3225.spd]  -> Letter Gothic Italic */
    {PCL66Name, PCL66Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

	/* Fixed Pitch 4409 Bold support: */
    /* [font3232.spd]  -> Fixed Pitch 4409 Bold */
    {PCLS12Name, PCLS12Hdr, pdlSupport, NULLCHARPTR, UP_FOUR, RES_NATIVE},
    /* [font3226.spd]  -> Letter Gothic Bold */
    {PCL65Name, PCL65Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
#ifdef PHASE1_FONTS
    /* [font3215.spd]  -> Albertus Medium */
    {PCL67Name, PCL67Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3216.spd]  -> Albertus Extrabold */
    {PCL68Name, PCL68Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
#else
    /* [font3215.spd]  -> Albertus Medium */
    {PCL67Name, PCL67Hdr, pdlPCL, NULLCHARPTR, SANS, RES_NATIVE},
    /* [font3216.spd]  -> Albertus Extrabold */
    {PCL68Name, PCL68Hdr, pdlPCL, NULLCHARPTR, SANS_BOLD, RES_NATIVE},
#endif
    /* [font3214.spd]  -> Clarendon Condensed */
    {PCL69Name, PCL69Hdr, pdlPCL, NULLCHARPTR, SERIF_BOLD, RES_NATIVE},
    /* [font3217.spd]  -> Coronet */
    {PCL70Name, PCL70Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},
    /* [font3218.spd]  -> Marigold */
    {PCL71Name, PCL71Hdr, pdlPCL, NULLCHARPTR, SERIF, RES_NATIVE},

/*------------ PostScript 13 and 35 ----------------*/
#if INCL_MFT
    /* [font3150.spd] Dutch SWC -> CG Times */
    {PS1SName, PS1MFTHdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_1000},
    /* [font3000.spd] Dutch SWA -> Times */
    {PS1Name, PS1Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3151.spd] Dutch SWC -> CG Times */
    {PS2SName, PS2MFTHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_1000},
    /* [font3001.spd] Dutch SWA -> Times Italic */
    {PS2Name, PS2Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3152.spd] Dutch SWC -> CG Times */
    {PS3SName, PS3MFTHdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_1000},
    /* [font3002.spd] Dutch SWA -> Times Bold */
    {PS3Name, PS3Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3153.spd] Dutch SWC -> CG Times */
    {PS4SName, PS4MFTHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_1000},
    /* [font3003.spd] Dutch SWA -> Times Bold Italic */
    {PS4Name, PS4Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},

    {PS5SName, PS5MFTHdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_1000},
    /* [font3004.spd] Swiss SWA -> Helvetica */
    {PS5Name, PS5Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3239.spd] Swiss SWM -> Arial */
    {PS6SName, PS6MFTHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_1000},
    /* [font3005.spd] Swiss SWA -> Helvetica Italic */
    {PS6Name, PS6Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3240.spd] Swiss SWM -> Arial */
    {PS7SName, PS7MFTHdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_1000},
    /* [font3006.spd] Swiss SWA -> Helvetica Bold */
    {PS7Name, PS7Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3241.spd] Swiss SWM -> Arial */
    {PS8SName, PS8MFTHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_1000},
    /* [font3007.spd] Swiss SWA -> Helvetica Bold Italic */
    {PS8Name, PS8Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3220.spd] Courier 810 SWC -> Courier */
    {PS9SName, PS9MFTHdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_1000},
    /* [font3008.spd] Courier SWA -> Courier */
    {PS9Name, PS9Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3222.spd] Courier 810 SWC -> Courier */
    {PS10SName, PS10MFTHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_1000},
    /* [font3009.spd] Courier SWA -> Courier Italic */
    {PS10Name, PS10Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3221.spd] Courier 810 SWC -> Courier */
    {PS11SName, PS11MFTHdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_1000},
    /* [font3010.spd] Courier SWA -> Courier Bold */
    {PS11Name, PS11Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3223.spd] Courier 810 SWC -> Courier */
    {PS12SName, PS12MFTHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_1000},
    /* [font3011.spd] Courier SWA -> Courier Bold Italic */
    {PS12Name, PS12Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3245.spd] Symbol Set SWM -> Symbol */
    {PS13SName, PS13MFTHdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_1000},
    /* [font3012.spd] Symbol Set SWA -> Symbol */
    {PS13Name, PS13Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
#else
    /* [font3000.spd] Dutch SWA -> Times */
    {PS1Name, PS1Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3001.spd] Dutch SWA -> Times Italic */
    {PS2Name, PS2Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3002.spd] Dutch SWA -> Times Bold */
    {PS3Name, PS3Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3003.spd] Dutch SWA -> Times Bold Italic */
    {PS4Name, PS4Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3004.spd] Swiss SWA -> Helvetica */
    {PS5Name, PS5Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3005.spd] Swiss SWA -> Helvetica Italic */
    {PS6Name, PS6Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3006.spd] Swiss SWA -> Helvetica Bold */
    {PS7Name, PS7Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3007.spd] Swiss SWA -> Helvetica Bold Italic */
    {PS8Name, PS8Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3008.spd] Courier SWA -> Courier */
    {PS9Name, PS9Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3009.spd] Courier SWA -> Courier Italic */
    {PS10Name, PS10Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3010.spd] Courier SWA -> Courier Bold */
    {PS11Name, PS11Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3011.spd] Courier SWA -> Courier Bold Italic */
    {PS12Name, PS12Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3012.spd] Symbol Set SWA -> Symbol */
    {PS13Name, PS13Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
#endif /* INCL_MFT */
    /* [font3013.spd] ZapfCallig SWA -> Palatino */
    {PS14Name, PS14Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3014.spd] ZapfCallig SWA -> Palatino Italic */
    {PS15Name, PS15Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3015.spd] ZapfCallig SWA -> Palatino Bold */
    {PS16Name, PS16Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3016.spd] ZapfCallig SWA -> Palatino Bold Italic */
    {PS17Name, PS17Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3021.spd] Bookman SWA -> ITC Bookman */
    {PS18Name, PS18Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3022.spd] Bookman SWA -> ITC Bookman Italic */
    {PS19Name, PS19Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3023.spd] Bookman SWA -> ITC Bookman Bold */
    {PS20Name, PS20Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3024.spd] Bookman SWA -> ITC Bookman Bold Italic */
    {PS21Name, PS21Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3025.spd] SwissNarrow SWA -> Helvetica Narrow */
    {PS22Name, PS22Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3026.spd] SwissNarrow SWA -> Helvetica Narrow Italic */
    {PS23Name, PS23Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3027.spd] SwissNarrow SWA -> Helvetica Narrow Bold */
    {PS24Name, PS24Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3028.spd] SwissNarrow SWA -> Helvetica Narrow Bold Italic */
    {PS25Name, PS25Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3031.spd] CenturySchbk SWA -> Century Schoolbook */
    {PS26Name, PS26Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3032.spd] CenturySchbk SWA -> Century Schoolbook Italic */
    {PS27Name, PS27Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3033.spd] CenturySchbk SWA -> Century Schoolbook Bold */
    {PS28Name, PS28Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3034.spd] CenturySchbk SWA -> Century Schoolbook Bold Italic */
    {PS29Name, PS29Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3017.spd] AvantGarde SWA -> ITC Avant Garde */
    {PS30Name, PS30Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3018.spd] AvantGarde SWA -> ITC Avant Garde Italic */
    {PS31Name, PS31Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3019.spd] AvantGarde SWA -> ITC Avant Garde Bold */
    {PS32Name, PS32Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3020.spd] AvantGarde SWA -> ITC Avant Garde Bold Italic */
    {PS33Name, PS33Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3029.spd] ZapfChancery SWA -> ITC Zapf Chancery */
    {PS34Name, PS34Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3030.spd] ZapfDingbats SWA -> ITC Zapf Dingbats */
    {PS35Name, PS35Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3069.spd] Swiss Condensed SWA -> Helvetica-Condensed */
    {PS36Name, PS36Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3070.spd] Swiss Condensed SWA -> Helvetica-Condensed-Oblique */
    {PS37Name, PS37Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3071.spd] Swiss Condensed SWA -> Helvetica-Condensed-Bold */
    {PS38Name, PS38Hdr, pdlPostScript, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3072.spd] Swiss Condensed SWA -> Helvetica-Condensed-BoldObl */
    {PS39Name, PS39Hdr, pdlPostScript, NULLCHARPTR, UP_ONE, RES_NATIVE},

/*------------ TrueType fonts ----------------*/
    /* [font3238.spd] Swiss SWM -> Arial */
    {TT1Name, TT1Hdr, pdlGDI, NULLCHARPTR, DOWN_FOUR, RES_NATIVE},
    /* [font3239.spd] Swiss SWM -> Arial Italic */
    {TT2Name, TT2Hdr, pdlGDI, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3240.spd] Swiss SWM -> Arial Bold */
    {TT3Name, TT3Hdr, pdlGDI, NULLCHARPTR, DOWN_TWO, RES_NATIVE},
    /* [font3241.spd] Swiss SWM -> Arial Bold Italic */
    {TT4Name, TT4Hdr, pdlGDI, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3233.spd] Universal Support SWMs */
    {TTS1Name, TTS1Hdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_NATIVE}, /* the buck stops here */

#if INCL_MFT
    /* [font3150.spd] Dutch SWC -> Times New Roman */
    {TT5SName, TT5MFTHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_2048},
    /* Skeletal [font3234.spd] TFS support: */
    {TT5Name, TT5Hdr, pdlGDI, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3151.spd] Dutch SWC -> Times New Roman Italic: */
    {TT6SName, TT6MFTHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_2048},
    /* Skeletal [font3235.spd] TFS support */
    {TT6Name, TT6Hdr, pdlGDI, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3152.spd] Dutch SWC -> Times New Roman Bold: */
    {TT7SName, TT7MFTHdr, pdlSupport, NULLCHARPTR, UP_FIVE, RES_2048},
    /* Skeletal [font3236.spd] TFS support */
    {TT7Name, TT7Hdr, pdlGDI, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3153.spd] Dutch SWM -> Times New Roman Bold Italic */
    {TT8SName, TT8MFTHdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_2048},
    /* Skeletal [font3237.spd] TFS support */
    {TT8Name, TT8Hdr, pdlGDI, NULLCHARPTR, UP_ONE, RES_NATIVE},
#else
    /* [font3234.spd] Dutch SWM -> Times New Roman */
    {TT5Name, TT5Hdr, pdlGDI, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3235.spd] Dutch SWM -> Times New Roman Italic */
    {TT6Name, TT6Hdr, pdlGDI, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3236.spd] Dutch SWM -> Times New Roman Bold */
    {TT7Name, TT7Hdr, pdlGDI, NULLCHARPTR, UP_THREE, RES_NATIVE},
    /* [font3237.spd] Dutch SWM -> Times New Roman Bold Italic */
    {TT8Name, TT8Hdr, pdlGDI, NULLCHARPTR, UP_ONE, RES_NATIVE},
#endif


	/* Fixed Pitch 5291 Support */
    /* [font3227.spd]  -> Fixed Pitch 5291 Universal */
    {PCLS7Name, PCLS7Hdr, pdlSupport, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3229.spd]  -> Fixed Pitch 5291 Roman */
    {PCLS8Name, PCLS8Hdr, pdlSupport, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3220.spd]  -> Courier Medium */
    {PCL60Name, PCL60Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3221.spd]  -> Courier Italic */
    {PCL62Name, PCL62Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

	/* Fixed Pitch 5291 Bold Support: */
    /* [font3230.spd]  -> Fixed Pitch 5291 Bold */
    {PCLS9Name, PCLS9Hdr, pdlSupport, NULLCHARPTR, UP_FOUR, RES_NATIVE},
    /* [font3222.spd]  -> Courier Bold */
    {PCL61Name, PCL61Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},
    /* [font3223.spd]  -> Courier Bold Italic */
    {PCL63Name, PCL63Hdr, pdlPCL, NULLCHARPTR, UP_ONE, RES_NATIVE},

    /* [font3246.spd] Symbol Set SWM -> Symbol */
    {TT13Name, TT13Hdr, pdlGDI, NULLCHARPTR, NEXT_NONE, RES_NATIVE},
    /* [font3219.spd] More Wingbats SWC -> Wingdings */
    {TT14Name, TT14Hdr, pdlGDI, NULLCHARPTR, NEXT_NONE, RES_NATIVE}
};

#define NUM_LOG_FONTS ((sizeof gSrcFontInfoTable) / sizeof(struct FontInfoType_t))

FontInfoType gFontInfoTable[NUM_LOG_FONTS];



/* possible ItemRet codes for the Menu Items: */
enum
{
    NO_ITEM_SEL = -1, 
#if PCL5 || PCL5e
    ITEM_LJIII,
    ITEM_LJIIISI,
    ITEM_HPCART1,
    ITEM_HPCART2,
    ITEM_HPCART3,
    ITEM_TRUETYPE14,
    ITEM_POSTSCRIPT13,
    ITEM_POSTSCRIPT35,
    ITEM_POSTSCRIPT13andTRUETYPE14,
    ITEM_POSTSCRIPT35andTRUETYPE14,
    ITEM_POSTSCRIPT39,
    ITEM_LJ4L,
    ITEM_LJ4,
    ITEM_COMBINED_LJ4_PS35,
    ITEM_LJ5L,
#endif
#if PCL6
    ITEM_LJ5,
    ITEM_COMBINED_LJ5_PS35,
#if 0
    ITEM_LJ6L,
#endif
#endif
    ITEM_MAXITEMS    /* terminator, insert new items before */
};

/* current default file name, set based on menu choice: */
char gOutFileDefault[32];

/* current file name, set from default, or by gets(): */
char gCurFileName[128];

/* number of items in the Menu: */
int gNMenuItems = ITEM_MAXITEMS+1;

/* structure definition of the Menu: */
typedef struct
{
    char   *ItemStr;
    int     ItemRet;
}MenuType, *MenuTypePtr;

/* the Menu declaration itself: */
MenuType gTheMenu[] =
{
#if PCL5 || PCL5e
    {" 1) HP LaserJet III Compatible Core   (13 fonts)", ITEM_LJIII},
    {" 2) HP LaserJet IIIsi Compatible Core (18 fonts)", ITEM_LJIIISI},
    {" 3) HP Cartridge 1                    (25 fonts)", ITEM_HPCART1},
    {" 4) HP Cartridge 2                    (26 fonts)", ITEM_HPCART2},
    {" 5) HP Cartridge 3                    (51 fonts)", ITEM_HPCART3},

    {" 6) TrueType   14                     (15 fonts)", ITEM_TRUETYPE14},

    {" 7) PostScript 13                     ( 9 fonts)", ITEM_POSTSCRIPT13},
    {" 8) PostScript 35                     (25 fonts)", ITEM_POSTSCRIPT35},

    {" 9) PostScript 13 & TrueType 14       (28 fonts)", ITEM_POSTSCRIPT13andTRUETYPE14},
    {"10) PostScript 35 & TrueType 14       (40 fonts)", ITEM_POSTSCRIPT35andTRUETYPE14},
    {"11) PostScript 39                     (27 fonts)", ITEM_POSTSCRIPT39},

    {"12) HP LJ4L 26 Set                    (37 fonts)", ITEM_LJ4L},
    {"13) HP LJ4  45 Set                    (70 fonts)", ITEM_LJ4},
    {"14) HP LJ4  45 & PostScript 35        (96 fonts)", ITEM_COMBINED_LJ4_PS35},

    {"15) HP LJ5L 26 Set                    (37 fonts)", ITEM_LJ5L},
                 
    {"16) Quit",                                         NO_ITEM_SEL}
#endif

#if PCL6
    {" 1) HP LJ5  45 Set                    (78 fonts)", ITEM_LJ5},
    {" 2) HP LJ5  45 & PostScript 35       (104 fonts)", ITEM_COMBINED_LJ5_PS35},

#if 0
    {" 3) HP LJ6L 26 Set                    (55 fonts)", ITEM_LJ6L},
#endif

    {" 3) Quit",                                         NO_ITEM_SEL}
#endif
};

/* default output file names for each non-quit choice in the Menu: */
char *gDefaultFileNames[] =
{
#if PCL5 || PCL5e
    "hplj3.bin",
    "hplj3si.bin",
    "hpcart1.bin",
    "hpcart2.bin",
    "hpcart3.bin",
    "tt14.bin",
    "ps13.bin",
    "ps35.bin",
    "ps13-tt14.bin",
    "ps35-tt14.bin",
    "ps39.bin",
    "hplj4l.bin",
    "hplj4.bin",
    "hplj4m.bin",
    "hplj5l.bin",
#endif
#if PCL6
    "hplj5.bin",
    "hplj5m.bin",

#if 0
    "hplj6l.bin",
#endif

#endif
    NULLCHARPTR    /* terminator, insert new ones before */
};

/* for suppressing the character complements on-the-fly for
	certain fonts (Core fonts and Cartridge fonts) in specific
	situtations where their compelements cannot be extended
	although this software may have been built with them
	extended by defines in pclhdrs.h: */
char gSuppressComplement = FALSE;

/* this boolean is used to extend the complement of the 4L
	fonts if that item is selected from the menu: */
char gExtendComplement = FALSE;

/* this boolean is used to dynamically insert TrueDoc pseudoItalic font entries
	in TrueDoc Compressed PFRS  */
char gCompressedPFR = FALSE;


/************************************************************************
    The main program.
    Process menu choices, writing out files, until finished.
************************************************************************/
FUNCTION int main(int argc, char *argv[])
{
int choice, nFonts;
char done = FALSE;
int Ok = TRUE;
size_t toWrite;
FILE *fp;

    while (!done && Ok)
        {
        nFonts = 0;
        done = ((choice = MenuSelect(gTheMenu, gNMenuItems)) == NO_ITEM_SEL);
        if (!done)
            {
            /* prepare gFontInfoTable[] based on choice: */
            nFonts = PrepareFontOutPut(choice);

            /* get the default file name for this menu choice: */
            strcpy(gOutFileDefault, gDefaultFileNames[choice]);

            /* let the user edit their own file name: */
            memset(gCurFileName, 0, sizeof(gCurFileName));
            fprintf(stderr,"\nOutput file name [default = %s]", gOutFileDefault);

            if(!gets(gCurFileName) || !strlen(gCurFileName))
                {
                /* some error, or user just hit RETURN */
                strcpy(gCurFileName, gOutFileDefault);
                }
                
            /* create the file: */
            fp = fopen(gCurFileName, "wb");
            if (fp)
                {/* created OK */
                fprintf(stderr,"\nWriting %s...", gCurFileName);
                toWrite = (size_t)nFonts * (size_t)sizeof(FontInfoType);
                Ok = (fwrite(gFontInfoTable, 1, toWrite, fp) == toWrite);
                if (Ok)
                    fprintf(stderr,"Written Ok...");
                else
                    fprintf(stderr,"Failed write...");
                fclose(fp);
                }
            else
                {/* failed creating file */
                fprintf(stderr, "Unable to create %s.", gCurFileName);
                Ok = FALSE;
                }
            }
        }
    exit(Ok ? 0 : 1);    /* exit 0 on success, 1 on error */
}

/************************************************************************
    The menu subroutine
    Display the menu, get input strings, verify input,
    and return the ItemRet code for the choice made.
************************************************************************/
FUNCTION int MenuSelect(MenuTypePtr aMenu, int nItems)
{
int i, theIdx, theChoice, Ok = TRUE;
char done, aStrBuf[128];

    theIdx = 0;
    done = FALSE;
    while (!done)
        {
        /* display menu items: */
        for (i = 0; i < nItems; i++)
            fprintf(stderr,"\n%s", aMenu[i].ItemStr);

        fprintf(stderr,"\n");

        /* set up for string edit: */
        memset(aStrBuf, 0, sizeof(aStrBuf));
        if (gets(aStrBuf) && strlen(aStrBuf))
            {
            theIdx =  atoi(aStrBuf);    /* see what integer was typed */
            Ok = ((theIdx >= 1) && (theIdx <= nItems));
            if (Ok)
                done = TRUE;    /* got a good one, we're finished! */
            theIdx--;
            }
        else
            {
            Ok = FALSE;
            done = TRUE;    /* empty string, or some error, we're done */
            }
        }
    theChoice = aMenu[theIdx].ItemRet;
    /* if all is well, return the ItemRet indicated by theChoice: */
    /* if not, pretend user chose quit: */
    return(Ok ? theChoice : NO_ITEM_SEL);
}

/***********************************************************************
    Central processor for "Add" routines
    "Adds" (copies) data elements to gFontInfoTable, based on the menu 
    choice (one of ITEM_LJIII - ITEM_MAXITEMS-1)
************************************************************************/
FUNCTION int PrepareFontOutPut(int choice)
{
int nFonts;
    gCurFontIdx = 0;
    nFonts = 0;
    switch(choice)
        {

#if PCL5 || PCL5e

        case ITEM_LJIII:
            nFonts += AddSupportSet();
			gSuppressComplement = TRUE;
            nFonts += AddCore(8);
			gSuppressComplement = FALSE;
            break;

        case ITEM_LJIIISI:
            nFonts += AddSupportSet();
			gSuppressComplement = TRUE;
            nFonts += AddCore(13);
			gSuppressComplement = FALSE;
            break;

        case ITEM_HPCART1:
			gSuppressComplement = TRUE;
            nFonts += AddCart1();
			gSuppressComplement = FALSE;
            break;

        case ITEM_HPCART2:
			gSuppressComplement = TRUE;
            nFonts += AddCart2();
			gSuppressComplement = FALSE;
            break;

        case ITEM_HPCART3:
			gSuppressComplement = TRUE;
            nFonts += AddCart1();
            nFonts += AddCart2();
			gSuppressComplement = FALSE;
            break;

	case ITEM_LJ4L:
                        gCompressedPFR = TRUE;
            nFonts += AddSupportSet();
            nFonts += AddCore(12);
            nFonts += AddAntiqueOlive(3);
            nFonts += AddCourier();
	    nFonts += AddLetterGothic();
	    nFonts += AddAlbertus();
	    nFonts += AddCoronet();
	    nFonts += AddWingdings();
                        gCompressedPFR = FALSE;
	    break;

        case ITEM_LJ4:
			gCompressedPFR = TRUE;
            nFonts += AddSupportSet();
            nFonts += AddCore(12);
            nFonts += AddAntiqueOlive(3);
            nFonts += AddOmega();
            nFonts += AddGaramond();
            nFonts += AddCourier();
            nFonts += AddPCL64toPCL71();
            nFonts += AddTrueType(8);
            nFonts += AddTrueTypeSymDings();
			gCompressedPFR = FALSE;
            break;

	case ITEM_COMBINED_LJ4_PS35:
			gCompressedPFR = TRUE;
            nFonts += AddSupportSet();
            nFonts += AddCore(12);
            nFonts += AddAntiqueOlive(3);
            nFonts += AddOmega();
            nFonts += AddGaramond();
            nFonts += AddCourier();
            nFonts += AddPCL64toPCL71();
            nFonts += AddTrueType(8);
            nFonts += AddTrueTypeSymDings();
            nFonts += AddPostScript(35, TRUE);
			gCompressedPFR = FALSE;
	    break;

	case ITEM_LJ5L:
                        gCompressedPFR = TRUE;
            nFonts += AddSupportSet();
            nFonts += AddCore(12);
            nFonts += AddCourier();
	    nFonts += AddLetterGothic();
	    nFonts += AddAlbertus();
            nFonts += AddAntiqueOlive(3);
	    nFonts += AddCoronet();
	    nFonts += AddWingdings();
                        gCompressedPFR = FALSE;
	    break;

        case ITEM_TRUETYPE14:
            nFonts += AddTrueType(14);
            break;

        case ITEM_POSTSCRIPT13:
                        gCompressedPFR = TRUE;  /* Don't include mutables */
            nFonts += AddPostScript(13, FALSE);
                        gCompressedPFR = FALSE;
            break;

        case ITEM_POSTSCRIPT35:
                        gCompressedPFR = TRUE;  /* Don't include mutables */
            nFonts += AddPostScript(35, FALSE);
                        gCompressedPFR = FALSE;
            break;

        case ITEM_POSTSCRIPT39:
                        gCompressedPFR = TRUE;  /* Don't include mutables */
            nFonts += AddPostScript(39, FALSE);
                        gCompressedPFR = FALSE;
            break;

        case ITEM_POSTSCRIPT13andTRUETYPE14:
                        gCompressedPFR = TRUE;  /* Don't include mutables */
            nFonts += AddPostScript(13, FALSE);
                        gCompressedPFR = FALSE;
            nFonts += AddTrueType(14);
            break;

        case ITEM_POSTSCRIPT35andTRUETYPE14:
                        gCompressedPFR = TRUE;  /* Don't include mutables */
            nFonts += AddPostScript(35, FALSE);
                        gCompressedPFR = FALSE;
            nFonts += AddTrueType(14);
            break;

#endif
#if PCL6

        case ITEM_LJ5:
			gCompressedPFR = TRUE;
            nFonts += AddSupportSet();
            nFonts += AddCore(12);
            nFonts += AddAntiqueOlive(3);
            nFonts += AddOmega();
            nFonts += AddGaramond();
            nFonts += AddCourier();
            nFonts += AddCourierLt();
            nFonts += AddPCL64toPCL71();
            nFonts += AddPCL6TrueType();
            nFonts += AddTrueTypeSymDings();
			gCompressedPFR = FALSE;
            break;

	case ITEM_COMBINED_LJ5_PS35:
			gCompressedPFR = TRUE;
            nFonts += AddSupportSet();
            nFonts += AddCore(12);
            nFonts += AddAntiqueOlive(3);
            nFonts += AddOmega();
            nFonts += AddGaramond();
            nFonts += AddCourier();
            nFonts += AddCourierLt();
            nFonts += AddPCL64toPCL71();
            nFonts += AddPCL6TrueType();
            nFonts += AddTrueTypeSymDings();
            nFonts += AddPostScript(35, TRUE);
			gCompressedPFR = FALSE;
	    break;

#if 0
	case ITEM_LJ6L:
			gCompressedPFR = TRUE;
            nFonts += AddSupportSet();
            nFonts += AddCore(12);
            nFonts += AddAntiqueOlive(3);
            nFonts += AddCourier();
            nFonts += AddCourierLt();
	    nFonts += AddLetterGothic();
	    nFonts += AddAlbertus();
	    nFonts += AddCoronet();
	    nFonts += AddWingdings();
			gCompressedPFR = FALSE;
	    break;
#endif

#endif

        default:
            nFonts = 0;
            break;
        }
    return(nFonts);
}

/*--------------------------------------------------------------------------
                the "Add" routines - 
    copy data from gSrcFontInfoTable to gFontInfoTable, bumping gCurFontIdx 
--------------------------------------------------------------------------*/
#ifdef HAVE_STICK_FONT
#define N_SUPPORT_FONTS    6
#else
#define N_SUPPORT_FONTS    5
#endif

/******* Add Support/Aux Fonts *******/
FUNCTION int AddSupportSet(void)
{
int i, base, nFonts = N_SUPPORT_FONTS;

    base = 0;
    for (i = 0; i < nFonts; i++)
        gFontInfoTable[gCurFontIdx++] = gSrcFontInfoTable[i+base];
    return(nFonts);
}

int PseudoFontRoots[] =
{
	3179, 3183, 3185, 3191, 3193, 3203, 3205, 3154, 3156, 3222, 3230, 3221, 3224, 3008, 0
};

FUNCTION int isoneof(int iArray[], int code, int *whichIdx)
{
char found = FALSE;
int i;
	for (i = 0; !found ; i++)
		{
		if (iArray[i] == code)
			{
			found = TRUE;
			*whichIdx = i;
			}
		if (!iArray[i])
			break;
		}
	return (int)found;
}

/******* Add Core Fonts *******/
FUNCTION int AddCore(int howMany)
{
int i, base, nFonts = howMany;
int fontID, whichIndex;
    base = N_SUPPORT_FONTS;
	nFonts = 0;
    for (i = 0; i < howMany; i++)
	{
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		fontID = gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number;
		if (gSuppressComplement)
			gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x7FFFFFF9;
		gCurFontIdx++;
		nFonts++;
		if (gCompressedPFR && isoneof(PseudoFontRoots, fontID, &whichIndex))
			{
        	gFontInfoTable[gCurFontIdx] = gPseudoFontInfoTable[whichIndex];
			gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number += 10000;
			if (gSuppressComplement)
				gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x7FFFFFF9;
			gCurFontIdx++;
			nFonts++;
			}
	}
    return(nFonts);
}

/******* Add Cartridge 1 Fonts *******/
FUNCTION int AddCart1(void)
{
int i, base, nFonts = 25;
    base = 8+N_SUPPORT_FONTS;
    for (i = 0; i < nFonts; i++)
	{
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		if (gSuppressComplement)
			gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x7FFFFFF9;
		gCurFontIdx++;
	}
    return(nFonts);
}

/******* Add Cartridge 2 Fonts *******/
FUNCTION int AddCart2(void)
{
int i, base, nFonts = 26;
    base = 33+N_SUPPORT_FONTS;
    for (i = 0; i < nFonts; i++)
	{
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		if (gSuppressComplement)
			gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x7FFFFFF9;
		gCurFontIdx++;
	}
    return(nFonts);
}

/******* Add Cartridge 3 (1+2) Fonts *******/
FUNCTION int AddAntiqueOlive(int howMany)
{
int i, base, nFonts;
int fontID, whichIndex;
    base = 21+N_SUPPORT_FONTS;
	nFonts = 0;
    for (i = 0; i < howMany; i++)
	{
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		fontID = gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number;
		if (gExtendComplement)
			gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x1FFFFFF9;
		gCurFontIdx++;
		nFonts++;
		if (gCompressedPFR && isoneof(PseudoFontRoots, fontID, &whichIndex))
			{
        	gFontInfoTable[gCurFontIdx] = gPseudoFontInfoTable[whichIndex];
			gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number += 10000;
			if (gSuppressComplement)
				gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x7FFFFFF9;
			gCurFontIdx++;
			nFonts++;
			}
	}
    return(nFonts);
}

/******* Add CG Omega Fonts *******/
FUNCTION int AddOmega(void)
{
int i, base, nFonts, howMany = 4;
int fontID, whichIndex;
    base = 33+N_SUPPORT_FONTS;
	nFonts = 0;
    for (i = 0; i < howMany; i++)
		{
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		fontID = gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number;
		gCurFontIdx++;
		nFonts++;
		if (gCompressedPFR && isoneof(PseudoFontRoots, fontID, &whichIndex))
			{
        	gFontInfoTable[gCurFontIdx] = gPseudoFontInfoTable[whichIndex];
			gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number += 10000;
			if (gSuppressComplement)
				gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x7FFFFFF9;
			gCurFontIdx++;
			nFonts++;
			}
		}
    return(nFonts);
}

/******* Add Garamond Fonts *******/
FUNCTION int AddGaramond(void)
{
int i, base, nFonts, howMany = 4;
int fontID, whichIndex;
    base = 49+N_SUPPORT_FONTS;
	nFonts = 0;
    for (i = 0; i < howMany; i++)
		{
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		fontID = gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number;
		gCurFontIdx++;
		nFonts++;
		if (gCompressedPFR && isoneof(PseudoFontRoots, fontID, &whichIndex))
			{
        	gFontInfoTable[gCurFontIdx] = gPseudoFontInfoTable[whichIndex];
			gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number += 10000;
			if (gSuppressComplement)
				gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x7FFFFFF9;
			gCurFontIdx++;
			nFonts++;
			}
		}
    return(nFonts);
}

/******* Add Standard Courier Fonts *******/
FUNCTION int AddCourier(void)
{
int i, base, nFonts, howMany = 7;
int fontID, whichIndex;

    base = 59+N_SUPPORT_FONTS;
    nFonts = 0;

    for (i = 0; i < howMany; i++)
    {
                gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		fontID = gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number;

		gCurFontIdx++;
		nFonts++;

		if (gCompressedPFR && isoneof(PseudoFontRoots, fontID, &whichIndex))
	        {
			if (fontID == 3230)
				gFontInfoTable[gCurFontIdx-1].nextSearchEncode = UP_ONE;
        	        gFontInfoTable[gCurFontIdx] = gPseudoFontInfoTable[whichIndex];
			/* these guys are drawn bold insertions, so different range: */
			gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number += 20000;
			if (gSuppressComplement)
				gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x7FFFFFF9;
			gCurFontIdx++;
			nFonts++;
		}
    }
    return(nFonts);
}

/******* Add Courier Light Fonts *******/
FUNCTION int AddCourierLt(void)
{
int i, base, howMany = 9;

    base = 14;

    for (i = 0; i < howMany; i++)
    {
        gFontInfoTable[gCurFontIdx] = gPseudoFontInfoTable[i+base];
        gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number += 30000;
	gCurFontIdx++;
    }
    return(howMany);
}

/******* Add PCL Fonts 64-71 *******/
FUNCTION int AddPCL64toPCL71(void)
{
int nFonts;

	nFonts =  AddLetterGothic();
	nFonts += AddAlbertus();
	nFonts += AddClarendon();
	nFonts += AddCoronet();
	nFonts += AddMarigold();

	return(nFonts);
}


FUNCTION int AddLetterGothic(void)
{
int i, base, nFonts, howMany = 6;
int fontID, whichIndex;
char patchIt = FALSE;
    base = 66+N_SUPPORT_FONTS;
	nFonts = 0;
    for (i = 0; i < howMany; i++)
		{
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		fontID = gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number;
		if (patchIt && (fontID == 3232))
			gFontInfoTable[gCurFontIdx].nextSearchEncode = UP_FIVE;
		gCurFontIdx++;
		nFonts++;
		if (gCompressedPFR && isoneof(PseudoFontRoots, fontID, &whichIndex))
			{
        	gFontInfoTable[gCurFontIdx] = gPseudoFontInfoTable[whichIndex];
			gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number += 10000;
			if (gSuppressComplement)
				gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x7FFFFFF9;
			gCurFontIdx++;
			nFonts++;
			patchIt = TRUE;
			}
		}
    return(nFonts);
}

FUNCTION int AddAlbertus(void)
{
int i, base, nFonts, howMany = 2;
    base = 72+N_SUPPORT_FONTS;
	nFonts = 0;
    for (i = 0; i < howMany; i++)
	{
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		if (gExtendComplement)
			gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x1FFFFFF9;
		gCurFontIdx++;
		nFonts++;
	}
    return(nFonts);
}

FUNCTION int AddClarendon(void)
{
int i, base, nFonts, howMany = 1;
    base = 74+N_SUPPORT_FONTS;
	nFonts = 0;
    for (i = 0; i < howMany; i++)
		{
        gFontInfoTable[gCurFontIdx++] = gSrcFontInfoTable[i+base];
		nFonts++;
		}
    return(nFonts);
}

FUNCTION int AddCoronet(void)
{
int i, base, nFonts, howMany = 1;
    base = 75+N_SUPPORT_FONTS;
	nFonts = 0;
    for (i = 0; i < howMany; i++)
	{
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		if (gExtendComplement)
			gFontInfoTable[gCurFontIdx].pclHdrInfo.char_complement_msw = 0x1FFFFFF9;
		gCurFontIdx++;
		nFonts++;
	}
    return(nFonts);
}

FUNCTION int AddMarigold(void)
{
int i, base, nFonts, howMany = 1;
    base = 76+N_SUPPORT_FONTS;
	nFonts = 0;
    for (i = 0; i < howMany; i++)
		{
        gFontInfoTable[gCurFontIdx++] = gSrcFontInfoTable[i+base];
		nFonts++;
		}
    return(nFonts);
}

/******* Add TrueType Fonts 1-?? *******/
FUNCTION int AddTrueType(int howMany)
{
int i, base, nFonts, nAdded;
int fontID, whichIndex;

#if INCL_MFT
	nFonts = howMany + 5;
        base = 129+N_SUPPORT_FONTS;
#else
	nFonts = howMany + 1;
        base = 116+N_SUPPORT_FONTS;
#endif

    nAdded = 0;
    for (i = 0; i < nFonts; i++)
		{
                gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		fontID = gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number;
		gCurFontIdx++;
		nAdded++;
		if (gCompressedPFR && isoneof(PseudoFontRoots, fontID, &whichIndex))
			{
			if (fontID == 3238)
				gFontInfoTable[gCurFontIdx-1].nextSearchEncode = DOWN_SIX;
			if (fontID == 3240)
				gFontInfoTable[gCurFontIdx-1].nextSearchEncode = DOWN_THREE;
                	gFontInfoTable[gCurFontIdx] = gPseudoFontInfoTable[whichIndex];
			gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number += 10000;
			gCurFontIdx++;
			nAdded++;
			}
		}
    return(nAdded);
}


/******* Add TrueType Fonts 1-8 *******/
FUNCTION int AddPCL6TrueType(void)
{
int base;

    base = 116+N_SUPPORT_FONTS;

    gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[base+0];    /* Arial */
    gFontInfoTable[gCurFontIdx++].nextSearchEncode = SANS;
    gFontInfoTable[gCurFontIdx++] = gSrcFontInfoTable[base+1];  /* Arial Italic */
    gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[base+2];    /* Arial Bold */
    gFontInfoTable[gCurFontIdx++].nextSearchEncode = SANS_BOLD;
    gFontInfoTable[gCurFontIdx++] = gSrcFontInfoTable[base+3];  /* Arial Bold Italic */

    gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[base+5];    /* TNER */
    gFontInfoTable[gCurFontIdx++].nextSearchEncode = SERIF;
    gFontInfoTable[gCurFontIdx++] = gSrcFontInfoTable[base+6];  /* TNER Italic */
    gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[base+7];    /* TNER Bold */
    gFontInfoTable[gCurFontIdx++].nextSearchEncode = SERIF_BOLD;
    gFontInfoTable[gCurFontIdx++] = gSrcFontInfoTable[base+8];  /* TNER Bold Italic */

    return(8);
}

/******* Add TrueType Fonts13 and 14  *******/
FUNCTION int AddTrueTypeSymDings(void)
{
int nFonts;
	nFonts = AddSymbol();
	nFonts += AddWingdings();
    return(nFonts);
}

FUNCTION int AddSymbol(void)
{
int i, base, nFonts = 1;
#if INCL_MFT
    base = 149+N_SUPPORT_FONTS;
#else
    base = 132+N_SUPPORT_FONTS;
#endif
    for (i = 0; i < nFonts; i++)
        gFontInfoTable[gCurFontIdx++] = gSrcFontInfoTable[i+base];
    return(nFonts);
}

FUNCTION int AddWingdings(void)
{
int i, base, nFonts = 1;
#if INCL_MFT
    base = 150+N_SUPPORT_FONTS;
#else
    base = 133+N_SUPPORT_FONTS;
#endif
    for (i = 0; i < nFonts; i++)
        gFontInfoTable[gCurFontIdx++] = gSrcFontInfoTable[i+base];
    return(nFonts);
}

/******* Add PostScript Fonts 1-?? *******/
FUNCTION int AddPostScript(int howMany, int doInsertion)
{
int i, base, nFonts, fontID, whichIndex;
btsbool patchIt;
#if INCL_MFT
	howMany += 12;
#endif
	nFonts = 0;
    base = 77+N_SUPPORT_FONTS;
	patchIt = FALSE;
    for (i = 0; i < howMany; i++)
		{
		if (gCompressedPFR &&
				(
				(i == 5) ||			/* Helvetica It */
				(i == 7) ||			/* Helvetica Bd It */
				(i == 9) ||			/* Courier It */
				(i == 11) ||		/* Courier Bd It */
				(i == 21) ||		/* Helvetica Narrow */
				(i == 22) ||		/* Helvetica Narrow It */
				(i == 23) ||		/* Helvetica Narrow Bd */
				(i == 24) ||		/* Helvetica Narrow Bd It */
				(i == 30) ||		/* Avant Garde It */
				(i == 32) ||		/* Avant Garde Bd It */
				(i == 36) ||		/* Helvetica Cd It */
				(i == 38)			/* Helvetica Cd Bd It */
				)
			)
			continue;	/* skip fonts not present in compressed/combined PFR */
        gFontInfoTable[gCurFontIdx] = gSrcFontInfoTable[i+base];
		fontID = gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number;
		if (patchIt && (fontID == 3010))
			{
			gFontInfoTable[gCurFontIdx].nextSearchEncode = UP_ONE;
			patchIt = FALSE;
			}
		gCurFontIdx++;
		nFonts++;
		if (doInsertion && gCompressedPFR && isoneof(PseudoFontRoots, fontID, &whichIndex))
			{
        	gFontInfoTable[gCurFontIdx] = gPseudoFontInfoTable[whichIndex];
			gFontInfoTable[gCurFontIdx].pclHdrInfo.font_number += 20000;
			gCurFontIdx++;
			nFonts++;
			if (fontID == 3008)
				patchIt = TRUE;
			}
		}
    return(nFonts);
}

/* eof finfotbl.c */
