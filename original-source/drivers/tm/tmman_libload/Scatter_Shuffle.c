/*
 *  COPYRIGHT (c) 1997 by Philips Semiconductors
 *
 *   +-----------------------------------------------------------------+
 *   | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *   | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *   | A LICENSE AND WITH THE INCLUSION OF THIS COPY RIGHT NOTICE.     |
 *   | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *   | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *   | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *   +-----------------------------------------------------------------+
 *
 *  Module name              : Scatter_Shuffle.c    1.14
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : 
 *
 *  Last update              : 14:41:17 - 98/04/24
 *
 *  Description              :  
 *
 */

/*---------------------------- Includes --------------------------------------*/

#include "Scatter_Shuffle.h"
#include "Lib_Mapping.h"
#include "Lib_Local.h"

/*-------------------------- Module State ------------------------------------*/


typedef struct {
	TMObj_Scatter  scatter; 
	TMObj_Word     offset;
} ScatterInstantiation;


/*--------------------------- Functions --------------------------------------*/


#define BYTESPERWORD         4
#define BITSPERBYTE          8
#define BITSPERWORD         32

#define BYTESPERSHUFFLEPAGE 32 


static void
	shuffle(
    		TMObj_Scatter_Dest     dest,
    		Int                    bit,
    		Int                    offset
        )
{
    Int offset_of_shuffle_page;
    Int offset_in_shuffle_page;

    Int new_bit;
    Int new_offset_in_shuffle_page;

    offset_in_shuffle_page = offset % BYTESPERSHUFFLEPAGE;
    offset_of_shuffle_page = offset - offset_in_shuffle_page;

    new_bit                    = offset_in_shuffle_page / BYTESPERWORD;
    new_offset_in_shuffle_page = ((BITSPERBYTE * offset_in_shuffle_page) + bit) % BITSPERWORD;

    dest->shift  = new_bit - bit;
    dest->offset = offset_of_shuffle_page + new_offset_in_shuffle_page;
}



static void
	unshuffle(
    		TMObj_Scatter_Dest     dest,
    		Int                    bit,
    		Int                    offset
        )
{
    Int offset_of_shuffle_page;
    Int offset_in_shuffle_page;

    Int new_bit;
    Int new_offset_in_shuffle_page;

    offset_in_shuffle_page = offset % BYTESPERSHUFFLEPAGE;
    offset_of_shuffle_page = offset - offset_in_shuffle_page;

    new_bit                    = offset_in_shuffle_page % BITSPERBYTE;
    new_offset_in_shuffle_page = offset_in_shuffle_page / BITSPERBYTE  +  BYTESPERWORD * bit;

    dest->shift  = new_bit - bit;
    dest->offset = offset_of_shuffle_page + new_offset_in_shuffle_page;
}



typedef void (*ScatterShuffler)(TMObj_Scatter_Dest,Int,Int);


static TMObj_Scatter
    scatter_mangle( 
            TMObj_Scatter    scatter, 
            TMObj_Word       offset,
            ScatterShuffler  shuffler
    )
{
    Int                    done;
    TMObj_Scatter          result;
    TMObj_Scatter_Dest     dest;
    TMObj_Scatter_Source   source;
    TMObj_Word             mask;
    Int                    cur_index;
    Int                    bit;

    Lib_Mem_NEW( result );

    result->dest   = Lib_Mem_MALLOC( BITSPERWORD * sizeof(TMObj_Scatter_Dest_Rec  ) );
    result->source = Lib_Mem_MALLOC( BITSPERWORD * sizeof(TMObj_Scatter_Source_Rec) );

    done      = 0;
    cur_index = 0;
    dest      = scatter->dest;
    source    = scatter->source;

    while ( (unsigned)done != 0xffffffff ) {
        mask  = source->mask;
        done |= mask;

        bit = 0;
        while (mask != 0) {
            if (mask & 0x1) {

                shuffler(
                   &result->dest[cur_index],
                    bit    + dest->shift,
                    offset + dest->offset
                );

                result->source[cur_index].mask  = 1 << bit;
                result->dest[cur_index].offset -= offset;
                result->dest[cur_index].shift  += dest->shift;

                cur_index++;
            }
            mask >>= 1;
            bit++;
        }

        dest++;
        source++;
    }

    return result;
}




/*--------------------------- Functions --------------------------------------*/

    
/*
this 'static' function is never used within this file
static void get_bitscatter( TMObj_Scatter scatter, Int bit, Int* offset, Int* bit_mask )
{
    Int mask= 1 << bit;
    Int i= 0;
    Int shift;

    while ( !(scatter->source[i].mask & mask) ) { i++; }

    *offset= scatter->dest[i].offset;
     shift = scatter->dest[i].shift;

     if (shift < 0) {
         *bit_mask= mask >> -shift;
     } else {
         *bit_mask= mask <<  shift;
     }
}
*/
/*
this 'static' function is never used within this file
static Bool same_scatter( TMObj_Scatter  l, TMObj_Scatter  r )
{
    Int i;

    for (i=0; i<32; i++) {
        Int loffset, lmask;
        Int roffset, rmask;

        get_bitscatter( l, i, &loffset, &lmask );
        get_bitscatter( r, i, &roffset, &rmask );

        if ( loffset != roffset || lmask != rmask ) { 
            return False;
        }
    }

    return True;
} 
*/



/*--------------------------- Functions --------------------------------------*/

    
static Int 
	scatter_dest_hash( 
		TMObj_Scatter_Dest l, 
		Int                length
        )
{
    Int i;
    Int result = 0;

    for (i=0; i<length; i++) {
        result ^= l[i].offset 
                ^ l[i].shift;
    }

    return result;
}

static Int 
	scatter_dest_equal( 
		TMObj_Scatter_Dest l, 
		TMObj_Scatter_Dest r,
		Int                length
	)
{
    Int i;

    if (l==r) { return True; }

    for (i=0; i<length; i++) {
        if (l[i].offset != r[i].offset) return False;
        if (l[i].shift  != r[i].shift ) return False;
    }

    return True;
}



                     /* -------- . -------- */


static Int 
	scatter_source_hash( 
		TMObj_Scatter_Source l,
		Int                 *length
	)
{
    Int done   = 0;
    Int result = 0;

   *length= 0;
    while ( (unsigned)done != 0xffffffff ) {
        result ^= l->mask;
        done   |= l->mask;
        l++;
        (*length)++;
    }

    return result;
}

static Bool 
	scatter_source_equal( 
		TMObj_Scatter_Source l, 
		TMObj_Scatter_Source r,
		Int                 *length
	)
{
    Int  done  = 0;
    Bool result= True;

   *length= 0;
    while ( (unsigned)done != 0xffffffff ) {
        if (l->mask != r->mask) result= False;
        done |= l->mask;
        l++;
        r++;
        (*length)++;
    }

    return result;
}



                     /* -------- . -------- */


static Int 
	scatter_hash( TMObj_Scatter l )
{
    Int length;
    Int sh=  scatter_source_hash (l->source, &length );
    Int dh=  scatter_dest_hash   (l->dest,    length );

    return sh^dh;
}

static Bool 
	scatter_equal( TMObj_Scatter l, TMObj_Scatter r )
{
    Int length;

    if (l==r) { return True; }
     
    return scatter_source_equal(l->source, r->source, &length) 
        && scatter_dest_equal  (l->dest,   r->dest,    length);
}



                     /* -------- . -------- */


static Int 
	scatter_inst_hash( ScatterInstantiation *l )
{
    return l->offset 
         ^ scatter_hash(l->scatter);
}

static Bool 
	scatter_inst_equal( ScatterInstantiation *l, ScatterInstantiation *r )
{
    return (l->offset == r->offset)
        && scatter_equal( l->scatter, r->scatter );
}



                     /* -------- . -------- */


/*
 * Function         : Convert a scatter which identifies
 *                    a word in *unshuffled* TM1 code, into an
 *                    equivalent scatter for the same code
 *                    after shuffling.
 * Parameters       : scatter   (I) input scatter
 *                    offset    (I) 'location' of the scattered
 *                                  word in the (unshuffled) code
 *                    memo      (IO) pointer to mapping for memoising
 *                                   shuffle results
 * Function Result  : shuffled scatter.
 *                    NB: this function will return earlier results
 *                        when called again with the same inputs
 */

TMObj_Scatter
    ScatShuf_shuffle( 
            TMObj_Scatter  scatter, 
            TMObj_Word     offset,
            Lib_Mapping   *memo 
    )
{
    ScatterInstantiation si;
    TMObj_Scatter        result;

   /*
    * Get a hash value on the reference offset, 
    * but prevent that offsetting this offset
    * with the scatter offsets results in a negative
    * value (% and / in C can't stand that).
    * Note that the actual offset value is not
    * important; only its value modulo BYTESPERSHUFFLEPAGE.
    */
    offset %= BYTESPERSHUFFLEPAGE;
    offset += 2*BYTESPERSHUFFLEPAGE;

    if (*memo == Null) {
        *memo= 
            Lib_Mapping_create(
               (Lib_Local_BoolResFunc)scatter_inst_equal, 	
               (Lib_Local_EltResFunc)scatter_inst_hash, 	
               1000 	
            );
    }

    si.scatter= scatter;
    si.offset = offset;

    result= Lib_Mapping_apply( *memo, &si );

    if (result == Null) {
        ScatterInstantiation *psi;

        Lib_Mem_NEW(psi);
        *psi= si;

        result= scatter_mangle(scatter,offset,shuffle);
        Lib_Mapping_define( *memo, psi, result );
    }

    return result;
}



/*
 * Function         : Convert a scatter which identifies
 *                    a word in *shuffled* TM1 code, into an
 *                    equivalent scatter for the same code
 *                    after unshuffling.
 * Parameters       : scatter   (I) input scatter
 *                    offset    (I) 'location' of the scattered
 *                                  word in the (unshuffled) code
 *                    memo      (IO) pointer to mapping for memoising
 *                                   unshuffle results
 * Function Result  : shuffled scatter.
 *                    NB: this function will return earlier results
 *                        when called again with the same inputs
 */

TMObj_Scatter
    ScatShuf_unshuffle( 
            TMObj_Scatter  scatter, 
            TMObj_Word     offset,
            Lib_Mapping   *memo 
    )
{
    ScatterInstantiation si;
    TMObj_Scatter        result;

   /*
    * Get a hash value on the reference offset, 
    * but prevent that offsetting this offset
    * with the scatter offsets results in a negative
    * value (% and / in C can't stand that).
    * Note that the actual offset value is not
    * important; only its value modulo BYTESPERSHUFFLEPAGE.
    */
    offset %= BYTESPERSHUFFLEPAGE;
    offset += 2*BYTESPERSHUFFLEPAGE;

    if (*memo == Null) {
        *memo= 
            Lib_Mapping_create(
               (Lib_Local_BoolResFunc)scatter_inst_equal, 	
               (Lib_Local_EltResFunc)scatter_inst_hash, 	
               1000 	
            );
    }

    si.scatter= scatter;
    si.offset = offset;

    result= Lib_Mapping_apply( *memo, &si );

    if (result == Null) {
        ScatterInstantiation *psi;

        Lib_Mem_NEW(psi);
        *psi= si;

        result= scatter_mangle(scatter,offset,unshuffle);
        Lib_Mapping_define( *memo, psi, result );
    }

    return result;
}
