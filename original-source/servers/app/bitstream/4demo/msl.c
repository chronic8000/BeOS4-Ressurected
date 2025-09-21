
/*      msl.c           */

/*
*
*
*       $Header: //bliss/user/archive/rcs/4in1/msl.c,v 14.0 97/03/17 17:22:28 shawn Release $
*
*
*       $Log:	msl.c,v $
 * Revision 14.0  97/03/17  17:22:28  shawn
 * TDIS Version 6.00
 * 
 * Revision 13.2  96/08/20  12:48:09  shawn
 * Added RCS/Header & Log keywords.
 * 
*
*
*/


#include "spdo_prv.h"               /* General definitions for Speedo    */
#if PROC_PCL
#include "fino.h"
#include "type1.h"
#include "finfotbl.h"
#include "ufe.h"					/* Unified Front End definitions */
#include "pcl5fnt.h"

typedef	struct
	{
	fix15		fileIndex;
	ufix16	charID;
	} pclEntry;


GLOBAL_PROTO btsbool  MSLtoIndex (SPGLOBPTR2 ufix16 STACKFAR *intPtr);

GLOBAL_PROTO fix15    MSLCompare (SPGLOBPTR2  fix31 idx, void STACKFAR *keyValue);

GLOBAL_PROTO void     BuildSortedMSLIDList (TPS_GPTR1);

static   fix15    numAdded;

btsbool  gListSorted = FALSE;

static   pclEntry gSortedMSLList[256];

extern   PC5HEAD  pcl5head;


FUNCTION fix15 MSLCompare (
				SPGLOBPTR2
				fix31 idx,
				void STACKFAR *keyValue
			  )
{
fix15 NumComp(ufix16 STACKFAR *, ufix16 STACKFAR *), result;
ufix16 theMSLID;
	theMSLID = gSortedMSLList[idx].charID;
	result = NumComp( (ufix16 STACKFAR *)keyValue, (ufix16 STACKFAR *)&theMSLID);
	return(result);
}

FUNCTION btsbool MSLtoIndex(SPGLOBPTR2 ufix16 STACKFAR *intPtr)
{
ufix16 outValue;
btsbool success = FALSE;
	outValue = 0;
	success = BSearch (PARAMS2  (fix15 STACKFAR*)&outValue, MSLCompare,
						(void STACKFAR *)intPtr, (fix31)numAdded );
	if (success)
		*intPtr = gSortedMSLList[outValue].fileIndex;
	return(success);
}

FUNCTION void BuildSortedMSLIDList(TPS_GPTR1)
{
   fix15 i, j, k, theSlot;
   ufix16   theMSL;
   btsbool  result, found;
#if PROC_TRUEDOC && REENTRANT_ALLOC
SPEEDO_GLOBALS STACKFAR *sp_global_ptr;
sp_global_ptr = GetspGlobalPtr(pCspGlobals);
#endif

   if (gListSorted)
      return;
   if (pcl5head.size != 80)
   {
      printf("do not support unbound pcl font\n");
      exit(1);
   }
   if ( (result = fi_select_symbol_set( CSP_PARAMS2 pcl5head.symbol_set )) == TRUE )
   {
      numAdded = 0;
      for (i=0; i < 256; i++)
      {
         if ( sp_globals.gCurrentSymbolSet[i] != UNKNOWN )
         {
      		result = fi_CharCodeXLate  ( (void STACKFAR *)&sp_globals.gCurrentSymbolSet[i],
               			   					(void STACKFAR *)&theMSL,
	   		            	   				protoBCID, protoMSL);
            if (result == TRUE)
            {
               theSlot = numAdded;
               found = FALSE;
               for (j=0; !found && (j < numAdded); j++)
               {
                  if (gSortedMSLList[j].charID > theMSL)
                  {
                     theSlot = j;
                     found = TRUE;
                  }
               }
               for (k=numAdded; k > theSlot; k--)
                  gSortedMSLList[k] = gSortedMSLList[k-1];
         		gSortedMSLList[theSlot].charID = theMSL;
         		gSortedMSLList[theSlot].fileIndex = i;
               numAdded++;
            }
         }
      }
   }         
   else
   {
      printf("could not find symbol set ID %d\n", pcl5head.symbol_set);
      exit(1);
   }
   gListSorted = TRUE;
}      

#endif /* PROC_PCL */
/* EOF msl.c */
