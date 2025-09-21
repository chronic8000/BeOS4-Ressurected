/*
 *  COPYRIGHT (c) 1997 by Philips Semiconductors
 *
 *   +-----------------------------------------------------------------+
 *   | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *   | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *   | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *   | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *   | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *   | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *   +-----------------------------------------------------------------+
 *
 *  Module name              : tmhost.h    1.4
 *
 *  Module type              : SPECIFICATION
 *
 *  Title                    : Host definitions
 *
 *  Last update              : 18:00:13 - 98/01/08
 *
 *  Description              :  
 *
 *               This header file enumerates the possible environments
 *               which are expected to serve as a TM1 host. In case
 *               new ones become available, they should be defined 
 *               here, to let all components of the toolset know.
 *
 *               The host values are passed by the monitors to the 
 *               downloader, and they can be inspected from the application
 *               by means of calls from the tmProcessor library.
 *               
 *
 */

#ifndef	_TMHOST_h
#define	_TMHOST_h

typedef enum {
   tmInvalidHost,
   tmNoHost,
   tmTmSimHost,
   tmWin32Host,
   tmWin95Host = tmWin32Host,
   tmMacOSHost,
   tmWinNTHost
}  tmHostType_t;

#endif
