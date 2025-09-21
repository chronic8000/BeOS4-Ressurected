/******************************************************************************
 *                                                                            *
 *  Copyright 1988, as an unpublished work by Bitstream Inc., Cambridge, MA   *
 *                               Patent Pending                               *
 *                                                                            *
 *         These programs are the sole property of Bitstream Inc. and         *
 *           contain its proprietary and confidential information.            *
 *                           All rights reserved                              *
 *                                                                            *
 * $Header: //bliss/user/archive/rcs/4in1/cmtglob.c,v 14.0 97/03/17 17:15:27 shawn Release $
 *
 * $Log:	cmtglob.c,v $
 * Revision 14.0  97/03/17  17:15:27  shawn
 * TDIS Version 6.00
 * 
 * Revision 13.3  97/03/11  11:49:06  shawn
 * Corrected character mapping for complete PCL6 support.
 * Reordered some character mapping entries.
 * 
 * Revision 13.2  96/12/05  10:26:39  shawn
 * Added or corrected entries for all 640 characters in the PCL6 printers.
 * 
 * Revision 13.1  96/02/07  13:32:10  roberte
 * Added Cyrillic mappings.
 * Made correction to mis-mapping of period_centered ans middot.
 * Corrected incorrect Unicode for BCID 536 (xi) to 0x3be.
 * 
 * Revision 13.0  95/02/15  16:36:58  roberte
 * Release
 * 
 * Revision 12.1  95/01/04  16:42:49  roberte
 * Release
 * 
 * Revision 11.2  94/12/21  15:06:38  shawn
 * Conversion of all function prototypes and declaration to true ANSI type. New macros for REENTRANT_ALLOC.
 * 
 * Revision 11.1  94/11/15  15:57:02  roberte
 *   Declare all PostScript Name strings as static GLOBALFAR character strings,
 * instead of declaring as immediate strings in place. For all platforms, this
 * squeezes out duplicate strings being declared. Also use a single null string
 * array throughout the table.
 * For PC builds, this allows these strings to all be far data.
 * A great help when 64K segment limit is always a problem.
 * 
 * Revision 11.0  94/05/04  09:36:00  roberte
 * Release
 * 
 * Revision 10.3  94/05/03  12:28:37  roberte
 * Removed extraneous mapping of ogonek to MSL 1030 and BCID 247.
 * 
 * Revision 10.2  94/04/08  14:37:15  roberte
 * Release
 * 
 * Revision 10.0  94/04/08  12:38:18  roberte
 * Release
 * 
 * Revision 9.7  94/04/08  11:14:52  roberte
 * Changed mapping of BCID 45 to "sfthyphen" to BCID 21325. Now all
 * PDL codes for soft hyphen should be correctly mapped to each other.
 * 
 * Revision 9.6  94/04/06  16:08:21  roberte
 * Numerous changes to support translations MSL->BCID BCID->MSL Unicode->BCID BCID->Unicode
 * for the complement of characters shared by the MSL and Unicode font of the LJ4 printer.
 * 
 * Revision 9.5  94/02/23  16:13:09  roberte
 * Translate all emdashes to our connecting emdash BCID 418.
 * 
 * Revision 9.4  94/02/23  11:30:55  roberte
 * Added new translation of BCID 5131 to MSL 526.
 * Removed duplicate translation of BCID 527 to MSL 526.
 * 
 * Revision 9.3  94/02/03  16:25:14  roberte
 * Added translations for floating accents (MSLs 1400-1424)
 * 
 * Revision 9.2  93/08/30  15:55:52  roberte
 * Release
 * 
 * Revision 2.24  93/08/20  11:52:18  weili
 * corrected the bug in testing CMT_EMPTY
 * 
 * Revision 2.23  93/08/17  14:59:02  weili
 * Added CMT_EMPTY conditional testing to turn off the whole character mapping table
 * 
 * Revision 2.22  93/07/30  15:21:48  roberte
 * Removed one middot entry, and have the Unicode for the remaining one now UNKNOWN
 * 
 * Character Mapping database version 4.2. Changed mapping of middot
 * No more Unicode value there.
 * Revision 2.21  93/05/04  11:57:03  roberte
 * Removed #include of type.h, not needed
 * 
 * Revision 2.20  93/03/23  16:49:08  roberte
 * Made Unicode entries for trademark serif and trademark sansserif UNKNOWN.
 * 
 * Revision 2.19  93/03/15  13:53:30  roberte
 * Release
 * 
 * Revision 2.4  93/02/24  16:20:16  weili
 * Added STACKFAR to declaration of Master Glyph Codes.
 * 
 * Revision 2.3  93/02/23  16:56:08  roberte
 * Added #include of finfotbl.h before #include of ufe.h.
 * 
 * Revision 2.2  92/12/08  10:41:57  roberte
 * Character Mapping database version 4.0. Addition of
 * Eastern European characters in Unicode.
 * 
 * Revision 2.1  92/12/02  12:27:29  laurar
 * include fino.h.
 * 
 * Revision 2.0  92/11/19  15:38:23  roberte
 * Release
 * 
 * Revision 1.8  92/11/05  12:12:18  roberte
 * Replace CMT with 3.2 version from Chuck R.
 * 
 * Revision 1.7  92/11/03  13:55:45  laurar
 * include type1.h for CHARACTERNAME declaration.
 * 
 * Revision 1.6  92/11/02  10:38:13  roberte
 * New version of character mapping table.
 * 
 * Revision 1.5  92/10/01  13:50:12  roberte
 * Changed the entire table to use macro declarations, so that individual columns
 * may be shut off using a macro in useropt.h. See ufe.h.
 * 
 * Revision 1.4  92/09/26  16:50:11  roberte
 * Added copyright header and RCS marker. Added comments.
 * 
 * 
******************************************************************************/
/***********************************************************************************************
    FILE:       CMTGLOB.C
	PROJECT:	4-in-1
	AUTHOR:		WT
	CONTENTS:	Character Mapping Table (gMasterGlyphCodes[])
				ufix16 GlyphNumElements()
				
***********************************************************************************************/

#include "spdo_prv.h"               /* General definitions for Speedo    */
#include "fino.h"
#include "finfotbl.h"
#include "ufe.h"

#define BciDECL(x) x

#if CMT_MSL
#define MslDECL(x) ,x
#else
#define MslDECL(x)
#endif

#if CMT_UNI
#define UniDECL(x) ,x
#else
#define UniDECL(x)
#endif
#if CMT_PS
#define PsDECL(x) ,x
#else
#define PsDECL(x)
#endif
#if CMT_USR
#define UsrDECL(x) ,x
#else
#define UsrDECL(x)
#endif

#undef NONE
/* but define it as the same thing, less negatively: */
#define NONE	0xffff

#if CMT_EMPTY
GlyphCodes  GLOBALFAR gMasterGlyphCodes [1];
#else
#if CMT_PS
/* far null string */
static char GLOBALFAR ns[1] = "";
/* all possible far strings for the PsDECL's below: */
static char GLOBALFAR ps_space[] = "space";
static char GLOBALFAR ps_exclam[] = "exclam";
static char GLOBALFAR ps_quotedbl[] = "quotedbl";
static char GLOBALFAR ps_numbersign[] = "numbersign";
static char GLOBALFAR ps_dollar[] = "dollar";
static char GLOBALFAR ps_percent[] = "percent";
static char GLOBALFAR ps_ampersand[] = "ampersand";
static char GLOBALFAR ps_quoteright[] = "quoteright";
static char GLOBALFAR ps_parenleft[] = "parenleft";
static char GLOBALFAR ps_parenright[] = "parenright";
static char GLOBALFAR ps_asterisk[] = "asterisk";
static char GLOBALFAR ps_plus[] = "plus";
static char GLOBALFAR ps_comma[] = "comma";
static char GLOBALFAR ps_hyphen[] = "hyphen";
static char GLOBALFAR ps_period[] = "period";
static char GLOBALFAR ps_slash[] = "slash";
static char GLOBALFAR ps_zero[] = "zero";
static char GLOBALFAR ps_one[] = "one";
static char GLOBALFAR ps_two[] = "two";
static char GLOBALFAR ps_three[] = "three";
static char GLOBALFAR ps_four[] = "four";
static char GLOBALFAR ps_five[] = "five";
static char GLOBALFAR ps_six[] = "six";
static char GLOBALFAR ps_seven[] = "seven";
static char GLOBALFAR ps_eight[] = "eight";
static char GLOBALFAR ps_nine[] = "nine";
static char GLOBALFAR ps_colon[] = "colon";
static char GLOBALFAR ps_semicolon[] = "semicolon";
static char GLOBALFAR ps_less[] = "less";
static char GLOBALFAR ps_equal[] = "equal";
static char GLOBALFAR ps_greater[] = "greater";
static char GLOBALFAR ps_question[] = "question";
static char GLOBALFAR ps_at[] = "at";
static char GLOBALFAR ps_A[] = "A";
static char GLOBALFAR ps_B[] = "B";
static char GLOBALFAR ps_C[] = "C";
static char GLOBALFAR ps_D[] = "D";
static char GLOBALFAR ps_E[] = "E";
static char GLOBALFAR ps_F[] = "F";
static char GLOBALFAR ps_G[] = "G";
static char GLOBALFAR ps_H[] = "H";
static char GLOBALFAR ps_I[] = "I";
static char GLOBALFAR ps_J[] = "J";
static char GLOBALFAR ps_K[] = "K";
static char GLOBALFAR ps_L[] = "L";
static char GLOBALFAR ps_M[] = "M";
static char GLOBALFAR ps_N[] = "N";
static char GLOBALFAR ps_O[] = "O";
static char GLOBALFAR ps_P[] = "P";
static char GLOBALFAR ps_Q[] = "Q";
static char GLOBALFAR ps_R[] = "R";
static char GLOBALFAR ps_S[] = "S";
static char GLOBALFAR ps_T[] = "T";
static char GLOBALFAR ps_U[] = "U";
static char GLOBALFAR ps_V[] = "V";
static char GLOBALFAR ps_W[] = "W";
static char GLOBALFAR ps_X[] = "X";
static char GLOBALFAR ps_Y[] = "Y";
static char GLOBALFAR ps_Z[] = "Z";
static char GLOBALFAR ps_bracketleft[] = "bracketleft";
static char GLOBALFAR ps_backslash[] = "backslash";
static char GLOBALFAR ps_bracketright[] = "bracketright";
static char GLOBALFAR ps_asciicircum[] = "asciicircum";
static char GLOBALFAR ps_underscore[] = "underscore";
static char GLOBALFAR ps_a[] = "a";
static char GLOBALFAR ps_b[] = "b";
static char GLOBALFAR ps_c[] = "c";
static char GLOBALFAR ps_d[] = "d";
static char GLOBALFAR ps_e[] = "e";
static char GLOBALFAR ps_f[] = "f";
static char GLOBALFAR ps_g[] = "g";
static char GLOBALFAR ps_h[] = "h";
static char GLOBALFAR ps_i[] = "i";
static char GLOBALFAR ps_j[] = "j";
static char GLOBALFAR ps_k[] = "k";
static char GLOBALFAR ps_l[] = "l";
static char GLOBALFAR ps_m[] = "m";
static char GLOBALFAR ps_n[] = "n";
static char GLOBALFAR ps_o[] = "o";
static char GLOBALFAR ps_p[] = "p";
static char GLOBALFAR ps_q[] = "q";
static char GLOBALFAR ps_r[] = "r";
static char GLOBALFAR ps_s[] = "s";
static char GLOBALFAR ps_t[] = "t";
static char GLOBALFAR ps_u[] = "u";
static char GLOBALFAR ps_v[] = "v";
static char GLOBALFAR ps_w[] = "w";
static char GLOBALFAR ps_x[] = "x";
static char GLOBALFAR ps_y[] = "y";
static char GLOBALFAR ps_z[] = "z";
static char GLOBALFAR ps_braceleft[] = "braceleft";
static char GLOBALFAR ps_bar[] = "bar";
static char GLOBALFAR ps_braceright[] = "braceright";
static char GLOBALFAR ps_fi[] = "fi";
static char GLOBALFAR ps_fl[] = "fl";
static char GLOBALFAR ps_sterling[] = "sterling";
static char GLOBALFAR ps_cent[] = "cent";
static char GLOBALFAR ps_florin[] = "florin";
static char GLOBALFAR ps_perthousand[] = "perthousand";
static char GLOBALFAR ps_fraction[] = "fraction";
static char GLOBALFAR ps_middot[] = "middot";
static char GLOBALFAR ps_quoteleft[] = "quoteleft";
static char GLOBALFAR ps_quotedblleft[] = "quotedblleft";
static char GLOBALFAR ps_quotedblright[] = "quotedblright";
static char GLOBALFAR ps_quotesinglbase[] = "quotesinglbase";
static char GLOBALFAR ps_quotedblbase[] = "quotedblbase";
static char GLOBALFAR ps_dagger[] = "dagger";
static char GLOBALFAR ps_daggerdbl[] = "daggerdbl";
static char GLOBALFAR ps_section[] = "section";
static char GLOBALFAR ps_endash[] = "endash";
static char GLOBALFAR ps_Aring[] = "Aring";
static char GLOBALFAR ps_AE[] = "AE";
static char GLOBALFAR ps_Oslash[] = "Oslash";
static char GLOBALFAR ps_OE[] = "OE";
static char GLOBALFAR ps_aring[] = "aring";
static char GLOBALFAR ps_ae[] = "ae";
static char GLOBALFAR ps_oslash[] = "oslash";
static char GLOBALFAR ps_oe[] = "oe";
static char GLOBALFAR ps_germandbls[] = "germandbls";
static char GLOBALFAR ps_dotlessi[] = "dotlessi";
static char GLOBALFAR ps_guilsinglleft[] = "guilsinglleft";
static char GLOBALFAR ps_guilsinglright[] = "guilsinglright";
static char GLOBALFAR ps_guillemotleft[] = "guillemotleft";
static char GLOBALFAR ps_guillemotright[] = "guillemotright";
static char GLOBALFAR ps_questiondown[] = "questiondown";
static char GLOBALFAR ps_exclamdown[] = "exclamdown";
static char GLOBALFAR ps_acute[] = "acute";
static char GLOBALFAR ps_grave[] = "grave";
static char GLOBALFAR ps_circumflex[] = "circumflex";
static char GLOBALFAR ps_dieresis[] = "dieresis";
static char GLOBALFAR ps_tilde[] = "tilde";
static char GLOBALFAR ps_caron[] = "caron";
static char GLOBALFAR ps_cedilla[] = "cedilla";
static char GLOBALFAR ps_macron[] = "macron";
static char GLOBALFAR ps_breve[] = "breve";
static char GLOBALFAR ps_ring[] = "ring";
static char GLOBALFAR ps_Ccedilla[] = "Ccedilla";
static char GLOBALFAR ps_ccedilla[] = "ccedilla";
static char GLOBALFAR ps_onequarter[] = "onequarter";
static char GLOBALFAR ps_onehalf[] = "onehalf";
static char GLOBALFAR ps_threequarters[] = "threequarters";
static char GLOBALFAR ps_onesuperior[] = "onesuperior";
static char GLOBALFAR ps_twosuperior[] = "twosuperior";
static char GLOBALFAR ps_threesuperior[] = "threesuperior";
static char GLOBALFAR ps_Eth[] = "Eth";
static char GLOBALFAR ps_Lslash[] = "Lslash";
static char GLOBALFAR ps_dmacron[] = "dmacron";
static char GLOBALFAR ps_lslash[] = "lslash";
static char GLOBALFAR ps_dotaccent[] = "dotaccent";
static char GLOBALFAR ps_hungarumlaut[] = "hungarumlaut";
static char GLOBALFAR ps_ogonek[] = "ogonek";
static char GLOBALFAR ps_ntilde[] = "ntilde";
static char GLOBALFAR ps_Ntilde[] = "Ntilde";
static char GLOBALFAR ps_oacute[] = "oacute";
static char GLOBALFAR ps_Oacute[] = "Oacute";
static char GLOBALFAR ps_ograve[] = "ograve";
static char GLOBALFAR ps_Ograve[] = "Ograve";
static char GLOBALFAR ps_ocircumflex[] = "ocircumflex";
static char GLOBALFAR ps_Ocircumflex[] = "Ocircumflex";
static char GLOBALFAR ps_odieresis[] = "odieresis";
static char GLOBALFAR ps_Odieresis[] = "Odieresis";
static char GLOBALFAR ps_otilde[] = "otilde";
static char GLOBALFAR ps_Otilde[] = "Otilde";
static char GLOBALFAR ps_uacute[] = "uacute";
static char GLOBALFAR ps_Uacute[] = "Uacute";
static char GLOBALFAR ps_ugrave[] = "ugrave";
static char GLOBALFAR ps_Ugrave[] = "Ugrave";
static char GLOBALFAR ps_ucircumflex[] = "ucircumflex";
static char GLOBALFAR ps_Ucircumflex[] = "Ucircumflex";
static char GLOBALFAR ps_udieresis[] = "udieresis";
static char GLOBALFAR ps_Udieresis[] = "Udieresis";
static char GLOBALFAR ps_ydieresis[] = "ydieresis";
static char GLOBALFAR ps_Ydieresis[] = "Ydieresis";
static char GLOBALFAR ps_yacute[] = "yacute";
static char GLOBALFAR ps_Yacute[] = "Yacute";
static char GLOBALFAR ps_Idieresis[] = "Idieresis";
static char GLOBALFAR ps_idieresis[] = "idieresis";
static char GLOBALFAR ps_Icircumflex[] = "Icircumflex";
static char GLOBALFAR ps_icircumflex[] = "icircumflex";
static char GLOBALFAR ps_Igrave[] = "Igrave";
static char GLOBALFAR ps_igrave[] = "igrave";
static char GLOBALFAR ps_Iacute[] = "Iacute";
static char GLOBALFAR ps_iacute[] = "iacute";
static char GLOBALFAR ps_Edieresis[] = "Edieresis";
static char GLOBALFAR ps_edieresis[] = "edieresis";
static char GLOBALFAR ps_Ecircumflex[] = "Ecircumflex";
static char GLOBALFAR ps_ecircumflex[] = "ecircumflex";
static char GLOBALFAR ps_Egrave[] = "Egrave";
static char GLOBALFAR ps_egrave[] = "egrave";
static char GLOBALFAR ps_Eacute[] = "Eacute";
static char GLOBALFAR ps_eacute[] = "eacute";
static char GLOBALFAR ps_Atilde[] = "Atilde";
static char GLOBALFAR ps_atilde[] = "atilde";
static char GLOBALFAR ps_Adieresis[] = "Adieresis";
static char GLOBALFAR ps_adieresis[] = "adieresis";
static char GLOBALFAR ps_Acircumflex[] = "Acircumflex";
static char GLOBALFAR ps_acircumflex[] = "acircumflex";
static char GLOBALFAR ps_Agrave[] = "Agrave";
static char GLOBALFAR ps_agrave[] = "agrave";
static char GLOBALFAR ps_Aacute[] = "Aacute";
static char GLOBALFAR ps_aacute[] = "aacute";
static char GLOBALFAR ps_ordmasculine[] = "ordmasculine";
static char GLOBALFAR ps_ordfeminine[] = "ordfeminine";
static char GLOBALFAR ps_quotesingle[] = "quotesingle";
static char GLOBALFAR ps_emdash[] = "emdash";
static char GLOBALFAR ps_ellipsis[] = "ellipsis";
static char GLOBALFAR ps_Thorn[] = "Thorn";
static char GLOBALFAR ps_thorn[] = "thorn";
static char GLOBALFAR ps_eth[] = "eth";
static char GLOBALFAR ps_yen[] = "yen";
static char GLOBALFAR ps_brokenbar[] = "brokenbar";
static char GLOBALFAR ps_currency[] = "currency";
static char GLOBALFAR ps_paragraph[] = "paragraph";
static char GLOBALFAR ps_minus[] = "minus";
static char GLOBALFAR ps_multiply[] = "multiply";
static char GLOBALFAR ps_divide[] = "divide";
static char GLOBALFAR ps_plusminus[] = "plusminus";
static char GLOBALFAR ps_equivalence[] = "equivalence";
static char GLOBALFAR ps_notequal[] = "notequal";
static char GLOBALFAR ps_approxequal[] = "approxequal";
static char GLOBALFAR ps_greaterequal[] = "greaterequal";
static char GLOBALFAR ps_lessequal[] = "lessequal";
static char GLOBALFAR ps_arrowdown[] = "arrowdown";
static char GLOBALFAR ps_arrowleft[] = "arrowleft";
static char GLOBALFAR ps_arrowright[] = "arrowright";
static char GLOBALFAR ps_arrowup[] = "arrowup";
static char GLOBALFAR ps_arrowboth[] = "arrowboth";
static char GLOBALFAR ps_element[] = "element";
static char GLOBALFAR ps_intersection[] = "intersection";
static char GLOBALFAR ps_union[] = "union";
static char GLOBALFAR ps_integral[] = "integral";
static char GLOBALFAR ps_integraltp[] = "integraltp";
static char GLOBALFAR ps_integralbt[] = "integralbt";
static char GLOBALFAR ps_radical[] = "radical";
static char GLOBALFAR ps_infinity[] = "infinity";
static char GLOBALFAR ps_logicalnot[] = "logicalnot";
static char GLOBALFAR ps_Alpha[] = "Alpha";
static char GLOBALFAR ps_Beta[] = "Beta";
static char GLOBALFAR ps_Gamma[] = "Gamma";
static char GLOBALFAR ps_Delta[] = "Delta";
static char GLOBALFAR ps_Epsilon[] = "Epsilon";
static char GLOBALFAR ps_Zeta[] = "Zeta";
static char GLOBALFAR ps_Eta[] = "Eta";
static char GLOBALFAR ps_Theta[] = "Theta";
static char GLOBALFAR ps_Iota[] = "Iota";
static char GLOBALFAR ps_Kappa[] = "Kappa";
static char GLOBALFAR ps_Lambda[] = "Lambda";
static char GLOBALFAR ps_Mu[] = "Mu";
static char GLOBALFAR ps_Nu[] = "Nu";
static char GLOBALFAR ps_Xi[] = "Xi";
static char GLOBALFAR ps_Omicron[] = "Omicron";
static char GLOBALFAR ps_Pi[] = "Pi";
static char GLOBALFAR ps_Rho[] = "Rho";
static char GLOBALFAR ps_Sigma[] = "Sigma";
static char GLOBALFAR ps_Tau[] = "Tau";
static char GLOBALFAR ps_Upsilon[] = "Upsilon";
static char GLOBALFAR ps_Phi[] = "Phi";
static char GLOBALFAR ps_Chi[] = "Chi";
static char GLOBALFAR ps_Psi[] = "Psi";
static char GLOBALFAR ps_Omega[] = "Omega";
static char GLOBALFAR ps_alpha[] = "alpha";
static char GLOBALFAR ps_beta[] = "beta";
static char GLOBALFAR ps_gamma[] = "gamma";
static char GLOBALFAR ps_delta[] = "delta";
static char GLOBALFAR ps_zeta[] = "zeta";
static char GLOBALFAR ps_eta[] = "eta";
static char GLOBALFAR ps_theta[] = "theta";
static char GLOBALFAR ps_iota[] = "iota";
static char GLOBALFAR ps_kappa[] = "kappa";
static char GLOBALFAR ps_lambda[] = "lambda";
static char GLOBALFAR ps_mu[] = "mu";
static char GLOBALFAR ps_nu[] = "nu";
static char GLOBALFAR ps_xi[] = "xi";
static char GLOBALFAR ps_omicron[] = "omicron";
static char GLOBALFAR ps_pi[] = "pi";
static char GLOBALFAR ps_rho[] = "rho";
static char GLOBALFAR ps_sigma[] = "sigma";
static char GLOBALFAR ps_sigma1[] = "sigma1";
static char GLOBALFAR ps_tau[] = "tau";
static char GLOBALFAR ps_upsilon[] = "upsilon";
static char GLOBALFAR ps_phi[] = "phi";
static char GLOBALFAR ps_chi[] = "chi";
static char GLOBALFAR ps_psi[] = "psi";
static char GLOBALFAR ps_omega[] = "omega";
static char GLOBALFAR ps_dotmath[] = "dotmath";
static char GLOBALFAR ps_copyright[] = "copyright";
static char GLOBALFAR ps_registered[] = "registered";
static char GLOBALFAR ps_trademark[] = "trademark";
static char GLOBALFAR ps_minute[] = "minute";
static char GLOBALFAR ps_second[] = "second";
static char GLOBALFAR ps_degree[] = "degree";
static char GLOBALFAR ps_bullet[] = "bullet";
static char GLOBALFAR ps_heart[] = "heart";
static char GLOBALFAR ps_club[] = "club";
static char GLOBALFAR ps_diamond[] = "diamond";
static char GLOBALFAR ps_spade[] = "spade";
static char GLOBALFAR ps_zcaron[] = "zcaron";
static char GLOBALFAR ps_Zcaron[] = "Zcaron";
static char GLOBALFAR ps_cacute[] = "cacute";
static char GLOBALFAR ps_Cacute[] = "Cacute";
static char GLOBALFAR ps_ccaron[] = "ccaron";
static char GLOBALFAR ps_Ccaron[] = "Ccaron";
static char GLOBALFAR ps_gbreve[] = "gbreve";
static char GLOBALFAR ps_Gbreve[] = "Gbreve";
static char GLOBALFAR ps_Idot[] = "Idot";
static char GLOBALFAR ps_scaron[] = "scaron";
static char GLOBALFAR ps_Scaron[] = "Scaron";
static char GLOBALFAR ps_proportional[] = "proportional";
static char GLOBALFAR ps_radicalex[] = "radicalex";
static char GLOBALFAR ps_suchthat[] = "suchthat";
static char GLOBALFAR ps_circleplus[] = "circleplus";
static char GLOBALFAR ps_circlemultiply[] = "circlemultiply";
static char GLOBALFAR ps_scedilla[] = "scedilla";
static char GLOBALFAR ps_Scedilla[] = "Scedilla";
static char GLOBALFAR ps_franc[] = "franc";
static char GLOBALFAR ps_asciitilde[] = "asciitilde";
static char GLOBALFAR ps_congruent[] = "congruent";
static char GLOBALFAR ps_propersuperset[] = "propersuperset";
static char GLOBALFAR ps_reflexsuperset[] = "reflexsuperset";
static char GLOBALFAR ps_propersubset[] = "propersubset";
static char GLOBALFAR ps_reflexsubset[] = "reflexsubset";
static char GLOBALFAR ps_notsubset[] = "notsubset";
static char GLOBALFAR ps_arrowdbldown[] = "arrowdbldown";
static char GLOBALFAR ps_arrowdblleft[] = "arrowdblleft";
static char GLOBALFAR ps_arrowdblright[] = "arrowdblright";
static char GLOBALFAR ps_arrowdblup[] = "arrowdblup";
static char GLOBALFAR ps_arrowdblboth[] = "arrowdblboth";
static char GLOBALFAR ps_perpendicular[] = "perpendicular";
static char GLOBALFAR ps_notelement[] = "notelement";
static char GLOBALFAR ps_logicaland[] = "logicaland";
static char GLOBALFAR ps_logicalor[] = "logicalor";
static char GLOBALFAR ps_angle[] = "angle";
static char GLOBALFAR ps_therefore[] = "therefore";
static char GLOBALFAR ps_emptyset[] = "emptyset";
static char GLOBALFAR ps_partialdiff[] = "partialdiff";
static char GLOBALFAR ps_integralex[] = "integralex";
static char GLOBALFAR ps_aleph[] = "aleph";
static char GLOBALFAR ps_bracketlefttp[] = "bracketlefttp";
static char GLOBALFAR ps_bracketleftbt[] = "bracketleftbt";
static char GLOBALFAR ps_bracketrighttp[] = "bracketrighttp";
static char GLOBALFAR ps_bracketrightbt[] = "bracketrightbt";
static char GLOBALFAR ps_universal[] = "universal";
static char GLOBALFAR ps_existential[] = "existential";
static char GLOBALFAR ps_asteriskmath[] = "asteriskmath";
static char GLOBALFAR ps_angleright[] = "angleright";
static char GLOBALFAR ps_angleleft[] = "angleleft";
static char GLOBALFAR ps_theta1[] = "theta1";
static char GLOBALFAR ps_omega1[] = "omega1";
static char GLOBALFAR ps_phi1[] = "phi1";
static char GLOBALFAR ps_epsilon[] = "epsilon";
static char GLOBALFAR ps_gradient[] = "gradient";
static char GLOBALFAR ps_parenlefttp[] = "parenlefttp";
static char GLOBALFAR ps_parenleftbt[] = "parenleftbt";
static char GLOBALFAR ps_parenrighttp[] = "parenrighttp";
static char GLOBALFAR ps_parenrightbt[] = "parenrightbt";
static char GLOBALFAR ps_weierstrass[] = "weierstrass";
static char GLOBALFAR ps_bracelefttp[] = "bracelefttp";
static char GLOBALFAR ps_braceleftmid[] = "braceleftmid";
static char GLOBALFAR ps_braceleftbt[] = "braceleftbt";
static char GLOBALFAR ps_braceex[] = "braceex";
static char GLOBALFAR ps_bracerighttp[] = "bracerighttp";
static char GLOBALFAR ps_bracerightmid[] = "bracerightmid";
static char GLOBALFAR ps_bracerightbt[] = "bracerightbt";
static char GLOBALFAR ps_product[] = "product";
static char GLOBALFAR ps_summation[] = "summation";
static char GLOBALFAR ps_periodcentered[] = "periodcentered";
static char GLOBALFAR ps_lozenge[] = "lozenge";
static char GLOBALFAR ps_Upsilon1[] = "Upsilon1";
static char GLOBALFAR ps_arrowvertex[] = "arrowvertex";
static char GLOBALFAR ps_arrowhorizex[] = "arrowhorizex";
static char GLOBALFAR ps_pencil[] = "pencil";
static char GLOBALFAR ps_scissors[] = "scissors";
static char GLOBALFAR ps_scissorscutting[] = "scissorscutting";
static char GLOBALFAR ps_readingglasses[] = "readingglasses";
static char GLOBALFAR ps_bell[] = "bell";
static char GLOBALFAR ps_book[] = "book";
static char GLOBALFAR ps_candle[] = "candle";
static char GLOBALFAR ps_telephonesolid[] = "telephonesolid";
static char GLOBALFAR ps_telhandsetcirc[] = "telhandsetcirc";
static char GLOBALFAR ps_envelopeback[] = "envelopeback";
static char GLOBALFAR ps_envelopefront[] = "envelopefront";
static char GLOBALFAR ps_mailboxflagdwn[] = "mailboxflagdwn";
static char GLOBALFAR ps_mailboxflagup[] = "mailboxflagup";
static char GLOBALFAR ps_mailbxopnflgup[] = "mailbxopnflgup";
static char GLOBALFAR ps_mailbxopnflgdwn[] = "mailbxopnflgdwn";
static char GLOBALFAR ps_folder[] = "folder";
static char GLOBALFAR ps_folderopen[] = "folderopen";
static char GLOBALFAR ps_filetalltext1[] = "filetalltext1";
static char GLOBALFAR ps_filetalltext[] = "filetalltext";
static char GLOBALFAR ps_filetalltext3[] = "filetalltext3";
static char GLOBALFAR ps_filecabinet[] = "filecabinet";
static char GLOBALFAR ps_hourglass[] = "hourglass";
static char GLOBALFAR ps_keyboard[] = "keyboard";
static char GLOBALFAR ps_mouse2button[] = "mouse2button";
static char GLOBALFAR ps_ballpoint[] = "ballpoint";
static char GLOBALFAR ps_pc[] = "pc";
static char GLOBALFAR ps_harddisk[] = "harddisk";
static char GLOBALFAR ps_floppy3[] = "floppy3";
static char GLOBALFAR ps_floppy5[] = "floppy5";
static char GLOBALFAR ps_tapereel[] = "tapereel";
static char GLOBALFAR ps_handwrite[] = "handwrite";
static char GLOBALFAR ps_handwriteleft[] = "handwriteleft";
static char GLOBALFAR ps_handv[] = "handv";
static char GLOBALFAR ps_handok[] = "handok";
static char GLOBALFAR ps_thumbup[] = "thumbup";
static char GLOBALFAR ps_thumbdown[] = "thumbdown";
static char GLOBALFAR ps_handptleft[] = "handptleft";
static char GLOBALFAR ps_handptright[] = "handptright";
static char GLOBALFAR ps_handptup[] = "handptup";
static char GLOBALFAR ps_handptdwn[] = "handptdwn";
static char GLOBALFAR ps_handhalt[] = "handhalt";
static char GLOBALFAR ps_smileface[] = "smileface";
static char GLOBALFAR ps_neutralface[] = "neutralface";
static char GLOBALFAR ps_frownface[] = "frownface";
static char GLOBALFAR ps_bomb[] = "bomb";
static char GLOBALFAR ps_skullcrossbones[] = "skullcrossbones";
static char GLOBALFAR ps_flag[] = "flag";
static char GLOBALFAR ps_pennant[] = "pennant";
static char GLOBALFAR ps_airplane[] = "airplane";
static char GLOBALFAR ps_sunshine[] = "sunshine";
static char GLOBALFAR ps_droplet[] = "droplet";
static char GLOBALFAR ps_snowflake[] = "snowflake";
static char GLOBALFAR ps_crossoutline[] = "crossoutline";
static char GLOBALFAR ps_crossshadow[] = "crossshadow";
static char GLOBALFAR ps_crossceltic[] = "crossceltic";
static char GLOBALFAR ps_crossmaltese[] = "crossmaltese";
static char GLOBALFAR ps_starofdavid[] = "starofdavid";
static char GLOBALFAR ps_crescentstar[] = "crescentstar";
static char GLOBALFAR ps_yinyang[] = "yinyang";
static char GLOBALFAR ps_om[] = "om";
static char GLOBALFAR ps_wheel[] = "wheel";
static char GLOBALFAR ps_aries[] = "aries";
static char GLOBALFAR ps_taurus[] = "taurus";
static char GLOBALFAR ps_gemini[] = "gemini";
static char GLOBALFAR ps_cancer[] = "cancer";
static char GLOBALFAR ps_leo[] = "leo";
static char GLOBALFAR ps_virgo[] = "virgo";
static char GLOBALFAR ps_libra[] = "libra";
static char GLOBALFAR ps_scorpio[] = "scorpio";
static char GLOBALFAR ps_saggitarius[] = "saggitarius";
static char GLOBALFAR ps_capricorn[] = "capricorn";
static char GLOBALFAR ps_aquarius[] = "aquarius";
static char GLOBALFAR ps_pisces[] = "pisces";
static char GLOBALFAR ps_ampersanditlc[] = "ampersanditlc";
static char GLOBALFAR ps_ampersandit[] = "ampersandit";
static char GLOBALFAR ps_cirlcle6[] = "cirlcle6";
static char GLOBALFAR ps_circleshadowdwn[] = "circleshadowdwn";
static char GLOBALFAR ps_square6[] = "square6";
static char GLOBALFAR ps_box3[] = "box3";
static char GLOBALFAR ps_box4[] = "box4";
static char GLOBALFAR ps_boxshadowdwn[] = "boxshadowdwn";
static char GLOBALFAR ps_boxshadowup[] = "boxshadowup";
static char GLOBALFAR ps_lozenge4[] = "lozenge4";
static char GLOBALFAR ps_lozenge6[] = "lozenge6";
static char GLOBALFAR ps_rhombus6[] = "rhombus6";
static char GLOBALFAR ps_xrhombus[] = "xrhombus";
static char GLOBALFAR ps_rhombus4[] = "rhombus4";
static char GLOBALFAR ps_clear[] = "clear";
static char GLOBALFAR ps_escape[] = "escape";
static char GLOBALFAR ps_command[] = "command";
static char GLOBALFAR ps_rosette[] = "rosette";
static char GLOBALFAR ps_rosettesolid[] = "rosettesolid";
static char GLOBALFAR ps_quotedbllftbld[] = "quotedbllftbld";
static char GLOBALFAR ps_quotedblrtbld[] = "quotedblrtbld";
static char GLOBALFAR ps_zerosans[] = "zerosans";
static char GLOBALFAR ps_onesans[] = "onesans";
static char GLOBALFAR ps_twosans[] = "twosans";
static char GLOBALFAR ps_threesans[] = "threesans";
static char GLOBALFAR ps_foursans[] = "foursans";
static char GLOBALFAR ps_fivesans[] = "fivesans";
static char GLOBALFAR ps_sixsans[] = "sixsans";
static char GLOBALFAR ps_sevensans[] = "sevensans";
static char GLOBALFAR ps_eightsans[] = "eightsans";
static char GLOBALFAR ps_ninesans[] = "ninesans";
static char GLOBALFAR ps_tensans[] = "tensans";
static char GLOBALFAR ps_zerosansinv[] = "zerosansinv";
static char GLOBALFAR ps_onesansinv[] = "onesansinv";
static char GLOBALFAR ps_twosansinv[] = "twosansinv";
static char GLOBALFAR ps_threesansinv[] = "threesansinv";
static char GLOBALFAR ps_foursansinv[] = "foursansinv";
static char GLOBALFAR ps_fivesansinv[] = "fivesansinv";
static char GLOBALFAR ps_sixsansinv[] = "sixsansinv";
static char GLOBALFAR ps_sevensansinv[] = "sevensansinv";
static char GLOBALFAR ps_eightsansinv[] = "eightsansinv";
static char GLOBALFAR ps_ninesansinv[] = "ninesansinv";
static char GLOBALFAR ps_tensansinv[] = "tensansinv";
static char GLOBALFAR ps_budleafne[] = "budleafne";
static char GLOBALFAR ps_budleafnw[] = "budleafnw";
static char GLOBALFAR ps_budleafsw[] = "budleafsw";
static char GLOBALFAR ps_budleafse[] = "budleafse";
static char GLOBALFAR ps_vineleafboldne[] = "vineleafboldne";
static char GLOBALFAR ps_vineleafboldnw[] = "vineleafboldnw";
static char GLOBALFAR ps_vineleafboldsw[] = "vineleafboldsw";
static char GLOBALFAR ps_fineleafboldse[] = "fineleafboldse";
static char GLOBALFAR ps_circle2[] = "circle2";
static char GLOBALFAR ps_cirlcle4[] = "cirlcle4";
static char GLOBALFAR ps_square2[] = "square2";
static char GLOBALFAR ps_ring2[] = "ring2";
static char GLOBALFAR ps_ring4[] = "ring4";
static char GLOBALFAR ps_ring6[] = "ring6";
static char GLOBALFAR ps_ringbutton2[] = "ringbutton2";
static char GLOBALFAR ps_target[] = "target";
static char GLOBALFAR ps_cirlcleshadowup[] = "cirlcleshadowup";
static char GLOBALFAR ps_square4[] = "square4";
static char GLOBALFAR ps_box2[] = "box2";
static char GLOBALFAR ps_tristar2[] = "tristar2";
static char GLOBALFAR ps_crosstar2[] = "crosstar2";
static char GLOBALFAR ps_pentastar2[] = "pentastar2";
static char GLOBALFAR ps_hexstar2[] = "hexstar2";
static char GLOBALFAR ps_octastar2[] = "octastar2";
static char GLOBALFAR ps_dodecastar3[] = "dodecastar3";
static char GLOBALFAR ps_octastar4[] = "octastar4";
static char GLOBALFAR ps_registersquare[] = "registersquare";
static char GLOBALFAR ps_registercircle[] = "registercircle";
static char GLOBALFAR ps_cuspopen[] = "cuspopen";
static char GLOBALFAR ps_cusopen1[] = "cusopen1";
static char GLOBALFAR ps_query[] = "query";
static char GLOBALFAR ps_circlestar[] = "circlestar";
static char GLOBALFAR ps_starshadow[] = "starshadow";
static char GLOBALFAR ps_oneoclock[] = "oneoclock";
static char GLOBALFAR ps_twooclock[] = "twooclock";
static char GLOBALFAR ps_threeoclock[] = "threeoclock";
static char GLOBALFAR ps_fouroclock[] = "fouroclock";
static char GLOBALFAR ps_fiveoclock[] = "fiveoclock";
static char GLOBALFAR ps_sixoclock[] = "sixoclock";
static char GLOBALFAR ps_sevenoclock[] = "sevenoclock";
static char GLOBALFAR ps_eightoclock[] = "eightoclock";
static char GLOBALFAR ps_nineoclock[] = "nineoclock";
static char GLOBALFAR ps_tenoclock[] = "tenoclock";
static char GLOBALFAR ps_elevenoclock[] = "elevenoclock";
static char GLOBALFAR ps_twelveoclock[] = "twelveoclock";
static char GLOBALFAR ps_arrowdwnleft1[] = "arrowdwnleft1";
static char GLOBALFAR ps_arrowdwnrt1[] = "arrowdwnrt1";
static char GLOBALFAR ps_arrowupleft1[] = "arrowupleft1";
static char GLOBALFAR ps_arrowuprt1[] = "arrowuprt1";
static char GLOBALFAR ps_arrowleftup1[] = "arrowleftup1";
static char GLOBALFAR ps_arrowrtup1[] = "arrowrtup1";
static char GLOBALFAR ps_arrowleftdwn1[] = "arrowleftdwn1";
static char GLOBALFAR ps_arrowrtdwn1[] = "arrowrtdwn1";
static char GLOBALFAR ps_quiltsquare2[] = "quiltsquare2";
static char GLOBALFAR ps_quiltsquare2inv[] = "quiltsquare2inv";
static char GLOBALFAR ps_leafccwsw[] = "leafccwsw";
static char GLOBALFAR ps_leafccwnw[] = "leafccwnw";
static char GLOBALFAR ps_leafccwse[] = "leafccwse";
static char GLOBALFAR ps_leafccwne[] = "leafccwne";
static char GLOBALFAR ps_leafnw[] = "leafnw";
static char GLOBALFAR ps_leafsw[] = "leafsw";
static char GLOBALFAR ps_leafne[] = "leafne";
static char GLOBALFAR ps_leafse[] = "leafse";
static char GLOBALFAR ps_deleteleft[] = "deleteleft";
static char GLOBALFAR ps_deleteright[] = "deleteright";
static char GLOBALFAR ps_head2left[] = "head2left";
static char GLOBALFAR ps_head2right[] = "head2right";
static char GLOBALFAR ps_head2up[] = "head2up";
static char GLOBALFAR ps_head2down[] = "head2down";
static char GLOBALFAR ps_circleleft[] = "circleleft";
static char GLOBALFAR ps_circleright[] = "circleright";
static char GLOBALFAR ps_circleup[] = "circleup";
static char GLOBALFAR ps_circledown[] = "circledown";
static char GLOBALFAR ps_barb2left[] = "barb2left";
static char GLOBALFAR ps_barb2right[] = "barb2right";
static char GLOBALFAR ps_barb2up[] = "barb2up";
static char GLOBALFAR ps_barb2down[] = "barb2down";
static char GLOBALFAR ps_barb2nw[] = "barb2nw";
static char GLOBALFAR ps_barb2ne[] = "barb2ne";
static char GLOBALFAR ps_barb2sw[] = "barb2sw";
static char GLOBALFAR ps_barb2se[] = "barb2se";
static char GLOBALFAR ps_barb4left[] = "barb4left";
static char GLOBALFAR ps_barb4right[] = "barb4right";
static char GLOBALFAR ps_barb4up[] = "barb4up";
static char GLOBALFAR ps_barb4down[] = "barb4down";
static char GLOBALFAR ps_barb4nw[] = "barb4nw";
static char GLOBALFAR ps_barb4ne[] = "barb4ne";
static char GLOBALFAR ps_barb4sw[] = "barb4sw";
static char GLOBALFAR ps_barb4se[] = "barb4se";
static char GLOBALFAR ps_bleft[] = "bleft";
static char GLOBALFAR ps_bright[] = "bright";
static char GLOBALFAR ps_bup[] = "bup";
static char GLOBALFAR ps_bdown[] = "bdown";
static char GLOBALFAR ps_bleftright[] = "bleftright";
static char GLOBALFAR ps_bupdown[] = "bupdown";
static char GLOBALFAR ps_bnw[] = "bnw";
static char GLOBALFAR ps_bne[] = "bne";
static char GLOBALFAR ps_bsw[] = "bsw";
static char GLOBALFAR ps_bse[] = "bse";
static char GLOBALFAR ps_bdash1[] = "bdash1";
static char GLOBALFAR ps_bdash2[] = "bdash2";
static char GLOBALFAR ps_xmarkbld[] = "xmarkbld";
static char GLOBALFAR ps_checkbld[] = "checkbld";
static char GLOBALFAR ps_boxxmarkbld[] = "boxxmarkbld";
static char GLOBALFAR ps_boxcheckbld[] = "boxcheckbld";
static char GLOBALFAR ps_windowslogo[] = "windowslogo";
static char GLOBALFAR ps_apple[] = "apple";
static char GLOBALFAR ps_parenleftex[] = "parenleftex";
static char GLOBALFAR ps_bracketleftex[] = "bracketleftex";
static char GLOBALFAR ps_parenrightex[] = "parenrightex";
static char GLOBALFAR ps_bracketrightex[] = "bracketrightex";
static char GLOBALFAR ps_copyrightserif[] = "copyrightserif";
static char GLOBALFAR ps_registerserif[] = "registerserif";
static char GLOBALFAR ps_trademarkserif[] = "trademarkserif";
static char GLOBALFAR ps_a2[] = "a2";
static char GLOBALFAR ps_a1[] = "a1";
static char GLOBALFAR ps_a117[] = "a117";
static char GLOBALFAR ps_a97[] = "a97";
static char GLOBALFAR ps_a98[] = "a98";
static char GLOBALFAR ps_a87[] = "a87";
static char GLOBALFAR ps_a88[] = "a88";
static char GLOBALFAR ps_a19[] = "a19";
static char GLOBALFAR ps_a71[] = "a71";
static char GLOBALFAR ps_a76[] = "a76";
static char GLOBALFAR ps_a92[] = "a92";
static char GLOBALFAR ps_a86[] = "a86";
static char GLOBALFAR ps_a91[] = "a91";
static char GLOBALFAR ps_a35[] = "a35";
static char GLOBALFAR ps_a36[] = "a36";
static char GLOBALFAR ps_a37[] = "a37";
static char GLOBALFAR ps_a68[] = "a68";
static char GLOBALFAR ps_a49[] = "a49";
static char GLOBALFAR ps_a41[] = "a41";
static char GLOBALFAR ps_a38[] = "a38";
static char GLOBALFAR ps_a50[] = "a50";
static char GLOBALFAR ps_a43[] = "a43";
static char GLOBALFAR ps_a42[] = "a42";
static char GLOBALFAR ps_a40[] = "a40";
static char GLOBALFAR ps_a39[] = "a39";
static char GLOBALFAR ps_a140[] = "a140";
static char GLOBALFAR ps_a141[] = "a141";
static char GLOBALFAR ps_a142[] = "a142";
static char GLOBALFAR ps_a143[] = "a143";
static char GLOBALFAR ps_a144[] = "a144";
static char GLOBALFAR ps_a145[] = "a145";
static char GLOBALFAR ps_a146[] = "a146";
static char GLOBALFAR ps_a147[] = "a147";
static char GLOBALFAR ps_a148[] = "a148";
static char GLOBALFAR ps_a149[] = "a149";
static char GLOBALFAR ps_a150[] = "a150";
static char GLOBALFAR ps_a151[] = "a151";
static char GLOBALFAR ps_a152[] = "a152";
static char GLOBALFAR ps_a153[] = "a153";
static char GLOBALFAR ps_a154[] = "a154";
static char GLOBALFAR ps_a155[] = "a155";
static char GLOBALFAR ps_a156[] = "a156";
static char GLOBALFAR ps_a157[] = "a157";
static char GLOBALFAR ps_a158[] = "a158";
static char GLOBALFAR ps_a159[] = "a159";
static char GLOBALFAR ps_a72[] = "a72";
static char GLOBALFAR ps_a167[] = "a167";
static char GLOBALFAR ps_a168[] = "a168";
static char GLOBALFAR ps_a169[] = "a169";
static char GLOBALFAR ps_a177[] = "a177";
static char GLOBALFAR ps_a161[] = "a161";
static char GLOBALFAR ps_a163[] = "a163";
static char GLOBALFAR ps_a164[] = "a164";
static char GLOBALFAR ps_a190[] = "a190";
static char GLOBALFAR ps_a186[] = "a186";
static char GLOBALFAR ps_a172[] = "a172";
static char GLOBALFAR ps_a175[] = "a175";
static char GLOBALFAR ps_a176[] = "a176";
static char GLOBALFAR ps_a166[] = "a166";
static char GLOBALFAR ps_a79[] = "a79";
static char GLOBALFAR ps_a23[] = "a23";
static char GLOBALFAR ps_a24[] = "a24";
static char GLOBALFAR ps_a29[] = "a29";
static char GLOBALFAR ps_a6[] = "a6";
static char GLOBALFAR ps_a7[] = "a7";
static char GLOBALFAR ps_a34[] = "a34";
static char GLOBALFAR ps_a118[] = "a118";
static char GLOBALFAR ps_a12[] = "a12";
static char GLOBALFAR ps_a11[] = "a11";
static char GLOBALFAR ps_a5[] = "a5";
static char GLOBALFAR ps_a107[] = "a107";
static char GLOBALFAR ps_a108[] = "a108";
static char GLOBALFAR ps_a93[] = "a93";
static char GLOBALFAR ps_a94[] = "a94";
static char GLOBALFAR ps_a20[] = "a20";
static char GLOBALFAR ps_a73[] = "a73";
static char GLOBALFAR ps_a77[] = "a77";
static char GLOBALFAR ps_a21[] = "a21";
static char GLOBALFAR ps_a22[] = "a22";
static char GLOBALFAR ps_a85[] = "a85";
static char GLOBALFAR ps_a81[] = "a81";
static char GLOBALFAR ps_a44[] = "a44";
static char GLOBALFAR ps_a45[] = "a45";
static char GLOBALFAR ps_a51[] = "a51";
static char GLOBALFAR ps_a52[] = "a52";
static char GLOBALFAR ps_a56[] = "a56";
static char GLOBALFAR ps_a54[] = "a54";
static char GLOBALFAR ps_a57[] = "a57";
static char GLOBALFAR ps_a62[] = "a62";
static char GLOBALFAR ps_a66[] = "a66";
static char GLOBALFAR ps_a67[] = "a67";
static char GLOBALFAR ps_a70[] = "a70";
static char GLOBALFAR ps_a69[] = "a69";
static char GLOBALFAR ps_a60[] = "a60";
static char GLOBALFAR ps_a120[] = "a120";
static char GLOBALFAR ps_a121[] = "a121";
static char GLOBALFAR ps_a122[] = "a122";
static char GLOBALFAR ps_a123[] = "a123";
static char GLOBALFAR ps_a124[] = "a124";
static char GLOBALFAR ps_a125[] = "a125";
static char GLOBALFAR ps_a126[] = "a126";
static char GLOBALFAR ps_a127[] = "a127";
static char GLOBALFAR ps_a128[] = "a128";
static char GLOBALFAR ps_a129[] = "a129";
static char GLOBALFAR ps_a130[] = "a130";
static char GLOBALFAR ps_a131[] = "a131";
static char GLOBALFAR ps_a132[] = "a132";
static char GLOBALFAR ps_a133[] = "a133";
static char GLOBALFAR ps_a134[] = "a134";
static char GLOBALFAR ps_a135[] = "a135";
static char GLOBALFAR ps_a136[] = "a136";
static char GLOBALFAR ps_a137[] = "a137";
static char GLOBALFAR ps_a138[] = "a138";
static char GLOBALFAR ps_a139[] = "a139";
static char GLOBALFAR ps_a75[] = "a75";
static char GLOBALFAR ps_a74[] = "a74";
static char GLOBALFAR ps_a173[] = "a173";
static char GLOBALFAR ps_a174[] = "a174";
static char GLOBALFAR ps_a189[] = "a189";
static char GLOBALFAR ps_a191[] = "a191";
static char GLOBALFAR ps_a165[] = "a165";
static char GLOBALFAR ps_a178[] = "a178";
static char GLOBALFAR ps_a184[] = "a184";
static char GLOBALFAR ps_a160[] = "a160";
static char GLOBALFAR ps_a185[] = "a185";
static char GLOBALFAR ps_a187[] = "a187";
static char GLOBALFAR ps_a188[] = "a188";
static char GLOBALFAR ps_a196[] = "a196";
static char GLOBALFAR ps_a197[] = "a197";
static char GLOBALFAR ps_a198[] = "a198";
static char GLOBALFAR ps_a9[] = "a9";
static char GLOBALFAR ps_a106[] = "a106";
static char GLOBALFAR ps_a13[] = "a13";
static char GLOBALFAR ps_a3[] = "a3";
static char GLOBALFAR ps_a4[] = "a4";
static char GLOBALFAR ps_a14[] = "a14";
static char GLOBALFAR ps_a15[] = "a15";
static char GLOBALFAR ps_a101[] = "a101";
static char GLOBALFAR ps_a99[] = "a99";
static char GLOBALFAR ps_a100[] = "a100";
static char GLOBALFAR ps_a89[] = "a89";
static char GLOBALFAR ps_a90[] = "a90";
static char GLOBALFAR ps_a103[] = "a103";
static char GLOBALFAR ps_a102[] = "a102";
static char GLOBALFAR ps_a78[] = "a78";
static char GLOBALFAR ps_a82[] = "a82";
static char GLOBALFAR ps_a83[] = "a83";
static char GLOBALFAR ps_a84[] = "a84";
static char GLOBALFAR ps_a95[] = "a95";
static char GLOBALFAR ps_a96[] = "a96";
static char GLOBALFAR ps_a55[] = "a55";
static char GLOBALFAR ps_a58[] = "a58";
static char GLOBALFAR ps_a46[] = "a46";
static char GLOBALFAR ps_a27[] = "a27";
static char GLOBALFAR ps_a28[] = "a28";
static char GLOBALFAR ps_a30[] = "a30";
static char GLOBALFAR ps_a31[] = "a31";
static char GLOBALFAR ps_a48[] = "a48";
static char GLOBALFAR ps_a47[] = "a47";
static char GLOBALFAR ps_a53[] = "a53";
static char GLOBALFAR ps_a119[] = "a119";
static char GLOBALFAR ps_a61[] = "a61";
static char GLOBALFAR ps_a59[] = "a59";
static char GLOBALFAR ps_a64[] = "a64";
static char GLOBALFAR ps_a65[] = "a65";
static char GLOBALFAR ps_a63[] = "a63";
static char GLOBALFAR ps_a104[] = "a104";
static char GLOBALFAR ps_a179[] = "a179";
static char GLOBALFAR ps_a180[] = "a180";
static char GLOBALFAR ps_a170[] = "a170";
static char GLOBALFAR ps_a171[] = "a171";
static char GLOBALFAR ps_a181[] = "a181";
static char GLOBALFAR ps_a182[] = "a182";
static char GLOBALFAR ps_a183[] = "a183";
static char GLOBALFAR ps_a109[] = "a109";
static char GLOBALFAR ps_a110[] = "a110";
static char GLOBALFAR ps_a111[] = "a111";
static char GLOBALFAR ps_a112[] = "a112";
static char GLOBALFAR ps_a32[] = "a32";
static char GLOBALFAR ps_a25[] = "a25";
static char GLOBALFAR ps_a8[] = "a8";
static char GLOBALFAR ps_a10[] = "a10";
static char GLOBALFAR ps_copyrightsans[] = "copyrightsans";
static char GLOBALFAR ps_registersans[] = "registersans";
static char GLOBALFAR ps_trademarksans[] = "trademarksans";
static char GLOBALFAR ps_a202[] = "a202";
static char GLOBALFAR ps_a105[] = "a105";
static char GLOBALFAR ps_a26[] = "a26";
static char GLOBALFAR ps_a33[] = "a33";
static char GLOBALFAR ps_a203[] = "a203";
static char GLOBALFAR ps_a204[] = "a204";
static char GLOBALFAR ps_a192[] = "a192";
static char GLOBALFAR ps_a162[] = "a162";
static char GLOBALFAR ps_a199[] = "a199";
static char GLOBALFAR ps_a195[] = "a195";
static char GLOBALFAR ps_a206[] = "a206";
static char GLOBALFAR ps_a205[] = "a205";
static char GLOBALFAR ps_a200[] = "a200";
static char GLOBALFAR ps_a201[] = "a201";
static char GLOBALFAR ps_a194[] = "a194";
static char GLOBALFAR ps_Ifraktur[] = "Ifraktur";
static char GLOBALFAR ps_Rfraktur[] = "Rfraktur";
static char GLOBALFAR ps_similar[] = "similar";
static char GLOBALFAR ps_carriagereturn[] = "carriagereturn";
static char GLOBALFAR ps_a16[] = "a16";
static char GLOBALFAR ps_a17[] = "a17";
static char GLOBALFAR ps_a193[] = "a193";
static char GLOBALFAR ps_a18[] = "a18";
static char GLOBALFAR ps_sfthyphen[] = "sfthyphen";
/* Cyrillic Names: */
static char GLOBALFAR ps_afii10023[] = "afii10023";
static char GLOBALFAR ps_afii10051[] = "afii10051";
static char GLOBALFAR ps_afii10052[] = "afii10052";
static char GLOBALFAR ps_afii10053[] = "afii10053";
static char GLOBALFAR ps_afii10054[] = "afii10054";
static char GLOBALFAR ps_afii10055[] = "afii10055";
static char GLOBALFAR ps_afii10056[] = "afii10056";
static char GLOBALFAR ps_afii10057[] = "afii10057";
static char GLOBALFAR ps_afii10058[] = "afii10058";
static char GLOBALFAR ps_afii10059[] = "afii10059";
static char GLOBALFAR ps_afii10060[] = "afii10060";
static char GLOBALFAR ps_afii10061[] = "afii10061";
static char GLOBALFAR ps_afii10062[] = "afii10062";
static char GLOBALFAR ps_afii10145[] = "afii10145";
static char GLOBALFAR ps_afii10017[] = "afii10017";
static char GLOBALFAR ps_afii10019[] = "afii10019";
static char GLOBALFAR ps_afii10018[] = "afii10018";
static char GLOBALFAR ps_afii10020[] = "afii10020";
static char GLOBALFAR ps_afii10021[] = "afii10021";
static char GLOBALFAR ps_afii10022[] = "afii10022";
static char GLOBALFAR ps_afii10024[] = "afii10024";
static char GLOBALFAR ps_afii10025[] = "afii10025";
static char GLOBALFAR ps_afii10026[] = "afii10026";
static char GLOBALFAR ps_afii10027[] = "afii10027";
static char GLOBALFAR ps_afii10028[] = "afii10028";
static char GLOBALFAR ps_afii10029[] = "afii10029";
static char GLOBALFAR ps_afii10030[] = "afii10030";
static char GLOBALFAR ps_afii10031[] = "afii10031";
static char GLOBALFAR ps_afii10032[] = "afii10032";
static char GLOBALFAR ps_afii10033[] = "afii10033";
static char GLOBALFAR ps_afii10034[] = "afii10034";
static char GLOBALFAR ps_afii10035[] = "afii10035";
static char GLOBALFAR ps_afii10036[] = "afii10036";
static char GLOBALFAR ps_afii10037[] = "afii10037";
static char GLOBALFAR ps_afii10038[] = "afii10038";
static char GLOBALFAR ps_afii10039[] = "afii10039";
static char GLOBALFAR ps_afii10040[] = "afii10040";
static char GLOBALFAR ps_afii10041[] = "afii10041";
static char GLOBALFAR ps_afii10042[] = "afii10042";
static char GLOBALFAR ps_afii10043[] = "afii10043";
static char GLOBALFAR ps_afii10044[] = "afii10044";
static char GLOBALFAR ps_afii10045[] = "afii10045";
static char GLOBALFAR ps_afii10046[] = "afii10046";
static char GLOBALFAR ps_afii10047[] = "afii10047";
static char GLOBALFAR ps_afii10048[] = "afii10048";
static char GLOBALFAR ps_afii10049[] = "afii10049";
static char GLOBALFAR ps_afii10065[] = "afii10065";
static char GLOBALFAR ps_afii10066[] = "afii10066";
static char GLOBALFAR ps_afii10067[] = "afii10067";
static char GLOBALFAR ps_afii10068[] = "afii10068";
static char GLOBALFAR ps_afii10069[] = "afii10069";
static char GLOBALFAR ps_afii10070[] = "afii10070";
static char GLOBALFAR ps_afii10072[] = "afii10072";
static char GLOBALFAR ps_afii10073[] = "afii10073";
static char GLOBALFAR ps_afii10074[] = "afii10074";
static char GLOBALFAR ps_afii10075[] = "afii10075";
static char GLOBALFAR ps_afii10076[] = "afii10076";
static char GLOBALFAR ps_afii10077[] = "afii10077";
static char GLOBALFAR ps_afii10078[] = "afii10078";
static char GLOBALFAR ps_afii10079[] = "afii10079";
static char GLOBALFAR ps_afii10080[] = "afii10080";
static char GLOBALFAR ps_afii10081[] = "afii10081";
static char GLOBALFAR ps_afii10082[] = "afii10082";
static char GLOBALFAR ps_afii10083[] = "afii10083";
static char GLOBALFAR ps_afii10084[] = "afii10084";
static char GLOBALFAR ps_afii10085[] = "afii10085";
static char GLOBALFAR ps_afii10086[] = "afii10086";
static char GLOBALFAR ps_afii10087[] = "afii10087";
static char GLOBALFAR ps_afii10088[] = "afii10088";
static char GLOBALFAR ps_afii10089[] = "afii10089";
static char GLOBALFAR ps_afii10090[] = "afii10090";
static char GLOBALFAR ps_afii10091[] = "afii10091";
static char GLOBALFAR ps_afii10092[] = "afii10092";
static char GLOBALFAR ps_afii10093[] = "afii10093";
static char GLOBALFAR ps_afii10094[] = "afii10094";
static char GLOBALFAR ps_afii10095[] = "afii10095";
static char GLOBALFAR ps_afii10096[] = "afii10096";
static char GLOBALFAR ps_afii10097[] = "afii10097";
static char GLOBALFAR ps_afii61352[] = "afii61352";
static char GLOBALFAR ps_afii10071[] = "afii10071";
static char GLOBALFAR ps_afii10099[] = "afii10099";
static char GLOBALFAR ps_afii10100[] = "afii10100";
static char GLOBALFAR ps_afii10101[] = "afii10101";
static char GLOBALFAR ps_afii10102[] = "afii10102";
static char GLOBALFAR ps_afii10103[] = "afii10103";
static char GLOBALFAR ps_afii10104[] = "afii10104";
static char GLOBALFAR ps_afii10105[] = "afii10105";
static char GLOBALFAR ps_afii10106[] = "afii10106";
static char GLOBALFAR ps_afii10107[] = "afii10107";
static char GLOBALFAR ps_afii10108[] = "afii10108";
static char GLOBALFAR ps_afii10109[] = "afii10109";
static char GLOBALFAR ps_afii10110[] = "afii10110";
static char GLOBALFAR ps_afii10193[] = "afii10193";
/* Baltic names */
static char GLOBALFAR ps_Amacron[] = "Amacron";
static char GLOBALFAR ps_amacron[] = "amacron";
static char GLOBALFAR ps_Emacron[] = "Emacron";
static char GLOBALFAR ps_emacron[] = "emacron";
static char GLOBALFAR ps_Edot[] = "Edot";
static char GLOBALFAR ps_edot[] = "edot";
static char GLOBALFAR ps_Gcedilla[] = "Gcedilla";
static char GLOBALFAR ps_gcedilla[] = "gcedilla";
static char GLOBALFAR ps_Itilde[] = "Itilde";
static char GLOBALFAR ps_itilde[] = "itilde";
static char GLOBALFAR ps_Imacron[] = "Imacron";
static char GLOBALFAR ps_imacron[] = "imacron";
static char GLOBALFAR ps_Iogonek[] = "Iogonek";
static char GLOBALFAR ps_iogonek[] = "iogonek";
static char GLOBALFAR ps_Kcedilla[] = "Kcedilla";
static char GLOBALFAR ps_kcedilla[] = "kcedilla";
static char GLOBALFAR ps_kra[] = "kra";
static char GLOBALFAR ps_Lcedilla[] = "Lcedilla";
static char GLOBALFAR ps_lcedilla[] = "lcedilla";
static char GLOBALFAR ps_Ncedilla[] = "Ncedilla";
static char GLOBALFAR ps_ncedilla[] = "ncedilla";
static char GLOBALFAR ps_Eng[] = "Eng";
static char GLOBALFAR ps_eng[] = "eng";
static char GLOBALFAR ps_Omacron[] = "Omacron";
static char GLOBALFAR ps_omacron[] = "omacron";
static char GLOBALFAR ps_Rcedilla[] = "Rcedilla";
static char GLOBALFAR ps_rcedilla[] = "rcedilla";
static char GLOBALFAR ps_Tbar[] = "Tbar";
static char GLOBALFAR ps_tbar[] = "tbar";
static char GLOBALFAR ps_Utilde[] = "Utilde";
static char GLOBALFAR ps_utilde[] = "utilde";
static char GLOBALFAR ps_Umacron[] = "Umacron";
static char GLOBALFAR ps_umacron[] = "umacron";
static char GLOBALFAR ps_Uogonek[] = "Uogonek";
static char GLOBALFAR ps_uogonek[] = "uogonek";
#endif /* CMT_PS */

GlyphCodes  GLOBALFAR gMasterGlyphCodes [] =
{
{BciDECL(32)	MslDECL(0)	UniDECL(0x0020)	PsDECL(ps_space)	UsrDECL(0)},
{BciDECL(33)	MslDECL(1)	UniDECL(0x0021)	PsDECL(ps_exclam)	UsrDECL(0)},
{BciDECL(34)	MslDECL(2)	UniDECL(0x0022)	PsDECL(ps_quotedbl)	UsrDECL(0)},
{BciDECL(35)	MslDECL(3)	UniDECL(0x0023)	PsDECL(ps_numbersign)	UsrDECL(0)},
{BciDECL(36)	MslDECL(4)	UniDECL(0x0024)	PsDECL(ps_dollar)	UsrDECL(0)},
{BciDECL(37)	MslDECL(5)	UniDECL(0x0025)	PsDECL(ps_percent)	UsrDECL(0)},
{BciDECL(38)	MslDECL(6)	UniDECL(0x0026)	PsDECL(ps_ampersand)	UsrDECL(0)},
{BciDECL(39)	MslDECL(8)	UniDECL(0x2019)	PsDECL(ps_quoteright)	UsrDECL(0)},
{BciDECL(40)	MslDECL(9)	UniDECL(0x0028)	PsDECL(ps_parenleft)	UsrDECL(0)},
{BciDECL(41)	MslDECL(10)	UniDECL(0x0029)	PsDECL(ps_parenright)	UsrDECL(0)},
{BciDECL(42)	MslDECL(11)	UniDECL(0x002a)	PsDECL(ps_asterisk)	UsrDECL(0)},
{BciDECL(43)	MslDECL(12)	UniDECL(0x002b)	PsDECL(ps_plus)         UsrDECL(0)},
{BciDECL(44)	MslDECL(13)	UniDECL(0x002c)	PsDECL(ps_comma)	UsrDECL(0)},
{BciDECL(45)	MslDECL(14)	UniDECL(0x002d)	PsDECL(ps_hyphen)	UsrDECL(0)},
{BciDECL(46)	MslDECL(15)	UniDECL(0x002e)	PsDECL(ps_period)	UsrDECL(0)},
{BciDECL(47)	MslDECL(16)	UniDECL(0x002f)	PsDECL(ps_slash)	UsrDECL(0)},
{BciDECL(48)	MslDECL(17)	UniDECL(0x0030)	PsDECL(ps_zero)         UsrDECL(0)},
{BciDECL(49)	MslDECL(18)	UniDECL(0x0031)	PsDECL(ps_one)          UsrDECL(0)},
{BciDECL(50)	MslDECL(19)	UniDECL(0x0032)	PsDECL(ps_two)		UsrDECL(0)},
{BciDECL(51)	MslDECL(20)	UniDECL(0x0033)	PsDECL(ps_three)	UsrDECL(0)},
{BciDECL(52)	MslDECL(21)	UniDECL(0x0034)	PsDECL(ps_four)		UsrDECL(0)},
{BciDECL(53)	MslDECL(22)	UniDECL(0x0035)	PsDECL(ps_five)		UsrDECL(0)},
{BciDECL(54)	MslDECL(23)	UniDECL(0x0036)	PsDECL(ps_six)		UsrDECL(0)},
{BciDECL(55)	MslDECL(24)	UniDECL(0x0037)	PsDECL(ps_seven)	UsrDECL(0)},
{BciDECL(56)	MslDECL(25)	UniDECL(0x0038)	PsDECL(ps_eight)	UsrDECL(0)},
{BciDECL(57)	MslDECL(26)	UniDECL(0x0039)	PsDECL(ps_nine)		UsrDECL(0)},
{BciDECL(58)	MslDECL(27)	UniDECL(0x003a)	PsDECL(ps_colon)	UsrDECL(0)},
{BciDECL(59)	MslDECL(28)	UniDECL(0x003b)	PsDECL(ps_semicolon)	UsrDECL(0)},
{BciDECL(60)	MslDECL(29)	UniDECL(0x003c)	PsDECL(ps_less)		UsrDECL(0)},
{BciDECL(61)	MslDECL(30)	UniDECL(0x003d)	PsDECL(ps_equal)	UsrDECL(0)},
{BciDECL(62)	MslDECL(31)	UniDECL(0x003e)	PsDECL(ps_greater)	UsrDECL(0)},
{BciDECL(63)	MslDECL(32)	UniDECL(0x003f)	PsDECL(ps_question)	UsrDECL(0)},
{BciDECL(64)	MslDECL(33)	UniDECL(0x0040)	PsDECL(ps_at)		UsrDECL(0)},
{BciDECL(65)	MslDECL(34)	UniDECL(0x0041)	PsDECL(ps_A)		UsrDECL(0)},
{BciDECL(66)	MslDECL(35)	UniDECL(0x0042)	PsDECL(ps_B)		UsrDECL(0)},
{BciDECL(67)	MslDECL(36)	UniDECL(0x0043)	PsDECL(ps_C)		UsrDECL(0)},
{BciDECL(68)	MslDECL(37)	UniDECL(0x0044)	PsDECL(ps_D)		UsrDECL(0)},
{BciDECL(69)	MslDECL(38)	UniDECL(0x0045)	PsDECL(ps_E)		UsrDECL(0)},
{BciDECL(70)	MslDECL(39)	UniDECL(0x0046)	PsDECL(ps_F)		UsrDECL(0)},
{BciDECL(71)	MslDECL(40)	UniDECL(0x0047)	PsDECL(ps_G)		UsrDECL(0)},
{BciDECL(72)	MslDECL(41)	UniDECL(0x0048)	PsDECL(ps_H)		UsrDECL(0)},
{BciDECL(73)	MslDECL(42)	UniDECL(0x0049)	PsDECL(ps_I)		UsrDECL(0)},
{BciDECL(74)	MslDECL(43)	UniDECL(0x004a)	PsDECL(ps_J)		UsrDECL(0)},
{BciDECL(75)	MslDECL(44)	UniDECL(0x004b)	PsDECL(ps_K)		UsrDECL(0)},
{BciDECL(76)	MslDECL(45)	UniDECL(0x004c)	PsDECL(ps_L)		UsrDECL(0)},
{BciDECL(77)	MslDECL(46)	UniDECL(0x004d)	PsDECL(ps_M)		UsrDECL(0)},
{BciDECL(78)	MslDECL(47)	UniDECL(0x004e)	PsDECL(ps_N)		UsrDECL(0)},
{BciDECL(79)	MslDECL(48)	UniDECL(0x004f)	PsDECL(ps_O)		UsrDECL(0)},
{BciDECL(80)	MslDECL(49)	UniDECL(0x0050)	PsDECL(ps_P)		UsrDECL(0)},
{BciDECL(81)	MslDECL(50)	UniDECL(0x0051)	PsDECL(ps_Q)		UsrDECL(0)},
{BciDECL(82)	MslDECL(51)	UniDECL(0x0052)	PsDECL(ps_R)		UsrDECL(0)},
{BciDECL(83)	MslDECL(52)	UniDECL(0x0053)	PsDECL(ps_S)		UsrDECL(0)},
{BciDECL(84)	MslDECL(53)	UniDECL(0x0054)	PsDECL(ps_T)		UsrDECL(0)},
{BciDECL(85)	MslDECL(54)	UniDECL(0x0055)	PsDECL(ps_U)		UsrDECL(0)},
{BciDECL(86)	MslDECL(55)	UniDECL(0x0056)	PsDECL(ps_V)		UsrDECL(0)},
{BciDECL(87)	MslDECL(56)	UniDECL(0x0057)	PsDECL(ps_W)		UsrDECL(0)},
{BciDECL(88)	MslDECL(57)	UniDECL(0x0058)	PsDECL(ps_X)		UsrDECL(0)},
{BciDECL(89)	MslDECL(58)	UniDECL(0x0059)	PsDECL(ps_Y)		UsrDECL(0)},
{BciDECL(90)	MslDECL(59)	UniDECL(0x005a)	PsDECL(ps_Z)		UsrDECL(0)},
{BciDECL(91)	MslDECL(60)	UniDECL(0x005b)	PsDECL(ps_bracketleft)	UsrDECL(0)},
{BciDECL(92)	MslDECL(61)	UniDECL(0x005c)	PsDECL(ps_backslash)	UsrDECL(0)},
{BciDECL(93)	MslDECL(62)	UniDECL(0x005d)	PsDECL(ps_bracketright)	UsrDECL(0)},
{BciDECL(94)	MslDECL(63)	UniDECL(0x005e)	PsDECL(ps_asciicircum)	UsrDECL(0)},
{BciDECL(95)	MslDECL(64)	UniDECL(0x005f)	PsDECL(ps_underscore)	UsrDECL(0)},
{BciDECL(97)	MslDECL(67)	UniDECL(0x0061)	PsDECL(ps_a)		UsrDECL(0)},
{BciDECL(98)	MslDECL(68)	UniDECL(0x0062)	PsDECL(ps_b)		UsrDECL(0)},
{BciDECL(99)	MslDECL(69)	UniDECL(0x0063)	PsDECL(ps_c)		UsrDECL(0)},
{BciDECL(100)	MslDECL(70)	UniDECL(0x0064)	PsDECL(ps_d)		UsrDECL(0)},
{BciDECL(101)	MslDECL(71)	UniDECL(0x0065)	PsDECL(ps_e)		UsrDECL(0)},
{BciDECL(102)	MslDECL(72)	UniDECL(0x0066)	PsDECL(ps_f)		UsrDECL(0)},
{BciDECL(103)	MslDECL(73)	UniDECL(0x0067)	PsDECL(ps_g)		UsrDECL(0)},
{BciDECL(104)	MslDECL(74)	UniDECL(0x0068)	PsDECL(ps_h)		UsrDECL(0)},
{BciDECL(105)	MslDECL(75)	UniDECL(0x0069)	PsDECL(ps_i)		UsrDECL(0)},
{BciDECL(106)	MslDECL(76)	UniDECL(0x006a)	PsDECL(ps_j)		UsrDECL(0)},
{BciDECL(107)	MslDECL(77)	UniDECL(0x006b)	PsDECL(ps_k)		UsrDECL(0)},
{BciDECL(108)	MslDECL(78)	UniDECL(0x006c)	PsDECL(ps_l)		UsrDECL(0)},
{BciDECL(109)	MslDECL(79)	UniDECL(0x006d)	PsDECL(ps_m)		UsrDECL(0)},
{BciDECL(110)	MslDECL(80)	UniDECL(0x006e)	PsDECL(ps_n)		UsrDECL(0)},
{BciDECL(111)	MslDECL(81)	UniDECL(0x006f)	PsDECL(ps_o)		UsrDECL(0)},
{BciDECL(112)	MslDECL(82)	UniDECL(0x0070)	PsDECL(ps_p)		UsrDECL(0)},
{BciDECL(113)	MslDECL(83)	UniDECL(0x0071)	PsDECL(ps_q)		UsrDECL(0)},
{BciDECL(114)	MslDECL(84)	UniDECL(0x0072)	PsDECL(ps_r)		UsrDECL(0)},
{BciDECL(115)	MslDECL(85)	UniDECL(0x0073)	PsDECL(ps_s)		UsrDECL(0)},
{BciDECL(116)	MslDECL(86)	UniDECL(0x0074)	PsDECL(ps_t)		UsrDECL(0)},
{BciDECL(117)	MslDECL(87)	UniDECL(0x0075)	PsDECL(ps_u)		UsrDECL(0)},
{BciDECL(118)	MslDECL(88)	UniDECL(0x0076)	PsDECL(ps_v)		UsrDECL(0)},
{BciDECL(119)	MslDECL(89)	UniDECL(0x0077)	PsDECL(ps_w)		UsrDECL(0)},
{BciDECL(120)	MslDECL(90)	UniDECL(0x0078)	PsDECL(ps_x)		UsrDECL(0)},
{BciDECL(121)	MslDECL(91)	UniDECL(0x0079)	PsDECL(ps_y)		UsrDECL(0)},
{BciDECL(122)	MslDECL(92)	UniDECL(0x007a)	PsDECL(ps_z)		UsrDECL(0)},
{BciDECL(123)	MslDECL(93)	UniDECL(0x007b)	PsDECL(ps_braceleft)	UsrDECL(0)},
{BciDECL(124)	MslDECL(94)	UniDECL(0x007c)	PsDECL(ps_bar)		UsrDECL(0)},
{BciDECL(125)	MslDECL(95)	UniDECL(0x007d)	PsDECL(ps_braceright)	UsrDECL(0)},
{BciDECL(129)	MslDECL(1040)	UniDECL(0xf001)	PsDECL(ps_fi)		UsrDECL(0)},
{BciDECL(130)	MslDECL(1041)	UniDECL(0xf002)	PsDECL(ps_fl)		UsrDECL(0)},
{BciDECL(131)	MslDECL(124)	UniDECL(0x00a3)	PsDECL(ps_sterling)	UsrDECL(0)},
{BciDECL(132)	MslDECL(128)	UniDECL(0x00a2)	PsDECL(ps_cent)		UsrDECL(0)},
{BciDECL(133)	MslDECL(127)	UniDECL(0x0192)	PsDECL(ps_florin)	UsrDECL(0)},
{BciDECL(134)	MslDECL(1068)	UniDECL(0x2030)	PsDECL(ps_perthousand)	UsrDECL(0)},
{BciDECL(135)	MslDECL(324)	UniDECL(0x2215)	PsDECL(ps_fraction)	  UsrDECL(0)},
{BciDECL(136)	MslDECL(302)	UniDECL(0x2219)	PsDECL(ps_periodcentered) UsrDECL(0)},
{BciDECL(137)	MslDECL(66)	UniDECL(0x2018)	PsDECL(ps_quoteleft)	  UsrDECL(0)},
{BciDECL(138)	MslDECL(1017)	UniDECL(0x201c)	PsDECL(ps_quotedblleft)	  UsrDECL(0)},
{BciDECL(139)	MslDECL(1018)	UniDECL(0x201d)	PsDECL(ps_quotedblright)  UsrDECL(0)},
{BciDECL(140)	MslDECL(1067)	UniDECL(0x201a)	PsDECL(ps_quotesinglbase) UsrDECL(0)},
{BciDECL(141)	MslDECL(1019)	UniDECL(0x201e)	PsDECL(ps_quotedblbase)	  UsrDECL(0)},
{BciDECL(142)	MslDECL(312)	UniDECL(0x2020)	PsDECL(ps_dagger)	UsrDECL(0)},
{BciDECL(143)	MslDECL(327)	UniDECL(0x2021)	PsDECL(ps_daggerdbl)	UsrDECL(0)},
{BciDECL(144)	MslDECL(126)	UniDECL(0x00a7)	PsDECL(ps_section)	UsrDECL(0)},
{BciDECL(145)	MslDECL(326)	UniDECL(0x2013)	PsDECL(ps_endash)	UsrDECL(0)},
{BciDECL(146)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(147)	MslDECL(145)	UniDECL(0x00c5)	PsDECL(ps_Aring)	UsrDECL(0)},
{BciDECL(148)	MslDECL(148)	UniDECL(0x00c6)	PsDECL(ps_AE)		UsrDECL(0)},
{BciDECL(149)	MslDECL(147)	UniDECL(0x00d8)	PsDECL(ps_Oslash)	UsrDECL(0)},
{BciDECL(150)	MslDECL(1091)	UniDECL(0x0152)	PsDECL(ps_OE)		UsrDECL(0)},
{BciDECL(151)	MslDECL(149)	UniDECL(0x00e5)	PsDECL(ps_aring)	UsrDECL(0)},
{BciDECL(152)	MslDECL(152)	UniDECL(0x00e6)	PsDECL(ps_ae)		UsrDECL(0)},
{BciDECL(153)	MslDECL(151)	UniDECL(0x00f8)	PsDECL(ps_oslash)	UsrDECL(0)},
{BciDECL(154)	MslDECL(1090)	UniDECL(0x0153)	PsDECL(ps_oe)		UsrDECL(0)},
{BciDECL(155)	MslDECL(159)	UniDECL(0x00df)	PsDECL(ps_germandbls)	UsrDECL(0)},
{BciDECL(156)	MslDECL(328)	UniDECL(0x0131)	PsDECL(ps_dotlessi)	UsrDECL(0)},
{BciDECL(157)	MslDECL(1092)	UniDECL(0x2039)	PsDECL(ps_guilsinglleft)  UsrDECL(0)},
{BciDECL(158)	MslDECL(1093)	UniDECL(0x203a)	PsDECL(ps_guilsinglright) UsrDECL(0)},
{BciDECL(159)	MslDECL(188)	UniDECL(0x00ab)	PsDECL(ps_guillemotleft)  UsrDECL(0)},
{BciDECL(160)	MslDECL(190)	UniDECL(0x00bb)	PsDECL(ps_guillemotright) UsrDECL(0)},
{BciDECL(161)	MslDECL(122)	UniDECL(0x00bf)	PsDECL(ps_questiondown)	UsrDECL(0)},
{BciDECL(162)	MslDECL(121)	UniDECL(0x00a1)	PsDECL(ps_exclamdown)	UsrDECL(0)},
{BciDECL(163)	MslDECL(106)	UniDECL(0x00b4)	PsDECL(ps_acute)	UsrDECL(0)},
{BciDECL(164)	MslDECL(317)	UniDECL(0xeff9)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(165)	MslDECL(107)	UniDECL(0x0060)	PsDECL(ps_grave)	UsrDECL(0)},
{BciDECL(166)	MslDECL(318)	UniDECL(0xeff8)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(167)	MslDECL(108)	UniDECL(0x02c6)	PsDECL(ps_circumflex)	UsrDECL(0)},
{BciDECL(168)	MslDECL(319)	UniDECL(0xeff7)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(169)	MslDECL(109)	UniDECL(0x00a8)	PsDECL(ps_dieresis)	UsrDECL(0)},
{BciDECL(170)	MslDECL(320)	UniDECL(0xeff6)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(171)	MslDECL(110)	UniDECL(0x02dc)	PsDECL(ps_tilde)	UsrDECL(0)},
{BciDECL(172)	MslDECL(321)	UniDECL(0xeff5)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(173)	MslDECL(315)	UniDECL(0x02c7)	PsDECL(ps_caron)	UsrDECL(0)},
{BciDECL(174)	MslDECL(322)	UniDECL(0xeff4)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(175)	MslDECL(199)	UniDECL(0x00b8)	PsDECL(ps_cedilla)	UsrDECL(0)},
{BciDECL(176)	MslDECL(330)	UniDECL(0xeff2)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(177)	MslDECL(1084)	UniDECL(0x02c9)	PsDECL(ps_macron)	UsrDECL(0)},
{BciDECL(178)	MslDECL(1085)	UniDECL(0xefef)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(179)	MslDECL(1086)	UniDECL(0x02d8)	PsDECL(ps_breve)	UsrDECL(0)},
{BciDECL(180)	MslDECL(1087)	UniDECL(0xefee)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(181)	MslDECL(316)	UniDECL(0x02da)	PsDECL(ps_ring)		UsrDECL(0)},
{BciDECL(182)	MslDECL(323)	UniDECL(0xeff3)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(183)	MslDECL(117)	UniDECL(0x00c7)	PsDECL(ps_Ccedilla)	UsrDECL(0)},
{BciDECL(184)	MslDECL(118)	UniDECL(0x00e7)	PsDECL(ps_ccedilla)	UsrDECL(0)},
{BciDECL(186)	MslDECL(184)	UniDECL(0x00bc)	PsDECL(ps_onequarter)	UsrDECL(0)},
{BciDECL(188)	MslDECL(185)	UniDECL(0x00bd)	PsDECL(ps_onehalf)	UsrDECL(0)},
{BciDECL(190)	MslDECL(182)	UniDECL(0x00be)	PsDECL(ps_threequarters) UsrDECL(0)},
{BciDECL(194)	MslDECL(200)	UniDECL(0x00b9)	PsDECL(ps_onesuperior)	 UsrDECL(0)},
{BciDECL(195)	MslDECL(197)	UniDECL(0x00b2)	PsDECL(ps_twosuperior)	 UsrDECL(0)},
{BciDECL(196)	MslDECL(198)	UniDECL(0x00b3)	PsDECL(ps_threesuperior) UsrDECL(0)},
{BciDECL(197)	MslDECL(1001)	UniDECL(0x2074)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(198)	MslDECL(1002)	UniDECL(0x2075)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(199)	MslDECL(1003)	UniDECL(0x2076)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(200)	MslDECL(1004)	UniDECL(0x2077)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(201)	MslDECL(1005)	UniDECL(0x2078)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(202)	MslDECL(1006)	UniDECL(0x2079)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(203)	MslDECL(1000)	UniDECL(0x2070)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(225)	MslDECL(164)	UniDECL(0x00d0)	PsDECL(ps_Eth)		UsrDECL(0)},
{BciDECL(225)	MslDECL(164)	UniDECL(0x0110)	PsDECL(ps_Eth)		UsrDECL(0)},
{BciDECL(226)	MslDECL(1095)	UniDECL(0x0141)	PsDECL(ps_Lslash)	UsrDECL(0)},
{BciDECL(227)	MslDECL(404)	UniDECL(0x0104)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(228)	MslDECL(422)	UniDECL(0x0118)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(229)	MslDECL(342)	UniDECL(0x0111)	PsDECL(ps_dmacron)	UsrDECL(0)},
{BciDECL(230)	MslDECL(1096)	UniDECL(0x0142)	PsDECL(ps_lslash)	UsrDECL(0)},
{BciDECL(231)	MslDECL(442)	UniDECL(0x013d)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(233)	MslDECL(405)	UniDECL(0x0105)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(234)	MslDECL(423)	UniDECL(0x0119)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(237)	MslDECL(1088)	UniDECL(0x02d9)	PsDECL(ps_dotaccent)	UsrDECL(0)},
{BciDECL(238)	MslDECL(1089)	UniDECL(0xefed)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(239)	MslDECL(1097)	UniDECL(0x02dd)	PsDECL(ps_hungarumlaut)	UsrDECL(0)},
{BciDECL(240)	MslDECL(1045)	UniDECL(0xeff0)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(246)	MslDECL(1098)	UniDECL(0x02db)	PsDECL(ps_ogonek)	UsrDECL(0)},
{BciDECL(247)	MslDECL(1030)	UniDECL(0xeff1)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(320)	MslDECL(447)	UniDECL(0x0144)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(321)	MslDECL(446)	UniDECL(0x0143)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(322)	MslDECL(120)	UniDECL(0x00f1)	PsDECL(ps_ntilde)	UsrDECL(0)},
{BciDECL(323)	MslDECL(119)	UniDECL(0x00d1)	PsDECL(ps_Ntilde)	UsrDECL(0)},
{BciDECL(324)	MslDECL(449)	UniDECL(0x0148)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(325)	MslDECL(448)	UniDECL(0x0147)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(326)	MslDECL(135)	UniDECL(0x00f3)	PsDECL(ps_oacute)	UsrDECL(0)},
{BciDECL(327)	MslDECL(168)	UniDECL(0x00d3)	PsDECL(ps_Oacute)	UsrDECL(0)},
{BciDECL(328)	MslDECL(139)	UniDECL(0x00f2)	PsDECL(ps_ograve)	UsrDECL(0)},
{BciDECL(329)	MslDECL(169)	UniDECL(0x00d2)	PsDECL(ps_Ograve)	UsrDECL(0)},
{BciDECL(330)	MslDECL(131)	UniDECL(0x00f4)	PsDECL(ps_ocircumflex)	UsrDECL(0)},
{BciDECL(331)	MslDECL(160)	UniDECL(0x00d4)	PsDECL(ps_Ocircumflex)	UsrDECL(0)},
{BciDECL(332)	MslDECL(143)	UniDECL(0x00f6)	PsDECL(ps_odieresis)	UsrDECL(0)},
{BciDECL(333)	MslDECL(155)	UniDECL(0x00d6)	PsDECL(ps_Odieresis)	UsrDECL(0)},
{BciDECL(334)	MslDECL(171)	UniDECL(0x00f5)	PsDECL(ps_otilde)	UsrDECL(0)},
{BciDECL(335)	MslDECL(170)	UniDECL(0x00d5)	PsDECL(ps_Otilde)	UsrDECL(0)},
{BciDECL(336)	MslDECL(136)	UniDECL(0x00fa)	PsDECL(ps_uacute)	UsrDECL(0)},
{BciDECL(337)	MslDECL(174)	UniDECL(0x00da)	PsDECL(ps_Uacute)	UsrDECL(0)},
{BciDECL(338)	MslDECL(140)	UniDECL(0x00f9)	PsDECL(ps_ugrave)	UsrDECL(0)},
{BciDECL(339)	MslDECL(111)	UniDECL(0x00d9)	PsDECL(ps_Ugrave)	UsrDECL(0)},
{BciDECL(340)	MslDECL(132)	UniDECL(0x00fb)	PsDECL(ps_ucircumflex)	UsrDECL(0)},
{BciDECL(341)	MslDECL(112)	UniDECL(0x00db)	PsDECL(ps_Ucircumflex)	UsrDECL(0)},
{BciDECL(342)	MslDECL(144)	UniDECL(0x00fc)	PsDECL(ps_udieresis)	UsrDECL(0)},
{BciDECL(343)	MslDECL(156)	UniDECL(0x00dc)	PsDECL(ps_Udieresis)	UsrDECL(0)},
{BciDECL(346)	MslDECL(477)	UniDECL(0x016f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(347)	MslDECL(476)	UniDECL(0x016e)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(348)	MslDECL(176)	UniDECL(0x00ff)	PsDECL(ps_ydieresis)	UsrDECL(0)},
{BciDECL(349)	MslDECL(175)	UniDECL(0x0178)	PsDECL(ps_Ydieresis)	UsrDECL(0)},
{BciDECL(350)	MslDECL(115)	UniDECL(0x00fd)	PsDECL(ps_yacute)	UsrDECL(0)},
{BciDECL(351)	MslDECL(114)	UniDECL(0x00dd)	PsDECL(ps_Yacute)	UsrDECL(0)},
{BciDECL(366)	MslDECL(113)	UniDECL(0x00af)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(368)	MslDECL(443)	UniDECL(0x013e)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(371)	MslDECL(105)	UniDECL(0x00cf)	PsDECL(ps_Idieresis)	UsrDECL(0)},
{BciDECL(372)	MslDECL(158)	UniDECL(0x00ef)	PsDECL(ps_idieresis)	UsrDECL(0)},
{BciDECL(373)	MslDECL(104)	UniDECL(0x00ce)	PsDECL(ps_Icircumflex)	UsrDECL(0)},
{BciDECL(374)	MslDECL(146)	UniDECL(0x00ee)	PsDECL(ps_icircumflex)	UsrDECL(0)},
{BciDECL(375)	MslDECL(167)	UniDECL(0x00cc)	PsDECL(ps_Igrave)	UsrDECL(0)},
{BciDECL(376)	MslDECL(154)	UniDECL(0x00ec)	PsDECL(ps_igrave)	UsrDECL(0)},
{BciDECL(377)	MslDECL(166)	UniDECL(0x00cd)	PsDECL(ps_Iacute)	UsrDECL(0)},
{BciDECL(378)	MslDECL(150)	UniDECL(0x00ed)	PsDECL(ps_iacute)	UsrDECL(0)},
{BciDECL(379)	MslDECL(416)	UniDECL(0x011a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(380)	MslDECL(417)	UniDECL(0x011b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(383)	MslDECL(103)	UniDECL(0x00cb)	PsDECL(ps_Edieresis)	UsrDECL(0)},
{BciDECL(384)	MslDECL(142)	UniDECL(0x00eb)	PsDECL(ps_edieresis)	UsrDECL(0)},
{BciDECL(385)	MslDECL(102)	UniDECL(0x00ca)	PsDECL(ps_Ecircumflex)	UsrDECL(0)},
{BciDECL(386)	MslDECL(130)	UniDECL(0x00ea)	PsDECL(ps_ecircumflex)	UsrDECL(0)},
{BciDECL(387)	MslDECL(101)	UniDECL(0x00c8)	PsDECL(ps_Egrave)	UsrDECL(0)},
{BciDECL(388)	MslDECL(138)	UniDECL(0x00e8)	PsDECL(ps_egrave)	UsrDECL(0)},
{BciDECL(389)	MslDECL(157)	UniDECL(0x00c9)	PsDECL(ps_Eacute)	UsrDECL(0)},
{BciDECL(390)	MslDECL(134)	UniDECL(0x00e9)	PsDECL(ps_eacute)	UsrDECL(0)},
{BciDECL(391)	MslDECL(162)	UniDECL(0x00c3)	PsDECL(ps_Atilde)	UsrDECL(0)},
{BciDECL(392)	MslDECL(163)	UniDECL(0x00e3)	PsDECL(ps_atilde)	UsrDECL(0)},
{BciDECL(393)	MslDECL(153)	UniDECL(0x00c4)	PsDECL(ps_Adieresis)	UsrDECL(0)},
{BciDECL(394)	MslDECL(141)	UniDECL(0x00e4)	PsDECL(ps_adieresis)	UsrDECL(0)},
{BciDECL(395)	MslDECL(100)	UniDECL(0x00c2)	PsDECL(ps_Acircumflex)	UsrDECL(0)},
{BciDECL(396)	MslDECL(129)	UniDECL(0x00e2)	PsDECL(ps_acircumflex)	UsrDECL(0)},
{BciDECL(397)	MslDECL(99)	UniDECL(0x00c0)	PsDECL(ps_Agrave)	UsrDECL(0)},
{BciDECL(398)	MslDECL(137)	UniDECL(0x00e0)	PsDECL(ps_agrave)	UsrDECL(0)},
{BciDECL(399)	MslDECL(161)	UniDECL(0x00c1)	PsDECL(ps_Aacute)	UsrDECL(0)},
{BciDECL(400)	MslDECL(133)	UniDECL(0x00e1)	PsDECL(ps_aacute)	UsrDECL(0)},
{BciDECL(401)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ordmasculine)	UsrDECL(0)},
{BciDECL(402)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ordfeminine)	UsrDECL(0)},
{BciDECL(404)	MslDECL(309)	UniDECL(0x0149)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(410)	MslDECL(329)	UniDECL(0x0027)	PsDECL(ps_quotesingle)	UsrDECL(0)},
{BciDECL(411)	MslDECL(221)	UniDECL(0x203c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(414)	MslDECL(232)	UniDECL(0x20a7)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(418)	MslDECL(325)	UniDECL(0x2014)	PsDECL(ps_emdash)	UsrDECL(0)},
{BciDECL(423)	MslDECL(1028)	UniDECL(0x2026)	PsDECL(ps_ellipsis)	UsrDECL(0)},
{BciDECL(424)	MslDECL(177)	UniDECL(0x00de)	PsDECL(ps_Thorn)	UsrDECL(0)},
{BciDECL(425)	MslDECL(178)	UniDECL(0x00fe)	PsDECL(ps_thorn)	UsrDECL(0)},
{BciDECL(426)	MslDECL(165)	UniDECL(0x00f0)	PsDECL(ps_eth)		UsrDECL(0)},
{BciDECL(427)	MslDECL(125)	UniDECL(0x00a5)	PsDECL(ps_yen)		UsrDECL(0)},
{BciDECL(433)	MslDECL(1047)	UniDECL(0x0133)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(434)	MslDECL(1107)	UniDECL(0x0132)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(435)	MslDECL(192)	UniDECL(0x00a6)	PsDECL(ps_brokenbar)	UsrDECL(0)},
{BciDECL(436)	MslDECL(123)	UniDECL(0x00a4)	PsDECL(ps_currency)	UsrDECL(0)},
{BciDECL(442)	MslDECL(181)	UniDECL(0x00b6)	PsDECL(ps_paragraph)	UsrDECL(0)},
{BciDECL(446)	MslDECL(1044)	UniDECL(0xfb04)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(447)	MslDECL(1043)	UniDECL(0xfb03)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(448)	MslDECL(1042)	UniDECL(0xfb00)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(449)	MslDECL(183)	UniDECL(0x2212)	PsDECL(ps_minus)	UsrDECL(0)},
{BciDECL(450)	MslDECL(201)	UniDECL(0x00d7)	PsDECL(ps_multiply)	UsrDECL(0)},
{BciDECL(451)	MslDECL(202)	UniDECL(0x00f7)	PsDECL(ps_divide)	UsrDECL(0)},
{BciDECL(452)	MslDECL(191)	UniDECL(0x00b1)	PsDECL(ps_plusminus)	UsrDECL(0)},
{BciDECL(453)	MslDECL(296)	UniDECL(0x2261)	PsDECL(ps_equivalence)	UsrDECL(0)},
{BciDECL(454)	MslDECL(519)	UniDECL(0x2260)	PsDECL(ps_notequal)	UsrDECL(0)},
{BciDECL(455)	MslDECL(301)	UniDECL(0x2248)	PsDECL(ps_approxequal)	UsrDECL(0)},
{BciDECL(456)	MslDECL(297)	UniDECL(0x2265)	PsDECL(ps_greaterequal)	UsrDECL(0)},
{BciDECL(457)	MslDECL(298)	UniDECL(0x2264)	PsDECL(ps_lessequal)	UsrDECL(0)},
{BciDECL(458)	MslDECL(225)	UniDECL(0x2193)	PsDECL(ps_arrowdown)	UsrDECL(0)},
{BciDECL(459)	MslDECL(227)	UniDECL(0x2190)	PsDECL(ps_arrowleft)	UsrDECL(0)},
{BciDECL(460)	MslDECL(226)	UniDECL(0x2192)	PsDECL(ps_arrowright)	UsrDECL(0)},
{BciDECL(461)	MslDECL(224)	UniDECL(0x2191)	PsDECL(ps_arrowup)	UsrDECL(0)},
{BciDECL(462)	MslDECL(220)	UniDECL(0x2195)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(463)	MslDECL(229)	UniDECL(0x2194)	PsDECL(ps_arrowboth)	UsrDECL(0)},
{BciDECL(464)	MslDECL(566)	UniDECL(0x2208)	PsDECL(ps_element)	UsrDECL(0)},
{BciDECL(465)	MslDECL(295)	UniDECL(0x2229)	PsDECL(ps_intersection)	UsrDECL(0)},
{BciDECL(466)	MslDECL(565)	UniDECL(0x222a)	PsDECL(ps_union)	UsrDECL(0)},
{BciDECL(469)	MslDECL(587)	UniDECL(0x222b)	PsDECL(ps_integral)	UsrDECL(0)},
{BciDECL(470)	MslDECL(299)	UniDECL(0x2320)	PsDECL(ps_integraltp)	UsrDECL(0)},
{BciDECL(471)	MslDECL(300)	UniDECL(0x2321)	PsDECL(ps_integralbt)	UsrDECL(0)},
{BciDECL(476)	MslDECL(303)	UniDECL(0x221a)	PsDECL(ps_radical)	UsrDECL(0)},
{BciDECL(478)	MslDECL(292)	UniDECL(0x221e)	PsDECL(ps_infinity)	UsrDECL(0)},
{BciDECL(479)	MslDECL(280)	UniDECL(0x2580)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(480)	MslDECL(277)	UniDECL(0x2584)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(481)	MslDECL(278)	UniDECL(0x258c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(482)	MslDECL(279)	UniDECL(0x2590)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(486)	MslDECL(194)	UniDECL(0x00ac)	PsDECL(ps_logicalnot)	UsrDECL(0)},
{BciDECL(487)	MslDECL(233)	UniDECL(0x2310)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(498)	MslDECL(NONE)	UniDECL(0x0391)	PsDECL(ps_Alpha)	UsrDECL(0)},
{BciDECL(499)	MslDECL(NONE)	UniDECL(0x0392)	PsDECL(ps_Beta)		UsrDECL(0)},
{BciDECL(500)	MslDECL(283)	UniDECL(0x0393)	PsDECL(ps_Gamma)	UsrDECL(0)},
{BciDECL(500)	MslDECL(505)	UniDECL(0x0393)	PsDECL(ps_Gamma)	UsrDECL(0)},
{BciDECL(501)	MslDECL(506)	UniDECL(0x2206)	PsDECL(ps_Delta)	UsrDECL(0)},
{BciDECL(502)	MslDECL(NONE)	UniDECL(0x0395)	PsDECL(ps_Epsilon)	UsrDECL(0)},
{BciDECL(503)	MslDECL(NONE)	UniDECL(0x0396)	PsDECL(ps_Zeta)		UsrDECL(0)},
{BciDECL(504)	MslDECL(NONE)	UniDECL(0x0397)	PsDECL(ps_Eta)		UsrDECL(0)},
{BciDECL(505)	MslDECL(289)	UniDECL(0x0398)	PsDECL(ps_Theta)	UsrDECL(0)},
{BciDECL(505)	MslDECL(507)	UniDECL(0x0398)	PsDECL(ps_Theta)	UsrDECL(0)},
{BciDECL(506)	MslDECL(NONE)	UniDECL(0x0399)	PsDECL(ps_Iota)		UsrDECL(0)},
{BciDECL(507)	MslDECL(NONE)	UniDECL(0x039a)	PsDECL(ps_Kappa)	UsrDECL(0)},
{BciDECL(508)	MslDECL(508)	UniDECL(0x039b)	PsDECL(ps_Lambda)	UsrDECL(0)},
{BciDECL(509)	MslDECL(NONE)	UniDECL(0x039c)	PsDECL(ps_Mu)		UsrDECL(0)},
{BciDECL(510)	MslDECL(NONE)	UniDECL(0x039d)	PsDECL(ps_Nu)		UsrDECL(0)},
{BciDECL(511)	MslDECL(509)	UniDECL(0x039e)	PsDECL(ps_Xi)		UsrDECL(0)},
{BciDECL(512)	MslDECL(NONE)	UniDECL(0x039f)	PsDECL(ps_Omicron)	UsrDECL(0)},
{BciDECL(513)	MslDECL(510)	UniDECL(0x03a0)	PsDECL(ps_Pi)		UsrDECL(0)},
{BciDECL(514)	MslDECL(NONE)	UniDECL(0x03a1)	PsDECL(ps_Rho)		UsrDECL(0)},
{BciDECL(515)	MslDECL(285)	UniDECL(0x03a3)	PsDECL(ps_Sigma)	UsrDECL(0)},
{BciDECL(515)	MslDECL(511)	UniDECL(0x03a3)	PsDECL(ps_Sigma)	UsrDECL(0)},
{BciDECL(516)	MslDECL(NONE)	UniDECL(0x03a4)	PsDECL(ps_Tau)		UsrDECL(0)},
{BciDECL(517)	MslDECL(NONE)	UniDECL(0x03a5)	PsDECL(ps_Upsilon)	UsrDECL(0)},
{BciDECL(518)	MslDECL(288)	UniDECL(0x03a6)	PsDECL(ps_Phi)		UsrDECL(0)},
{BciDECL(518)	MslDECL(513)	UniDECL(0x03a6)	PsDECL(ps_Phi)		UsrDECL(0)},
{BciDECL(519)	MslDECL(NONE)	UniDECL(0x03a7)	PsDECL(ps_Chi)		UsrDECL(0)},
{BciDECL(520)	MslDECL(514)	UniDECL(0x03a8)	PsDECL(ps_Psi)		UsrDECL(0)},
{BciDECL(521)	MslDECL(515)	UniDECL(0x2126)	PsDECL(ps_Omega)	UsrDECL(0)},
{BciDECL(521)	MslDECL(290)	UniDECL(0x2126)	PsDECL(ps_Omega)	UsrDECL(0)},
{BciDECL(522)	MslDECL(281)	UniDECL(0x03b1)	PsDECL(ps_alpha)	UsrDECL(0)},
{BciDECL(522)	MslDECL(522)	UniDECL(0x03b1)	PsDECL(ps_alpha)	UsrDECL(0)},
{BciDECL(523)	MslDECL(523)	UniDECL(0x03b2)	PsDECL(ps_beta)		UsrDECL(0)},
{BciDECL(525)	MslDECL(524)	UniDECL(0x03b3)	PsDECL(ps_gamma)	UsrDECL(0)},
{BciDECL(526)	MslDECL(291)	UniDECL(0x03b4)	PsDECL(ps_delta)	UsrDECL(0)},
{BciDECL(526)	MslDECL(525)	UniDECL(0x03b4)	PsDECL(ps_delta)	UsrDECL(0)},
{BciDECL(527)	MslDECL(294)	UniDECL(0x03b5)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(527)   MslDECL(526)    UniDECL(0x03b5) PsDECL(ns)              UsrDECL(0)},
{BciDECL(528)	MslDECL(527)	UniDECL(0x03b6)	PsDECL(ps_zeta)		UsrDECL(0)},
{BciDECL(529)	MslDECL(304)	UniDECL(0x03b7)	PsDECL(ps_eta)		UsrDECL(0)},
{BciDECL(529)	MslDECL(528)	UniDECL(0x03b7)	PsDECL(ps_eta)		UsrDECL(0)},
{BciDECL(530)	MslDECL(529)	UniDECL(0x03b8)	PsDECL(ps_theta)	UsrDECL(0)},
{BciDECL(531)	MslDECL(530)	UniDECL(0x03b9)	PsDECL(ps_iota)		UsrDECL(0)},
{BciDECL(532)	MslDECL(531)	UniDECL(0x03ba)	PsDECL(ps_kappa)	UsrDECL(0)},
{BciDECL(533)	MslDECL(532)	UniDECL(0x03bb)	PsDECL(ps_lambda)	UsrDECL(0)},
{BciDECL(534)	MslDECL(180)	UniDECL(0x00b5)	PsDECL(ps_mu)		UsrDECL(0)},
{BciDECL(534)	MslDECL(533)	UniDECL(0x00b5)	PsDECL(ps_mu)		UsrDECL(0)},
{BciDECL(535)	MslDECL(534)	UniDECL(0x03bd)	PsDECL(ps_nu)		UsrDECL(0)},
{BciDECL(536)	MslDECL(535)	UniDECL(0x03be)	PsDECL(ps_xi)		UsrDECL(0)},
{BciDECL(537)	MslDECL(536)	UniDECL(0x03bf)	PsDECL(ps_omicron)	UsrDECL(0)},
{BciDECL(538)	MslDECL(284)	UniDECL(0x03c0)	PsDECL(ps_pi)		UsrDECL(0)},
{BciDECL(538)	MslDECL(537)	UniDECL(0x03c0)	PsDECL(ps_pi)		UsrDECL(0)},
{BciDECL(539)	MslDECL(538)	UniDECL(0x03c1)	PsDECL(ps_rho)		UsrDECL(0)},
{BciDECL(540)	MslDECL(286)	UniDECL(0x03c3)	PsDECL(ps_sigma)	UsrDECL(0)},
{BciDECL(540)	MslDECL(539)	UniDECL(0x03c3)	PsDECL(ps_sigma)	UsrDECL(0)},
{BciDECL(541)	MslDECL(518)	UniDECL(0x03c2)	PsDECL(ps_sigma1)	UsrDECL(0)},
{BciDECL(542)	MslDECL(287)	UniDECL(0x03c4)	PsDECL(ps_tau)		UsrDECL(0)},
{BciDECL(542)	MslDECL(540)	UniDECL(0x03c4)	PsDECL(ps_tau)		UsrDECL(0)},
{BciDECL(543)	MslDECL(541)	UniDECL(0x03c5)	PsDECL(ps_upsilon)	UsrDECL(0)},
{BciDECL(544)	MslDECL(293)	UniDECL(0x03c6)	PsDECL(ps_phi)		UsrDECL(0)},
{BciDECL(544)	MslDECL(542)	UniDECL(0x03c6)	PsDECL(ps_phi)		UsrDECL(0)},
{BciDECL(545)	MslDECL(543)	UniDECL(0x03c7)	PsDECL(ps_chi)		UsrDECL(0)},
{BciDECL(546)	MslDECL(544)	UniDECL(0x03c8)	PsDECL(ps_psi)		UsrDECL(0)},
{BciDECL(547)	MslDECL(545)	UniDECL(0x03c9)	PsDECL(ps_omega)	UsrDECL(0)},
{BciDECL(558)	MslDECL(637)	UniDECL(0xefd5)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(561)	MslDECL(NONE)	UniDECL(0x22c5)	PsDECL(ps_dotmath)	UsrDECL(0)},
{BciDECL(562)	MslDECL(215)	UniDECL(0x266a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(563)	MslDECL(216)	UniDECL(0x266b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(564)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_copyright)	UsrDECL(0)},
{BciDECL(565)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_registered)	UsrDECL(0)},
{BciDECL(566)	MslDECL(313)	UniDECL(0x2122)	PsDECL(ps_trademark)	UsrDECL(0)},
{BciDECL(570)	MslDECL(310)	UniDECL(0x2032)	PsDECL(ps_minute)	UsrDECL(0)},
{BciDECL(571)	MslDECL(311)	UniDECL(0x2033)	PsDECL(ps_second)	UsrDECL(0)},
{BciDECL(572)	MslDECL(116)	UniDECL(0x00b0)	PsDECL(ps_degree)	UsrDECL(0)},
{BciDECL(573)	MslDECL(223)	UniDECL(0x21a8)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(575)	MslDECL(331)	UniDECL(0x2022)	PsDECL(ps_bullet)	UsrDECL(0)},
{BciDECL(576)	MslDECL(209)	UniDECL(0x25cf)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(577)	MslDECL(1111)	UniDECL(0xeffa)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(578)	MslDECL(1110)	UniDECL(0x25cb)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(579)	MslDECL(210)	UniDECL(0x25d8)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(581)	MslDECL(218)	UniDECL(0x25ba)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(582)	MslDECL(219)	UniDECL(0x25c4)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(583)	MslDECL(231)	UniDECL(0x25bc)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(584)	MslDECL(230)	UniDECL(0x25b2)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(585)	MslDECL(213)	UniDECL(0x2642)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(586)	MslDECL(214)	UniDECL(0x2640)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(587)	MslDECL(205)	UniDECL(0x2665)	PsDECL(ps_heart)	UsrDECL(0)},
{BciDECL(588)	MslDECL(207)	UniDECL(0x2663)	PsDECL(ps_club)		UsrDECL(0)},
{BciDECL(589)	MslDECL(206)	UniDECL(0x2666)	PsDECL(ps_diamond)	UsrDECL(0)},
{BciDECL(590)	MslDECL(208)	UniDECL(0x2660)	PsDECL(ps_spade)	UsrDECL(0)},
{BciDECL(591)	MslDECL(222)	UniDECL(0x25ac)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(593)	MslDECL(234)	UniDECL(0x2591)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(594)	MslDECL(97)	UniDECL(0x2592)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(595)	MslDECL(235)	UniDECL(0x2593)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(598)	MslDECL(217)	UniDECL(0x263c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(599)	MslDECL(203)	UniDECL(0x263a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(600)	MslDECL(204)	UniDECL(0x263b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(605)	MslDECL(467)	UniDECL(0x0165)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(609)	MslDECL(483)	UniDECL(0x017a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(610)	MslDECL(482)	UniDECL(0x0179)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(611)	MslDECL(1031)	UniDECL(0x017e)	PsDECL(ps_zcaron)	UsrDECL(0)},
{BciDECL(612)	MslDECL(1106)	UniDECL(0x017d)	PsDECL(ps_Zcaron)	UsrDECL(0)},
{BciDECL(613)	MslDECL(485)	UniDECL(0x017c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(614)	MslDECL(484)	UniDECL(0x017b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(619)	MslDECL(401)	UniDECL(0x0103)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(620)	MslDECL(400)	UniDECL(0x0102)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(621)	MslDECL(407)	UniDECL(0x0107)	PsDECL(ps_cacute)	UsrDECL(0)},
{BciDECL(622)	MslDECL(406)	UniDECL(0x0106)	PsDECL(ps_Cacute)	UsrDECL(0)},
{BciDECL(623)	MslDECL(411)	UniDECL(0x010d)	PsDECL(ps_ccaron)	UsrDECL(0)},
{BciDECL(624)	MslDECL(410)	UniDECL(0x010c)	PsDECL(ps_Ccaron)	UsrDECL(0)},
{BciDECL(625)	MslDECL(415)	UniDECL(0x010f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(627)	MslDECL(414)	UniDECL(0x010e)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(632)	MslDECL(1062)	UniDECL(0x011f)	PsDECL(ps_gbreve)	UsrDECL(0)},
{BciDECL(633)	MslDECL(1061)	UniDECL(0x011e)	PsDECL(ps_Gbreve)	UsrDECL(0)},
{BciDECL(641)	MslDECL(1065)	UniDECL(0x0130)	PsDECL(ps_Idot)		UsrDECL(0)},
{BciDECL(646)	MslDECL(441)	UniDECL(0x013a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(647)	MslDECL(440)	UniDECL(0x0139)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(656)	MslDECL(453)	UniDECL(0x0151)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(657)	MslDECL(452)	UniDECL(0x0150)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(658)	MslDECL(457)	UniDECL(0x0155)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(659)	MslDECL(456)	UniDECL(0x0154)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(660)	MslDECL(459)	UniDECL(0x0159)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(661)	MslDECL(458)	UniDECL(0x0158)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(664)	MslDECL(463)	UniDECL(0x015b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(665)	MslDECL(462)	UniDECL(0x015a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(666)	MslDECL(173)	UniDECL(0x0161)	PsDECL(ps_scaron)	UsrDECL(0)},
{BciDECL(667)	MslDECL(172)	UniDECL(0x0160)	PsDECL(ps_Scaron)	UsrDECL(0)},
{BciDECL(673)	MslDECL(466)	UniDECL(0x0164)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(680)	MslDECL(475)	UniDECL(0x0171)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(681)	MslDECL(474)	UniDECL(0x0170)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(693)	MslDECL(588)	UniDECL(0x222e)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(695)	MslDECL(501)	UniDECL(0x221d)	PsDECL(ps_proportional)	UsrDECL(0)},
{BciDECL(797)	MslDECL(248)	UniDECL(0x2510)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1364)	MslDECL(1400)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1365)	MslDECL(1413)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1368)	MslDECL(1401)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1369)	MslDECL(1414)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1372)	MslDECL(1402)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1373)	MslDECL(1415)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1376)	MslDECL(1403)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1377)	MslDECL(1416)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1380)	MslDECL(1404)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1381)	MslDECL(1417)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1384)	MslDECL(1405)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1385)	MslDECL(1418)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1388)	MslDECL(1411)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1389)	MslDECL(1424)	UniDECL(NONE)	PsDECL(ps_cedilla)	UsrDECL(0)},
{BciDECL(1392)	MslDECL(1410)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1393)	MslDECL(1423)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1396)	MslDECL(1406)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1397)	MslDECL(1419)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1400)	MslDECL(1408)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1401)	MslDECL(1421)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1661)	MslDECL(306)	UniDECL(0x013f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1667)	MslDECL(307)	UniDECL(0x0140)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1743)	MslDECL(1409)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1744)	MslDECL(1422)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1747)	MslDECL(1407)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1748)	MslDECL(1420)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1776)	MslDECL(1412)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1777)	MslDECL(1425)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(1996)	MslDECL(NONE)	UniDECL(0x203e)	PsDECL(ps_radicalex)	UsrDECL(0)},
{BciDECL(1999)	MslDECL(NONE)	UniDECL(0x220b)	PsDECL(ps_suchthat)	UsrDECL(0)},
{BciDECL(2303)	MslDECL(575)	UniDECL(0x2295)	PsDECL(ps_circleplus)	UsrDECL(0)},
{BciDECL(2304)	MslDECL(577)	UniDECL(0x2297)	PsDECL(ps_circlemultiply) UsrDECL(0)},
{BciDECL(3738)	MslDECL(1064)	UniDECL(0x015f)	PsDECL(ps_scedilla)	UsrDECL(0)},
{BciDECL(3744)	MslDECL(1063)	UniDECL(0x015e)	PsDECL(ps_Scedilla)	UsrDECL(0)},
{BciDECL(4086)	MslDECL(469)	UniDECL(0x0163)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4092)	MslDECL(468)	UniDECL(0x0162)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4472)	MslDECL(314)	UniDECL(0x2017)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4524)	MslDECL(1066)	UniDECL(0x20a3)	PsDECL(ps_franc)	UsrDECL(0)},
{BciDECL(4545)	MslDECL(563)	UniDECL(0x22a4)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4736)	MslDECL(620)	UniDECL(0x2213)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4741)	MslDECL(550)	UniDECL(0x2262)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4744)	MslDECL(96)	UniDECL(0x007e)	PsDECL(ps_asciitilde)	UsrDECL(0)},
{BciDECL(4746)	MslDECL(624)	UniDECL(0x2245)	PsDECL(ps_congruent)	UsrDECL(0)},
{BciDECL(4751)	MslDECL(549)	UniDECL(0x2243)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4803)	MslDECL(570)	UniDECL(0x2283)	PsDECL(ps_propersuperset) UsrDECL(0)},
{BciDECL(4804)	MslDECL(574)	UniDECL(0x2287)	PsDECL(ps_reflexsuperset) UsrDECL(0)},
{BciDECL(4808)	MslDECL(572)	UniDECL(0x2285)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4813)	MslDECL(569)	UniDECL(0x2282)	PsDECL(ps_propersubset)	UsrDECL(0)},
{BciDECL(4814)	MslDECL(573)	UniDECL(0x2286)	PsDECL(ps_reflexsubset)	UsrDECL(0)},
{BciDECL(4819)	MslDECL(571)	UniDECL(0x2284)	PsDECL(ps_notsubset)	UsrDECL(0)},
{BciDECL(4825)	MslDECL(553)	UniDECL(0x21d3)	PsDECL(ps_arrowdbldown)	UsrDECL(0)},
{BciDECL(4828)	MslDECL(558)	UniDECL(0x21c6)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4829)	MslDECL(557)	UniDECL(0x21c4)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4833)	MslDECL(554)	UniDECL(0x21d0)	PsDECL(ps_arrowdblleft)	 UsrDECL(0)},
{BciDECL(4840)	MslDECL(552)	UniDECL(0x21d2)	PsDECL(ps_arrowdblright) UsrDECL(0)},
{BciDECL(4843)	MslDECL(551)	UniDECL(0x21d1)	PsDECL(ps_arrowdblup)	UsrDECL(0)},
{BciDECL(4848)	MslDECL(555)	UniDECL(0x21d5)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4849)	MslDECL(556)	UniDECL(0x21d4)	PsDECL(ps_arrowdblboth)	UsrDECL(0)},
{BciDECL(4851)	MslDECL(628)	UniDECL(0x2196)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4852)	MslDECL(626)	UniDECL(0x2198)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4853)	MslDECL(625)	UniDECL(0x2197)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4854)	MslDECL(627)	UniDECL(0x2199)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4867)	MslDECL(586)	UniDECL(0x22a2)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4874)	MslDECL(564)	UniDECL(0x22a5)	PsDECL(ps_perpendicular) UsrDECL(0)},
{BciDECL(4880)	MslDECL(633)	UniDECL(0x226a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4881)	MslDECL(634)	UniDECL(0x226b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4892)	MslDECL(567)	UniDECL(0x220b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4893)	MslDECL(568)	UniDECL(0x2209)	PsDECL(ps_notelement)	UsrDECL(0)},
{BciDECL(4901)	MslDECL(580)	UniDECL(0x2227)	PsDECL(ps_logicaland)	UsrDECL(0)},
{BciDECL(4902)	MslDECL(581)	UniDECL(0x2228)	PsDECL(ps_logicalor)	UsrDECL(0)},
{BciDECL(4931)	MslDECL(589)	UniDECL(0x2220)	PsDECL(ps_angle)	UsrDECL(0)},
{BciDECL(4934)	MslDECL(629)	UniDECL(0x25b5)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4935)	MslDECL(632)	UniDECL(0x25c3)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4936)	MslDECL(631)	UniDECL(0x25bf)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4937)	MslDECL(630)	UniDECL(0x25b9)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4950)	MslDECL(582)	UniDECL(0x22bb)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4957)	MslDECL(504)	UniDECL(0x2234)	PsDECL(ps_therefore)	UsrDECL(0)},
{BciDECL(4958)	MslDECL(521)	UniDECL(0x2235)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4961)	MslDECL(576)	UniDECL(0x2299)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4967)	MslDECL(578)	UniDECL(0x2296)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4968)	MslDECL(584)	UniDECL(0x20dd)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4982)	MslDECL(635)	UniDECL(0x2237)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(4984)	MslDECL(605)	UniDECL(0x2225)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5001)	MslDECL(590)	UniDECL(0x2205)	PsDECL(ps_emptyset)	UsrDECL(0)},
{BciDECL(5031)	MslDECL(500)	UniDECL(0xefbf)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5032)	MslDECL(615)	UniDECL(0xefdc)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5039)	MslDECL(517)	UniDECL(0x2202)	PsDECL(ps_partialdiff)	UsrDECL(0)},
{BciDECL(5075)	MslDECL(643)	UniDECL(0x301a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5080)	MslDECL(645)	UniDECL(0x301b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5085)	MslDECL(613)	UniDECL(0xefdd)	PsDECL(ps_integralex)	UsrDECL(0)},
{BciDECL(5116)	MslDECL(1036)	UniDECL(0x211e)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5131)	MslDECL(526)	UniDECL(0x03b5)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5136)	MslDECL(591)	UniDECL(0x2135)	PsDECL(ps_aleph)	UsrDECL(0)},
{BciDECL(5137)	MslDECL(592)	UniDECL(0x2136)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5138)	MslDECL(593)	UniDECL(0x2137)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5142)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bracketlefttp)  UsrDECL(0)},
{BciDECL(5143)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bracketleftbt)  UsrDECL(0)},
{BciDECL(5144)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bracketrighttp) UsrDECL(0)},
{BciDECL(5145)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bracketrightbt) UsrDECL(0)},
{BciDECL(5147)	MslDECL(308)	UniDECL(0x2113)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5148)	MslDECL(638)	UniDECL(0x210f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5153)	MslDECL(561)	UniDECL(0x2200)	PsDECL(ps_universal)	UsrDECL(0)},
{BciDECL(5155)	MslDECL(562)	UniDECL(0x2203)	PsDECL(ps_existential)	UsrDECL(0)},
{BciDECL(5196)	MslDECL(276)	UniDECL(0x2588)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5197)	MslDECL(664)	UniDECL(0x2217)	PsDECL(ps_asteriskmath)	UsrDECL(0)},
{BciDECL(5215)	MslDECL(622)	UniDECL(0x232a)	PsDECL(ps_angleright)	UsrDECL(0)},
{BciDECL(5216)	MslDECL(621)	UniDECL(0x2329)	PsDECL(ps_angleleft)	UsrDECL(0)},
{BciDECL(5220)	MslDECL(579)	UniDECL(0x2298)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5225)	MslDECL(585)	UniDECL(0x22a3)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5249)	MslDECL(228)	UniDECL(0x221f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5258)	MslDECL(546)	UniDECL(0x03d1)	PsDECL(ps_theta1)	UsrDECL(0)},
{BciDECL(5259)	MslDECL(548)	UniDECL(0x03d6)	PsDECL(ps_omega1)	UsrDECL(0)},
{BciDECL(5260)	MslDECL(547)	UniDECL(0x03d5)	PsDECL(ps_phi1)		UsrDECL(0)},
{BciDECL(5262)	MslDECL(503)	UniDECL(0xefec)	PsDECL(ps_epsilon)	UsrDECL(0)},
{BciDECL(5268)	MslDECL(516)	UniDECL(0x2207)	PsDECL(ps_gradient)	UsrDECL(0)},
{BciDECL(5371)	MslDECL(193)	UniDECL(0x00a9)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5372)	MslDECL(196)	UniDECL(0x00ae)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5374)	MslDECL(1060)	UniDECL(0x2105)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5403)	MslDECL(211)	UniDECL(0xeffd)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5404)	MslDECL(1109)	UniDECL(0x25e6)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5408)	MslDECL(212)	UniDECL(0x25d9)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5409)	MslDECL(661)	UniDECL(0xeffb)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5410)	MslDECL(189)	UniDECL(0x25a0)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5411)	MslDECL(305)	UniDECL(0x25aa)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5412)	MslDECL(653)	UniDECL(0xeffc)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5413)	MslDECL(1094)	UniDECL(0x25a1)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5414)	MslDECL(1108)	UniDECL(0x25ab)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5418)	MslDECL(333)	UniDECL(0x2302)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5421)	MslDECL(253)	UniDECL(0x2500)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5422)	MslDECL(275)	UniDECL(0x250c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5423)	MslDECL(274)	UniDECL(0x2518)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5424)	MslDECL(249)	UniDECL(0x2514)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5427)	MslDECL(254)	UniDECL(0x253c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5428)	MslDECL(251)	UniDECL(0x252c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5429)	MslDECL(250)	UniDECL(0x2534)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5430)	MslDECL(252)	UniDECL(0x251c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5431)	MslDECL(237)	UniDECL(0x2524)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5432)	MslDECL(236)	UniDECL(0x2502)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5434)	MslDECL(262)	UniDECL(0x2550)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5435)	MslDECL(244)	UniDECL(0x2557)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5436)	MslDECL(258)	UniDECL(0x2554)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5437)	MslDECL(245)	UniDECL(0x255d)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5438)	MslDECL(257)	UniDECL(0x255a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5441)	MslDECL(263)	UniDECL(0x256c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5442)	MslDECL(260)	UniDECL(0x2566)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5443)	MslDECL(259)	UniDECL(0x2569)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5444)	MslDECL(261)	UniDECL(0x2560)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5445)	MslDECL(242)	UniDECL(0x2563)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5446)	MslDECL(243)	UniDECL(0x2551)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5461)	MslDECL(255)	UniDECL(0x255e)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5462)	MslDECL(238)	UniDECL(0x2561)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5463)	MslDECL(265)	UniDECL(0x2568)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5464)	MslDECL(267)	UniDECL(0x2565)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5465)	MslDECL(273)	UniDECL(0x256a)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5466)	MslDECL(272)	UniDECL(0x256b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5467)	MslDECL(256)	UniDECL(0x255f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5468)	MslDECL(239)	UniDECL(0x2562)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5510)	MslDECL(186)	UniDECL(0x00aa)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5523)	MslDECL(332)	UniDECL(0x207f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5524)	MslDECL(187)	UniDECL(0x00ba)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5536)	MslDECL(264)	UniDECL(0x2567)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5537)	MslDECL(266)	UniDECL(0x2564)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5538)	MslDECL(240)	UniDECL(0x2556)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5539)	MslDECL(241)	UniDECL(0x2555)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5540)	MslDECL(246)	UniDECL(0x255c)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5541)	MslDECL(247)	UniDECL(0x255b)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5542)	MslDECL(268)	UniDECL(0x2559)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5543)	MslDECL(269)	UniDECL(0x2558)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5544)	MslDECL(271)	UniDECL(0x2553)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5545)	MslDECL(270)	UniDECL(0x2552)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5580)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_parenlefttp)	UsrDECL(0)},
{BciDECL(5581)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_parenleftbt)	UsrDECL(0)},
{BciDECL(5583)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_parenrighttp)	UsrDECL(0)},
{BciDECL(5584)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_parenrightbt)	UsrDECL(0)},
{BciDECL(5594)	MslDECL(98)	UniDECL(0x00a0)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5595)	MslDECL(1021)	UniDECL(0x2002)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5596)	MslDECL(1020)	UniDECL(0x2003)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5704)	MslDECL(641)	UniDECL(0x2118)	PsDECL(ps_weierstrass)	UsrDECL(0)},
{BciDECL(5718)	MslDECL(600)	UniDECL(0xefe3)	PsDECL(ps_bracelefttp)	UsrDECL(0)},
{BciDECL(5719)	MslDECL(601)	UniDECL(0xefe2)	PsDECL(ps_braceleftmid)	UsrDECL(0)},
{BciDECL(5720)	MslDECL(602)	UniDECL(0xefe1)	PsDECL(ps_braceleftbt)	UsrDECL(0)},
{BciDECL(5721)	MslDECL(614)	UniDECL(0x2223)	PsDECL(ps_braceex)	UsrDECL(0)},
{BciDECL(5722)	MslDECL(610)	UniDECL(0xefe0)	PsDECL(ps_bracerighttp)	 UsrDECL(0)},
{BciDECL(5723)	MslDECL(611)	UniDECL(0xefdf)	PsDECL(ps_bracerightmid) UsrDECL(0)},
{BciDECL(5724)	MslDECL(612)	UniDECL(0xefde)	PsDECL(ps_bracerightbt)	UsrDECL(0)},
{BciDECL(5781)  MslDECL(669)    UniDECL(0x220f) PsDECL(ps_product)	UsrDECL(0)},
{BciDECL(5782)	MslDECL(642)	UniDECL(0x2211)	PsDECL(ps_summation)	UsrDECL(0)},
{BciDECL(5819)	MslDECL(179)	UniDECL(0x00b7)	PsDECL(ps_middot)	UsrDECL(0)},
{BciDECL(5947)	MslDECL(NONE)	UniDECL(0x0394)	PsDECL(ps_Delta)	UsrDECL(0)},
{BciDECL(5984)	MslDECL(646)	UniDECL(0x256d)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5986)	MslDECL(655)	UniDECL(0x256e)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5987)	MslDECL(647)	UniDECL(0x2570)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(5989)	MslDECL(656)	UniDECL(0x256f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(6101)	MslDECL(684)	UniDECL(0x25ca)	PsDECL(ps_lozenge)	UsrDECL(0)},
{BciDECL(6103)	MslDECL(654)	UniDECL(0x25c7)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(6104)	MslDECL(662)	UniDECL(0x25c6)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(6120)	MslDECL(512)	UniDECL(0x03a5)	PsDECL(ps_Upsilon1)	UsrDECL(0)},
{BciDECL(6121)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_arrowvertex)	UsrDECL(0)},
{BciDECL(6122)	MslDECL(665)	UniDECL(0xefe8)	PsDECL(ps_arrowhorizex)	UsrDECL(0)},
{BciDECL(6233)	MslDECL(NONE)	UniDECL(0x03bc)	PsDECL(ps_mu)		UsrDECL(0)},
{BciDECL(6559)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_pencil)	UsrDECL(0)},
{BciDECL(6560)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_scissors)	UsrDECL(0)},
{BciDECL(6561)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_scissorscutting) UsrDECL(0)},
{BciDECL(6562)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_readingglasses)  UsrDECL(0)},
{BciDECL(6563)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bell)		UsrDECL(0)},
{BciDECL(6564)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_book)		UsrDECL(0)},
{BciDECL(6565)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_candle)	UsrDECL(0)},
{BciDECL(6566)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_telephonesolid)  UsrDECL(0)},
{BciDECL(6567)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_telhandsetcirc)  UsrDECL(0)},
{BciDECL(6568)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_envelopeback)	   UsrDECL(0)},
{BciDECL(6569)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_envelopefront)   UsrDECL(0)},
{BciDECL(6570)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_mailboxflagdwn)  UsrDECL(0)},
{BciDECL(6571)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_mailboxflagup)   UsrDECL(0)},
{BciDECL(6572)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_mailbxopnflgup)  UsrDECL(0)},
{BciDECL(6573)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_mailbxopnflgdwn) UsrDECL(0)},
{BciDECL(6574)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_folder)	UsrDECL(0)},
{BciDECL(6575)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_folderopen)	UsrDECL(0)},
{BciDECL(6576)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_filetalltext1) UsrDECL(0)},
{BciDECL(6577)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_filetalltext)	 UsrDECL(0)},
{BciDECL(6578)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_filetalltext3) UsrDECL(0)},
{BciDECL(6579)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_filecabinet)	UsrDECL(0)},
{BciDECL(6580)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_hourglass)	UsrDECL(0)},
{BciDECL(6581)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_keyboard)	UsrDECL(0)},
{BciDECL(6582)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_mouse2button)	UsrDECL(0)},
{BciDECL(6583)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ballpoint)	UsrDECL(0)},
{BciDECL(6584)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_pc)		UsrDECL(0)},
{BciDECL(6585)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_harddisk)	UsrDECL(0)},
{BciDECL(6586)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_floppy3)	UsrDECL(0)},
{BciDECL(6587)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_floppy5)	UsrDECL(0)},
{BciDECL(6588)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_tapereel)	UsrDECL(0)},
{BciDECL(6589)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_handwrite)	UsrDECL(0)},
{BciDECL(6590)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_handwriteleft) UsrDECL(0)},
{BciDECL(6591)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_handv)	UsrDECL(0)},
{BciDECL(6592)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_handok)	UsrDECL(0)},
{BciDECL(6593)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_thumbup)	UsrDECL(0)},
{BciDECL(6594)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_thumbdown)	UsrDECL(0)},
{BciDECL(6595)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_handptleft)	UsrDECL(0)},
{BciDECL(6596)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_handptright)	UsrDECL(0)},
{BciDECL(6597)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_handptup)	UsrDECL(0)},
{BciDECL(6598)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_handptdwn)	UsrDECL(0)},
{BciDECL(6599)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_handhalt)	UsrDECL(0)},
{BciDECL(6600)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_smileface)	UsrDECL(0)},
{BciDECL(6601)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_neutralface)	UsrDECL(0)},
{BciDECL(6602)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_frownface)	UsrDECL(0)},
{BciDECL(6603)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bomb)		UsrDECL(0)},
{BciDECL(6604)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_skullcrossbones) UsrDECL(0)},
{BciDECL(6605)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_flag)		UsrDECL(0)},
{BciDECL(6606)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_pennant)	UsrDECL(0)},
{BciDECL(6607)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_airplane)	UsrDECL(0)},
{BciDECL(6608)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_sunshine)	UsrDECL(0)},
{BciDECL(6609)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_droplet)	UsrDECL(0)},
{BciDECL(6610)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_snowflake)	UsrDECL(0)},
{BciDECL(6611)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_crossoutline)	UsrDECL(0)},
{BciDECL(6612)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_crossshadow)	UsrDECL(0)},
{BciDECL(6613)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_crossceltic)	UsrDECL(0)},
{BciDECL(6614)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_crossmaltese)	UsrDECL(0)},
{BciDECL(6615)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_starofdavid)	UsrDECL(0)},
{BciDECL(6616)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_crescentstar)	UsrDECL(0)},
{BciDECL(6617)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_yinyang)	UsrDECL(0)},
{BciDECL(6618)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_om)		UsrDECL(0)},
{BciDECL(6619)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_wheel)	UsrDECL(0)},
{BciDECL(6620)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_aries)	UsrDECL(0)},
{BciDECL(6621)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_taurus)	UsrDECL(0)},
{BciDECL(6622)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_gemini)	UsrDECL(0)},
{BciDECL(6623)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_cancer)	UsrDECL(0)},
{BciDECL(6624)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_leo)		UsrDECL(0)},
{BciDECL(6625)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_virgo)	UsrDECL(0)},
{BciDECL(6626)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_libra)	UsrDECL(0)},
{BciDECL(6627)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_scorpio)	UsrDECL(0)},
{BciDECL(6628)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_saggitarius)	UsrDECL(0)},
{BciDECL(6629)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_capricorn)	UsrDECL(0)},
{BciDECL(6630)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_aquarius)	UsrDECL(0)},
{BciDECL(6631)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_pisces)	UsrDECL(0)},
{BciDECL(6632)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ampersanditlc) UsrDECL(0)},
{BciDECL(6633)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ampersandit)	UsrDECL(0)},
{BciDECL(6634)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_cirlcle6)	UsrDECL(0)},
{BciDECL(6635)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_circleshadowdwn) UsrDECL(0)},
{BciDECL(6636)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_square6)	UsrDECL(0)},
{BciDECL(6637)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_box3)		UsrDECL(0)},
{BciDECL(6638)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_box4)		UsrDECL(0)},
{BciDECL(6639)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_boxshadowdwn)	UsrDECL(0)},
{BciDECL(6640)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_boxshadowup)	UsrDECL(0)},
{BciDECL(6641)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_lozenge4)	UsrDECL(0)},
{BciDECL(6642)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_lozenge6)	UsrDECL(0)},
{BciDECL(6643)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_rhombus6)	UsrDECL(0)},
{BciDECL(6644)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_xrhombus)	UsrDECL(0)},
{BciDECL(6645)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_rhombus4)	UsrDECL(0)},
{BciDECL(6646)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_clear)	UsrDECL(0)},
{BciDECL(6647)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_escape)	UsrDECL(0)},
{BciDECL(6648)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_command)	UsrDECL(0)},
{BciDECL(6649)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_rosette)	UsrDECL(0)},
{BciDECL(6650)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_rosettesolid)	UsrDECL(0)},
{BciDECL(6651)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_quotedbllftbld) UsrDECL(0)},
{BciDECL(6652)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_quotedblrtbld)  UsrDECL(0)},
{BciDECL(6653)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_zerosans)	UsrDECL(0)},
{BciDECL(6654)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_onesans)	UsrDECL(0)},
{BciDECL(6655)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_twosans)	UsrDECL(0)},
{BciDECL(6656)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_threesans)	UsrDECL(0)},
{BciDECL(6657)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_foursans)	UsrDECL(0)},
{BciDECL(6658)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_fivesans)	UsrDECL(0)},
{BciDECL(6659)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_sixsans)	UsrDECL(0)},
{BciDECL(6660)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_sevensans)	UsrDECL(0)},
{BciDECL(6661)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_eightsans)	UsrDECL(0)},
{BciDECL(6662)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ninesans)	UsrDECL(0)},
{BciDECL(6663)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_tensans)	UsrDECL(0)},
{BciDECL(6664)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_zerosansinv)	UsrDECL(0)},
{BciDECL(6665)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_onesansinv)	UsrDECL(0)},
{BciDECL(6666)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_twosansinv)	UsrDECL(0)},
{BciDECL(6667)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_threesansinv)	UsrDECL(0)},
{BciDECL(6668)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_foursansinv)	UsrDECL(0)},
{BciDECL(6669)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_fivesansinv)	UsrDECL(0)},
{BciDECL(6670)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_sixsansinv)	UsrDECL(0)},
{BciDECL(6671)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_sevensansinv)	UsrDECL(0)},
{BciDECL(6672)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_eightsansinv)	UsrDECL(0)},
{BciDECL(6673)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ninesansinv)	UsrDECL(0)},
{BciDECL(6674)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_tensansinv)	UsrDECL(0)},
{BciDECL(6675)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_budleafne)	UsrDECL(0)},
{BciDECL(6676)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_budleafnw)	UsrDECL(0)},
{BciDECL(6677)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_budleafsw)	UsrDECL(0)},
{BciDECL(6678)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_budleafse)	UsrDECL(0)},
{BciDECL(6679)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_vineleafboldne) UsrDECL(0)},
{BciDECL(6680)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_vineleafboldnw) UsrDECL(0)},
{BciDECL(6681)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_vineleafboldsw) UsrDECL(0)},
{BciDECL(6682)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_fineleafboldse) UsrDECL(0)},
{BciDECL(6683)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_circle2)	UsrDECL(0)},
{BciDECL(6684)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_cirlcle4)	UsrDECL(0)},
{BciDECL(6685)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_square2)	UsrDECL(0)},
{BciDECL(6686)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ring2)	UsrDECL(0)},
{BciDECL(6687)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ring4)	UsrDECL(0)},
{BciDECL(6688)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ring6)	UsrDECL(0)},
{BciDECL(6689)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_ringbutton2)	UsrDECL(0)},
{BciDECL(6690)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_target)	UsrDECL(0)},
{BciDECL(6691)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_cirlcleshadowup) UsrDECL(0)},
{BciDECL(6692)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_square4)	UsrDECL(0)},
{BciDECL(6693)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_box2)		UsrDECL(0)},
{BciDECL(6694)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_tristar2)	UsrDECL(0)},
{BciDECL(6695)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_crosstar2)	UsrDECL(0)},
{BciDECL(6696)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_pentastar2)	UsrDECL(0)},
{BciDECL(6697)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_hexstar2)	UsrDECL(0)},
{BciDECL(6698)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_octastar2)	UsrDECL(0)},
{BciDECL(6699)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_dodecastar3)	UsrDECL(0)},
{BciDECL(6700)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_octastar4)	UsrDECL(0)},
{BciDECL(6701)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_registersquare) UsrDECL(0)},
{BciDECL(6702)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_registercircle) UsrDECL(0)},
{BciDECL(6703)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_cuspopen)	UsrDECL(0)},
{BciDECL(6704)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_cusopen1)	UsrDECL(0)},
{BciDECL(6706)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_query)	UsrDECL(0)},
{BciDECL(6707)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_circlestar)	UsrDECL(0)},
{BciDECL(6708)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_starshadow)	UsrDECL(0)},
{BciDECL(6709)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_oneoclock)	UsrDECL(0)},
{BciDECL(6710)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_twooclock)	UsrDECL(0)},
{BciDECL(6711)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_threeoclock)	UsrDECL(0)},
{BciDECL(6712)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_fouroclock)	UsrDECL(0)},
{BciDECL(6713)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_fiveoclock)	UsrDECL(0)},
{BciDECL(6714)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_sixoclock)	UsrDECL(0)},
{BciDECL(6715)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_sevenoclock)	UsrDECL(0)},
{BciDECL(6716)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_eightoclock)	UsrDECL(0)},
{BciDECL(6717)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_nineoclock)	UsrDECL(0)},
{BciDECL(6718)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_tenoclock)	UsrDECL(0)},
{BciDECL(6719)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_elevenoclock)	UsrDECL(0)},
{BciDECL(6720)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_twelveoclock)	UsrDECL(0)},
{BciDECL(6721)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_arrowdwnleft1) UsrDECL(0)},
{BciDECL(6722)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_arrowdwnrt1)	UsrDECL(0)},
{BciDECL(6723)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_arrowupleft1)	UsrDECL(0)},
{BciDECL(6724)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_arrowuprt1)	UsrDECL(0)},
{BciDECL(6725)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_arrowleftup1)	UsrDECL(0)},
{BciDECL(6726)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_arrowrtup1)	UsrDECL(0)},
{BciDECL(6727)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_arrowleftdwn1)   UsrDECL(0)},
{BciDECL(6728)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_arrowrtdwn1)	   UsrDECL(0)},
{BciDECL(6729)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_quiltsquare2)	   UsrDECL(0)},
{BciDECL(6730)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_quiltsquare2inv) UsrDECL(0)},
{BciDECL(6731)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_leafccwsw)	UsrDECL(0)},
{BciDECL(6732)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_leafccwnw)	UsrDECL(0)},
{BciDECL(6733)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_leafccwse)	UsrDECL(0)},
{BciDECL(6734)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_leafccwne)	UsrDECL(0)},
{BciDECL(6735)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_leafnw)	UsrDECL(0)},
{BciDECL(6736)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_leafsw)	UsrDECL(0)},
{BciDECL(6737)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_leafne)	UsrDECL(0)},
{BciDECL(6738)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_leafse)	UsrDECL(0)},
{BciDECL(6739)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_deleteleft)	UsrDECL(0)},
{BciDECL(6740)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_deleteright)	UsrDECL(0)},
{BciDECL(6741)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_head2left)	UsrDECL(0)},
{BciDECL(6742)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_head2right)	UsrDECL(0)},
{BciDECL(6743)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_head2up)	UsrDECL(0)},
{BciDECL(6744)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_head2down)	UsrDECL(0)},
{BciDECL(6745)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_circleleft)	UsrDECL(0)},
{BciDECL(6746)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_circleright)	UsrDECL(0)},
{BciDECL(6747)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_circleup)	UsrDECL(0)},
{BciDECL(6748)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_circledown)	UsrDECL(0)},
{BciDECL(6749)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb2left)	UsrDECL(0)},
{BciDECL(6750)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb2right)	UsrDECL(0)},
{BciDECL(6751)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb2up)	UsrDECL(0)},
{BciDECL(6752)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb2down)	UsrDECL(0)},
{BciDECL(6753)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb2nw)	UsrDECL(0)},
{BciDECL(6754)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb2ne)	UsrDECL(0)},
{BciDECL(6755)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb2sw)	UsrDECL(0)},
{BciDECL(6756)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb2se)	UsrDECL(0)},
{BciDECL(6757)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb4left)	UsrDECL(0)},
{BciDECL(6758)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb4right)	UsrDECL(0)},
{BciDECL(6759)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb4up)	UsrDECL(0)},
{BciDECL(6760)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb4down)	UsrDECL(0)},
{BciDECL(6761)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb4nw)	UsrDECL(0)},
{BciDECL(6762)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb4ne)	UsrDECL(0)},
{BciDECL(6763)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb4sw)	UsrDECL(0)},
{BciDECL(6764)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_barb4se)	UsrDECL(0)},
{BciDECL(6765)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bleft)	UsrDECL(0)},
{BciDECL(6766)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bright)	UsrDECL(0)},
{BciDECL(6767)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bup)		UsrDECL(0)},
{BciDECL(6768)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bdown)	UsrDECL(0)},
{BciDECL(6769)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bleftright)	UsrDECL(0)},
{BciDECL(6770)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bupdown)	UsrDECL(0)},
{BciDECL(6771)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bnw)		UsrDECL(0)},
{BciDECL(6772)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bne)		UsrDECL(0)},
{BciDECL(6773)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bsw)		UsrDECL(0)},
{BciDECL(6774)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bse)		UsrDECL(0)},
{BciDECL(6775)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bdash1)	UsrDECL(0)},
{BciDECL(6776)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bdash2)	UsrDECL(0)},
{BciDECL(6777)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_xmarkbld)	UsrDECL(0)},
{BciDECL(6778)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_checkbld)	UsrDECL(0)},
{BciDECL(6779)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_boxxmarkbld)	UsrDECL(0)},
{BciDECL(6780)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_boxcheckbld)	UsrDECL(0)},
{BciDECL(6781)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_windowslogo)	UsrDECL(0)},
{BciDECL(7159)	MslDECL(640)	UniDECL(0xeffe)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(7160)	MslDECL(520)	UniDECL(0xefeb)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8347)	MslDECL(1034)	UniDECL(0x2120)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8593)	MslDECL(603)	UniDECL(0xefd4)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8594)	MslDECL(604)	UniDECL(0xefd3)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8595)	MslDECL(616)	UniDECL(0xefd0)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8596)	MslDECL(619)	UniDECL(0xefcd)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8597)	MslDECL(617)	UniDECL(0xefcf)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8598)	MslDECL(607)	UniDECL(0xefd1)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8599)	MslDECL(606)	UniDECL(0xefd2)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8600)	MslDECL(618)	UniDECL(0xefce)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8601)	MslDECL(636)	UniDECL(0xefca)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8602)	MslDECL(657)	UniDECL(0xefc3)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8603)	MslDECL(623)	UniDECL(0xefff)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8604)	MslDECL(3812)	UniDECL(0xf000)	PsDECL(ps_apple)	  UsrDECL(0)},
{BciDECL(8607)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_parenleftex)	  UsrDECL(0)},
{BciDECL(8608)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bracketleftex)  UsrDECL(0)},
{BciDECL(8609)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_parenrightex)	  UsrDECL(0)},
{BciDECL(8610)	MslDECL(NONE)	UniDECL(NONE)	PsDECL(ps_bracketrightex) UsrDECL(0)},
{BciDECL(8612)	MslDECL(1101)	UniDECL(0xefda)	PsDECL(ps_copyrightserif) UsrDECL(0)},
{BciDECL(8614)	MslDECL(1100)	UniDECL(0xefdb)	PsDECL(ps_registerserif)  UsrDECL(0)},
{BciDECL(8617)	MslDECL(1102)	UniDECL(0xefd9)	PsDECL(ps_trademarkserif) UsrDECL(0)},
{BciDECL(8627)	MslDECL(2094)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8628)	MslDECL(2107)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8629)	MslDECL(2095)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8630)	MslDECL(2096)	UniDECL(0x2702)	PsDECL(ps_a2)		UsrDECL(0)},
{BciDECL(8631)	MslDECL(2109)	UniDECL(0x2701)	PsDECL(ps_a1)		UsrDECL(0)},
{BciDECL(8632)	MslDECL(2097)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8633)	MslDECL(2110)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8634)	MslDECL(2111)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8635)	MslDECL(2098)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8636)	MslDECL(2108)	UniDECL(0x2709)	PsDECL(ps_a117)		UsrDECL(0)},
{BciDECL(8637)	MslDECL(2115)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8638)	MslDECL(2112)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8639)	MslDECL(2099)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8640)	MslDECL(2100)	UniDECL(0x275b)	PsDECL(ps_a97)		UsrDECL(0)},
{BciDECL(8641)	MslDECL(2113)	UniDECL(0x275c)	PsDECL(ps_a98)		UsrDECL(0)},
{BciDECL(8642)	MslDECL(2101)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8643)	MslDECL(2114)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8644)	MslDECL(2093)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8645)	MslDECL(2063)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8646)	MslDECL(2025)	UniDECL(NONE)	PsDECL(ps_a87)		UsrDECL(0)},
{BciDECL(8647)	MslDECL(2026)	UniDECL(NONE)	PsDECL(ps_a88)		UsrDECL(0)},
{BciDECL(8648)	MslDECL(2028)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8649)	MslDECL(2010)	UniDECL(0x2713)	PsDECL(ps_a19)		UsrDECL(0)},
{BciDECL(8650)	MslDECL(2091)	UniDECL(NONE)	PsDECL(ps_a71)		UsrDECL(0)},
{BciDECL(8651)	MslDECL(2059)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8652)	MslDECL(2048)	UniDECL(NONE)	PsDECL(ps_a76)		UsrDECL(0)},
{BciDECL(8653)	MslDECL(2080)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8654)	MslDECL(2086)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8655)	MslDECL(2054)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8656)	MslDECL(2068)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8657)	MslDECL(2049)	UniDECL(NONE)	PsDECL(ps_a92)		UsrDECL(0)},
{BciDECL(8658)	MslDECL(2036)	UniDECL(NONE)	PsDECL(ps_a86)		UsrDECL(0)},
{BciDECL(8659)	MslDECL(2081)	UniDECL(NONE)	PsDECL(ps_a91)		UsrDECL(0)},
{BciDECL(8660)	MslDECL(2083)	UniDECL(0x2605)	PsDECL(ps_a35)		UsrDECL(0)},
{BciDECL(8661)	MslDECL(2051)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8662)	MslDECL(2088)	UniDECL(0x2729)	PsDECL(ps_a36)		UsrDECL(0)},
{BciDECL(8663)	MslDECL(2052)	UniDECL(0x272a)	PsDECL(ps_a37)		UsrDECL(0)},
{BciDECL(8664)	MslDECL(2056)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8665)	MslDECL(2072)	UniDECL(0x2749)	PsDECL(ps_a68)		UsrDECL(0)},
{BciDECL(8666)	MslDECL(2040)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8667)	MslDECL(2046)	UniDECL(0x2736)	PsDECL(ps_a49)		UsrDECL(0)},
{BciDECL(8668)	MslDECL(2078)	UniDECL(0x272e)	PsDECL(ps_a41)		UsrDECL(0)},
{BciDECL(8669)	MslDECL(2084)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8670)	MslDECL(2047)	UniDECL(0x272b)	PsDECL(ps_a38)		UsrDECL(0)},
{BciDECL(8671)	MslDECL(2079)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8672)	MslDECL(2090)	UniDECL(0x2737)	PsDECL(ps_a50)		UsrDECL(0)},
{BciDECL(8673)	MslDECL(2058)	UniDECL(0x2730)	PsDECL(ps_a43)		UsrDECL(0)},
{BciDECL(8674)	MslDECL(2092)	UniDECL(0x272f)	PsDECL(ps_a42)		UsrDECL(0)},
{BciDECL(8675)	MslDECL(2060)	UniDECL(0x272d)	PsDECL(ps_a40)		UsrDECL(0)},
{BciDECL(8676)	MslDECL(2064)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8677)	MslDECL(2032)	UniDECL(0x272c)	PsDECL(ps_a39)		UsrDECL(0)},
{BciDECL(8678)	MslDECL(2016)	UniDECL(0x2780)	PsDECL(ps_a140)		UsrDECL(0)},
{BciDECL(8679)	MslDECL(2017)	UniDECL(0x2781)	PsDECL(ps_a141)		UsrDECL(0)},
{BciDECL(8680)	MslDECL(2018)	UniDECL(0x2782)	PsDECL(ps_a142)		UsrDECL(0)},
{BciDECL(8681)	MslDECL(2019)	UniDECL(0x2783)	PsDECL(ps_a143)		UsrDECL(0)},
{BciDECL(8682)	MslDECL(2020)	UniDECL(0x2784)	PsDECL(ps_a144)		UsrDECL(0)},
{BciDECL(8683)	MslDECL(2021)	UniDECL(0x2785)	PsDECL(ps_a145)		UsrDECL(0)},
{BciDECL(8684)	MslDECL(2022)	UniDECL(0x2786)	PsDECL(ps_a146)		UsrDECL(0)},
{BciDECL(8685)	MslDECL(2023)	UniDECL(0x2787)	PsDECL(ps_a147)		UsrDECL(0)},
{BciDECL(8686)	MslDECL(2024)	UniDECL(0x2788)	PsDECL(ps_a148)		UsrDECL(0)},
{BciDECL(8687)	MslDECL(2015)	UniDECL(0x2789)	PsDECL(ps_a149)		UsrDECL(0)},
{BciDECL(8688)	MslDECL(2000)	UniDECL(0x278a)	PsDECL(ps_a150)		UsrDECL(0)},
{BciDECL(8689)	MslDECL(2031)	UniDECL(0x278b)	PsDECL(ps_a151)		UsrDECL(0)},
{BciDECL(8690)	MslDECL(2002)	UniDECL(0x278c)	PsDECL(ps_a152)		UsrDECL(0)},
{BciDECL(8691)	MslDECL(2003)	UniDECL(0x278d)	PsDECL(ps_a153)		UsrDECL(0)},
{BciDECL(8692)	MslDECL(2004)	UniDECL(0x278e)	PsDECL(ps_a154)		UsrDECL(0)},
{BciDECL(8693)	MslDECL(2061)	UniDECL(0x278f)	PsDECL(ps_a155)		UsrDECL(0)},
{BciDECL(8694)	MslDECL(2005)	UniDECL(0x2790)	PsDECL(ps_a156)		UsrDECL(0)},
{BciDECL(8695)	MslDECL(2009)	UniDECL(0x2791)	PsDECL(ps_a157)		UsrDECL(0)},
{BciDECL(8696)	MslDECL(2007)	UniDECL(0x2792)	PsDECL(ps_a158)		UsrDECL(0)},
{BciDECL(8697)	MslDECL(2008)	UniDECL(0x2793)	PsDECL(ps_a159)		UsrDECL(0)},
{BciDECL(8698)	MslDECL(2116)	UniDECL(0x274d)	PsDECL(ps_a72)		UsrDECL(0)},
{BciDECL(8699)	MslDECL(2030)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8700)	MslDECL(2050)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8701)	MslDECL(2082)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8702)	MslDECL(2067)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8703)	MslDECL(2035)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8704)	MslDECL(2069)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8705)	MslDECL(2037)	UniDECL(0x279c)	PsDECL(ps_a167)		UsrDECL(0)},
{BciDECL(8706)	MslDECL(2070)	UniDECL(0x279d)	PsDECL(ps_a168)		UsrDECL(0)},
{BciDECL(8707)	MslDECL(2038)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8708)	MslDECL(2071)	UniDECL(0x279e)	PsDECL(ps_a169)		UsrDECL(0)},
{BciDECL(8709)	MslDECL(2039)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8710)	MslDECL(2041)	UniDECL(0x27a7)	PsDECL(ps_a177)		UsrDECL(0)},
{BciDECL(8711)	MslDECL(2073)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8712)	MslDECL(2062)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8713)	MslDECL(2042)	UniDECL(NONE)	PsDECL(ps_a161)		UsrDECL(0)},
{BciDECL(8714)	MslDECL(2074)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8715)	MslDECL(2043)	UniDECL(NONE)	PsDECL(ps_a163)		UsrDECL(0)},
{BciDECL(8716)	MslDECL(2075)	UniDECL(NONE)	PsDECL(ps_a164)		UsrDECL(0)},
{BciDECL(8717)	MslDECL(2117)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8718)	MslDECL(2104)	UniDECL(0x27bd)	PsDECL(ps_a190)		UsrDECL(0)},
{BciDECL(8719)	MslDECL(2012)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8720)	MslDECL(2105)	UniDECL(0x27b8)	PsDECL(ps_a186)		UsrDECL(0)},
{BciDECL(8721)	MslDECL(2118)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8722)	MslDECL(2106)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8723)	MslDECL(2119)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8724)	MslDECL(2001)	UniDECL(0x27a1)	PsDECL(ps_a172)		UsrDECL(0)},
{BciDECL(8725)	MslDECL(2006)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8726)	MslDECL(2006)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8726)	MslDECL(2057)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8727)	MslDECL(2055)	UniDECL(0x27a5)	PsDECL(ps_a175)		UsrDECL(0)},
{BciDECL(8728)	MslDECL(2087)	UniDECL(0x27a6)	PsDECL(ps_a176)		UsrDECL(0)},
{BciDECL(8729)	MslDECL(2034)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8730)	MslDECL(2066)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8731)	MslDECL(2085)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8732)	MslDECL(2053)	UniDECL(0x279b)	PsDECL(ps_a166)		UsrDECL(0)},
{BciDECL(8733)	MslDECL(2065)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8734)	MslDECL(2033)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8735)	MslDECL(2089)	UniDECL(0x2756)	PsDECL(ps_a79)		UsrDECL(0)},
{BciDECL(8736)	MslDECL(2045)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8737)	MslDECL(2027)	UniDECL(0x2717)	PsDECL(ps_a23)		UsrDECL(0)},
{BciDECL(8738)	MslDECL(2077)	UniDECL(0x2718)	PsDECL(ps_a24)		UsrDECL(0)},
{BciDECL(8739)	MslDECL(2029)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8740)	MslDECL(2014)	UniDECL(0x2722)	PsDECL(ps_a29)		UsrDECL(0)},
{BciDECL(8741)	MslDECL(2044)	UniDECL(0x271d)	PsDECL(ps_a6)		UsrDECL(0)},
{BciDECL(8742)	MslDECL(2076)	UniDECL(0x271e)	PsDECL(ps_a7)		UsrDECL(0)},
{BciDECL(8743)	MslDECL(2102)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8744)	MslDECL(2103)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8745)	MslDECL(2011)	UniDECL(0x2727)	PsDECL(ps_a34)		UsrDECL(0)},
{BciDECL(8746)	MslDECL(2013)	UniDECL(0x2708)	PsDECL(ps_a118)		UsrDECL(0)},
{BciDECL(8747)	MslDECL(2294)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8748)	MslDECL(2311)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8749)	MslDECL(2295)	UniDECL(0x261e)	PsDECL(ps_a12)		UsrDECL(0)},
{BciDECL(8750)	MslDECL(2312)	UniDECL(0x261b)	PsDECL(ps_a11)		UsrDECL(0)},
{BciDECL(8751)	MslDECL(2296)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8752)	MslDECL(2314)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8753)	MslDECL(2297)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8754)	MslDECL(2298)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8755)	MslDECL(2315)	UniDECL(0x2706)	PsDECL(ps_a5)		UsrDECL(0)},
{BciDECL(8756)	MslDECL(2316)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8757)	MslDECL(2299)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8758)	MslDECL(2317)	UniDECL(0x2766)	PsDECL(ps_a107)		UsrDECL(0)},
{BciDECL(8759)	MslDECL(2300)	UniDECL(0x2767)	PsDECL(ps_a108)		UsrDECL(0)},
{BciDECL(8760)	MslDECL(2301)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8761)	MslDECL(2318)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8762)	MslDECL(2319)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8763)	MslDECL(2320)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8764)	MslDECL(2293)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8765)	MslDECL(2263)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8766)	MslDECL(2225)	UniDECL(NONE)	PsDECL(ps_a93)		UsrDECL(0)},
{BciDECL(8767)	MslDECL(2226)	UniDECL(NONE)	PsDECL(ps_a94)		UsrDECL(0)},
{BciDECL(8768)	MslDECL(2210)	UniDECL(0x2714)	PsDECL(ps_a20)		UsrDECL(0)},
{BciDECL(8769)	MslDECL(2228)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8770)	MslDECL(2291)	UniDECL(NONE)	PsDECL(ps_a73)		UsrDECL(0)},
{BciDECL(8771)	MslDECL(2259)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8772)	MslDECL(2248)	UniDECL(NONE)	PsDECL(ps_a77)		UsrDECL(0)},
{BciDECL(8773)	MslDECL(2280)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8774)	MslDECL(2286)	UniDECL(0x2715)	PsDECL(ps_a21)		UsrDECL(0)},
{BciDECL(8775)	MslDECL(2254)	UniDECL(0x2716)	PsDECL(ps_a22)		UsrDECL(0)},
{BciDECL(8776)	MslDECL(2268)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8777)	MslDECL(2236)	UniDECL(NONE)	PsDECL(ps_a85)		UsrDECL(0)},
{BciDECL(8778)	MslDECL(2249)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8779)	MslDECL(2281)	UniDECL(NONE)	PsDECL(ps_a81)		UsrDECL(0)},
{BciDECL(8780)	MslDECL(2283)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8781)	MslDECL(2251)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8782)	MslDECL(2256)	UniDECL(0x2731)	PsDECL(ps_a44)		UsrDECL(0)},
{BciDECL(8783)	MslDECL(2284)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8784)	MslDECL(2252)	UniDECL(0x2732)	PsDECL(ps_a45)		UsrDECL(0)},
{BciDECL(8785)	MslDECL(2240)	UniDECL(0x2738)	PsDECL(ps_a51)		UsrDECL(0)},
{BciDECL(8786)	MslDECL(2272)	UniDECL(0x2739)	PsDECL(ps_a52)		UsrDECL(0)},
{BciDECL(8787)	MslDECL(2278)	UniDECL(0x273d)	PsDECL(ps_a56)		UsrDECL(0)},
{BciDECL(8788)	MslDECL(2246)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8789)	MslDECL(2279)	UniDECL(0x273b)	PsDECL(ps_a54)		UsrDECL(0)},
{BciDECL(8790)	MslDECL(2247)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8791)	MslDECL(2290)	UniDECL(0x273e)	PsDECL(ps_a57)		UsrDECL(0)},
{BciDECL(8792)	MslDECL(2258)	UniDECL(0x2743)	PsDECL(ps_a62)		UsrDECL(0)},
{BciDECL(8793)	MslDECL(2260)	UniDECL(0x2747)	PsDECL(ps_a66)		UsrDECL(0)},
{BciDECL(8794)	MslDECL(2292)	UniDECL(0x2748)	PsDECL(ps_a67)		UsrDECL(0)},
{BciDECL(8795)	MslDECL(2232)	UniDECL(0x274b)	PsDECL(ps_a70)		UsrDECL(0)},
{BciDECL(8796)	MslDECL(2264)	UniDECL(0x274a)	PsDECL(ps_a69)		UsrDECL(0)},
{BciDECL(8797)	MslDECL(2288)	UniDECL(0x2741)	PsDECL(ps_a60)		UsrDECL(0)},
{BciDECL(8798)	MslDECL(2216)	UniDECL(0x2460)	PsDECL(ps_a120)		UsrDECL(0)},
{BciDECL(8799)	MslDECL(2217)	UniDECL(0x2461)	PsDECL(ps_a121)		UsrDECL(0)},
{BciDECL(8800)	MslDECL(2218)	UniDECL(0x2462)	PsDECL(ps_a122)		UsrDECL(0)},
{BciDECL(8801)	MslDECL(2219)	UniDECL(0x2463)	PsDECL(ps_a123)		UsrDECL(0)},
{BciDECL(8802)	MslDECL(2220)	UniDECL(0x2464)	PsDECL(ps_a124)		UsrDECL(0)},
{BciDECL(8803)	MslDECL(2221)	UniDECL(0x2465)	PsDECL(ps_a125)		UsrDECL(0)},
{BciDECL(8804)	MslDECL(2222)	UniDECL(0x2466)	PsDECL(ps_a126)		UsrDECL(0)},
{BciDECL(8805)	MslDECL(2223)	UniDECL(0x2467)	PsDECL(ps_a127)		UsrDECL(0)},
{BciDECL(8806)	MslDECL(2224)	UniDECL(0x2468)	PsDECL(ps_a128)		UsrDECL(0)},
{BciDECL(8807)	MslDECL(2215)	UniDECL(0x2469)	PsDECL(ps_a129)		UsrDECL(0)},
{BciDECL(8808)	MslDECL(2200)	UniDECL(0x2776)	PsDECL(ps_a130)		UsrDECL(0)},
{BciDECL(8809)	MslDECL(2231)	UniDECL(0x2777)	PsDECL(ps_a131)		UsrDECL(0)},
{BciDECL(8810)	MslDECL(2202)	UniDECL(0x2778)	PsDECL(ps_a132)		UsrDECL(0)},
{BciDECL(8811)	MslDECL(2203)	UniDECL(0x2779)	PsDECL(ps_a133)		UsrDECL(0)},
{BciDECL(8812)	MslDECL(2204)	UniDECL(0x277a)	PsDECL(ps_a134)		UsrDECL(0)},
{BciDECL(8813)	MslDECL(2261)	UniDECL(0x277b)	PsDECL(ps_a135)		UsrDECL(0)},
{BciDECL(8814)	MslDECL(2205)	UniDECL(0x277c)	PsDECL(ps_a136)		UsrDECL(0)},
{BciDECL(8815)	MslDECL(2209)	UniDECL(0x277d)	PsDECL(ps_a137)		UsrDECL(0)},
{BciDECL(8816)	MslDECL(2207)	UniDECL(0x277e)	PsDECL(ps_a138)		UsrDECL(0)},
{BciDECL(8817)	MslDECL(2208)	UniDECL(0x277f)	PsDECL(ps_a139)		UsrDECL(0)},
{BciDECL(8818)	MslDECL(2250)	UniDECL(0x2751)	PsDECL(ps_a75)		UsrDECL(0)},
{BciDECL(8819)	MslDECL(2282)	UniDECL(0x274f)	PsDECL(ps_a74)		UsrDECL(0)},
{BciDECL(8820)	MslDECL(2267)	UniDECL(0x27a2)	PsDECL(ps_a173)		UsrDECL(0)},
{BciDECL(8821)	MslDECL(2235)	UniDECL(0x27a4)	PsDECL(ps_a174)		UsrDECL(0)},
{BciDECL(8822)	MslDECL(2269)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8823)	MslDECL(2237)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8824)	MslDECL(2270)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8825)	MslDECL(2238)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8826)	MslDECL(2271)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8827)	MslDECL(2239)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8828)	MslDECL(2273)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8829)	MslDECL(2241)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8830)	MslDECL(2242)	UniDECL(0x27bc)	PsDECL(ps_a189)		UsrDECL(0)},
{BciDECL(8831)	MslDECL(2274)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8832)	MslDECL(2275)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8833)	MslDECL(2243)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8834)	MslDECL(2321)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8835)	MslDECL(2304)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8836)	MslDECL(2212)	UniDECL(0x27be)	PsDECL(ps_a191)		UsrDECL(0)},
{BciDECL(8837)	MslDECL(2322)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8838)	MslDECL(2305)	UniDECL(0x2799)	PsDECL(ps_a165)		UsrDECL(0)},
{BciDECL(8839)	MslDECL(2262)	UniDECL(0x27a8)	PsDECL(ps_a178)		UsrDECL(0)},
{BciDECL(8840)	MslDECL(2234)	UniDECL(0x27b3)	PsDECL(ps_a184)		UsrDECL(0)},
{BciDECL(8841)	MslDECL(2323)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8842)	MslDECL(2306)	UniDECL(0x2794)	PsDECL(ps_a160)		UsrDECL(0)},
{BciDECL(8843)	MslDECL(2206)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8844)	MslDECL(2201)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8845)	MslDECL(2289)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8846)	MslDECL(2257)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8847)	MslDECL(2287)	UniDECL(0x27b5)	PsDECL(ps_a185)		UsrDECL(0)},
{BciDECL(8848)	MslDECL(2255)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8849)	MslDECL(2266)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8850)	MslDECL(2285)	UniDECL(0x27ba)	PsDECL(ps_a187)		UsrDECL(0)},
{BciDECL(8851)	MslDECL(2253)	UniDECL(0x27bb)	PsDECL(ps_a188)		UsrDECL(0)},
{BciDECL(8852)	MslDECL(2233)	UniDECL(0x2798)	PsDECL(ps_a196)		UsrDECL(0)},
{BciDECL(8853)	MslDECL(2265)	UniDECL(0x27b4)	PsDECL(ps_a197)		UsrDECL(0)},
{BciDECL(8854)	MslDECL(2245)	UniDECL(0x27b7)	PsDECL(ps_a198)		UsrDECL(0)},
{BciDECL(8855)	MslDECL(2277)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8856)	MslDECL(2244)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8857)	MslDECL(2227)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8858)	MslDECL(2276)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8859)	MslDECL(2229)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8860)	MslDECL(2302)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8861)	MslDECL(2303)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8862)	MslDECL(2230)	UniDECL(0x2720)	PsDECL(ps_a9)		UsrDECL(0)},
{BciDECL(8863)	MslDECL(2211)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8863)	MslDECL(2213)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8864)	MslDECL(2213)	UniDECL(0x2765)	PsDECL(ps_a106)		UsrDECL(0)},
{BciDECL(8865)	MslDECL(2313)	UniDECL(0x270c)	PsDECL(ps_a13)		UsrDECL(0)},
{BciDECL(8866)	MslDECL(2214)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8867)	MslDECL(2494)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8868)	MslDECL(2510)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8869)	MslDECL(2511)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8870)	MslDECL(2495)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8871)	MslDECL(2497)	UniDECL(0x2704)	PsDECL(ps_a3)		UsrDECL(0)},
{BciDECL(8872)	MslDECL(2512)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8873)	MslDECL(2496)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8874)	MslDECL(2498)	UniDECL(0x260e)	PsDECL(ps_a4)		UsrDECL(0)},
{BciDECL(8875)	MslDECL(2514)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8876)	MslDECL(2515)	UniDECL(0x270d)	PsDECL(ps_a14)		UsrDECL(0)},
{BciDECL(8877)	MslDECL(2499)	UniDECL(0x270e)	PsDECL(ps_a15)		UsrDECL(0)},
{BciDECL(8878)	MslDECL(2500)	UniDECL(0x2761)	PsDECL(ps_a101)		UsrDECL(0)},
{BciDECL(8879)	MslDECL(2516)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8880)	MslDECL(2501)	UniDECL(0x275d)	PsDECL(ps_a99)		UsrDECL(0)},
{BciDECL(8881)	MslDECL(2517)	UniDECL(0x275e)	PsDECL(ps_a100)		UsrDECL(0)},
{BciDECL(8882)	MslDECL(2518)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8883)	MslDECL(2519)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8884)	MslDECL(2463)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8885)	MslDECL(2493)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8886)	MslDECL(2425)	UniDECL(NONE)	PsDECL(ps_a89)		UsrDECL(0)},
{BciDECL(8887)	MslDECL(2426)	UniDECL(NONE)	PsDECL(ps_a90)		UsrDECL(0)},
{BciDECL(8888)	MslDECL(2410)	UniDECL(0x2763)	PsDECL(ps_a103)		UsrDECL(0)},
{BciDECL(8889)	MslDECL(2428)	UniDECL(0x2762)	PsDECL(ps_a102)		UsrDECL(0)},
{BciDECL(8890)	MslDECL(2491)	UniDECL(0x25c6)	PsDECL(ps_a78)		UsrDECL(0)},
{BciDECL(8891)	MslDECL(2459)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8892)	MslDECL(2480)	UniDECL(0x2758)	PsDECL(ps_a82)		UsrDECL(0)},
{BciDECL(8893)	MslDECL(2486)	UniDECL(0x2759)	PsDECL(ps_a83)		UsrDECL(0)},
{BciDECL(8894)	MslDECL(2454)	UniDECL(0x275a)	PsDECL(ps_a84)		UsrDECL(0)},
{BciDECL(8895)	MslDECL(2448)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8896)	MslDECL(2449)	UniDECL(NONE)	PsDECL(ps_a95)		UsrDECL(0)},
{BciDECL(8897)	MslDECL(2481)	UniDECL(NONE)	PsDECL(ps_a96)		UsrDECL(0)},
{BciDECL(8898)	MslDECL(2436)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8899)	MslDECL(2468)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8900)	MslDECL(2483)	UniDECL(0x273c)	PsDECL(ps_a55)		UsrDECL(0)},
{BciDECL(8901)	MslDECL(2451)	UniDECL(0x273f)	PsDECL(ps_a58)		UsrDECL(0)},
{BciDECL(8902)	MslDECL(2484)	UniDECL(0x2733)	PsDECL(ps_a46)		UsrDECL(0)},
{BciDECL(8903)	MslDECL(2488)	UniDECL(0x271b)	PsDECL(ps_a27)		UsrDECL(0)},
{BciDECL(8904)	MslDECL(2456)	UniDECL(0x271c)	PsDECL(ps_a28)		UsrDECL(0)},
{BciDECL(8905)	MslDECL(2472)	UniDECL(0x2723)	PsDECL(ps_a30)		UsrDECL(0)},
{BciDECL(8906)	MslDECL(2440)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8907)	MslDECL(2478)	UniDECL(0x2724)	PsDECL(ps_a31)		UsrDECL(0)},
{BciDECL(8908)	MslDECL(2479)	UniDECL(0x2735)	PsDECL(ps_a48)		UsrDECL(0)},
{BciDECL(8909)	MslDECL(2447)	UniDECL(0x2734)	PsDECL(ps_a47)		UsrDECL(0)},
{BciDECL(8910)	MslDECL(2452)	UniDECL(0x273a)	PsDECL(ps_a53)		UsrDECL(0)},
{BciDECL(8911)	MslDECL(2458)	UniDECL(0x2707)	PsDECL(ps_a119)		UsrDECL(0)},
{BciDECL(8912)	MslDECL(2490)	UniDECL(0x2742)	PsDECL(ps_a61)		UsrDECL(0)},
{BciDECL(8913)	MslDECL(2460)	UniDECL(0x2740)	PsDECL(ps_a59)		UsrDECL(0)},
{BciDECL(8914)	MslDECL(2446)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8915)	MslDECL(2492)	UniDECL(0x2745)	PsDECL(ps_a64)		UsrDECL(0)},
{BciDECL(8916)	MslDECL(2432)	UniDECL(0x2746)	PsDECL(ps_a65)		UsrDECL(0)},
{BciDECL(8917)	MslDECL(2464)	UniDECL(0x2744)	PsDECL(ps_a63)		UsrDECL(0)},
{BciDECL(8918)	MslDECL(2416)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8919)	MslDECL(2417)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8920)	MslDECL(2418)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8921)	MslDECL(2419)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8922)	MslDECL(2420)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8923)	MslDECL(2421)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8924)	MslDECL(2422)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8925)	MslDECL(2423)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8926)	MslDECL(2424)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8927)	MslDECL(2415)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8928)	MslDECL(2400)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8929)	MslDECL(2431)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8930)	MslDECL(2402)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8931)	MslDECL(2403)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8932)	MslDECL(2404)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8933)	MslDECL(2461)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8934)	MslDECL(2405)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8935)	MslDECL(2409)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8936)	MslDECL(2407)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8937)	MslDECL(2408)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8938)	MslDECL(2482)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8939)	MslDECL(2450)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8940)	MslDECL(2414)	UniDECL(0x2764)	PsDECL(ps_a104)		UsrDECL(0)},
{BciDECL(8941)	MslDECL(2513)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8942)	MslDECL(2467)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8943)	MslDECL(2435)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8944)	MslDECL(2437)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8945)	MslDECL(2469)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8946)	MslDECL(2438)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8947)	MslDECL(2439)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8948)	MslDECL(2471)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8949)	MslDECL(2470)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8950)	MslDECL(2441)	UniDECL(0x27a9)	PsDECL(ps_a179)		UsrDECL(0)},
{BciDECL(8951)	MslDECL(2473)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8952)	MslDECL(2442)	UniDECL(0x27ab)	PsDECL(ps_a180)		UsrDECL(0)},
{BciDECL(8953)	MslDECL(2474)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8954)	MslDECL(2520)	UniDECL(0x279f)	PsDECL(ps_a170)		UsrDECL(0)},
{BciDECL(8955)	MslDECL(2504)	UniDECL(0x27a0)	PsDECL(ps_a171)		UsrDECL(0)},
{BciDECL(8956)	MslDECL(2443)	UniDECL(0x27ad)	PsDECL(ps_a181)		UsrDECL(0)},
{BciDECL(8957)	MslDECL(2475)	UniDECL(0x27af)	PsDECL(ps_a182)		UsrDECL(0)},
{BciDECL(8958)	MslDECL(2412)	UniDECL(0x27b2)	PsDECL(ps_a183)		UsrDECL(0)},
{BciDECL(8959)	MslDECL(2434)	UniDECL(NONE)	PsDECL(ps_a109)		UsrDECL(0)},
{BciDECL(8960)	MslDECL(2453)	UniDECL(NONE)	PsDECL(ps_a110)		UsrDECL(0)},
{BciDECL(8961)	MslDECL(2433)	UniDECL(NONE)	PsDECL(ps_a111)		UsrDECL(0)},
{BciDECL(8962)	MslDECL(2445)	UniDECL(NONE)	PsDECL(ps_a112)		UsrDECL(0)},
{BciDECL(8963)	MslDECL(2466)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8964)	MslDECL(2485)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8965)	MslDECL(2465)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8966)	MslDECL(2477)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8967)	MslDECL(2521)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8968)	MslDECL(2505)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8969)	MslDECL(2506)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8970)	MslDECL(2406)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8971)	MslDECL(2401)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8972)	MslDECL(2489)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8973)	MslDECL(2457)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8974)	MslDECL(2522)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8975)	MslDECL(2487)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8976)	MslDECL(2455)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8977)	MslDECL(2427)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8978)	MslDECL(2429)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8979)	MslDECL(2502)	UniDECL(0x2725)	PsDECL(ps_a32)		UsrDECL(0)},
{BciDECL(8980)	MslDECL(2503)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8981)	MslDECL(2444)	UniDECL(0x2719)	PsDECL(ps_a25)		UsrDECL(0)},
{BciDECL(8982)	MslDECL(2476)	UniDECL(0x271f)	PsDECL(ps_a8)		UsrDECL(0)},
{BciDECL(8983)	MslDECL(2411)	UniDECL(0x2721)	PsDECL(ps_a10)		UsrDECL(0)},
{BciDECL(8984)	MslDECL(2413)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8985)	MslDECL(2430)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(8986)	MslDECL(2462)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(9042)	MslDECL(1104)	UniDECL(0xefd7)	PsDECL(ps_copyrightsans) UsrDECL(0)},
{BciDECL(9044)	MslDECL(1103)	UniDECL(0xefd8)	PsDECL(ps_registersans)	 UsrDECL(0)},
{BciDECL(9047)	MslDECL(1105)	UniDECL(0xefd6)	PsDECL(ps_trademarksans) UsrDECL(0)},
{BciDECL(9072)	MslDECL(2120)	UniDECL(0x2703)	PsDECL(ps_a202)		UsrDECL(0)},
{BciDECL(9073)	MslDECL(2509)	UniDECL(0x2710)	PsDECL(ps_a105)		UsrDECL(0)},
{BciDECL(9074)	MslDECL(NONE)	UniDECL(0x271a)	PsDECL(ps_a26)		UsrDECL(0)},
{BciDECL(9075)	MslDECL(2726)	UniDECL(0x2726)	PsDECL(ps_a33)		UsrDECL(0)},
{BciDECL(9076)	MslDECL(2325)	UniDECL(0x2750)	PsDECL(ps_a203)		UsrDECL(0)},
{BciDECL(9077)	MslDECL(2308)	UniDECL(0x2752)	PsDECL(ps_a204)		UsrDECL(0)},
{BciDECL(9078)	MslDECL(2310)	UniDECL(0x279a)	PsDECL(ps_a192)		UsrDECL(0)},
{BciDECL(9079)	MslDECL(2326)	UniDECL(0x27a3)	PsDECL(ps_a162)		UsrDECL(0)},
{BciDECL(9080)	MslDECL(2507)	UniDECL(0x27ac)	PsDECL(ps_a199)		UsrDECL(0)},
{BciDECL(9081)	MslDECL(2309)	UniDECL(0x27b9)	PsDECL(ps_a195)		UsrDECL(0)},
{BciDECL(9082)	MslDECL(2122)	UniDECL(NONE)	PsDECL(ps_a206)		UsrDECL(0)},
{BciDECL(9083)	MslDECL(2328)	UniDECL(NONE)	PsDECL(ps_a205)		UsrDECL(0)},
{BciDECL(9084)	MslDECL(2508)	UniDECL(0x27ae)	PsDECL(ps_a200)		UsrDECL(0)},
{BciDECL(9091)	MslDECL(2524)	UniDECL(0x27b1)	PsDECL(ps_a201)		UsrDECL(0)},
{BciDECL(9092)	MslDECL(2327)	UniDECL(0x27b6)	PsDECL(ps_a194)		UsrDECL(0)},
{BciDECL(9117)	MslDECL(1023)	UniDECL(0x2009)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18274)	MslDECL(663)	UniDECL(0x220d)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18280)	MslDECL(595)	UniDECL(0x2111)	PsDECL(ps_Ifraktur)	UsrDECL(0)},
{BciDECL(18281)	MslDECL(596)	UniDECL(0x211c)	PsDECL(ps_Rfraktur)	UsrDECL(0)},
{BciDECL(18282)	MslDECL(NONE)	UniDECL(0x223c)	PsDECL(ps_similar)	UsrDECL(0)},
{BciDECL(18283)	MslDECL(1099)	UniDECL(0x21b5)	PsDECL(ps_carriagereturn) UsrDECL(0)},
{BciDECL(18291)	MslDECL(644)	UniDECL(0xefc9)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18292)	MslDECL(648)	UniDECL(0xefc8)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18307)	MslDECL(650)	UniDECL(0xefc6)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18308)	MslDECL(649)	UniDECL(0xefc7)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18309)	MslDECL(658)	UniDECL(0xefc2)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18310)	MslDECL(502)	UniDECL(0x212f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18314)	MslDECL(583)	UniDECL(0x2218)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18315)	MslDECL(651)	UniDECL(0xefc5)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18316)	MslDECL(652)	UniDECL(0xefc4)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18317)	MslDECL(659)	UniDECL(0xefc1)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18318)	MslDECL(660)	UniDECL(0xefc0)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18319)	MslDECL(559)	UniDECL(0xefe9)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18426)	MslDECL(1114)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(18742)	MslDECL(2307)	UniDECL(0x270f)	PsDECL(ps_a16)		UsrDECL(0)},
{BciDECL(18743)	MslDECL(2324)	UniDECL(0x2711)	PsDECL(ps_a17)		UsrDECL(0)},
{BciDECL(18744)	MslDECL(2523)	UniDECL(0x27aa)	PsDECL(ps_a193)		UsrDECL(0)},
{BciDECL(18745)	MslDECL(2121)	UniDECL(0x2712)	PsDECL(ps_a18)		UsrDECL(0)},
{BciDECL(19573)	MslDECL(666)	UniDECL(0xefcb)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(19574)	MslDECL(667)	UniDECL(0xefcc)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(19575)	MslDECL(668)	UniDECL(0x221f)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(19579)	MslDECL(560)	UniDECL(0xefea)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(19709)	MslDECL(1113)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(19710)	MslDECL(1112)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(20001)	MslDECL(598)	UniDECL(0xefe7)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(20002)	MslDECL(599)	UniDECL(0xefe6)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(20003)	MslDECL(608)	UniDECL(0xefe5)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(20004)	MslDECL(609)	UniDECL(0xefe4)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(20985)	MslDECL(594)	UniDECL(0x212d)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(20986)	MslDECL(597)	UniDECL(0x2128)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(20987)	MslDECL(639)	UniDECL(0x2112)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(20988)	MslDECL(1115)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(20989)	MslDECL(1116)	UniDECL(NONE)	PsDECL(ns)		UsrDECL(0)},
{BciDECL(21325)	MslDECL(195)	UniDECL(0x00ad)	PsDECL(ps_sfthyphen)	UsrDECL(0)},

/* Cyrillic Translations: */
{BciDECL(8156)	MslDECL(1714)	UniDECL(0x0410)	PsDECL(ps_afii10017)	UsrDECL(0)},
{BciDECL(8157)	MslDECL(1716)	UniDECL(0x0412)	PsDECL(ps_afii10018)	UsrDECL(0)},
{BciDECL(8158)	MslDECL(1715)	UniDECL(0x0411)	PsDECL(ps_afii10019)	UsrDECL(0)},
{BciDECL(8159)	MslDECL(1717)	UniDECL(0x0413)	PsDECL(ps_afii10020)	UsrDECL(0)},
{BciDECL(8160)	MslDECL(1718)	UniDECL(0x0414)	PsDECL(ps_afii10021)	UsrDECL(0)},
{BciDECL(8161)	MslDECL(1719)	UniDECL(0x0415)	PsDECL(ps_afii10022)	UsrDECL(0)},
{BciDECL(8162)	MslDECL(1700)	UniDECL(0x0401)	PsDECL(ps_afii10023)	UsrDECL(0)},
{BciDECL(8163)	MslDECL(1720)	UniDECL(0x0416)	PsDECL(ps_afii10024)	UsrDECL(0)},
{BciDECL(8164)	MslDECL(1721)	UniDECL(0x0417)	PsDECL(ps_afii10025)	UsrDECL(0)},
{BciDECL(8165)	MslDECL(1722)	UniDECL(0x0418)	PsDECL(ps_afii10026)	UsrDECL(0)},
{BciDECL(8166)	MslDECL(1723)	UniDECL(0x0419)	PsDECL(ps_afii10027)	UsrDECL(0)},
{BciDECL(8167)	MslDECL(1724)	UniDECL(0x041a)	PsDECL(ps_afii10028)	UsrDECL(0)},
{BciDECL(8168)	MslDECL(1725)	UniDECL(0x041b)	PsDECL(ps_afii10029)	UsrDECL(0)},
{BciDECL(8169)	MslDECL(1726)	UniDECL(0x041c)	PsDECL(ps_afii10030)	UsrDECL(0)},
{BciDECL(8170)	MslDECL(1727)	UniDECL(0x041d)	PsDECL(ps_afii10031)	UsrDECL(0)},
{BciDECL(8171)	MslDECL(1728)	UniDECL(0x041e)	PsDECL(ps_afii10032)	UsrDECL(0)},
{BciDECL(8172)	MslDECL(1729)	UniDECL(0x041f)	PsDECL(ps_afii10033)	UsrDECL(0)},
{BciDECL(8173)	MslDECL(1730)	UniDECL(0x0420)	PsDECL(ps_afii10034)	UsrDECL(0)},
{BciDECL(8174)	MslDECL(1731)	UniDECL(0x0421)	PsDECL(ps_afii10035)	UsrDECL(0)},
{BciDECL(8175)	MslDECL(1732)	UniDECL(0x0422)	PsDECL(ps_afii10036)	UsrDECL(0)},
{BciDECL(8176)	MslDECL(1733)	UniDECL(0x0423)	PsDECL(ps_afii10037)	UsrDECL(0)},
{BciDECL(8177)	MslDECL(1734)	UniDECL(0x0424)	PsDECL(ps_afii10038)	UsrDECL(0)},
{BciDECL(8178)	MslDECL(1735)	UniDECL(0x0425)	PsDECL(ps_afii10039)	UsrDECL(0)},
{BciDECL(8179)	MslDECL(1736)	UniDECL(0x0426)	PsDECL(ps_afii10040)	UsrDECL(0)},
{BciDECL(8180)	MslDECL(1737)	UniDECL(0x0427)	PsDECL(ps_afii10041)	UsrDECL(0)},
{BciDECL(8181)	MslDECL(1738)	UniDECL(0x0428)	PsDECL(ps_afii10041)	UsrDECL(0)},
{BciDECL(8182)	MslDECL(1739)	UniDECL(0x0429)	PsDECL(ps_afii10043)	UsrDECL(0)},
{BciDECL(8183)	MslDECL(1740)	UniDECL(0x042a)	PsDECL(ps_afii10044)	UsrDECL(0)},
{BciDECL(8184)	MslDECL(1741)	UniDECL(0x042b)	PsDECL(ps_afii10045)	UsrDECL(0)},
{BciDECL(8185)	MslDECL(1742)	UniDECL(0x042c)	PsDECL(ps_afii10046)	UsrDECL(0)},
{BciDECL(8186)	MslDECL(1743)	UniDECL(0x042d)	PsDECL(ps_afii10047)	UsrDECL(0)},
{BciDECL(8187)	MslDECL(1744)	UniDECL(0x042e)	PsDECL(ps_afii10048)	UsrDECL(0)},
{BciDECL(8188)	MslDECL(1745)	UniDECL(0x042f)	PsDECL(ps_afii10049)	UsrDECL(0)},
{BciDECL(8189)	MslDECL(1703)	UniDECL(0x0404)	PsDECL(ps_afii10053)	UsrDECL(0)},
{BciDECL(8190)	MslDECL(1706)	UniDECL(0x0407)	PsDECL(ps_afii10056)	UsrDECL(0)},
{BciDECL(8191)	MslDECL(1705)	UniDECL(0x0406)	PsDECL(ps_afii10055)	UsrDECL(0)},
{BciDECL(8193)	MslDECL(1710)	UniDECL(0x040b)	PsDECL(ps_afii10060)	UsrDECL(0)},
{BciDECL(8194)	MslDECL(1701)	UniDECL(0x0402)	PsDECL(ps_afii10051)	UsrDECL(0)},
{BciDECL(8195)	MslDECL(1713)	UniDECL(0x040f)	PsDECL(ps_afii10145)	UsrDECL(0)},
{BciDECL(8196)	MslDECL(1707)	UniDECL(0x0408)	PsDECL(ps_afii10057)	UsrDECL(0)},
{BciDECL(8197)	MslDECL(1708)	UniDECL(0x0409)	PsDECL(ps_afii10058)	UsrDECL(0)},
{BciDECL(8198)	MslDECL(1709)	UniDECL(0x040a)	PsDECL(ps_afii10059)	UsrDECL(0)},
{BciDECL(8199)	MslDECL(1704)	UniDECL(0x0405)	PsDECL(ps_afii10054)	UsrDECL(0)},
{BciDECL(8201)	MslDECL(1746)	UniDECL(0x0430)	PsDECL(ps_afii10065)	UsrDECL(0)},
{BciDECL(8202)	MslDECL(1747)	UniDECL(0x0431)	PsDECL(ps_afii10066)	UsrDECL(0)},
{BciDECL(8203)	MslDECL(1748)	UniDECL(0x0432)	PsDECL(ps_afii10067)	UsrDECL(0)},
{BciDECL(8204)	MslDECL(1749)	UniDECL(0x0433)	PsDECL(ps_afii10068)	UsrDECL(0)},
{BciDECL(8205)	MslDECL(1750)	UniDECL(0x0434)	PsDECL(ps_afii10069)	UsrDECL(0)},
{BciDECL(8206)	MslDECL(1751)	UniDECL(0x0435)	PsDECL(ps_afii10070)	UsrDECL(0)},
{BciDECL(8207)	MslDECL(1778)	UniDECL(0x0451)	PsDECL(ps_afii10071)	UsrDECL(0)},
{BciDECL(8208)	MslDECL(1752)	UniDECL(0x0436)	PsDECL(ps_afii10072)	UsrDECL(0)},
{BciDECL(8209)	MslDECL(1753)	UniDECL(0x0437)	PsDECL(ps_afii10073)	UsrDECL(0)},
{BciDECL(8210)	MslDECL(1754)	UniDECL(0x0438)	PsDECL(ps_afii10074)	UsrDECL(0)},
{BciDECL(8211)	MslDECL(1755)	UniDECL(0x0439)	PsDECL(ps_afii10075)	UsrDECL(0)},
{BciDECL(8212)	MslDECL(1756)	UniDECL(0x043a)	PsDECL(ps_afii10076)	UsrDECL(0)},
{BciDECL(8213)	MslDECL(1757)	UniDECL(0x043b)	PsDECL(ps_afii10077)	UsrDECL(0)},
{BciDECL(8214)	MslDECL(1758)	UniDECL(0x043c)	PsDECL(ps_afii10078)	UsrDECL(0)},
{BciDECL(8215)	MslDECL(1759)	UniDECL(0x043d)	PsDECL(ps_afii10079)	UsrDECL(0)},
{BciDECL(8216)	MslDECL(1760)	UniDECL(0x043e)	PsDECL(ps_afii10080)	UsrDECL(0)},
{BciDECL(8217)	MslDECL(1761)	UniDECL(0x043f)	PsDECL(ps_afii10081)	UsrDECL(0)},
{BciDECL(8218)	MslDECL(1762)	UniDECL(0x0440)	PsDECL(ps_afii10082)	UsrDECL(0)},
{BciDECL(8219)	MslDECL(1763)	UniDECL(0x0441)	PsDECL(ps_afii10083)	UsrDECL(0)},
{BciDECL(8220)	MslDECL(1764)	UniDECL(0x0442)	PsDECL(ps_afii10084)	UsrDECL(0)},
{BciDECL(8221)	MslDECL(1765)	UniDECL(0x0443)	PsDECL(ps_afii10085)	UsrDECL(0)},
{BciDECL(8222)	MslDECL(1766)	UniDECL(0x0444)	PsDECL(ps_afii10086)	UsrDECL(0)},
{BciDECL(8223)	MslDECL(1767)	UniDECL(0x0445)	PsDECL(ps_afii10087)	UsrDECL(0)},
{BciDECL(8224)	MslDECL(1768)	UniDECL(0x0446)	PsDECL(ps_afii10088)	UsrDECL(0)},
{BciDECL(8225)	MslDECL(1769)	UniDECL(0x0447)	PsDECL(ps_afii10089)	UsrDECL(0)},
{BciDECL(8226)	MslDECL(1770)	UniDECL(0x0448)	PsDECL(ps_afii10090)	UsrDECL(0)},
{BciDECL(8227)	MslDECL(1771)	UniDECL(0x0449)	PsDECL(ps_afii10091)	UsrDECL(0)},
{BciDECL(8228)	MslDECL(1772)	UniDECL(0x044a)	PsDECL(ps_afii10092)	UsrDECL(0)},
{BciDECL(8229)	MslDECL(1773)	UniDECL(0x044b)	PsDECL(ps_afii10093)	UsrDECL(0)},
{BciDECL(8230)	MslDECL(1774)	UniDECL(0x044c)	PsDECL(ps_afii10094)	UsrDECL(0)},
{BciDECL(8231)	MslDECL(1775)	UniDECL(0x044d)	PsDECL(ps_afii10095)	UsrDECL(0)},
{BciDECL(8232)	MslDECL(1776)	UniDECL(0x044e)	PsDECL(ps_afii10096)	UsrDECL(0)},
{BciDECL(8233)	MslDECL(1777)	UniDECL(0x044f)	PsDECL(ps_afii10097)	UsrDECL(0)},
{BciDECL(8234)	MslDECL(1781)	UniDECL(0x0454)	PsDECL(ps_afii10101)	UsrDECL(0)},
{BciDECL(8235)	MslDECL(1784)	UniDECL(0x0457)	PsDECL(ps_afii10104)	UsrDECL(0)},
{BciDECL(8236)	MslDECL(1783)	UniDECL(0x0456)	PsDECL(ps_afii10103)	UsrDECL(0)},
{BciDECL(8238)	MslDECL(1788)	UniDECL(0x045b)	PsDECL(ps_afii10108)	UsrDECL(0)},
{BciDECL(8239)	MslDECL(1779)	UniDECL(0x0452)	PsDECL(ps_afii10099)	UsrDECL(0)},
{BciDECL(8240)	MslDECL(1791)	UniDECL(0x045e)	PsDECL(ps_afii10193)	UsrDECL(0)},
{BciDECL(8241)	MslDECL(1785)	UniDECL(0x0458)	PsDECL(ps_afii10105)	UsrDECL(0)},
{BciDECL(8242)	MslDECL(1786)	UniDECL(0x0459)	PsDECL(ps_afii10106)	UsrDECL(0)},
{BciDECL(8243)	MslDECL(1787)	UniDECL(0x045a)	PsDECL(ps_afii10107)	UsrDECL(0)},
{BciDECL(8244)	MslDECL(1782)	UniDECL(0x0455)	PsDECL(ps_afii10102)	UsrDECL(0)},
{BciDECL(9089)	MslDECL(1032)	UniDECL(0x2116)	PsDECL(ps_afii61352)	UsrDECL(0)},
{BciDECL(9271)	MslDECL(1702)	UniDECL(0x0403)	PsDECL(ps_afii10052)	UsrDECL(0)},
{BciDECL(9272)	MslDECL(1780)	UniDECL(0x0453)	PsDECL(ps_afii10100)	UsrDECL(0)},
{BciDECL(9273)	MslDECL(1711)	UniDECL(0x040c)	PsDECL(ps_afii10061)	UsrDECL(0)},
{BciDECL(9274)	MslDECL(1789)	UniDECL(0x045c)	PsDECL(ps_afii10109)	UsrDECL(0)},
{BciDECL(9275)	MslDECL(1712)	UniDECL(0x040e)	PsDECL(ps_afii10062)	UsrDECL(0)},
{BciDECL(9276)	MslDECL(1790)	UniDECL(0x045d)	PsDECL(ps_afii10110)	UsrDECL(0)},

/* Baltic Translations */
{BciDECL(289)	MslDECL(339)	UniDECL(0x0167)	PsDECL(ps_tbar)		UsrDECL(0)},
{BciDECL(290)	MslDECL(338)	UniDECL(0x0166)	PsDECL(ps_Tbar)		UsrDECL(0)},
{BciDECL(344)	MslDECL(471)	UniDECL(0x0169)	PsDECL(ps_utilde)	UsrDECL(0)},
{BciDECL(345)	MslDECL(470)	UniDECL(0x0168)	PsDECL(ps_Utilde)	UsrDECL(0)},
{BciDECL(369)	MslDECL(486)	UniDECL(0x0128)	PsDECL(ps_Itilde)	UsrDECL(0)},
{BciDECL(370)	MslDECL(487)	UniDECL(0x0129)	PsDECL(ps_itilde)	UsrDECL(0)},
{BciDECL(403)	MslDECL(335)	UniDECL(0x0138)	PsDECL(ps_kra)		UsrDECL(0)},
{BciDECL(405)	MslDECL(340)	UniDECL(0x014a)	PsDECL(ps_Eng)		UsrDECL(0)},
{BciDECL(406)	MslDECL(341)	UniDECL(0x014b)	PsDECL(ps_eng)		UsrDECL(0)},
{BciDECL(417)	MslDECL(480)	UniDECL(0x0172)	PsDECL(ps_Uogonek)	UsrDECL(0)},
{BciDECL(421)	MslDECL(481)	UniDECL(0x0173)	PsDECL(ps_uogonek)	UsrDECL(0)},
{BciDECL(630)	MslDECL(421)	UniDECL(0x0113)	PsDECL(ps_emacron)	UsrDECL(0)},
{BciDECL(631)	MslDECL(420)	UniDECL(0x0112)	PsDECL(ps_Emacron)	UsrDECL(0)},
{BciDECL(637)	MslDECL(428)	UniDECL(0x0122)	PsDECL(ps_Gcedilla)	UsrDECL(0)},
{BciDECL(638)	MslDECL(435)	UniDECL(0x012b)	PsDECL(ps_imacron)	UsrDECL(0)},
{BciDECL(639)	MslDECL(434)	UniDECL(0x012a)	PsDECL(ps_Imacron)	UsrDECL(0)},
{BciDECL(642)	MslDECL(433)	UniDECL(0x012f)	PsDECL(ps_iogonek)	UsrDECL(0)},
{BciDECL(643)	MslDECL(432)	UniDECL(0x012e)	PsDECL(ps_Iogonek)	UsrDECL(0)},
{BciDECL(644)	MslDECL(439)	UniDECL(0x0137)	PsDECL(ps_kcedilla)	UsrDECL(0)},
{BciDECL(645)	MslDECL(438)	UniDECL(0x0136)	PsDECL(ps_Kcedilla)	UsrDECL(0)},
{BciDECL(650)	MslDECL(445)	UniDECL(0x013c)	PsDECL(ps_kcedilla)	UsrDECL(0)},
{BciDECL(651)	MslDECL(444)	UniDECL(0x013b)	PsDECL(ps_Lcedilla)	UsrDECL(0)},
{BciDECL(654)	MslDECL(451)	UniDECL(0x0146)	PsDECL(ps_ncedilla)	UsrDECL(0)},
{BciDECL(655)	MslDECL(450)	UniDECL(0x0145)	PsDECL(ps_Ncedilla)	UsrDECL(0)},
{BciDECL(662)	MslDECL(461)	UniDECL(0x0157)	PsDECL(ps_rcedilla)	UsrDECL(0)},
{BciDECL(663)	MslDECL(460)	UniDECL(0x0156)	PsDECL(ps_Rcedilla)	UsrDECL(0)},
{BciDECL(678)	MslDECL(479)	UniDECL(0x016b)	PsDECL(ps_umacron)	UsrDECL(0)},
{BciDECL(679)	MslDECL(478)	UniDECL(0x016a)	PsDECL(ps_Umacron)	UsrDECL(0)},
{BciDECL(2647)	MslDECL(403)	UniDECL(0x0101)	PsDECL(ps_amacron)	UsrDECL(0)},
{BciDECL(2653)	MslDECL(402)	UniDECL(0x0100)	PsDECL(ps_Amacron)	UsrDECL(0)},
{BciDECL(2876)	MslDECL(419)	UniDECL(0x0117)	PsDECL(ps_edot)		UsrDECL(0)},
{BciDECL(2882)	MslDECL(418)	UniDECL(0x0116)	PsDECL(ps_Edot)		UsrDECL(0)},
{BciDECL(3464)	MslDECL(455)	UniDECL(0x014d)	PsDECL(ps_omacron)	UsrDECL(0)},
{BciDECL(3470)	MslDECL(454)	UniDECL(0x014c)	PsDECL(ps_Omacron)	UsrDECL(0)},
{BciDECL(9100)	MslDECL(429)	UniDECL(0x0123)	PsDECL(ps_gcedilla)	UsrDECL(0)}
};
#endif

/*************************************************************************************
*	GlyphNumElements()
*		returns number of elements in gMasterGlyphTable[]
*		NOTE: this function must be in this file to work properly!
*	RETURNS:
*************************************************************************************/

FUNCTION ufix16 GlyphNumElements(void)
{
	ufix16 itemp1 = sizeof (struct Mapping_Table);
	ufix16 itemp2 = sizeof (gMasterGlyphCodes);

	return(itemp2/itemp1);
}

/* EOF cmtglob.c */
