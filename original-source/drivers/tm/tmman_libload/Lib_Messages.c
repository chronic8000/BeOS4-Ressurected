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
 *  Module name              : Lib_Messages.c    1.32
 *
 *  Module type              : IMPLEMENTATION
 *
 *  Title                    : Application message handling
 *
 *  Last update              : 21:49:46 - 98/02/06
 *
 *  Description              : 
 */

/*--------------------------------- Includes: ------------------------------*/

#include "Lib_Messages.h"
#include "Lib_Exceptions.h"
#include "Lib_Local.h"

/*--------------------------------- Constants: -----------------------------*/


/* 
 * Size of temporary buffer in Lib_Msg_report. 
 * Awkward things happen when the representation of
 * a reported message (defined in Messages.table) 
 * comes near this constant, so it is chosen 
 * 'sufficiently large':
 */

#define PRINT_BUFFER_SIZE   2000
#define FATAL_BUFFER_SIZE    200 



/*
 * Uniform format used by Lib_Msg_report
 */

#define MESSAGE_TYPE_FORMAT           "%s %-8s:   "
#define FILEPOS_FORMAT                 "\"%s\", line %d:   "

#define MESSAGE_FORMAT                MESSAGE_TYPE_FORMAT                "%s\n"
#define MESSAGE_FORMAT_WITH_POS       MESSAGE_TYPE_FORMAT FILEPOS_FORMAT "%s\n"

#define PRINT_LEN  100  /* max nr of chars in error line printed */


static void stderr_reporter
              (  String         toolname,
                 Lib_Msg_Type   type,
                 String         filename,  /* or Null, when no position info */
                 Int            lineno,    /* when filename != Null          */
                 Int            linepos,   /* when filename != Null          */
                 String         format,
                 va_list        ap 
              );

void (*Lib_Msg_error_reporter)
              (  String         toolname,
                 Lib_Msg_Type   type,
                 String         filename,  /* or Null, when no position info */
                 Int            lineno,    /* when filename != Null          */
                 Int            linepos,   /* when filename != Null          */
                 String         format,
                 va_list        ap 
              ) = stderr_reporter;

String Lib_Msg_toolname= "";




/*-------------------------- Message Table Handling: -----------------------*/



typedef struct {
     Lib_Msg_Type     type;
     String           text;
} MessageDescriptor;


typedef struct {
     Int          nr_reported;
     String           representation;
} TypeDescriptor;


/*
 * Array of message representations:
 */

#define XMT(x,y)     
#define XAM(x,y,z)   { y, z },

static MessageDescriptor message_descriptors[]
                    = { 
#include "Messages.table"
			{ Lib_Msg_LAST_MESSAGE, "" }
                      };
#undef XAM
#undef XMT


#define XMT(x,y)     { 0, y },
#define XAM(x,y,z)   

static TypeDescriptor type_descriptors[]
                    = { 
#include "Messages.table"
			{ 0, "" }
                      };
#undef XAM
#undef XMT


static Char fatal_error_buffer[FATAL_BUFFER_SIZE+1];  /*
                                                       * One additional
                                                       * character, since
                                                       * strncpy does not
                                                       * guarantee a terminating
                                                       * null char.
                                                       */


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
 * Parameters       : message     (I) message to print
 *                    ...         (I) additional arguments for substitution
 *                                    into the message's representation string. 
 *                    Substitution is similar to substitution performed by
 *                    standard "C" print routines print(3), printf(3) 
 *                    and prints(3).
 * NB               : This procedure can handle at 
 *                    most eight additional arguments       
 */

void Lib_Msg_report( Lib_Msg_Message message, ... )
{
    va_list ap;

    va_start( ap, message );
    Lib_Msg_vreport(message, ap);
    va_end( ap );
}

void Lib_Msg_report_with_pos ( Lib_Msg_Message message, String file, Int line, Int pos, ...        )
{
    va_list ap;

    va_start( ap, pos );
    Lib_Msg_vreport_with_pos(message, file, line, pos, ap);
    va_end( ap );
}





void Lib_Msg_vreport_with_pos( Lib_Msg_Message message, String file, Int line, Int pos, va_list ap )
{
    MessageDescriptor *mdesc;
    TypeDescriptor    *tdesc;
    Char               format_buffer[PRINT_BUFFER_SIZE];

    mdesc = &message_descriptors [ message     ];
    tdesc = &type_descriptors    [ mdesc->type ];

    if (mdesc->type == Msg_Fatal || mdesc->type == Msg_Intern) { 
         vsprintf(format_buffer, mdesc->text, ap);

         sprintf(fatal_error_buffer, FILEPOS_FORMAT, file, line );

         memcpy(fatal_error_buffer + strlen(fatal_error_buffer),
                 format_buffer,
                 FATAL_BUFFER_SIZE  - strlen(fatal_error_buffer));

         printf("MSG_report(): %s\n", fatal_error_buffer);
         /* DL: k_fatal(0,0); */  exit(-1);
         Lib_Excp_raise(message);
    } else {
         Lib_Msg_error_reporter( Lib_Msg_toolname, mdesc->type, file, line, pos, mdesc->text, ap );

         tdesc->nr_reported++;
    }
}



void Lib_Msg_vreport( Lib_Msg_Message message, va_list ap )
{
    MessageDescriptor *mdesc;
    TypeDescriptor    *tdesc;
    Char               format_buffer[PRINT_BUFFER_SIZE];

    mdesc = &message_descriptors [ message     ];
    tdesc = &type_descriptors    [ mdesc->type ];

    if (mdesc->type == Msg_Fatal || mdesc->type == Msg_Intern) { 
         vsprintf(format_buffer, mdesc->text, ap);

         memcpy(fatal_error_buffer,format_buffer,FATAL_BUFFER_SIZE);
         printf("%s\n", format_buffer);
         Lib_Excp_raise(message);
    } else {
         Lib_Msg_error_reporter( Lib_Msg_toolname, mdesc->type, Null, 0, 0, mdesc->text, ap );

         tdesc->nr_reported++;
    }
}



static FILE * curfile;
static char * curfile_name;
static int    curfile_pos;
static int    curfile_end;
static int    curline_length;

static void stderr_reporter
              (  String         toolname,
                 Lib_Msg_Type   type,
                 String         filename,  /* or Null, when no position info */
                 Int            lineno,    /* when filename != Null          */
                 Int            linepos,   /* when filename != Null          */
                 String         format,
                 va_list        ap 
              )
{
    char format_buffer[PRINT_BUFFER_SIZE];

    if (filename) {
         if (True) {
	     if ( (curfile != NULL) && 
                   ( (strcmp(curfile_name,filename) != 0) || (lineno < curfile_pos) )
                ) {
		 free   (curfile_name);  curfile_name = NULL;
		 fclose (curfile     );  curfile      = NULL;
	     } 
    
	     if ( curfile == NULL ) {
		 curfile      = fopen(filename,"r");
		 curfile_name = (void*)malloc(strlen(filename)+1);
		 curfile_pos  = 0;
		 curfile_end  = False;
    
		 if (curfile != NULL && curfile_name == NULL) {
		     fclose(curfile); curfile= NULL;
		 } else
		 if (curfile_name != NULL && curfile == NULL) {
		     free(curfile_name); curfile_name= NULL;
		 } else {
		     strcpy(curfile_name,filename);
		 }
	     }
    
	     if (curfile != NULL) {
		 while (curfile_pos < lineno && !curfile_end) {
		     int i= 0, c= 0;
    
		     while (c != EOF && c != '\n' && i < (PRINT_BUFFER_SIZE-1)) {
			c= getc(curfile); 
			if (c == EOF) {
			    curfile_end= True;
			} else if (c == '\n') {
    
			} else if (i == (PRINT_BUFFER_SIZE-1)) {
			    do { c= getc(curfile); } while (c != EOF && c != '\n');
			} else {
			    format_buffer[i++]= c;
			}
		     } 
    
		     curfile_pos++;
		     format_buffer[i]= 0;
		 } 
    
		 if (!curfile_end) {
                     int i;
		     strcpy( format_buffer+PRINT_LEN, "..." );
                     curline_length= strlen( format_buffer );
	
                     for (i=strlen(format_buffer); i>=0; i--) { 
                         if (format_buffer[i]=='\t') format_buffer[i]= ' ';
                     }

		     fprintf(stderr,"\n");
		     fprintf(stderr,"###\n");
		     fprintf(stderr,"### %s\n", format_buffer);
		 }
	     }
         }

         if (curfile != NULL && !curfile_end) {
             fprintf(stderr,"### ");

             if (linepos > 0 && linepos <= curline_length) {
                 int i;
                 for (i=1; i<linepos; i++) { fputc(' ', stderr); }
                 fputc('^', stderr);
             }

             fprintf(stderr,"\n");
         } 

         sprintf( format_buffer, 
             MESSAGE_FORMAT_WITH_POS,  
    	     toolname, 
             type_descriptors[type].representation,
             filename, lineno, 
             format
         );

         vfprintf(stderr, format_buffer, ap);


    } else {
         sprintf( format_buffer, 
             MESSAGE_FORMAT,  
    	     toolname, 
             type_descriptors[type].representation,
             format
         );

//         vfprintf(stderr, format_buffer, ap);
         vprintf(format_buffer, ap);
    }

}




/*
 * Function         : Return nrof errors reported until 'now'.
 * Parameters       : -
 * Return value     : number of messages of type Msg_Fatal 
 *                    or Msg_Error have been reported. 
 */
Int Lib_Msg_nrof_errors_found(void)
{
    return type_descriptors[Msg_Error].nr_reported  
         + type_descriptors[Msg_Fatal].nr_reported
         ;
}



/*
 * Function         : Obtain the text corresponding with the
 *                    last fatal error which was raised.
 * Parameters       : -
 * Return value     : see above
 */
String Lib_Msg_get_last_fatal_message(void)
{
    return fatal_error_buffer;
}

/*
 * Function         : Clear last error
 * Parameters       : -
 * Return value     : 
 */
void Lib_Msg_clear_last_fatal_message(void)
{
    fatal_error_buffer[0] = '\0';
}


/*
 * Function         : Print the text corresponding with the
 *                    last fatal error which was raised.
 * Parameters       : -
 * Return value     : -
 */
void Lib_Msg_print_last_fatal_message(void)
{
    Lib_Msg_error_reporter
              (  Lib_Msg_toolname,
                 Msg_Fatal,
                 Null, 
                 0,
                 0,
                 fatal_error_buffer,
                 Null 
              );
}



