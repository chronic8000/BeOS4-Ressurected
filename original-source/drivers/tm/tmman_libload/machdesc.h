/*
 *  +-------------------------------------------------------------------+
 *  | Copyright (c) 1995,1996,1997 by Philips Semiconductors.           |
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
 *  Module name              : machdesc.h    1.7
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : 
 *
 *  Last update              : 08:38:07 - 97/03/19
 *
 *  Description              :  
 *
 * 	This module offers a generic machine description file database. 
 *      The database is built with a call to MD_parse which takes a 
 *      pathname as input. After that, the database can be accessed in 
 *      two ways. 
 *      
 *      (1) direct access via the MD_Machine structure that contains 
 *          global data about the machine like datacache linesize, like 
 *          bitwidth of operation parts etc. 
 *      
 *      (2) lookup of opcodes, unit and unittypes. Special access 
 *          routines are provided for this. Opcodes and unittypes
 *          can be looked up via their name, and opcodes can also 
 *          be looked up via their opcode (value).
 *
 *      ALL ACCESSES TO THE DATABASE ARE ASSUMED TO BE READ ONLY.
 *	
 *	There is not way to delete the claimed data yet.
 */

#ifndef  MD_INCLUDED
#define  MD_INCLUDED


/*---------------------------- Includes --------------------------------------*/

#include "tmtypes.h"
#include "Lib_List.h"
#include "Lib_Set.h"
#include "Lib_Mapping.h"


/*--------------------------- Definitions  -----------------------------------*/


typedef Address  MD_Address;
typedef UInt8     MD_Register;

typedef struct MD_Opcode        *MD_Opcode;
typedef struct MD_UnitType    	*MD_UnitType;
typedef struct MD_Unit     	*MD_Unit;
typedef struct MD_Machine     	MD_Machine;


struct MD_Opcode {
	String		name;      	/* Unique name */
	Int16	 	value;		/* opcode */                   

	long		param_hi;      
	long 		param_lo;                
	Int8 		param_step;
	Int8 		param_shift;  

	Int8 		arity;                    

	Bool 	resultless : 1;       
	Bool 	loadclass  : 1;
	Bool 	storeclass : 1;
	Bool 	exception  : 1;
	Bool 	flow 	   : 1;
	Bool 	floating   : 1;
	Bool 	imm 	   : 1;
	Bool 	parametric : 1;
	Bool 	param_signed  : 1;      
	Bool 	value_defined : 1;     
	Bool 	pseudo 	      : 1;            

	MD_UnitType	unittype; 
};



struct MD_UnitType {
	String		name;      	/* Unique name */
	UInt8 		latency;
	UInt8 		recovery;
	UInt8 		delay;
	UInt8		number;		/* the <unittypeid> number */
	UInt8		mask;		/* mask for input crossbar matrix */
	Lib_Set		opcodes;	/* set of MD_Opcode */       
};


     /* 
      * assumption is that each unit/unittype has 1 connection 
      * to an issue slot, thus unit type + slot uniquely 
      * determines the unit instance 
      */
struct MD_Unit {
	MD_UnitType	unittype;
	UInt8		slot;		/* issue slot for this instanced unit */
	UInt8		number;	/* the <unitid> number */
};



struct MD_Machine {
	UInt8 		nrof_issue_slots;
	UInt8 		nrof_registers;
	UInt8 		nrof_writebuses;
	Int 		dcache_size;
	Int 		icache_size;
	Int 		dcache_line_size;
	Int 		icache_line_size;
	Int 		oper_bitwidth;		/* bitwidth of an operation */
	Int 		instr_bitwidth;		/* bitwidth of an instruction */
	Int8 		opcode_bitwidth;
	Int8 		modifier_bitwidth;
	Int8 		guard_bitwidth;
	Int8 		arg1_bitwidth;
	Int8 		arg2_bitwidth;
	Int8 		dest_bitwidth;
	Int8 		opcode_offset;
	Int8 		modifier_offset;
	Int8 		guard_offset;
	Int8 		arg1_offset;
	Int8 		arg2_offset;
	Int8 		dest_offset;
	
	Lib_Set		units;	/* MD_Unit */
	Lib_Mapping	opcodes;/* String -> MD_opcode */
};

extern MD_Machine	Machine;

/*--------------------------- Functions --------------------------------------*/

/*
 * Function         : MD_parse parses a machine description file and
 *                    builds the internal database that can be used
 *                    for lookup later on. 
 * Parameters       : pathname (I) full pathname of a machine 
 *                    description file
 * Function Result  : 0 on success, non zero on failure.
 * Precondition     : -
 * Postcondition    : - 
 * Sideeffects      : -
 */
extern int
MD_parse( String filename );



/*
 * Function         : return an MD_UnitType for a given unit name.
 * Parameters       : unitname (I) the name of the unittype to be looked up
 * Function Result  : MD_UnitType or NULL when the unittype is not found.
 * Precondition     : a successful call to MD_parse is made before. 
 * Postcondition    : - 
 * Sideeffects      : -
 */
extern MD_UnitType
MD_get_unittype_from_name( String unitname );



/*
 * Function         : return an MD_Opcode for a given opcode name.
 * Parameters       : opcodename (I) the name of the opcode to be looked up
 * Function Result  : MD_Opcode or NULL when the opcode is not found.
 * Precondition     : a successful call to MD_parse is made before. 
 * Postcondition    : - 
 * Sideeffects      : -
 */
extern MD_Opcode
MD_get_opcode_from_name( String opcodename );



/*
 * Function         : return an MD_Opcode for a given opcode value.
 * Parameters       : opcode (I) the opcode (value) to be looked up
 * Function Result  : MD_Opcode or NULL when the opcode is not found.
 * Precondition     : a successful call to MD_parse is made before. 
 * Postcondition    : - 
 * Sideeffects      : -
 */
extern MD_Opcode
MD_get_opcode_from_opcode( Int opcode );



/*
 * Function         : traverse all opcodes with a function and data. 
 *                    For each opcode in the machine the function
 *                    is called. The function is given as first parameter
 *                    the opcode (MD_Opcode) and as second parameter 
 *                    the second given to MD_traverse_opcodes
 * Parameters       : traverse_function (I) function that takes two parameters,
 *                            the first an MD_Opcode the second is user data.
 *                    data (I) user data.
 * Function Result  : -
 * Precondition     : a successful call to MD_parse is made before. 
 * Postcondition    : - 
 * Sideeffects      : -
 */
typedef void (*MD_OpcodeTravFunc)(MD_Opcode opcode, Pointer data);

extern void
MD_traverse_opcodes( MD_OpcodeTravFunc traverse_function, Pointer data);



/*
 * Function         : traverse all units with a function and data. 
 *                    For each unit in the machine the function
 *                    is called. The function is given as first parameter
 *                    the unit (MD_Unit) and as second parameter the second
 *                    given to MD_traverse_units
 * Parameters       : traverse_function (I) function that takes two parameters,
 *                            the first an MD_Unit the second is user data.
 *                    data (I) user data.
 * Function Result  : -
 * Precondition     : a successful call to MD_parse is made before. 
 * Postcondition    : - 
 * Sideeffects      : -
 */
typedef void (*MD_UnitTravFunc)(MD_Unit unit, Pointer data);

extern void
MD_traverse_units( MD_UnitTravFunc traverse_function, Pointer data );



/*
 * Function         : traverse all unittypes with a function and data. 
 *                    For each unittype in the machine the function
 *                    is called. The function is given as first parameter
 *                    the unit (MD_UnitType) and as second parameter the second
 *                    given to MD_traverse_unittypes
 * Parameters       : traverse_function (I) function that takes two parameters,
 *                            the first an MD_UnitType the second is user data.
 *                    data (I) user data.
 * Function Result  : -
 * Precondition     : a successful call to MD_parse is made before. 
 * Postcondition    : - 
 * Sideeffects      : -
 */
typedef void (*MD_UnitTypeTravFunc)(MD_UnitType unittype, Pointer data);

extern void
MD_traverse_unittypes( MD_UnitTypeTravFunc traverse_function, Pointer data );


#endif /* MD_INCLUDED */
     
   
