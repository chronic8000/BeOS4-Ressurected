#include "tmtypes.h"

#ifdef __cplusplus
extern "C" {
#endif


#define FILE_VER_MAJ	5
#define FILE_VER_MIN	2
#define FILE_VER_BLD	0
#define PROD_VER_MAJ	2
#define PROD_VER_MIN	0
#define FILE_VER_STR	"v5.2.0"
#define PROD_VER_STR	"v2.0(Beta)"
#define PRODUCT_NAME	"TriMedia Software Development Environment"
#define COMPANY_NAME	"Philips Semiconductors"
#define LEGAL_COPYRIGHT	"(c) Philips Semiconductors 1998"
#define LEGAL_TRADEMARKS	"TriMedia is a registererd  trademark of Philips Semiconductors"


UInt32	verGetFileMajorVersion ( void );
UInt32	verGetFileMinorVersion ( void );
UInt32	verGetFileBuildVersion ( void );

UInt32	verGetProductMajorVersion ( void );
UInt32	verGetProductMinorVersion ( void );

Pointer	verGetFileVersionString ( void );
Pointer	verGetProductVersionString ( void );

#ifdef __cplusplus
};
#endif	/* __cplusplus */
