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
 *  Module name              : Lib_Messages.h    1.15
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Application message handling
 *
 *  Last update              : 18:11:56 - 97/12/12
 *
 *  Description              : 
 */

#ifndef Lib_Messages_INCLUDED
#define Lib_Messages_INCLUDED

/*--------------------------------- Includes: ------------------------------*/

#include "Lib_Local.h"
#include <stdarg.h>

/*---------------------------- Type definitions: ---------------------------*/

#define XMT(x,y)     
#define XAM(x,y,z)  x,

typedef enum {
#include "Messages.table"
      Lib_Msg_LAST_MESSAGE
} Lib_Msg_Message;

#undef XAM
#undef XMT



#define XMT(x,y)    x,
#define XAM(x,y,z) 

typedef enum { 
#include "Messages.table"
         Lib_Msg_LAST_TYPE 
} Lib_Msg_Type;

#undef XAM
#undef XMT


extern String Lib_Msg_toolname;	/* tool can install its own name */

extern void (*Lib_Msg_error_reporter)
              (  String         toolname,
                 Lib_Msg_Type   type,
                 String         filename,  /* or Null, when no position info */
                 Int            lineno,    /* when filename != Null          */
                 Int            linepos,   /* when filename != Null          */
                 String         format,
                 va_list        ap 
              );


/*--------------------------- Function definitions: ------------------------*/


/*
 * Function         : Report a message, and perform the here unspecified
 *                    actions which are needed. This might be printing 
 *                    a formatted string, or whatever. What is specified
 *                    on this interface is the following:
 *                    When the type of the message is Msg_Fatal, this function
 *                    will terminate by raising the integer value of
 *                    'message' as an exception using the Lib_Exceptions 
 *                    interface. This exception may be caught in higher
 *                    levels of software, but if not, Lib_Exceptions will
 *                    perform a call to exit, with the mentioned numerical   
 *                    value as argument. 
 *            NB:     There is an important difference between fatal 
 *                    errors and other messages, which makes sense when
 *                    they are raised in libraries: unsimilar to other
 *                    messages, fatal errors are *not* immiately printed.
 *                    The excption is silently raised, and the error text
 *                    is preserved, to be optionally printed by the catcher
 *                    of the exception (which may be the top-level 
 *                    exception handler immediately before exiting).
 *                    See Lib_Msg_print_last_fatal_message.
 * Parameters       : message     (I) message to print
 *                    ...         (I) additional arguments for substitution
 *                                    into the message's representation string. 
 *                    Substitution is similar to substitution performed by
 *                    standard "C" print routines print(3), printf(3) 
 *                    and prints(3).
 * NB               : This procedure can handle at 
 *                    most eight additional arguments       
 */

void Lib_Msg_report ( Lib_Msg_Message message, ...        );
void Lib_Msg_vreport( Lib_Msg_Message message, va_list ap );

void Lib_Msg_report_with_pos ( Lib_Msg_Message message, String file, Int line, Int pos, ...        );
void Lib_Msg_vreport_with_pos( Lib_Msg_Message message, String file, Int line, Int pos, va_list ap );



/*
 * Function         : Obtain the text corresponding with the
 *                    last fatal error which was raised.
 * Parameters       : -
 * Return value     : see above
 */
String Lib_Msg_get_last_fatal_message();



/*
 * Function         : Clear last error
 * Parameters       : -
 * Return value     : 
 */
void Lib_Msg_clear_last_fatal_message(void);

/*
 * Function         : Print the text corresponding with the
 *                    last fatal error which was raised.
 * Parameters       : -
 * Return value     : -
 */
void Lib_Msg_print_last_fatal_message(void);



/*
 * Function         : Return nrof errors reported until 'now'.
 * Parameters       : -
 * Return value     : number of messages of type Msg_Fatal 
 *                    or Msg_Error have been reported. 
 */
Int Lib_Msg_nrof_errors_found(void);



#endif /* Lib_Messages_INCLUDED */


     
   
