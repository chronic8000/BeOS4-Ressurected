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
 *  Module name              : Lib_Exceptions.h    1.12
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Exception handling support
 *
 *  Last update              : 10:43:08 - 98/03/20
 *
 *  Description              : 
 *
 *             This unit provides the facilities for exception 
 *             handling in the form of a set of macros which
 *             allows exception handlers to be installed for 
 *             'protecting' a region, or sequence of C statements 
 *             from raised exceptions during the execution of that
 *             protected region. 
 *
 *             Protected regions can be dynamically nested, e.g.
 *             within a protected region another may be 'installed',
 *             or functions may be called which also install regions.
 *             This means that protected regions are dynamically stacked.
 *             Exceptions are numerically- valued objects; when they 
 *             are raised, they immediately terminate the last installed
 *             (and not terminted) region, wherever the exception happens
 *             to be raised. The terminated, or catching, region might
 *             specify code which is to be executed in this case. This
 *             code may be conditional on the value of the exception 
 *             raised. 
 *
 *             Two forms of excption handlers are supported: one which
 *             installs exception handling code which is to be invoked
 *             upon each exception, regardless its value, and one which
 *             selects upon the exception value in a switch- like way.
 * 
 *             SYNTAX (simple form):
 *             
 *                    Lib_Excp_try {
 *                        ...'regular' code...
 *                    } 
 *                    Lib_Excp_otherwise {
 *                        ...exception handling code...
 *                    }
 *                    Lib_Excp_end_try;
 *
 *
 *             SYNTAX (selecting form):
 *             
 *                    Lib_Excp_try {
 *                        ...'regular' code...
 *                    } 
 *                    Lib_Excp_otherwise_switch {
 *                       Lib_Excp_case( ERROR1 )
 *                           .. exception handling code for ERROR1 ..
 *                           break;
 *
 *                       Lib_Excp_case( ERROR2 )
 *                           .. exception handling code for ERROR2 ..
 *                           break;
 *
 *                       Lib_Excp_default()
 *                           .. exception handling code for all others ..
 *                           break;
 *                    }
 *                    Lib_Excp_end_try;
 *
 *             The simple form catches all errors: when entering the
 *             exception handling code, the exception is 'gone', and
 *             execution resumes with the catching code, and thereafter
 *             with the code following the end_try. The selecting form
 *             is slightly different; first, it is able to select on
 *             the caught exception value in a switch like way, with
 *             specific cases and an optional default. Second, in case
 *             a default case is *not* specified, and an exception occurs
 *             which is not handled, the same exception is raised again,
 *             to be propagated to the next embracing exception handler.
 *
 *             In either form, the current exception can be explicitly
 *             propagated from the exception handling code (using macro
 *             Lib_Excp_propagate() ).
 *
 *             CAVEAT:
 *             -  It is not allowed to jump out of 
 *                either the regular code or the
 *                exception handling code (by means of
 *                returns, breaks, etc). That is, the 
 *                Lib_Excp_end_try *must* be executed.
 *             -  The exception handler, or the code after the
 *                exception handling construct must *not* use
 *                local values (i.e. not residing in global 
 *                variables) which have been computed before
 *                the construct. This is because exception 
 *                handling is implemented by means of setjmp/
 *                longjmp calls which may forget register-
 *                allocated values; see also the information
 *                on SETJMP(3V)
 *
 *             EXAMPLE:
 *
 *                   void f( int n ) {
 *                       Lib_Excp_try {
 *                             if (n>0) g(n-1);
 *                             else Lib_Excp_raise();
 *                       } 
 *                       Lib_Excp_otherwise {
 *                             printf("returning f(%d)\n",n);
 *                             Lib_Excp_raise();
 *                       }
 *                       Lib_Excp_end_try;
 *                   }
 *
 *                   void g( int n ) {
 *                       Lib_Excp_try {
 *                             if (n>0) f(n-1);
 *                             else Lib_Excp_raise();
 *                       } 
 *                       Lib_Excp_otherwise_switch {
 *                          Lib_Excp_case(25)
 *                             printf("returning g(%d)\n",n);
 *                             Lib_Excp_propagate();
 *                             break;
 *
 *                          Lib_Excp_default()
 *                             break;
 *                       }
 *                       Lib_Excp_end_try;
 *                   }
 *
 *                    main() { f(25); }
 */

#ifndef Lib_Exceptions_INCLUDED
#define Lib_Exceptions_INCLUDED

/*--------------------------------- Includes: ------------------------------*/

#include "Lib_Local.h"


/*--------------------------- Function definitions: ------------------------*/


/*
 * Top handler on handler stack, and 
 * buffer for raised exception
 * These should *not* be accessed, except
 * via the macros below:
 */
extern jmp_buf *___Lib_Excp_last_handler___;
extern Int  ___Lib_Excp_raised_excep___;





#define Lib_Excp_try \
           {  \
              jmp_buf  Lib_Excp_my_environment;                     \
              jmp_buf *Lib_Excp_previous_handler                    \
                                  =  ___Lib_Excp_last_handler___;   \
              ___Lib_Excp_last_handler___                           \
                                  = (jmp_buf *) &Lib_Excp_my_environment;       \
              if ( !setjmp(Lib_Excp_my_environment) ) {             \


#define Lib_Excp_otherwise \
                    ___Lib_Excp_last_handler___               	\
                             = Lib_Excp_previous_handler;     	\
              } else {                                          \
		    Bool Lib_Excp_caught  = True;	    	\
                    Int Lib_Excp_current = ___Lib_Excp_raised_excep___; \
                    ___Lib_Excp_last_handler___               	\
                             = Lib_Excp_previous_handler;     	\
		    {    					\


#define Lib_Excp_otherwise_switch \
                    ___Lib_Excp_last_handler___               	\
                             = Lib_Excp_previous_handler;     	\
              } else {                                        	\
		    Bool Lib_Excp_caught  = False;	      	\
                    Int Lib_Excp_current = ___Lib_Excp_raised_excep___; \
                    ___Lib_Excp_last_handler___               	\
                             = Lib_Excp_previous_handler;     	\
		    switch (Lib_Excp_current) {    		\


#define Lib_Excp_case(error) \
			case error: 			      	\
				Lib_Excp_caught= True;


#define Lib_Excp_default() \
			default: 				\
				Lib_Excp_caught= True;


#define Lib_Excp_end_try \
                    }						\
		    if (!Lib_Excp_caught) {		    	\
			Lib_Excp_propagate();		    	\
           }  }     }





#define Lib_Excp_raise(exception) \
	    if (___Lib_Excp_last_handler___ != Null) {			\
                ___Lib_Excp_raised_excep___= (Int)(exception);	\
                longjmp( *___Lib_Excp_last_handler___, 1);		\
            } else {							\
                Lib_Msg_print_last_fatal_message();			\
		exit(-1); /* DL: k_fatal(0,0); */		\
            }


#define Lib_Excp_propagate() \
	    Lib_Excp_raise(Lib_Excp_current);




#endif /* Lib_Exceptions_INCLUDED */


     
   
