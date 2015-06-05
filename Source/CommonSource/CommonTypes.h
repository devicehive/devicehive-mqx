/*-----------------------------------------------------------------------------
Autor                  Datum
E. Fitze               21.12.2012

-- BESCHREIBUNG ---------------------------------------------------------------

Allgemeine Typendefinitionen


-- AENDERUNGEN ----------------------------------------------------------------

Autor                   Datum


-----------------------------------------------------------------------------*/
#pragma once
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
typedef unsigned char            UInt8;
typedef unsigned short           UInt16;
typedef unsigned int             UInt32;

typedef char                     Int8;
typedef short                    Int16;
typedef int                      Int32;

typedef UInt8                    Bool;

typedef void                     *PVOID;
//typedef UInt32                   DWORD;



//-----------------------------------------------------------------------------
typedef unsigned char  BOOLE;
typedef signed short   INT16;
typedef unsigned short UINT16;
typedef signed char    INT8;
typedef unsigned char  UINT8;
typedef unsigned long  UINTL32;
typedef signed long    INTL32;
typedef char           TXTCHR;   /* Text-Zeichen */
typedef float          FLOAT32;
typedef double         FLOAT64;


//-----------------------------------------------------------------------------
#ifndef TRUE
   #define TRUE          1
#endif
#ifndef FALSE
   #define FALSE         0
#endif


//-----------------------------------------------------------------------------
#define ABS(val)  ((val) < 0? -(val): (val))

//-----------------------------------------------------------------------------
#ifndef MAX_INT16
   #define MAX_INT16     (0x7FFF)
#endif
#ifndef MAX_UINT16
   #define MAX_UINT16    (0xFFFF)
#endif
#ifndef MAX_INT32
   #define MAX_INT32     (0x7FFFFFFFL)
#endif
#ifndef MAX_UINT32
   #define MAX_UINT32    (0xFFFFFFFFUL)
#endif
#ifndef MAX_FLOAT32
   #define MAX_FLOAT32   (3.4028235E38)
#endif

//-----------------------------------------------------------------------------
#define BIT0    (1 <<  0)
#define BIT1    (1 <<  1)
#define BIT2    (1 <<  2)
#define BIT3    (1 <<  3)
#define BIT4    (1 <<  4)
#define BIT5    (1 <<  5)
#define BIT6    (1 <<  6)
#define BIT7    (1 <<  7)
#define BIT8    (1 <<  8)
#define BIT9    (1 <<  9)
#define BIT10   (1 <<  10)
#define BIT11   (1 <<  11)
#define BIT12   (1 <<  12)
#define BIT13   (1 <<  13)
#define BIT14   (1 <<  14)
#define BIT15   (1 <<  15)
#define BIT16   (1 <<  16)
#define BIT17   (1 <<  17)
#define BIT18   (1 <<  18)
#define BIT19   (1 <<  19)
#define BIT20   (1 <<  20)
#define BIT21   (1 <<  21)
#define BIT22   (1 <<  22)
#define BIT23   (1 <<  23)
#define BIT24   (1 <<  24)
#define BIT25   (1 <<  25)
#define BIT26   (1 <<  26)
#define BIT27   (1 <<  27)
#define BIT28   (1 <<  28)
#define BIT29   (1 <<  29)
#define BIT30   (1 <<  30)
#define BIT31   (1 <<  31)


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
